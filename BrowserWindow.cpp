// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "BrowserWindow.h"
#include "shlobj.h"
#include "WebView2EnvironmentOptions.h"
#include <dxgi1_6.h>
#include <filesystem>
#include <Urlmon.h>
#pragma comment (lib, "Urlmon.lib")
#pragma comment (lib, "dxgi.lib")

using namespace Microsoft::WRL;

WCHAR BrowserWindow::s_windowClass[] = { 0 };
WCHAR BrowserWindow::s_title[] = { 0 };

//
//  FUNCTION: RegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM BrowserWindow::RegisterClass(_In_ HINSTANCE hInstance)
{
    // Initialize window class string
    LoadStringW(hInstance, IDC_WEBVIEWBROWSERAPP, s_windowClass, MAX_LOADSTRING);
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProcStatic;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WEBVIEWBROWSERAPP));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WEBVIEWBROWSERAPP);
    wcex.lpszClassName = s_windowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//  FUNCTION: WndProcStatic(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Redirect messages to approriate instance or call default proc
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK BrowserWindow::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Get the ptr to the BrowserWindow instance who created this hWnd.
    // The pointer was set when the hWnd was created during InitInstance.
    BrowserWindow* browser_window = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (browser_window != nullptr)
    {
        return browser_window->WndProc(hWnd, message, wParam, lParam);  // Forward message to instance-aware WndProc
    }
    else
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for each browser window instance.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK BrowserWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* minmax = reinterpret_cast<MINMAXINFO*>(lParam);
        minmax->ptMinTrackSize.x = m_minWindowWidth;
        minmax->ptMinTrackSize.y = m_minWindowHeight;
    }
    break;
    case WM_DPICHANGED:
    {
        UpdateMinWindowSize();
    }
    case WM_SIZE:
    {
        // WM_DPICHANGED falls through here, with a DPI in wParam.
        if (message == WM_SIZE)
        {
            SetMinimized(wParam == SIZE_MINIMIZED);
        }
        ResizeUIWebViews();
        if (m_tabs.find(m_activeTabId) != m_tabs.end())
        {
            m_tabs.at(m_activeTabId)->ResizeWebView();
        }
    }
    break;
    case WM_CLOSE:
    {
        web::json::value jsonObj = web::json::value::parse(L"{}");
        jsonObj[L"message"] = web::json::value(MG_CLOSE_WINDOW);
        jsonObj[L"args"] = web::json::value::parse(L"{}");

        CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Try again.");
    }
    break;
    case WM_TIMER:
    {
        // A hidden tab's timer, named by its ID, see Tab::SetVisible.
        auto tab = m_tabs.find(wParam);
        if (tab != m_tabs.end())
        {
            tab->second->OnHiddenTimer();
        }
        else
        {
            KillTimer(hWnd, wParam);
        }
    }
    break;
    case WM_NCDESTROY:
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
        delete this;
        PostQuitMessage(0);
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;
    default:
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    break;
    }
    return 0;
}


BOOL BrowserWindow::LaunchWindow(_In_ HINSTANCE hInstance, _In_ int nCmdShow)
{
    // BrowserWindow keeps a reference to itself in its host window and will
    // delete itself when the window is destroyed.
    BrowserWindow* window = new BrowserWindow();
    if (!window->InitInstance(hInstance, nCmdShow))
    {
        delete window;
        return FALSE;
    }
    return TRUE;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL BrowserWindow::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    m_hInst = hInstance; // Store app instance handle
    LoadStringW(m_hInst, IDS_APP_TITLE, s_title, MAX_LOADSTRING);

    SetUIMessageBroker();

    m_hWnd = CreateWindowW(s_windowClass, s_title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, m_hInst, nullptr);

    if (!m_hWnd)
    {
        return FALSE;
    }

    // Make the BrowserWindow instance ptr available through the hWnd
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    UpdateMinWindowSize();
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    // Get directory for user data. The browser UI keeps its data apart from
    // the tabs' in a profile of its own (c_uiProfileName).
    std::wstring userDataDirectory = GetAppDataDirectory();
    userDataDirectory.append(L"\\User Data");
    MoveBrowserUIData(userDataDirectory);

    auto options = Make<CoreWebView2EnvironmentOptions>();
    options->put_AdditionalBrowserArguments(GetBrowserArguments().c_str());

    // Create the WebView environment for both the tabs and the browser UI:
    // one browser process, GPU process and network service for all of them,
    // where a second environment for the UI would start a second set. It is
    // created first so the UI can request new tabs when it's ready.
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, userDataDirectory.c_str(),
        options.Get(), Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
    {
        RETURN_IF_FAILED(result);

        m_env = env;
        HRESULT hr = InitUIWebViews();

        if (!SUCCEEDED(hr))
        {
            OutputDebugString(L"UI WebViews environment creation failed\n");
        }

        return hr;
    }).Get());

    if (!SUCCEEDED(hr))
    {
        OutputDebugString(L"Content WebViews environment creation failed\n");
        return FALSE;
    }

    return TRUE;
}

HRESULT BrowserWindow::InitUIWebViews()
{
    RETURN_IF_FAILED(CreateBrowserControlsWebView());
    RETURN_IF_FAILED(CreateBrowserOptionsWebView());

    return S_OK;
}

// Creates a WebView for the browser UI, in the UI's profile so its data stays
// isolated from web content requested by the user. Runtimes before profiles
// (101) put it in the tabs' profile.
HRESULT BrowserWindow::CreateUIController(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* handler)
{
    ComPtr<ICoreWebView2Environment10> env10;
    if (FAILED(m_env.As(&env10)))
    {
        return m_env->CreateCoreWebView2Controller(m_hWnd, handler);
    }

    wil::com_ptr<ICoreWebView2ControllerOptions> options;
    RETURN_IF_FAILED(env10->CreateCoreWebView2ControllerOptions(&options));
    RETURN_IF_FAILED(options->put_ProfileName(c_uiProfileName));

    return env10->CreateCoreWebView2ControllerWithOptions(m_hWnd, options.get(), handler);
}

// The browser UI used to run in a second environment with its data, the
// favorites and history in IndexedDB, in "Browser Data". It now has a profile
// in the tabs' environment, and its data moves there once.
void BrowserWindow::MoveBrowserUIData(const std::wstring& userDataDirectory)
{
    namespace fs = std::filesystem;
    std::error_code error;

    fs::path oldIndexedDB = fs::path(GetAppDataDirectory()) / L"Browser Data" / L"EBWebView" / L"Default" / L"IndexedDB";
    fs::path profile = fs::path(userDataDirectory) / c_uiProfileFolder;
    if (!fs::exists(oldIndexedDB, error) || fs::exists(profile, error))
    {
        return;
    }

    fs::create_directories(profile, error);
    fs::copy(oldIndexedDB, profile / L"IndexedDB", fs::copy_options::recursive, error);
}

// Once there are c_rendererProcessLimit renderer processes, a new tab or frame
// goes into a process of its site if there is one, rather than one of its own;
// a new site still gets a process. Chromium otherwise gives every tab its own
// process however many there are (Chrome puts up to two tabs of a site in one,
// which WebView2 doesn't do), so tabs of one site each paid for a renderer.
// Below the limit each tab keeps a process of its own, so a busy background
// tab doesn't slow the one in front.
std::wstring BrowserWindow::GetBrowserArguments()
{
    std::wstring arguments = L"--renderer-process-limit=" + std::to_wstring(c_rendererProcessLimit);
    if (HasDiscreteGpu())
    {
        arguments.append(L" --force-high-performance-gpu");
    }

    return arguments;
}

// Chromium draws on the GPU Windows gives it. Where a discrete GPU (NVIDIA or
// AMD) is there, beside an integrated one or alone, it is asked for the high
// performance one, which with a single GPU is that GPU anyway.
bool BrowserWindow::HasDiscreteGpu()
{
    ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
    {
        return false;
    }

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; i++)
    {
        DXGI_ADAPTER_DESC1 desc;
        if (FAILED(adapter->GetDesc1(&desc)) || (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
        {
            continue;
        }

        const UINT nvidia = 0x10DE;
        const UINT amd = 0x1002;
        if (desc.VendorId == nvidia || desc.VendorId == amd)
        {
            return true;
        }
    }

    return false;
}

void BrowserWindow::SetMemoryUsage(ICoreWebView2* webview, COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL level)
{
    // Runtimes before 114 keep it all.
    ComPtr<ICoreWebView2_19> webview19;
    if (webview && SUCCEEDED(webview->QueryInterface(IID_PPV_ARGS(&webview19))))
    {
        webview19->put_MemoryUsageTargetLevel(level);
    }
}

HRESULT BrowserWindow::CreateBrowserControlsWebView()
{
    return CreateUIController(Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this](HRESULT result, ICoreWebView2Controller* host) -> HRESULT
    {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Controls WebView creation failed\n");
            return result;
        }
        // WebView created
        m_controlsController = host;
        CheckFailure(m_controlsController->get_CoreWebView2(&m_controlsWebView), L"");

        wil::com_ptr<ICoreWebView2Settings> settings;
        RETURN_IF_FAILED(m_controlsWebView->get_Settings(&settings));
        RETURN_IF_FAILED(settings->put_AreDevToolsEnabled(FALSE));

        RETURN_IF_FAILED(m_controlsController->add_ZoomFactorChanged(Callback<ICoreWebView2ZoomFactorChangedEventHandler>(
            [](ICoreWebView2Controller* host, IUnknown* args) -> HRESULT
        {
            host->put_ZoomFactor(1.0);
            return S_OK;
        }
        ).Get(), &m_controlsZoomToken));

        RETURN_IF_FAILED(m_controlsWebView->add_WebMessageReceived(m_uiMessageBroker.Get(), &m_controlsUIMessageBrokerToken));
        RETURN_IF_FAILED(ResizeUIWebViews());

        std::wstring controlsPath = GetFullPathFor(L"wvbrowser_ui\\controls_ui\\default.html");
        RETURN_IF_FAILED(m_controlsWebView->Navigate(controlsPath.c_str()));

        return S_OK;
    }).Get());
}

HRESULT BrowserWindow::CreateBrowserOptionsWebView()
{
    return CreateUIController(Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this](HRESULT result, ICoreWebView2Controller* host) -> HRESULT
    {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Options WebView creation failed\n");
            return result;
        }
        // WebView created
        m_optionsController = host;
        CheckFailure(m_optionsController->get_CoreWebView2(&m_optionsWebView), L"");

        wil::com_ptr<ICoreWebView2Settings> settings;
        RETURN_IF_FAILED(m_optionsWebView->get_Settings(&settings));
        RETURN_IF_FAILED(settings->put_AreDevToolsEnabled(FALSE));

        RETURN_IF_FAILED(m_optionsController->add_ZoomFactorChanged(Callback<ICoreWebView2ZoomFactorChangedEventHandler>(
            [](ICoreWebView2Controller* host, IUnknown* args) -> HRESULT
        {
            host->put_ZoomFactor(1.0);
            return S_OK;
        }
        ).Get(), &m_optionsZoomToken));

        // Hide by default, keeping little memory until it shows
        RETURN_IF_FAILED(m_optionsController->put_IsVisible(FALSE));
        SetMemoryUsage(m_optionsWebView.Get(), COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_LOW);
        RETURN_IF_FAILED(m_optionsWebView->add_WebMessageReceived(m_uiMessageBroker.Get(), &m_optionsUIMessageBrokerToken));

        // Hide menu when focus is lost
        RETURN_IF_FAILED(m_optionsController->add_LostFocus(Callback<ICoreWebView2FocusChangedEventHandler>(
            [this](ICoreWebView2Controller* sender, IUnknown* args) -> HRESULT
        {
            web::json::value jsonObj = web::json::value::parse(L"{}");
            jsonObj[L"message"] = web::json::value(MG_OPTIONS_LOST_FOCUS);
            jsonObj[L"args"] = web::json::value::parse(L"{}");

            PostJsonToWebView(jsonObj, m_controlsWebView.Get());

            return S_OK;
        }).Get(), &m_lostOptionsFocus));

        RETURN_IF_FAILED(ResizeUIWebViews());

        std::wstring optionsPath = GetFullPathFor(L"wvbrowser_ui\\controls_ui\\options.html");
        RETURN_IF_FAILED(m_optionsWebView->Navigate(optionsPath.c_str()));

        return S_OK;
    }).Get());
}

// Set the message broker for the UI webview. This will capture messages from ui web content.
// Lambda is used to capture the instance while satisfying Microsoft::WRL::Callback<T>()
void BrowserWindow::SetUIMessageBroker()
{
    m_uiMessageBroker = Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [this](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* eventArgs) -> HRESULT
    {
        wil::unique_cotaskmem_string jsonString;
        CheckFailure(eventArgs->get_WebMessageAsJson(&jsonString), L"");  // Get the message from the UI WebView as JSON formatted string
        web::json::value jsonObj = web::json::value::parse(jsonString.get());

        if (!jsonObj.has_field(L"message"))
        {
            OutputDebugString(L"No message code provided\n");
            return S_OK;
        }

        if (!jsonObj.has_field(L"args"))
        {
            OutputDebugString(L"The message has no args field\n");
            return S_OK;
        }

        int message = jsonObj.at(L"message").as_integer();
        web::json::value args = jsonObj.at(L"args");

        switch (message)
        {
        case MG_CREATE_TAB:
        {
            size_t id = args.at(L"tabId").as_number().to_uint32();
            bool shouldBeActive = args.at(L"active").as_bool();
            // A tab the toolbar adds for a window a page asked for is for the
            // oldest one, as the toolbar adds them in turn.
            Tab::NewWindow newWindow;
            if (args.at(L"newWindow").as_bool() && !m_newWindows.empty())
            {
                newWindow = m_newWindows.front();
                m_newWindows.pop_front();
            }
            std::unique_ptr<Tab> newTab = Tab::CreateNewTab(m_hWnd, m_env.Get(), id, shouldBeActive, newWindow);

            std::map<size_t, std::unique_ptr<Tab>>::iterator it = m_tabs.find(id);
            if (it == m_tabs.end())
            {
                m_tabs.insert(std::pair<size_t,std::unique_ptr<Tab>>(id, std::move(newTab)));
            }
            else
            {
                m_tabs.at(id)->m_contentController->Close();
                it->second = std::move(newTab);
            }
        }
        break;
        case MG_NAVIGATE:
        {
            std::wstring uri(args.at(L"uri").as_string());
            std::wstring browserScheme(L"browser://");

            if (uri.substr(0, browserScheme.size()).compare(browserScheme) == 0)
            {
                // No encoded search URI
                std::wstring path = uri.substr(browserScheme.size());
                if (path.compare(L"newtab") == 0 ||
                    path.compare(L"favorites") == 0 ||
                    path.compare(L"settings") == 0 ||
                    path.compare(L"history") == 0)
                {
                    std::wstring filePath(L"wvbrowser_ui\\content_ui\\");
                    filePath.append(path);
                    filePath.append(L".html");
                    std::wstring fullPath = GetFullPathFor(filePath.c_str());
                    CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->Navigate(fullPath.c_str()), L"Can't navigate to browser page.");
                }
                else
                {
                    OutputDebugString(L"Requested unknown browser page\n");
                }
            }
            else if (!SUCCEEDED(m_tabs.at(m_activeTabId)->m_contentWebView->Navigate(uri.c_str())))
            {
                CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->Navigate(args.at(L"encodedSearchURI").as_string().c_str()), L"Can't navigate to requested page.");
            }
        }
        break;
        case MG_GO_FORWARD:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->GoForward(), L"");
        }
        break;
        case MG_GO_BACK:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->GoBack(), L"");
        }
        break;
        case MG_RELOAD:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->Reload(), L"");
        }
        break;
        case MG_CANCEL:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->Stop(), L"");
        }
        break;
        case MG_SWITCH_TAB:
        {
            size_t tabId = args.at(L"tabId").as_number().to_uint32();

            SwitchToTab(tabId);
        }
        break;
        case MG_WAKE_TAB:
        {
            size_t tabId = args.at(L"tabId").as_number().to_uint32();
            auto tab = m_tabs.find(tabId);
            if (tab != m_tabs.end() && tab->second->m_contentWebView)
            {
                tab->second->Wake();
            }
        }
        break;
        case MG_CLOSE_TAB:
        {
            size_t id = args.at(L"tabId").as_number().to_uint32();
            m_tabs.at(id)->m_contentController->Close();
            m_tabs.erase(id);
        }
        break;
        case MG_CLOSE_WINDOW:
        {
            DestroyWindow(m_hWnd);
        }
        break;
        case MG_SHOW_OPTIONS:
        {
            SetMemoryUsage(m_optionsWebView.Get(), COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_NORMAL);
            CheckFailure(m_optionsController->put_IsVisible(TRUE), L"");
            m_optionsController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }
        break;
        case MG_HIDE_OPTIONS:
        {
            CheckFailure(m_optionsController->put_IsVisible(FALSE), L"Something went wrong when trying to close the options dropdown.");
            SetMemoryUsage(m_optionsWebView.Get(), COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_LOW);
        }
        break;
        case MG_OPTION_SELECTED:
        {
            m_tabs.at(m_activeTabId)->m_contentController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }
        break;
        case MG_GET_FAVORITES:
        case MG_GET_SETTINGS:
        case MG_GET_HISTORY:
        {
            // Forward back to requesting tab
            size_t tabId = args.at(L"tabId").as_number().to_uint32();
            jsonObj[L"args"].erase(L"tabId");

            CheckFailure(PostJsonToWebView(jsonObj, m_tabs.at(tabId)->m_contentWebView.Get()), L"Requesting history failed.");
        }
        break;
        default:
        {
            OutputDebugString(L"Unexpected message\n");
        }
        break;
        }

        return S_OK;
    });
}

HRESULT BrowserWindow::SwitchToTab(size_t tabId)
{
    size_t previousActiveTab = m_activeTabId;

    RETURN_IF_FAILED(m_tabs.at(tabId)->ResizeWebView());
    RETURN_IF_FAILED(m_tabs.at(tabId)->SetVisible(true));
    m_activeTabId = tabId;

    if (previousActiveTab != INVALID_TAB_ID && previousActiveTab != m_activeTabId) {
        auto previousTabIterator = m_tabs.find(previousActiveTab);
        if (previousTabIterator != m_tabs.end() && previousTabIterator->second &&
            previousTabIterator->second->m_contentController)
        {
            auto hr = m_tabs.at(previousActiveTab)->SetVisible(false);
            if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_STATE)) {
                HandleTabCloseRequested(previousActiveTab);
            }
            RETURN_IF_FAILED(hr);
        }
    }

    return S_OK;
}

// A minimized window shows nothing: its tab is hidden as a background tab is,
// so it keeps little memory and later sleeps, and the toolbar stops drawing.
// Both show again as the window does.
void BrowserWindow::SetMinimized(bool minimized)
{
    if (minimized == m_minimized)
    {
        return;
    }
    m_minimized = minimized;

    if (m_controlsController)
    {
        m_controlsController->put_IsVisible(!minimized);
    }
    auto tab = m_tabs.find(m_activeTabId);
    if (tab != m_tabs.end() && tab->second->m_contentController)
    {
        tab->second->SetVisible(!minimized);
    }
}

HRESULT BrowserWindow::HandleTabURIUpdate(size_t tabId, ICoreWebView2* webview)
{
    wil::unique_cotaskmem_string source;
    RETURN_IF_FAILED(webview->get_Source(&source));

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_UPDATE_URI);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
    jsonObj[L"args"][L"uri"] = web::json::value(source.get());

    std::wstring uriToShow = GetURIToShow(source.get());
    if (!uriToShow.empty())
    {
        jsonObj[L"args"][L"uriToShow"] = web::json::value(uriToShow);
    }

    RETURN_IF_FAILED(PostJsonToWebView(jsonObj, m_controlsWebView.Get()));

    return S_OK;
}

// The address bar shows the browser's own pages as browser://<page>, and
// nothing for a new tab's.
std::wstring BrowserWindow::GetURIToShow(const std::wstring& uri)
{
    for (LPCWSTR page : { L"newtab", L"favorites", L"settings", L"history" })
    {
        std::wstring path = std::wstring(L"wvbrowser_ui\\content_ui\\") + page + L".html";
        if (uri.compare(GetFilePathAsURI(GetFullPathFor(path.c_str()))) == 0)
        {
            return std::wstring(L"browser://") + page;
        }
    }

    return L"";
}

HRESULT BrowserWindow::HandleTabHistoryUpdate(size_t tabId, ICoreWebView2* webview)
{
    wil::unique_cotaskmem_string source;
    RETURN_IF_FAILED(webview->get_Source(&source));
    
    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_UPDATE_URI);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
    jsonObj[L"args"][L"uri"] = web::json::value(source.get());

    std::wstring uriToShow = GetURIToShow(source.get());
    if (!uriToShow.empty())
    {
        jsonObj[L"args"][L"uriToShow"] = web::json::value(uriToShow);
    }

    BOOL canGoForward = FALSE;
    RETURN_IF_FAILED(webview->get_CanGoForward(&canGoForward));
    jsonObj[L"args"][L"canGoForward"] = web::json::value::boolean(canGoForward);

    BOOL canGoBack = FALSE;
    RETURN_IF_FAILED(webview->get_CanGoBack(&canGoBack));
    jsonObj[L"args"][L"canGoBack"] = web::json::value::boolean(canGoBack);

    RETURN_IF_FAILED(PostJsonToWebView(jsonObj, m_controlsWebView.Get()));

    return S_OK;
}

HRESULT BrowserWindow::HandleTabNavStarting(size_t tabId, ICoreWebView2* webview)
{
    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_NAV_STARTING);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

    return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
}

HRESULT BrowserWindow::HandleTabNavCompleted(size_t tabId, ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args)
{
    std::wstring getTitleScript(
        // Look for a title tag
        L"(() => {"
        L"    const titleTag = document.getElementsByTagName('title')[0];"
        L"    if (titleTag) {"
        L"        return titleTag.innerHTML;"
        L"    }"
        // No title tag, look for the file name
        L"    pathname = window.location.pathname;"
        L"    var filename = pathname.split('/').pop();"
        L"    if (filename) {"
        L"        return filename;"
        L"    }"
        // No file name, look for the hostname
        L"    const hostname =  window.location.hostname;"
        L"    if (hostname) {"
        L"        return hostname;"
        L"    }"
        // Fallback: let the UI use a generic title
        L"    return '';"
        L"})();"
    );

    std::wstring getFaviconURI(
        L"(() => {"
        // Let the UI use a fallback favicon
        L"    let faviconURI = '';"
        L"    let links = document.getElementsByTagName('link');"
        // Test each link for a favicon
        L"    Array.from(links).map(element => {"
        L"        let rel = element.rel;"
        // Favicon is declared, try to get the href
        L"        if (rel && (rel == 'shortcut icon' || rel == 'icon')) {"
        L"            if (!element.href) {"
        L"                return;"
        L"            }"
        // href to icon found, check it's full URI
        L"            try {"
        L"                let urlParser = new URL(element.href);"
        L"                faviconURI = urlParser.href;"
        L"            } catch(e) {"
        // Try prepending origin
        L"                let origin = window.location.origin;"
        L"                let faviconLocation = `${origin}/${element.href}`;"
        L"                try {"
        L"                    urlParser = new URL(faviconLocation);"
        L"                    faviconURI = urlParser.href;"
        L"                } catch (e2) {"
        L"                    return;"
        L"                }"
        L"            }"
        L"        }"
        L"    });"
        L"    return faviconURI;"
        L"})();"
    );

    CheckFailure(webview->ExecuteScript(getTitleScript.c_str(), Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [this, tabId](HRESULT error, PCWSTR result) -> HRESULT
    {
        RETURN_IF_FAILED(error);

        web::json::value jsonObj = web::json::value::parse(L"{}");
        jsonObj[L"message"] = web::json::value(MG_UPDATE_TAB);
        jsonObj[L"args"] = web::json::value::parse(L"{}");
        jsonObj[L"args"][L"title"] = web::json::value::parse(result);
        jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

        CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Can't update title.");
        return S_OK;
    }).Get()), L"Can't update title.");

    CheckFailure(webview->ExecuteScript(getFaviconURI.c_str(), Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [this, tabId](HRESULT error, PCWSTR result) -> HRESULT
    {
        RETURN_IF_FAILED(error);

        web::json::value jsonObj = web::json::value::parse(L"{}");
        jsonObj[L"message"] = web::json::value(MG_UPDATE_FAVICON);
        jsonObj[L"args"] = web::json::value::parse(L"{}");
        jsonObj[L"args"][L"uri"] = web::json::value::parse(result);
        jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

        CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Can't update favicon.");
        return S_OK;
    }).Get()), L"Can't update favicon");

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_NAV_COMPLETED);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

    BOOL navigationSucceeded = FALSE;
    if (SUCCEEDED(args->get_IsSuccess(&navigationSucceeded)))
    {
        jsonObj[L"args"][L"isError"] = web::json::value::boolean(!navigationSucceeded);
    }

    return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
}

// The lock in the address bar, from the page's address: WebView2 shows a page
// over HTTPS once its certificate is valid, or once the user goes on past the
// warning for a certificate error. The browser used to ask DevTools, keeping a
// DevTools session on every tab, and runtime 155 never sends the event it
// listened for: every page, HTTPS too, showed "Not secure".
HRESULT BrowserWindow::HandleTabSecurityUpdate(size_t tabId, ICoreWebView2* webview)
{
    wil::unique_cotaskmem_string source;
    RETURN_IF_FAILED(webview->get_Source(&source));

    // The browser's own pages, about:blank and the like.
    std::wstring state(L"neutral");
    DWORD scheme = URL_SCHEME_UNKNOWN;
    std::wstring host = GetHost(source.get(), &scheme);
    if (scheme == URL_SCHEME_HTTPS)
    {
        state = m_certificateErrorHosts.count(host) ? L"insecure-broken" : L"secure";
    }
    else if (scheme == URL_SCHEME_HTTP)
    {
        state = L"insecure";
    }

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_SECURITY_UPDATE);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
    jsonObj[L"args"][L"state"] = web::json::value(state);

    return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
}

HRESULT BrowserWindow::HandleTabCertificateError(size_t tabId, ICoreWebView2* webview, LPCWSTR uri)
{
    DWORD scheme = URL_SCHEME_UNKNOWN;
    m_certificateErrorHosts.insert(GetHost(uri, &scheme));

    return HandleTabSecurityUpdate(tabId, webview);
}

std::wstring BrowserWindow::GetHost(LPCWSTR uri, DWORD* scheme)
{
    std::wstring host;
    ComPtr<IUri> parsedUri;
    if (SUCCEEDED(CreateUri(uri, Uri_CREATE_CANONICALIZE, 0, &parsedUri)))
    {
        parsedUri->GetScheme(scheme);
        wil::unique_bstr hostName;
        if (SUCCEEDED(parsedUri->GetHost(&hostName)) && hostName)
        {
            host = std::wstring(hostName.get());
        }
    }

    return host;
}

// A page asked for a new window: a link that opens in one (target="_blank",
// Shift- or Ctrl-click), window.open() or "Open link in new window". WebView2
// would open a window of its own, outside the tab strip, that is never
// trimmed or suspended. The page opens in a new tab instead, which the
// toolbar adds: in front, or behind for a Ctrl-click, as in Edge.
HRESULT BrowserWindow::HandleTabNewWindowRequested(size_t tabId, ICoreWebView2NewWindowRequestedEventArgs* args)
{
    // Unless a tab is set as the window, window.open() gets null.
    RETURN_IF_FAILED(args->put_Handled(TRUE));

    // Edge's popup blocker is off in WebView2. A window the user didn't ask
    // for, e.g. one a page opens as it loads or from a timer, is blocked, as
    // the settings page says.
    BOOL userInitiated = FALSE;
    RETURN_IF_FAILED(args->get_IsUserInitiated(&userInitiated));
    if (!userInitiated)
    {
        return S_OK;
    }

    Tab::NewWindow newWindow = { args };
    RETURN_IF_FAILED(args->GetDeferral(&newWindow.deferral));
    m_newWindows.push_back(newWindow);

    bool behind = GetAsyncKeyState(VK_CONTROL) < 0 && GetAsyncKeyState(VK_SHIFT) >= 0;
    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_CREATE_TAB);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"active"] = web::json::value::boolean(!behind);
    jsonObj[L"args"][L"openerTabId"] = web::json::value::number(tabId);

    return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
}

// The toolbar closes the tab, as if the user had, and the host then closes it
// (see MG_CLOSE_TAB).
HRESULT BrowserWindow::HandleTabCloseRequested(size_t tabId)
{
    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_CLOSE_TAB);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

    return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
}

void BrowserWindow::HandleTabCreated(size_t tabId, bool shouldBeActive)
{
    if (shouldBeActive)
    {
        CheckFailure(SwitchToTab(tabId), L"");
    }
    else
    {
        // A tab opened behind lays its page out at the size it will show at,
        // and is hidden as other tabs behind are.
        CheckFailure(m_tabs.at(tabId)->ResizeWebView(), L"");
        CheckFailure(m_tabs.at(tabId)->SetVisible(false), L"");
    }
}

HRESULT BrowserWindow::HandleTabMessageReceived(size_t tabId, ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* eventArgs)
{
    wil::unique_cotaskmem_string jsonString;
    RETURN_IF_FAILED(eventArgs->get_WebMessageAsJson(&jsonString));
    web::json::value jsonObj = web::json::value::parse(jsonString.get());

    wil::unique_cotaskmem_string uri;
    RETURN_IF_FAILED(webview->get_Source(&uri));

    int message = jsonObj.at(L"message").as_integer();
    web::json::value args = jsonObj.at(L"args");

    wil::unique_cotaskmem_string source;
    RETURN_IF_FAILED(webview->get_Source(&source));

    switch (message)
    {
    case MG_GET_FAVORITES:
    case MG_REMOVE_FAVORITE:
    {
        std::wstring fileURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\favorites.html"));
        // Only the favorites UI can request favorites
        if (fileURI.compare(source.get()) == 0)
        {
            jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
            CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Couldn't perform favorites operation.");
        }
    }
    break;
    case MG_GET_SETTINGS:
    {
        std::wstring fileURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\settings.html"));
        // Only the settings UI can request settings
        if (fileURI.compare(source.get()) == 0)
        {
            jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
            CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Couldn't retrieve settings.");
        }
    }
    break;
    case MG_CLEAR_CACHE:
    {
        std::wstring fileURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\settings.html"));
        // Only the settings UI can request cache clearing
        if (fileURI.compare(uri.get()) == 0)
        {
            jsonObj[L"args"][L"content"] = web::json::value::boolean(false);
            jsonObj[L"args"][L"controls"] = web::json::value::boolean(false);

            if (SUCCEEDED(ClearContentCache()))
            {
                jsonObj[L"args"][L"content"] = web::json::value::boolean(true);
            }

            if (SUCCEEDED(ClearControlsCache()))
            {
                jsonObj[L"args"][L"controls"] = web::json::value::boolean(true);
            }

            CheckFailure(PostJsonToWebView(jsonObj, m_tabs.at(tabId)->m_contentWebView.Get()), L"");
        }
    }
    break;
    case MG_CLEAR_COOKIES:
    {
        std::wstring fileURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\settings.html"));
        // Only the settings UI can request cookies clearing
        if (fileURI.compare(uri.get()) == 0)
        {
            jsonObj[L"args"][L"content"] = web::json::value::boolean(false);
            jsonObj[L"args"][L"controls"] = web::json::value::boolean(false);

            if (SUCCEEDED(ClearContentCookies()))
            {
                jsonObj[L"args"][L"content"] = web::json::value::boolean(true);
            }


            if (SUCCEEDED(ClearControlsCookies()))
            {
                jsonObj[L"args"][L"controls"] = web::json::value::boolean(true);
            }

            CheckFailure(PostJsonToWebView(jsonObj, m_tabs.at(tabId)->m_contentWebView.Get()), L"");
        }
    }
    break;
    case MG_GET_HISTORY:
    case MG_REMOVE_HISTORY_ITEM:
    case MG_CLEAR_HISTORY:
    {
        std::wstring fileURI = GetFilePathAsURI(GetFullPathFor(L"wvbrowser_ui\\content_ui\\history.html"));
        // Only the history UI can request history
        if (fileURI.compare(uri.get()) == 0)
        {
            jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
            CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Couldn't perform history operation");
        }
    }
    break;
    default:
    {
        OutputDebugString(L"Unexpected message\n");
    }
    break;
    }

    return S_OK;
}

HRESULT BrowserWindow::ClearContentCache()
{
    return ClearBrowsingData(m_tabs.at(m_activeTabId)->m_contentWebView.Get(), COREWEBVIEW2_BROWSING_DATA_KINDS_DISK_CACHE);
}

HRESULT BrowserWindow::ClearControlsCache()
{
    return ClearBrowsingData(m_controlsWebView.Get(), COREWEBVIEW2_BROWSING_DATA_KINDS_DISK_CACHE);
}

HRESULT BrowserWindow::ClearContentCookies()
{
    return ClearBrowsingData(m_tabs.at(m_activeTabId)->m_contentWebView.Get(), COREWEBVIEW2_BROWSING_DATA_KINDS_COOKIES);
}

HRESULT BrowserWindow::ClearControlsCookies()
{
    return ClearBrowsingData(m_controlsWebView.Get(), COREWEBVIEW2_BROWSING_DATA_KINDS_COOKIES);
}

// Clears data of the WebView's profile through the profile rather than a
// DevTools session, which would then stay on the WebView. Older runtimes can't.
HRESULT BrowserWindow::ClearBrowsingData(ICoreWebView2* webview, COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds)
{
    ComPtr<ICoreWebView2_13> webview13;
    ComPtr<ICoreWebView2Profile> profile;
    ComPtr<ICoreWebView2Profile2> profile2;
    RETURN_IF_FAILED(webview->QueryInterface(IID_PPV_ARGS(&webview13)));
    RETURN_IF_FAILED(webview13->get_Profile(&profile));
    RETURN_IF_FAILED(profile.As(&profile2));

    return profile2->ClearBrowsingData(dataKinds, Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>(
        [](HRESULT result) -> HRESULT
    {
        return S_OK;
    }).Get());
}

HRESULT BrowserWindow::ResizeUIWebViews()
{
    if (m_controlsWebView != nullptr)
    {
        RECT bounds;
        GetClientRect(m_hWnd, &bounds);
        bounds.bottom = bounds.top + GetDPIAwareBound(c_uiBarHeight);
        bounds.bottom += 1;

        RETURN_IF_FAILED(m_controlsController->put_Bounds(bounds));
    }

    if (m_optionsWebView != nullptr)
    {
        RECT bounds;
        GetClientRect(m_hWnd, &bounds);
        bounds.top = GetDPIAwareBound(c_uiBarHeight);
        bounds.bottom = bounds.top + GetDPIAwareBound(c_optionsDropdownHeight);
        bounds.left = bounds.right - GetDPIAwareBound(c_optionsDropdownWidth);

        RETURN_IF_FAILED(m_optionsController->put_Bounds(bounds));
    }

    // Workaround for black controls WebView issue in Windows 7
    HWND wvWindow = GetWindow(m_hWnd, GW_CHILD);
    while (wvWindow != nullptr)
    {
        UpdateWindow(wvWindow);
        wvWindow = GetWindow(wvWindow, GW_HWNDNEXT);
    }

    return S_OK;
}

void BrowserWindow::UpdateMinWindowSize()
{
    RECT clientRect;
    RECT windowRect;

    GetClientRect(m_hWnd, &clientRect);
    GetWindowRect(m_hWnd, &windowRect);

    int bordersWidth = (windowRect.right - windowRect.left) - clientRect.right;
    int bordersHeight = (windowRect.bottom - windowRect.top) - clientRect.bottom;

    m_minWindowWidth = GetDPIAwareBound(MIN_WINDOW_WIDTH) + bordersWidth;
    m_minWindowHeight = GetDPIAwareBound(MIN_WINDOW_HEIGHT) + bordersHeight;
}

void BrowserWindow::CheckFailure(HRESULT hr, LPCWSTR errorMessage)
{
    if (FAILED(hr))
    {
        std::wstring message;
        if (!errorMessage || !errorMessage[0])
        {
            message = std::wstring(L"Something went wrong.");
        }
        else
        {
            message = std::wstring(errorMessage);
        }

        MessageBoxW(nullptr, message.c_str(), nullptr, MB_OK);
    }
}

int BrowserWindow::GetDPIAwareBound(int bound)
{
    // Remove the GetDpiForWindow call when using Windows 7 or any version
    // below 1607 (Windows 10). You will also have to make sure the build
    // directory is clean before building again.
    return (bound * GetDpiForWindow(m_hWnd) / DEFAULT_DPI);
}

std::wstring BrowserWindow::GetAppDataDirectory()
{
    TCHAR path[MAX_PATH];
    std::wstring dataDirectory;
    HRESULT hr = SHGetFolderPath(nullptr, CSIDL_APPDATA, NULL, 0, path);
    if (SUCCEEDED(hr))
    {
        dataDirectory = std::wstring(path);
        dataDirectory.append(L"\\Microsoft\\");
    }
    else
    {
        dataDirectory = std::wstring(L".\\");
    }

    dataDirectory.append(s_title);
    return dataDirectory;
}

std::wstring BrowserWindow::GetFullPathFor(LPCWSTR relativePath)
{
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(m_hInst, path, MAX_PATH);
    std::wstring pathName(path);

    std::size_t index = pathName.find_last_of(L"\\") + 1;
    pathName.replace(index, pathName.length(), relativePath);

    return pathName;
}

std::wstring BrowserWindow::GetFilePathAsURI(std::wstring fullPath)
{
    std::wstring fileURI;
    ComPtr<IUri> uri;
    DWORD uriFlags = Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME;
    HRESULT hr = CreateUri(fullPath.c_str(), uriFlags, 0, &uri);

    if (SUCCEEDED(hr))
    {
        wil::unique_bstr absoluteUri;
        uri->GetAbsoluteUri(&absoluteUri);
        fileURI = std::wstring(absoluteUri.get());
    }

    return fileURI;
}

HRESULT BrowserWindow::PostJsonToWebView(web::json::value jsonObj, ICoreWebView2* webview)
{
    utility::stringstream_t stream;
    jsonObj.serialize(stream);

    return webview->PostWebMessageAsJson(stream.str().c_str());
}
