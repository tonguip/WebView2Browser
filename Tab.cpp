// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "BrowserWindow.h"
#include "Tab.h"

using namespace Microsoft::WRL;

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
        [this, shouldBeActive](HRESULT result, ICoreWebView2Controller* host) -> HRESULT {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Tab WebView creation failed\n");
            return result;
        }
        m_contentController = host;
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
            BrowserWindow::CheckFailure(browserWindow->HandleTabNavCompleted(m_tabId, webview, args), L"Can't udpate reload button");
            return S_OK;
        }).Get(), &m_navCompletedToken));

        // Enable listening for security events to update secure icon
        RETURN_IF_FAILED(m_contentWebView->CallDevToolsProtocolMethod(L"Security.enable", L"{}", nullptr));

        BrowserWindow::CheckFailure(m_contentWebView->GetDevToolsProtocolEventReceiver(L"Security.securityStateChanged", &m_securityStateChangedReceiver), L"");

        // Forward security status updates to browser
        RETURN_IF_FAILED(m_securityStateChangedReceiver->add_DevToolsProtocolEventReceived(Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
            [this, browserWindow](ICoreWebView2* webview, ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabSecurityUpdate(m_tabId, webview, args), L"Can't udpate security icon");
            return S_OK;
        }).Get(), &m_securityUpdateToken));

        SetTrackingPrevention();

        // A new tab shows a page of the browser's own, which loads at once
        // and keeps little memory, until the user says where to go. A web
        // page there would load, and keep a renderer busy, for nothing.
        std::wstring newTabPath = browserWindow->GetFullPathFor(L"wvbrowser_ui\\content_ui\\newtab.html");
        RETURN_IF_FAILED(m_contentWebView->Navigate(newTabPath.c_str()));
        browserWindow->HandleTabCreated(m_tabId, shouldBeActive);

        return S_OK;
    }).Get());
}

void Tab::SetMessageBroker()
{
    m_messageBroker = Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [this](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* eventArgs) -> HRESULT
    {
        BrowserWindow* browserWindow = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
        BrowserWindow::CheckFailure(browserWindow->HandleTabMessageReceived(m_tabId, webview, eventArgs), L"");

        return S_OK;
    });
}

// Pages load ad and tracker frames from many sites, each in a renderer
// process of its own: on news sites they take more memory than the page.
// Strict tracking prevention, as in Edge, blocks most of them. Runtimes
// before 115 keep the default, Balanced.
void Tab::SetTrackingPrevention()
{
    ComPtr<ICoreWebView2_13> webview13;
    ComPtr<ICoreWebView2Profile> profile;
    ComPtr<ICoreWebView2Profile3> profile3;
    if (SUCCEEDED(m_contentWebView.As(&webview13)) && SUCCEEDED(webview13->get_Profile(&profile))
        && SUCCEEDED(profile.As(&profile3)))
    {
        profile3->put_PreferredTrackingPreventionLevel(COREWEBVIEW2_TRACKING_PREVENTION_LEVEL_STRICT);
    }
}

Tab::~Tab()
{
    KillTimer(m_parentHWnd, m_tabId);
}

bool Tab::IsTrimmed() const
{
    return *m_memory == Memory::Low || *m_memory == Memory::Settled || *m_memory == Memory::Suspended;
}

// Shows or hides the tab. Once hidden for c_trimAfterMs, WebView2 keeps its
// memory low, as Edge does for background tabs, and later suspends it, like
// a sleeping tab. Its timer is named by the tab ID and arrives as WM_TIMER in
// BrowserWindow.
HRESULT Tab::SetVisible(bool visible)
{
    RETURN_IF_FAILED(m_contentController->put_IsVisible(visible));

    if (visible)
    {
        KillTimer(m_parentHWnd, m_tabId);
        // WebView2 resumes a suspended tab as it shows.
        if (IsTrimmed())
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

// The tab is likely to show soon, e.g. the pointer is on it in the tab strip:
// it gets its memory back and resumes now rather than as it shows. It is
// trimmed again if it stays hidden.
void Tab::Wake()
{
    if (!IsTrimmed())
    {
        return;
    }

    // Also for a TrySuspend still under way.
    ComPtr<ICoreWebView2_3> webview3;
    if (SUCCEEDED(m_contentWebView.As(&webview3)))
    {
        webview3->Resume();
    }
    BrowserWindow::SetMemoryUsage(m_contentWebView.Get(), COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_NORMAL);
    *m_memory = Memory::Hidden;
    SetTimer(m_parentHWnd, m_tabId, c_trimAfterMs, nullptr);
}

void Tab::OnHiddenTimer()
{
    KillTimer(m_parentHWnd, m_tabId);

    if (*m_memory == Memory::Hidden)
    {
        // A page playing sound keeps its memory, and is looked at again later.
        ComPtr<ICoreWebView2_8> webview8;
        BOOL playing = FALSE;
        if (SUCCEEDED(m_contentWebView.As(&webview8)))
        {
            webview8->get_IsDocumentPlayingAudio(&playing);
        }
        if (playing)
        {
            SetTimer(m_parentHWnd, m_tabId, c_trimAfterMs, nullptr);
            return;
        }

        BrowserWindow::SetMemoryUsage(m_contentWebView.Get(), COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_LOW);
        *m_memory = Memory::Low;
        SetTimer(m_parentHWnd, m_tabId, c_retrimAfterMs, nullptr);
    }
    else if (*m_memory == Memory::Low)
    {
        // Through Normal, so that WebView2 sees the level change and trims.
        BrowserWindow::SetMemoryUsage(m_contentWebView.Get(), COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_NORMAL);
        BrowserWindow::SetMemoryUsage(m_contentWebView.Get(), COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_LOW);
        *m_memory = Memory::Settled;
        SetTimer(m_parentHWnd, m_tabId, c_suspendAfterMs, nullptr);
    }
    else if (*m_memory == Memory::Settled)
    {
        ComPtr<ICoreWebView2_3> webview3;
        if (FAILED(m_contentWebView.As(&webview3)))
        {
            return;
        }

        // A page WebView2 won't suspend (a call, a camera) is asked again later.
        std::weak_ptr<Memory> memory = m_memory;
        HWND hWnd = m_parentHWnd;
        size_t tabId = m_tabId;
        webview3->TrySuspend(Callback<ICoreWebView2TrySuspendCompletedHandler>(
            [memory, hWnd, tabId](HRESULT errorCode, BOOL isSuccessful) -> HRESULT
        {
            std::shared_ptr<Memory> state = memory.lock();
            // Unless the tab closed or showed meanwhile.
            if (state && *state == Memory::Settled)
            {
                if (SUCCEEDED(errorCode) && isSuccessful)
                {
                    *state = Memory::Suspended;
                }
                else
                {
                    SetTimer(hWnd, tabId, c_suspendAfterMs, nullptr);
                }
            }
            return S_OK;
        }).Get());
    }
}

HRESULT Tab::ResizeWebView()
{
    RECT bounds;
    GetClientRect(m_parentHWnd, &bounds);

    BrowserWindow* browserWindow = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
    bounds.top += browserWindow->GetDPIAwareBound(BrowserWindow::c_uiBarHeight);

    return m_contentController->put_Bounds(bounds);
}
