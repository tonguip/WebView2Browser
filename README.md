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

WebView2Browser has a multi-WebView approach to integrate web content and application UI into a Windows Desktop application. This allows the browser to use standard web technologies (HTML, CSS, JavaScript) to light up the interface but also enables the app to fetch favicons from the web and use IndexedDB for storing favorites and history.

The multi-WebView approach uses one WebView environment with two profiles: UI WebViews (controls and options dropdown) use the `BrowserUI` profile, while web content WebViews (one per tab) use the default profile. Each profile keeps its own cookies, cache and storage, so the UI stays isolated from web content, but all WebViews share one browser process, GPU process and network service. (Earlier versions used a second environment for the UI, which started a second set of those processes; the UI's favorites and history move to the new profile on first run.)

![Browser layout](https://raw.githubusercontent.com/MicrosoftEdge/WebView2Browser/master/screenshots/layout.png)

## Features

WebView2Browser provides all the functionalities to make a basic web browser, but there's plenty of room for you to play around.

* Go back/forward
* Reload page
* Cancel navigation
* Multiple tabs
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
ICoreWebView2DevToolsProtocolEventReceivedEventHandler | Used along with add_DevToolsProtocolEventReceived to listen for CDP security events to update the lock icon in the browser UI. |
ICoreWebView2DevToolsProtocolEventReceiver | Used along with add_DevToolsProtocolEventReceived to listen for CDP security events to update the lock icon in the browser UI. |
ICoreWebView2ExecuteScriptCompletedHandler | Used along with ExecuteScript to get the title and favicon from the visited page. |
ICoreWebView2FocusChangedEventHandler | Used along with add_LostFocus to hide the browser options dropdown when it loses focus.
ICoreWebView2HistoryChangedEventHandler | Used along with add_HistoryChanged to update the navigation buttons in the browser UI. |
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
ExecuteScript | Used to get the title and favicon of a visited page.
PostWebMessageAsJson | Used to communicate WebViews. All messages use JSON to pass parameters needed.
add_WebMessageReceived | Used to handle web messages posted to the WebView.
CallDevToolsProtocolMethod | Used to enable listening for security events, which will notify of security status changes in a document.
put_MemoryUsageTargetLevel | Used to keep the memory of hidden tabs and the hidden options dropdown low.
TrySuspend | Used to suspend tabs that have been hidden for five minutes.
get_Profile | Used with ICoreWebView2Profile3::put_PreferredTrackingPreventionLevel to block most trackers in tabs.

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
  * [Keeping hidden tabs light](#keeping-hidden-tabs-light)
  * [Updating the security icon](#updating-the-security-icon)
  * [Populating the history](#populating-the-history)
* [Handling JSON and URIs](#handling-json-and-uris)

## The basics

### Set up the environment, create a WebView

WebView2 allows you to host web content in your Windows app. It exposes the globals [CreateCoreWebView2Environment](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/webview2-idl#createcorewebview2environment) and [CreateCoreWebView2EnvironmentWithOptions](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/webview2-idl#createcorewebview2environmentwithoptions) from which we create the one environment for the browser's UI and content.

When a discrete GPU (NVIDIA or AMD) is installed, `GetBrowserArguments` finds it with DXGI and passes `--force-high-performance-gpu`, so on a laptop with an integrated GPU as well, Chromium draws pages on the discrete one.

```cpp
    // Get directory for user data. The browser UI keeps its data apart from
    // the tabs' in a profile of its own (c_uiProfileName).
    std::wstring userDataDirectory = GetAppDataDirectory();
    userDataDirectory.append(L"\\User Data");
    MoveBrowserUIData(userDataDirectory);

    auto options = Make<CoreWebView2EnvironmentOptions>();
    std::wstring browserArguments = GetBrowserArguments();
    if (!browserArguments.empty())
    {
        options->put_AdditionalBrowserArguments(browserArguments.c_str());
    }

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
                if (path.compare(L"favorites") == 0 ||
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

WebView2Browser will check the URI against browser pages (i.e. favorites, settings, history) and navigate to the requested location or use the provided URI to search Bing as a fallback.

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
            CheckFailure(m_tabs.at(m_activeTabId)->m_contentWebView->CallDevToolsProtocolMethod(L"Page.stopLoading", L"{}", nullptr), L"");
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

        RETURN_IF_FAILED(m_contentWebView->Navigate(L"https://www.bing.com"));
        browserWindow->HandleTabCreated(m_tabId, shouldBeActive);

        return S_OK;
    }).Get());
}
```

The tab registers all handlers so it can forward updates to the controls WebView when events fire. The tab is ready and will be shown on the content area of the browser. Clicking on a tab in the controls WebView will post a message to the host application, which will in turn hide the WebView for the previously active tab and show the one for the clicked tab.

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

### Keeping hidden tabs light

`Tab::SetVisible` shows or hides the tab's WebView and, as Edge does for background tabs, lowers what a hidden tab keeps in memory in two steps. A tab hidden for 30 seconds gets [MemoryUsageTargetLevel](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2_19#put_memoryusagetargetlevel) `LOW`: Chromium frees the page's caches and garbage and lets Windows page out the rest. Five minutes after it was hidden, it is suspended with [TrySuspend](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2_3#trysuspend), which pauses its scripts, timers and animations like a sleeping tab. Showing it again sets the level back to `NORMAL`, and WebView2 resumes a suspended WebView as it becomes visible. A tab playing sound keeps its memory, and WebView2 refuses to suspend pages that can't sleep (a call, a camera), which are asked again later. The timers are window timers named by the tab ID, handled as `WM_TIMER` in `BrowserWindow`.

```cpp
HRESULT Tab::SetVisible(bool visible)
{
    RETURN_IF_FAILED(m_contentController->put_IsVisible(visible));

    if (visible)
    {
        KillTimer(m_parentHWnd, m_tabId);
        // WebView2 resumes a suspended tab as it shows.
        if (*m_memory == Memory::Low || *m_memory == Memory::Suspended)
        {
            BrowserWindow::SetMemoryUsage(m_contentWebView.Get(), COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_NORMAL);
        }
        *m_memory = Memory::Normal;
    }
    else if (*m_memory == Memory::Normal)
    {
        *m_memory = Memory::Hidden;
        SetTimer(m_parentHWnd, m_tabId, c_trimAfterMs, nullptr);
    }

    return S_OK;
}
```

Waking a sleeping tab takes a moment, so it starts before the click: when the pointer enters a tab in the strip, the controls WebView posts `MG_WAKE_TAB`, and `Tab::Wake` resumes the tab and gives it its memory back. If it isn't clicked, it is trimmed again 30 seconds later.

The options dropdown, hidden most of the time, keeps its memory low while it is hidden too.

### Blocking trackers

Chromium runs each site in a renderer process of its own, and that includes the frames a page embeds from other sites. On news sites most of those frames are ad and tracker frames, each site in its own process: loading eight common sites (Wikipedia, GitHub, YouTube, Reddit, BBC, The Verge, Stack Overflow, CNN) made 83 such frames and 39 renderer processes. `Tab::SetTrackingPrevention` sets the tabs' profile to [Strict tracking prevention](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2profile3#put_preferredtrackingpreventionlevel), as Edge offers it, which blocks most of them: the same sites then make 4 such frames and 13 renderers, and take about 1 GB less committed memory. Pages draw less too. As in Edge, a site that needs a blocked tracker may lose that part of the page.

### Updating the security icon

We use the [CallDevToolsProtocolMethod](https://learn.microsoft.com/microsoft-edge/webview2/reference/win32/icorewebview2#calldevtoolsprotocolmethod) to enable listening for security events. Whenever a `securityStateChanged` event is fired, we will use the new state to update the security icon on the controls WebView.

```cpp
        // Enable listening for security events to update secure icon
        RETURN_IF_FAILED(m_contentWebView->CallDevToolsProtocolMethod(L"Security.enable", L"{}", nullptr));

        BrowserWindow::CheckFailure(m_contentWebView->GetDevToolsProtocolEventReceiver(L"Security.securityStateChanged", &m_securityStateChangedReceiver), L"");

        // Forward security status updates to browser
        RETURN_IF_FAILED(m_securityStateChangedReceiver->add_DevToolsProtocolEventReceived(Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
            [this, browserWindow](ICoreWebView2* webview, ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabSecurityUpdate(m_tabId, webview, args), L"Can't update security icon");
            return S_OK;
        }).Get(), &m_securityUpdateToken));
```

```cpp
HRESULT BrowserWindow::HandleTabSecurityUpdate(size_t tabId, ICoreWebView2* webview, ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args)
{
    wil::unique_cotaskmem_string jsonArgs;
    RETURN_IF_FAILED(args->get_ParameterObjectAsJson(&jsonArgs));
    web::json::value securityEvent = web::json::value::parse(jsonArgs.get());

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_SECURITY_UPDATE);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
    jsonObj[L"args"][L"state"] = securityEvent.at(L"securityState");

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

### Populating the history

WebView2Browser uses IndexedDB in the controls WebView to store history items, just an example of how WebView2 enables you to access standard web technologies as you would in the browser. The item for a navigation will be created as soon as the URI is updated. These items are then retrieved by the history UI in a tab making use of `window.chrome.postMessage`.

In this case, most functionality is implemented using JavaScript on both ends (controls WebView and content WebView loading the UI) so the host application is only acting as a message broker to communicate those ends.

```javascript
        case commands.MG_UPDATE_URI:
            if (isValidTabId(args.tabId)) {
                // ...

                // Don't add history entry if URI has not changed
                if (tab.uri == previousURI) {
                    break;
                }

                // Filter URIs that should not appear in history
                if (!tab.uri || tab.uri == 'about:blank') {
                    tab.historyItemId = INVALID_HISTORY_ID;
                    break;
                }

                if (tab.uriToShow && tab.uriToShow.substring(0, 10) == 'browser://') {
                    tab.historyItemId = INVALID_HISTORY_ID;
                    break;
                }

                addHistoryItem(historyItemFromTab(args.tabId), (id) => {
                    tab.historyItemId = id;
                });
            }
            break;
```

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
