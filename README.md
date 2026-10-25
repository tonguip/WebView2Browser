---
description: "A web browser built with the Microsoft Edge WebView2 control."
extendedZipContent: 
  - 
    path: LICENSE
    target: LICENSE
languages: 
  - cpp
  - javascript
page_type: sample
products: 
  - microsoft-edge
urlFragment: webview2browser
---
# WebView2Browser

A web browser built with the [Microsoft Edge WebView2](https://aka.ms/webview2) control.

![WebView2Browser](https://raw.githubusercontent.com/MicrosoftEdge/WebView2Browser/master/screenshots/WebView2Browser.png)

WebView2Browser is a sample Windows desktop application demonstrating the WebView2 control capabilities. It is built as a Win32 [Visual Studio 2019](https://visualstudio.microsoft.com/vs/) project and makes use of both C++ and JavaScript in the WebView2 environment to power its features.

WebView2Browser shows some of the simplest uses of WebView2 -such as creating and navigating a WebView, but also some more complex workflows like using the [PostWebMessageAsJson API](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2#postwebmessageasjson) to communicate WebViews in separate profiles. It is intended as a rich code sample to look at how you can use WebView2 APIs to build your own app.

For more information, see the article in the Microsoft Edge Developer documentation: [WebView2Browser](https://learn.microsoft.com/microsoft-edge/webview2/samples/webview2browser).

## Requisites

* [Microsoft Edge (Chromium)](https://www.microsoftedgeinsider.com/download/) installed on a supported OS.
* [Visual Studio](https://visualstudio.microsoft.com/vs/) with C++ support installed.

The [WebView2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/) is recommended for the installation and the minimum version is 86.0.622.38.

## Build the browser

Clone the repository and open the solution in Visual Studio. WebView2 is already included as a NuGet package* in the project!

* Clone this repository
* Open the solution in Visual Studio 2019**
* Make the changes listed below if you're using a Windows version below Windows 10.
* Set the target you want to build (Debug/Release, x86/x64)
* Build the solution

That's it. Everything should be ready to just launch the app.

*You can get the WebView2 NuGet Package through the Visual Studio NuGet Package Manager.  
**You can also use Visual Studio 2017 by changing the project's Platform Toolset in Project Properties/Configuration properties/General/Platform Toolset. You might also need to change the Windows SDK to the latest version available to you.

## Using versions below Windows 10

There's a couple of changes you need to make if you want to build and run the browser in other versions of Windows. This is because of how DPI is handled in Windows 10 vs previous versions of Windows.

In `WebViewBrowserApp.cpp`, you will need to replace the call to `SetProcessDpiAwarenessContext` and call `SetProcessDPIAware` instead.

```cpp
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Call SetProcessDPIAware() instead when using Windows 7 or any version
    // below 1703 (Windows 10).
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    BrowserWindow::RegisterClass(hInstance);

    // ...
```

In `BrowserWindow.cpp`, you will need to remove the call to `GetDpiForWindow`.

```cpp
int BrowserWindow::GetDPIAwareBound(int bound)
{
    // Remove the GetDpiForWindow call when using Windows 7 or any version
    // below 1607 (Windows 10). You will also have to make sure the build
    // directory is clean before building again.
    return (bound * GetDpiForWindow(m_hWnd) / DEFAULT_DPI);
}
```

## Browser layout

WebView2Browser has a multi-WebView approach to integrate web content and application UI into a Windows Desktop application. This allows the browser to use standard web technologies (HTML, CSS, JavaScript) to light up the interface but also enables the app to use IndexedDB for storing favorites and history.

The multi-WebView approach uses one WebView environment with two profiles: UI WebViews (controls and options dropdown) use the `BrowserUI` profile, while web content WebViews (one per tab) use the default profile. Each profile keeps its own cookies, cache and storage, so the UI stays isolated from web content, but all WebViews share one browser process, GPU process and network service. (Earlier versions used a second environment for the UI, which started a second set of those processes; the UI's favorites and history move to the new profile on first run.)

![Browser layout](https://raw.githubusercontent.com/MicrosoftEdge/WebView2Browser/master/screenshots/layout.png)

## Features

WebView2Browser provides all the functionalities to make a basic web browser, but there's plenty of room for you to play around.

* Go back/forward
* Reload page
* Cancel navigation
* Multiple tabs
* Opening new windows as tabs, blocking popups
* History
* Favorites
* Search from the address bar
* Page security status
* Clearing cache and cookies

## WebView2 APIs

WebView2Browser makes use of a handful of the APIs available in WebView2. For the APIs not used here, you can find more about them in the [Microsoft Edge WebView2 Reference](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32). The following is a list of the most interesting APIs WebView2Browser uses and the feature(s) they enable.

API | Feature(s)
:--- | :---
CreateCoreWebView2EnvironmentWithOptions | Used to create the one environment for UI and content WebViews, asking for the high performance GPU when a discrete one is installed. |
ICoreWebView2ControllerOptions | Used with CreateCoreWebView2ControllerWithOptions to put the UI WebViews in a profile of their own, isolated from web content. |
ICoreWebView2 | There are several WebViews in WebView2Browser and most features make use of members in this interface, the table below shows how they're used.
ICoreWebView2ServerCertificateErrorDetectedEventHandler | Used along with add_ServerCertificateErrorDetected to show pages of a host with a certificate error as not secure. |
ICoreWebView2DocumentTitleChangedEventHandler | Used along with add_DocumentTitleChanged to show the page's title in its tab and history entry as soon as it has one. |
ICoreWebView2FaviconChangedEventHandler | Used along with add_FaviconChanged to show the page's favicon in the address bar and its history entry. |
ICoreWebView2GetFaviconCompletedHandler | Used along with GetFavicon to get the favicon's image from the tab, which has loaded it already. |
ICoreWebView2FocusChangedEventHandler | Used along with add_LostFocus to hide the browser options dropdown when it loses focus.
ICoreWebView2HistoryChangedEventHandler | Used along with add_HistoryChanged to update the navigation buttons in the browser UI. |
ICoreWebView2NewWindowRequestedEventArgs | Used with put_NewWindow and GetDeferral to open the window a page asks for in a new tab, and with get_IsUserInitiated to block popups the user didn't ask for. |
ICoreWebView2Controller | There are several WebViewControllers in WebView2Browser and we fetch the associated WebViews from them.
ICoreWebView2NavigationCompletedEventHandler | Used along with add_NavigationCompleted to update the reload button in the browser UI.
ICoreWebView2Settings | Used to disable DevTools in the browser UI.
ICoreWebView2SourceChangedEventHandler | Used along with add_SourceChanged to update the address bar in the browser UI. |
ICoreWebView2WebMessageReceivedEventHandler | This is one of the most important APIs to WebView2Browser. Most functionalities involving communication across WebViews use this.

ICoreWebView2 API | Feature(s)
:--- | :---
add_NavigationStarting | Used to display the cancel navigation button in the controls WebView.
add_SourceChanged | Used to update the address bar.
add_HistoryChanged | Used to update go back/forward buttons.
add_NavigationCompleted | Used to display the reload button once a navigation completes.
add_DocumentTitleChanged | Used with get_DocumentTitle to update the tab's title and history entry each time the page's title changes.
add_FaviconChanged | Used with get_FaviconUri and GetFavicon to show the page's favicon without loading it again.
PostWebMessageAsJson | Used to communicate WebViews. All messages use JSON to pass parameters needed.
add_WebMessageReceived | Used to handle web messages posted to the WebView.
Stop | Used to cancel a navigation.
add_ServerCertificateErrorDetected | Used to show pages of a host with a certificate error as not secure.
put_MemoryUsageTargetLevel | Used to keep the memory of hidden tabs and the hidden options dropdown low.
TrySuspend | Used to suspend tabs that have been hidden for five minutes.
add_ProcessFailed | Used to load a tab's page again when its renderer process ends.
add_NewWindowRequested | Used to open the windows pages ask for in new tabs.
add_WindowCloseRequested | Used to close a tab when its page closes its window.
get_Profile | Used with ICoreWebView2Profile3::put_PreferredTrackingPreventionLevel to block most trackers in tabs, and with ICoreWebView2Profile2::ClearBrowsingData to clear the cache and cookies.

ICoreWebView2Controller API | Feature(s)
:--- | :---
get_CoreWebView2 | Used to get the CoreWebView2 associated with this CoreWebView2Controller.
add_LostFocus | Used to hide the options dropdown when the user clicks away of it.

## Implementing the features

The sections below describe how some of the features in WebView2Browser were implemented. You can look at the source code for more details about how everything works here.

* [The basics](#the-basics)
  * [Set up the environment, create a WebView](#set-up-the-environment-create-a-webview)
  * [Navigate to web page](#navigate-to-web-page)
  * [Updating the address bar](#updating-the-address-bar)
  * [Going back, going forward](#going-back-going-forward)
* [Some interesting features](#some-interesting-features)
  * [Communicating the WebViews](#communicating-the-webviews)
  * [Tab handling](#tab-handling)
  * [Opening new windows as tabs](#opening-new-windows-as-tabs)
  * [Keeping hidden tabs light](#keeping-hidden-tabs-light)
  * [Blocking trackers](#blocking-trackers)
  * [Sharing renderer processes](#sharing-renderer-processes)
  * [Recovering from a renderer crash](#recovering-from-a-renderer-crash)
  * [Updating the security icon](#updating-the-security-icon)
  * [Populating the history](#populating-the-history)
* [Handling JSON and URIs](#handling-json-and-uris)

## The basics

### Set up the environment, create a WebView

WebView2 allows you to host web content in your Windows app. It exposes the globals [CreateCoreWebView2Environment](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/webview2-idl#createcorewebview2environment) and [CreateCoreWebView2EnvironmentWithOptions](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/webview2-idl#createcorewebview2environmentwithoptions) from which we create the one environment for the browser's UI and content.

`GetBrowserArguments` passes Chromium a renderer process limit (see [Sharing renderer processes](#sharing-renderer-processes)). When a discrete GPU (NVIDIA or AMD) is installed, `HasDiscreteGpu` finds it with DXGI and `--force-high-performance-gpu` is added, so on a laptop with an integrated GPU as well, Chromium draws pages on the discrete one.

```cpp
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
```

The UI WebViews are created in their own profile:

```cpp
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
```

We use the [ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2createcorewebview2environmentcompletedhandler) to create the UI WebViews once the environment is ready.

```cpp
HRESULT BrowserWindow::CreateBrowserControlsWebView()
{
    return CreateUIController(Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
    {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Controls WebView creation failed\n");
            return result;
        }
        // WebView created
        m_controlsController = controller;
        CheckFailure(m_controlsController->get_CoreWebView2(&m_controlsWebView), L"");

        wil::com_ptr<ICoreWebView2Settings> settings;
        RETURN_IF_FAILED(m_controlsWebView->get_Settings(&settings));
        RETURN_IF_FAILED(settings->put_AreDevToolsEnabled(FALSE));

        RETURN_IF_FAILED(m_controlsController->add_ZoomFactorChanged(Callback<ICoreWebView2ZoomFactorChangedEventHandler>(
            [](ICoreWebView2Controller* controller, IUnknown* args) -> HRESULT
        {
            controller->put_ZoomFactor(1.0);
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
```

We're setting up a few things here. The [ICoreWebView2Settings](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2settings) interface is used to disable DevTools in the WebView powering the browser controls. We're also adding a handler for received web messages. This handler will enable us to do something when the user interacts with the controls in this WebView.

### Navigate to web page

You can navigate to a web page by entering its URI in the address bar. When pressing Enter, the controls WebView will post a web message to the host app so it can navigate the active tab to the specified location. Code below shows how the host Win32 application will handle that message.

```cpp
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
```

WebView2Browser will check the URI against browser pages (i.e. new tab, favorites, settings, history) and navigate to the requested location or use the provided URI to search Bing as a fallback.

### Updating the address bar

The address bar is updated every time there is a change in the active tab's document source and along with other controls when switching tabs. Each WebView will fire an event when the state of the document changes, we can use this event to get the new source on updates and forward the change to the controls WebView (we'll also update the go back and go forward buttons).

```cpp
        // Register event handler for doc state change
        RETURN_IF_FAILED(m_contentWebView->add_SourceChanged(Callback<ICoreWebView2SourceChangedEventHandler>(
            [this, browserWindow](ICoreWebView2* webview, ICoreWebView2SourceChangedEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabURIUpdate(m_tabId, webview), L"Can't update address bar");

            return S_OK;
        }).Get(), &m_uriUpdateForwarderToken));
```

```cpp
HRESULT BrowserWindow::HandleTabURIUpdate(size_t tabId, ICoreWebView2* webview)
{
    wil::unique_cotaskmem_string source;
    RETURN_IF_FAILED(webview->get_Source(&source));

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_UPDATE_URI);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
    jsonObj[L"args"][L"uri"] = web::json::value(source.get());

    // ...

    RETURN_IF_FAILED(PostJsonToWebView(jsonObj, m_controlsWebView.Get()));

    return S_OK;
}

HRESULT BrowserWindow::HandleTabHistoryUpdate(size_t tabId, ICoreWebView2* webview)
{
    // ...

    BOOL canGoForward = FALSE;
    RETURN_IF_FAILED(webview->get_CanGoForward(&canGoForward));
    jsonObj[L"args"][L"canGoForward"] = web::json::value::boolean(canGoForward);

    BOOL canGoBack = FALSE;
    RETURN_IF_FAILED(webview->get_CanGoBack(&canGoBack));
    jsonObj[L"args"][L"canGoBack"] = web::json::value::boolean(canGoBack);

    RETURN_IF_FAILED(PostJsonToWebView(jsonObj, m_controlsWebView.Get()));

    return S_OK;
}
```

We have sent the `MG_UPDATE_URI` message along with the URI to the controls WebView. Now we want to reflect those changes on the tab state and update the UI if necessary.

```javascript
        case commands.MG_UPDATE_URI:
            if (isValidTabId(args.tabId)) {
                const tab = tabs.get(args.tabId);
                let previousURI = tab.uri;

                // Update the tab state
                tab.uri = args.uri;
                tab.uriToShow = args.uriToShow;
                tab.canGoBack = args.canGoBack;
                tab.canGoForward = args.canGoForward;

                // If the tab is active, update the controls UI
                if (args.tabId == activeTabId) {
                    updateNavigationUI(message);
                }

                // ...
            }
            break;
```

### Going back, going forward

Each WebView will keep a history for the navigations it has performed so we only need to connect the browser UI with the corresponding methods. If the active tab's WebView can be navigated back/forward, the buttons will post a web message to the host application when clicked.

The JavaScript side:

```javascript
    document.querySelector('#btn-forward').addEventListener('click', function(e) {
        if (document.getElementById('btn-forward').className === 'btn') {
            var message = {
                message: commands.MG_GO_FORWARD,
                args: {}
            };
            window.chrome.webview.postMessage(message);
        }
    });

    document.querySelector('#btn-back').addEventListener('click', function(e) {
        if (document.getElementById('btn-back').className === 'btn') {
            var message = {
                message: commands.MG_GO_BACK,
                args: {}
            };
            window.chrome.webview.postMessage(message);
        }
    });
```

The host application side:

```cpp
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
```

### Reloading, stop navigation

We use the `NavigationStarting` event fired by a content WebView to update its associated tab loading state in the controls WebView. Similarly, when a WebView fires the `NavigationCompleted` event, we use that event to instruct the controls WebView to update the tab state. The active tab state in the controls WebView will determine whether to show the reload or the cancel button. Each of those will post a message back to the host application when clicked, so that the WebView for that tab can be reloaded or have its navigation canceled, accordingly.

```javascript
function reloadActiveTabContent() {
    var message = {
        message: commands.MG_RELOAD,
        args: {}
    };
    window.chrome.webview.postMessage(message);
}

 // ...

    document.querySelector('#btn-reload').addEventListener('click', function(e) {
        var btnReload = document.getElementById('btn-reload');
        if (btnReload.className === 'btn-cancel') {
            var message = {
                message: commands.MG_CANCEL,
                args: {}
            };
            window.chrome.webview.postMessage(message);
        } else if (btnReload.className === 'btn') {
            reloadActiveTabContent();
        }
    });
```

```cpp
        case MG_RELOAD:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->Reload(), L"");
        }
        break;
        case MG_CANCEL:
        {
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->Stop(), L"");
        }
```

## Some interesting features

### Communicating the WebViews

We need to communicate the WebViews powering tabs and UI so that user interactions in one have the desired effect in the other. WebView2Browser makes use of set of very useful WebView2 APIs for this purpose, including [PostWebMessageAsJson](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2#postwebmessageasjson), [add_WebMessageReceived](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2#add_webmessagereceived) and [ICoreWebView2WebMessageReceivedEventHandler](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2webmessagereceivedeventhandler). On the JavaScript side, we're making use of the `window.chrome.webview` object exposed to call the `postMessage` method and add an event lister for received messages.

```cpp
HRESULT BrowserWindow::CreateBrowserControlsWebView()
{
    return m_uiEnv->CreateCoreWebView2Controller(m_hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
    {
        // ...

        RETURN_IF_FAILED(m_controlsWebView->add_WebMessageReceived(m_uiMessageBroker.Get(), &m_controlsUIMessageBrokerToken));

        // ...

        return S_OK;
    }).Get());
}
```

```cpp
HRESULT BrowserWindow::PostJsonToWebView(web::json::value jsonObj, ICoreWebView2* webview)
{
    utility::stringstream_t stream;
    jsonObj.serialize(stream);

    return webview->PostWebMessageAsJson(stream.str().c_str());
}

// ...

HRESULT BrowserWindow::HandleTabNavStarting(size_t tabId, ICoreWebView2* webview)
{
    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_NAV_STARTING);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);

    return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
}
```

```javascript
function init() {
    window.chrome.webview.addEventListener('message', messageHandler);
    refreshControls();
    refreshTabs();

    createNewTab(true);
}

// ...

function reloadActiveTabContent() {
    var message = {
        message: commands.MG_RELOAD,
        args: {}
    };
    window.chrome.webview.postMessage(message);
}
```

### Tab handling

A new tab will be created whenever the user clicks on the new tab button to the right of the open tabs. The controls WebView will post a message to the host application to create the WebView for that tab and create an object tracking its state.

```javascript
function createNewTab(shouldBeActive) {
    const tabId = getNewTabId();

    var message = {
        message: commands.MG_CREATE_TAB,
        args: {
            tabId: parseInt(tabId),
            active: shouldBeActive || false
        }
    };

    window.chrome.webview.postMessage(message);

    tabs.set(parseInt(tabId), {
        title: 'New Tab',
        uri: '',
        uriToShow: '',
        favicon: 'img/favicon.png',
        isFavorite: false,
        isLoading: false,
        canGoBack: false,
        canGoForward: false,
        securityState: 'unknown',
        historyItemId: INVALID_HISTORY_ID
    });

    loadTabUI(tabId);

    if (shouldBeActive) {
        switchToTab(tabId, false);
    }
}
```

On the host app side, the registered [ICoreWebView2WebMessageReceivedEventHandler](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2webmessagereceivedeventhandler) will catch the message and create the WebView for that tab.

```cpp
        case MG_CREATE_TAB:
        {
            size_t id = args.at(L"tabId").as_number().to_uint32();
            bool shouldBeActive = args.at(L"active").as_bool();
            std::unique_ptr<Tab> newTab = Tab::CreateNewTab(m_hWnd, m_contentEnv.Get(), id, shouldBeActive);

            std::map<size_t, std::unique_ptr<Tab>>::iterator it = m_tabs.find(id);
            if (it == m_tabs.end())
            {
                m_tabs.insert(std::pair<size_t,std::unique_ptr<Tab>>(id, std::move(newTab)));
            }
            else
            {
                m_tabs.at(id)->m_contentWebView->Close();
                it->second = std::move(newTab);
            }
        }
        break;
```

```cpp
std::unique_ptr<Tab> Tab::CreateNewTab(HWND hWnd, ICoreWebView2Environment* env, size_t id, bool shouldBeActive)
{
    std::unique_ptr<Tab> tab = std::make_unique<Tab>();

    tab->m_parentHWnd = hWnd;
    tab->m_tabId = id;
    tab->SetMessageBroker();
    tab->Init(env, shouldBeActive);

    return tab;
}

HRESULT Tab::Init(ICoreWebView2Environment* env, bool shouldBeActive)
{
    return env->CreateCoreWebView2Controller(m_parentHWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this, shouldBeActive](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Tab WebView creation failed\n");
            return result;
        }
        m_contentController = controller;
        BrowserWindow::CheckFailure(m_contentController->get_CoreWebView2(&m_contentWebView), L"");
        BrowserWindow* browserWindow = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
        RETURN_IF_FAILED(m_contentWebView->add_WebMessageReceived(m_messageBroker.Get(), &m_messageBrokerToken));

        // Register event handler for history change
        RETURN_IF_FAILED(m_contentWebView->add_HistoryChanged(Callback<ICoreWebView2HistoryChangedEventHandler>(
            [this, browserWindow](ICoreWebView2* webview, IUnknown* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabHistoryUpdate(m_tabId, webview), L"Can't update go back/forward buttons.");

            return S_OK;
        }).Get(), &m_historyUpdateForwarderToken));

        // Register event handler for source change
        RETURN_IF_FAILED(m_contentWebView->add_SourceChanged(Callback<ICoreWebView2SourceChangedEventHandler>(
            [this, browserWindow](ICoreWebView2* webview, ICoreWebView2SourceChangedEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabURIUpdate(m_tabId, webview), L"Can't update address bar");

            return S_OK;
        }).Get(), &m_uriUpdateForwarderToken));

        RETURN_IF_FAILED(m_contentWebView->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
            [this, browserWindow](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabNavStarting(m_tabId, webview), L"Can't update reload button");

            return S_OK;
        }).Get(), &m_navStartingToken));

        RETURN_IF_FAILED(m_contentWebView->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this, browserWindow](ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabNavCompleted(m_tabId, webview, args), L"Can't update reload button");
            return S_OK;
        }).Get(), &m_navCompletedToken));

        // Handle security state updates

        std::wstring newTabPath = browserWindow->GetFullPathFor(L"wvbrowser_ui\\content_ui\\newtab.html");
        RETURN_IF_FAILED(m_contentWebView->Navigate(newTabPath.c_str()));
        browserWindow->HandleTabCreated(m_tabId, shouldBeActive);

        return S_OK;
    }).Get());
}
```

The tab registers all handlers so it can forward updates to the controls WebView when events fire. It opens on `browser://newtab`, a blank page of the browser's own, with nothing in the address bar to type over. A web page there, like Bing's start page, would load in every new tab before the user says where to go, and keep a renderer of 110 to 160 MB while the tab stays on it; the local page loads at once in a few MB. Search from the address bar still goes to Bing. The tab is ready and will be shown on the content area of the browser. Clicking on a tab in the controls WebView will post a message to the host application, which will in turn hide the WebView for the previously active tab and show the one for the clicked tab.

```cpp
HRESULT BrowserWindow::SwitchToTab(size_t tabId)
{
    size_t previousActiveTab = m_activeTabId;

    RETURN_IF_FAILED(m_tabs.at(tabId)->ResizeWebView());
    RETURN_IF_FAILED(m_tabs.at(tabId)->SetVisible(true));
    m_activeTabId = tabId;

    if (previousActiveTab != INVALID_TAB_ID && previousActiveTab != m_activeTabId)
    {
        RETURN_IF_FAILED(m_tabs.at(previousActiveTab)->SetVisible(false));
    }

    return S_OK;
}
```

### Opening new windows as tabs

A page asks for a new window with a link that opens in one (`target="_blank"`, a Shift- or Ctrl-click), with `window.open()`, or through "Open link in new window" in its context menu. Left to itself, WebView2 opens a window of its own for it, outside the tab strip and never hidden, so it is never trimmed or suspended and its memory escapes [Keeping hidden tabs light](#keeping-hidden-tabs-light). Instead, the tab handles [NewWindowRequested](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2#add_newwindowrequested): `BrowserWindow::HandleTabNewWindowRequested` takes a deferral, keeps the request and posts `MG_CREATE_TAB` to the controls WebView, which adds a tab as the new tab button does and posts `MG_CREATE_TAB` back. The new tab's WebView is then given to the request with [put_NewWindow](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2newwindowrequestedeventargs#put_newwindow), in place of the new tab page, and the deferral completes.

So the page loads in a tab, and `window.open()` gives the page that asked that tab's window, to script and to message as it would a window. A page that closes its window, as sign-in pages do once the user is in, raises `WindowCloseRequested`: its tab closes, and the tab that opened it comes back to the front. The tab opens in front, or behind for a Ctrl-click, as in Edge, and a tab behind is hidden at once like any other. Edge's popup blocker is off in WebView2, so a window the user didn't ask for, one a page opens as it loads or from a timer, is blocked: `window.open()` gets `null`, as the settings page says.

Three Wikipedia articles opened with `window.open()` from a page, then back to that page: as windows, 212-233 MB private working set; as tabs, 131-140 MB, and one window rather than four. The pages stay loaded either way, so committed memory is the same, about 360 MB, until the tabs sleep.

```cpp
HRESULT BrowserWindow::HandleTabNewWindowRequested(size_t tabId, ICoreWebView2NewWindowRequestedEventArgs* args)
{
    // Unless a tab is set as the window, window.open() gets null.
    RETURN_IF_FAILED(args->put_Handled(TRUE));

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
```

### Keeping hidden tabs light

`Tab::SetVisible` shows or hides the tab's WebView and, as Edge does for background tabs, lowers what a hidden tab keeps in memory in steps. A tab hidden for 10 seconds gets [MemoryUsageTargetLevel](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2_19#put_memoryusagetargetlevel) `LOW`: Chromium frees the page's caches and garbage and lets Windows page out the rest. Showing a low tab costs a few milliseconds, so even tabs the user flips between don't keep all their memory. A page that has just loaded is still fetching and running scripts and takes much of it back, so 20 seconds later the tab is trimmed again, the level going to `NORMAL` and back to `LOW`. Five minutes after it was hidden, it is suspended with [TrySuspend](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2_3#trysuspend), which pauses its scripts, timers and animations like a sleeping tab. Showing it again sets the level back to `NORMAL`, and WebView2 resumes a suspended WebView as it becomes visible. A tab playing sound keeps its memory, and WebView2 refuses to suspend pages that can't sleep (a call, a camera), which are asked again later. The timers are window timers named by the tab ID, handled as `WM_TIMER` in `BrowserWindow`.

```cpp
HRESULT Tab::SetVisible(bool visible)
{
    RETURN_IF_FAILED(m_contentController->put_IsVisible(visible));

    if (visible)
    {
        KillTimer(m_parentHWnd, m_tabId);
        // WebView2 resumes a suspended tab as it shows, which sets the level
        // back to NORMAL; a tab that is only low is set back here.
        if (IsTrimmed())
        {
            BrowserWindow::SetMemoryUsage(m_contentWebView.Get(), COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_NORMAL);
        }
        *m_memory = Memory::Normal;

        if (m_reloadWhenShown)
        {
            ReloadAfterCrash();
        }
    }
    else if (*m_memory == Memory::Normal)
    {
        *m_memory = Memory::Hidden;
        SetTimer(m_parentHWnd, m_tabId, c_trimAfterMs, nullptr);
    }

    return S_OK;
}
```

Waking a sleeping tab takes a moment, so it starts before the click: when the pointer enters a tab in the strip, the controls WebView posts `MG_WAKE_TAB`, and `Tab::Wake` resumes the tab and gives it its memory back. If it isn't clicked, it is trimmed again 10 seconds later.

WebView2's documentation advises using either `TrySuspend` and `Resume` or `MemoryUsageTargetLevel`, not both. A hidden tab uses both, one after the other, and never sets the level while it is suspended, when WebView2 would ignore it: `TrySuspend` sets `LOW` itself, WebView2 sets `NORMAL` again as soon as a suspended tab shows or is resumed, and `SetVisible` and `Wake` set it back for a tab that is only low. On WebView2 155 and 156, all 34 suspends from `LOW` succeeded, and every tab came back at `NORMAL`, drawing and running its timers at full rate, even when shown just as it was being suspended. Each API alone keeps more. With eight sites hidden, 1, 4 and 7 minutes after the last tab switch (one run on each runtime):

- Both: 532-555, 802-830 and 377-378 MB private working set.
- `LOW` only: 509-547, 774-779 and 787-789 MB. The pages run on and take memory back, and used 0.9-1.3 s of renderer CPU in the last two minutes, against 0.1 s.
- `TrySuspend` only: 1,132-1,166, 1,075-1,122 and 320-402 MB.

Committed memory is about the same in all three, 1.46-1.57 GB, as hidden pages are paged out rather than freed. A suspended tab draws its first two frames a median 80-119 ms after it is shown, a low one 61-62 ms.

The options dropdown, hidden most of the time, keeps its memory low while it is hidden too.

A minimized window shows nothing, so `BrowserWindow::SetMinimized`, called on `WM_SIZE`, hides its active tab as if it were in the background, which then keeps little memory and sleeps in time like any other, and stops the controls WebView drawing. Restoring the window shows both again.

### Blocking trackers

Chromium runs each site in a renderer process of its own, and that includes the frames a page embeds from other sites. On news sites most of those frames are ad and tracker frames, each site in its own process: loading eight common sites (Wikipedia, GitHub, YouTube, Reddit, BBC, The Verge, Stack Overflow, CNN) made 83 such frames and 39 renderer processes. `Tab::SetTrackingPrevention` sets the tabs' profile to [Strict tracking prevention](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2profile3#put_preferredtrackingpreventionlevel), as Edge offers it, which blocks most of them: the same sites then make 4 such frames and 13 renderers, and take about 1 GB less committed memory. Pages draw less too. As in Edge, a site that needs a blocked tracker may lose that part of the page.

### Sharing renderer processes

Chromium runs every tab in a renderer process of its own, however many tabs there are. Chrome puts up to two tabs of one site in a process, but that doesn't happen in WebView2, so ten tabs on four sites took ten renderers, each with its own copy of the engine. `GetBrowserArguments` passes `--renderer-process-limit` (the browser UI's two WebViews included): once there are that many renderers, a new tab or frame goes into an existing process of its site, while a new site still gets a process of its own. Sites stay isolated from each other. `GetRendererProcessLimit` sets the limit from the memory installed, one renderer for every 2 GB, from 4 to 16: 4 with 8 GB, 8 with 16 GB, 16 with 32 GB.

Below the limit, every tab keeps its process, so a background tab busy with scripts can't slow the tab in front. Above it, it can if they are of one site, until the background tab goes to sleep (see [Keeping hidden tabs light](#keeping-hidden-tabs-light)). Ten tabs on Wikipedia, GitHub, Reddit and Stack Overflow, switched every 3 seconds: 14 renderers and 402-409 MB private working set without a limit, 10 renderers and 352 MB with 8, peaking at 414 MB rather than 538-565. Switching tabs and the first scroll take as long, and a new tab of a site already open can load faster, its process being up already.

The same ten tabs with other limits, 90 seconds of switching each (one run each, on a 16 GB machine):

| Limit | Renderers | Private working set, mean and peak | Committed, mean | Tab switch, median |
| --- | --- | --- | --- | --- |
| 4 | 8 | 332 and 361 MB | 962 MB | 65 ms |
| 6 | 9 | 324 and 360 MB | 990 MB | 66 ms |
| 8 | 10 | 347 and 392 MB | 1,037 MB | 56 ms |
| 12 | 12 | 360 and 413 MB | 1,155 MB | 59 ms |

Each renderer fewer saves 40-60 MB of committed memory but little working set, as hidden tabs are paged out anyway, and with four sites a limit below 8 can't take away more than two. Switching back to the tabs after a minute hidden took a median 47-69 ms at every limit. An 8 GB machine was stood in for by holding 4.6 GB of RAM in another process, leaving about 1.4 GB available, as with a few other apps open: tab switches then took a median 77 and 80 ms at limits 4 and 6, and 101 ms at 8. This is untested on real 8 GB hardware, and Chromium's own memory-based sizes (GPU caches, saved frames) only step down below 4 GB, so it runs the same on 8 GB as on 16 GB until memory runs low.

### What the GPU process keeps

The GPU process draws for every WebView. With the eight sites above open and used, it had 221-226 MB private working set, and 351-356 MB of the GPU's own memory and 83-84 MB of shared memory. Chromium's memory dumps (`Tracing.requestMemoryDump` over the DevTools protocol) show what it holds:

- Its heap, 145-168 MB, of which the Windows heap, mostly the graphics driver's, is 36-40 MB.
- Caches: Skia's scratch textures (23 MB) and images the pages uploaded (23 MB), which it frees once the tabs are hidden.
- For every tab, hidden or not, the swap chain its WebView draws into, and a hidden tab's last frame. Nine tabs held 43 MB of swap chains and about 60 MB of last frames with the window at its default size, a 1904x644 page, and twice the swap chains and 1.6 times the frames at 2560x1040.

With all tabs hidden, Chromium's critical memory pressure (`Memory.simulatePressureNotification`) took the GPU process from 144-154 MB to 95-102 MB, but none of hiding, `LOW`, `TrySuspend`, memory pressure or Chromium's `EvictionUnlocksResources` and `ScalableFrameEviction` features freed the swap chains and last frames. Each hidden tab keeps about 20 MB of GPU memory at the default window size. The tile budget Chromium gives the tab in front on desktop, 512 MB, is far above what pages used (up to 71 MB), so lowering it with `--force-gpu-mem-available-mb` wouldn't save anything.

With a discrete GPU, that is memory on the graphics card. Integrated graphics have no memory of their own, so on a laptop without a discrete GPU it comes out of RAM: about 30 MB for each hidden tab with a full HD window, by the same measure. That is untested, as is a laptop with both kinds of GPU, where `--force-high-performance-gpu` keeps it on the discrete GPU but keeps that GPU powered. Only closing a hidden tab's WebView frees it, which a browser for small machines could do with tabs hidden for long, loading them again when shown, as Chrome's Memory Saver does.

### Recovering from a renderer crash

When a tab's renderer process ends, because it crashed, ran out of memory or was ended from outside, the page is gone until the tab is reloaded. Once tabs of a site share a process, that takes several tabs at once. WebView2 raises `ProcessFailed` with `COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_EXITED` for each, and `Tab::OnRendererExited` loads the page again: the tab in front at once, a hidden tab when it is shown, so that the tabs of a crashed process don't all load in the background together. If the page in front ends its renderer again within 30 seconds of being loaded again, it is left for the user to reload, as the page itself is then likely the cause.

```cpp
void Tab::OnRendererExited()
{
    BOOL visible = FALSE;
    m_contentController->get_IsVisible(&visible);
    if (!visible)
    {
        m_reloadWhenShown = true;
        return;
    }

    // It ended again soon after it was loaded again in front: the page may
    // well be what ends it, and it's left for the user to reload. A hidden
    // tab is loaded again in any case, as the tab in front sharing its
    // process is the likelier cause.
    if (m_reloadedAfterCrashAt && GetTickCount64() - m_reloadedAfterCrashAt < c_crashLoopMs)
    {
        return;
    }

    ReloadAfterCrash();
}
```

### Updating the security icon

The lock in the address bar follows the page's address: WebView2 shows a page over HTTPS once its certificate is valid, or once the user goes on past the warning for a certificate error, which `ServerCertificateErrorDetected` reports. So an HTTPS page is secure unless its host had a certificate error, an HTTP page is not secure, and the browser's own pages are neutral. `HandleTabSecurityUpdate` works this out when a tab's source changes and posts it to the controls WebView.

The browser used to listen for DevTools' `Security.securityStateChanged` instead, through [CallDevToolsProtocolMethod](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2#calldevtoolsprotocolmethod). Runtime 155 never sent that event, so the lock showed "Not secure" on every page, HTTPS too, and every tab kept a DevTools session. No tab uses DevTools now: cancelling a navigation calls `Stop`, and clearing the cache and cookies goes through the profile's `ClearBrowsingData`.

```cpp
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
```

```javascript
        case commands.MG_SECURITY_UPDATE:
            if (isValidTabId(args.tabId)) {
                const tab = tabs.get(args.tabId);
                tab.securityState = args.state;

                if (args.tabId == activeTabId) {
                    updateNavigationUI(message);
                }
            }
            break;
```

### Showing the favicon

The address bar shows the favicon the tab itself loaded. `FaviconChanged` fires as each new page starts, when it has no favicon yet, and again once the tab has the page's icon (or the one at the site's root), or when a script changes it. `HandleTabFaviconUpdate` gets the image from the tab with `GetFavicon` and posts it to the controls WebView as a data URI, with the icon's address. History and favorites keep the address, as before.

The toolbar used to ask the page for the icon's address once it had loaded, and load the icon itself. In its own profile, with its own cache and connections, that was a second download of every icon and a second connection to every site visited. Over 14 popular sites the toolbar made 14 requests, 75 KB, over 12 new connections, and makes none now. Local test sites got each icon twice and get it once, over 22 to 24 connections instead of 32 to 34. The icon shows a median of 40 to 55 ms after the page loads, where it took 125 to 270 ms. An icon a script sets now shows, and a tab whose icon comes while it's hidden puts it in its history entry. A page left within some 30 ms of loading, before the tab has its icon, keeps the default icon in history; the toolbar used to load it, and 4 or 5 of 90 times put it on the next page's entry.

`GetFavicon` gives the icon at 16 by 16 pixels, also with the display scaled to 200%, and the address bar shows it at that size. The toolbar used to show the site's own icon, often 32 to 64 pixels, scaled down: on a screen at 100% the two look alike, while at 200% the icon is now softer.

```cpp
HRESULT BrowserWindow::HandleTabFaviconUpdate(size_t tabId, ICoreWebView2* webview)
{
    ComPtr<ICoreWebView2_15> webview15;
    RETURN_IF_FAILED(webview->QueryInterface(IID_PPV_ARGS(&webview15)));

    wil::unique_cotaskmem_string uri;
    RETURN_IF_FAILED(webview15->get_FaviconUri(&uri));
    std::wstring faviconUri(uri.get());

    // Without an image the tab shows the default favicon
    auto postFavicon = [this, tabId, faviconUri](const std::vector<unsigned char>& image) -> HRESULT
    {
        // ...
        jsonObj[L"args"][L"uri"] = web::json::value(faviconUri);
        if (!image.empty())
        {
            jsonObj[L"args"][L"image"] = web::json::value(L"data:image/png;base64," + utility::conversions::to_base64(image));
        }

        return PostJsonToWebView(jsonObj, m_controlsWebView.Get());
    };

    // Each new page has none until it gives one, or until WebView2 finds the
    // one at the site's root. GetFavicon would give WebView2's own default.
    if (faviconUri.empty())
    {
        return postFavicon({});
    }

    return webview15->GetFavicon(COREWEBVIEW2_FAVICON_IMAGE_FORMAT_PNG, Callback<ICoreWebView2GetFaviconCompletedHandler>(
        [webview15, faviconUri, postFavicon](HRESULT error, IStream* stream) -> HRESULT
    {
        // ...
        return postFavicon(image);
    }).Get());
}
```

```javascript
function updateTabFavicon(tabId, uri, image) {
    let tab = tabs.get(tabId);
    tab.favicon = image ? uri : 'img/favicon.png';
    tab.faviconImage = image || 'img/favicon.png';

    if (tabId == activeTabId) {
        updateNavigationUI(commands.MG_UPDATE_FAVICON);
    }

    // Update favicon in history item
    updateTabHistoryItem(tabId);
}
```

### Populating the history

WebView2Browser uses IndexedDB in the controls WebView to store history items, just an example of how WebView2 enables you to access standard web technologies as you would in the browser. The item for a navigation will be created as soon as the URI is updated. These items are then retrieved by the history UI in a tab making use of `window.chrome.postMessage`. `queryDB` opens the database once and keeps the connection for the queries that follow, several for each page loaded.

In this case, most functionality is implemented using JavaScript on both ends (controls WebView and content WebView loading the UI) so the host application is only acting as a message broker to communicate those ends.

```javascript
        case commands.MG_UPDATE_URI:
            if (isValidTabId(args.tabId)) {
                // ...

                // Don't add history entry if URI has not changed
                if (tab.uri == previousURI) {
                    break;
                }

                // The previous page's entry is done with
                tab.historyItemId = INVALID_HISTORY_ID;
                tab.addingHistoryItem = null;

                // Filter URIs that should not appear in history
                if (!tab.uri || tab.uri == 'about:blank') {
                    break;
                }

                if (tab.uriToShow && tab.uriToShow.substring(0, 10) == 'browser://') {
                    break;
                }

                // The page's title and favicon, while its entry is being
                // added, go in once it has an ID (see updateTabHistoryItem)
                const adding = { item: historyItemFromTab(args.tabId), update: null };
                tab.addingHistoryItem = adding;
                addHistoryItem(adding.item, (id) => {
                    // Unless the tab went on to another page meanwhile
                    if (tab.addingHistoryItem == adding) {
                        tab.addingHistoryItem = null;
                        tab.historyItemId = id;
                    }

                    if (adding.update) {
                        updateHistoryItem(id, adding.update);
                    }
                });
            }
            break;
```

The entry takes the page's title from `DocumentTitleChanged`, which fires as soon as the page has a title and again each time the page changes it. The title used to be asked of the page once it finished loading, so a page the user left before then kept the title of the page before it: going from a Wikipedia article to GitHub and on before GitHub finished loading left a GitHub entry titled "Chromium (web browser) - Wikipedia". The title now comes a few milliseconds after the address, often before the entry has been added. Until then the tab only has the previous page's entry ID, and updating that entry would replace the previous page's entry with this one. `updateTabHistoryItem` keeps the update for the entry being added instead:

```javascript
function updateTabHistoryItem(tabId) {
    if (!isValidTabId(tabId)) {
        return;
    }

    const tab = tabs.get(tabId);
    if (tab.historyItemId != INVALID_HISTORY_ID) {
        updateHistoryItem(tab.historyItemId, historyItemFromTab(tabId));
    } else if (tab.addingHistoryItem) {
        tab.addingHistoryItem.update = historyItemFromTab(tabId);
    }
}
```

Going from a Wikipedia article to GitHub and on to example.com, the GitHub entry had Wikipedia's title in 6 of 7 tries and has GitHub's in 7 of 7 now. With pages that go on to the next by themselves within 0 to 300 ms, 3 to 5 of 216 entries had the previous page's title, and none do now. With adds made 30 ms slower, as on a busy disk, 3 of 216 pages had lost their entry to the next page's, and none do now.

```javascript
function addHistoryItem(item, callback) {
    queryDB((db) => {
        let transaction = db.transaction(['history'], 'readwrite');
        let historyStore = transaction.objectStore('history');

        // Check if an item for this URI exists on this day
        let currentDate = new Date();
        let year = currentDate.getFullYear();
        let month = currentDate.getMonth();
        let date = currentDate.getDate();
        let todayDate = new Date(year, month, date);

        let existingItemsIndex = historyStore.index('stampedURI');
        let lowerBound = [item.uri, todayDate];
        let upperBound = [item.uri, currentDate];
        let range = IDBKeyRange.bound(lowerBound, upperBound);
        let request = existingItemsIndex.openCursor(range);

        request.onsuccess = function(event) {
            let cursor = event.target.result;
            if (cursor) {
                // There's an entry for this URI, update the item
                cursor.value.timestamp = item.timestamp;
                let updateRequest = cursor.update(cursor.value);

                updateRequest.onsuccess = function(event) {
                    if (callback) {
                        callback(event.target.result.primaryKey);
                    }
                };
            } else {
                // No entry for this URI, add item
                let addItemRequest = historyStore.add(item);

                addItemRequest.onsuccess = function(event) {
                    if (callback) {
                        callback(event.target.result);
                    }
                };
            }
        };

    });
}
```

## Handling JSON and URIs

WebView2Browser uses Microsoft's [cpprestsdk (Casablanca)](https://github.com/Microsoft/cpprestsdk) to handle all JSON in the C++ side of things. IUri and CreateUri are also used to parse file paths into URIs and can be used to for other URIs as well.

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
