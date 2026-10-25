// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "BrowserWindow.h"
#include "Tab.h"

using namespace Microsoft::WRL;

std::unique_ptr<Tab> Tab::CreateNewTab(HWND hWnd, ICoreWebView2Environment* env, size_t id, bool shouldBeActive, NewWindow newWindow)
{
    std::unique_ptr<Tab> tab = std::make_unique<Tab>();

    tab->m_parentHWnd = hWnd;
    tab->m_tabId = id;
    tab->SetMessageBroker();
    tab->Init(env, shouldBeActive, newWindow);

    return tab;
}

HRESULT Tab::Init(ICoreWebView2Environment* env, bool shouldBeActive, NewWindow newWindow)
{
    return env->CreateCoreWebView2Controller(m_parentHWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this, shouldBeActive, newWindow](HRESULT result, ICoreWebView2Controller* host) -> HRESULT {
        // The page that asked for the window waits for this tab's WebView. If
        // the tab isn't set as the window, its window.open() gets null.
        auto completeNewWindow = wil::scope_exit([&newWindow]
        {
            if (newWindow.deferral)
            {
                newWindow.deferral->Complete();
            }
        });

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
            BrowserWindow::CheckFailure(browserWindow->HandleTabSecurityUpdate(m_tabId, webview), L"Can't update security icon");

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

        // The title as soon as the page has one, and each time it changes it
        RETURN_IF_FAILED(m_contentWebView->add_DocumentTitleChanged(Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
            [this, browserWindow](ICoreWebView2* webview, IUnknown* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabTitleUpdate(m_tabId, webview), L"Can't update title.");
            return S_OK;
        }).Get(), &m_titleChangedToken));

        // The favicon each time the page has another one. Older runtimes
        // show the default one.
        ComPtr<ICoreWebView2_15> webview15;
        if (SUCCEEDED(m_contentWebView.As(&webview15)))
        {
            RETURN_IF_FAILED(webview15->add_FaviconChanged(Callback<ICoreWebView2FaviconChangedEventHandler>(
                [this, browserWindow](ICoreWebView2* webview, IUnknown* args) -> HRESULT
            {
                BrowserWindow::CheckFailure(browserWindow->HandleTabFaviconUpdate(m_tabId, webview), L"Can't update favicon.");
                return S_OK;
            }).Get(), &m_faviconChangedToken));
        }

        // A certificate error shows the host's pages as not secure, also once
        // the user goes on past the warning. Older runtimes don't say.
        ComPtr<ICoreWebView2_14> webview14;
        if (SUCCEEDED(m_contentWebView.As(&webview14)))
        {
            RETURN_IF_FAILED(webview14->add_ServerCertificateErrorDetected(Callback<ICoreWebView2ServerCertificateErrorDetectedEventHandler>(
                [this, browserWindow](ICoreWebView2* webview, ICoreWebView2ServerCertificateErrorDetectedEventArgs* args) -> HRESULT
            {
                wil::unique_cotaskmem_string uri;
                RETURN_IF_FAILED(args->get_RequestUri(&uri));
                BrowserWindow::CheckFailure(browserWindow->HandleTabCertificateError(m_tabId, webview, uri.get()), L"Can't update security icon");
                return S_OK;
            }).Get(), &m_certificateErrorToken));
        }

        RETURN_IF_FAILED(m_contentWebView->add_ProcessFailed(Callback<ICoreWebView2ProcessFailedEventHandler>(
            [this](ICoreWebView2* webview, ICoreWebView2ProcessFailedEventArgs* args) -> HRESULT
        {
            COREWEBVIEW2_PROCESS_FAILED_KIND kind;
            RETURN_IF_FAILED(args->get_ProcessFailedKind(&kind));
            if (kind == COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_EXITED)
            {
                OnRendererExited();
            }
            return S_OK;
        }).Get(), &m_processFailedToken));

        RETURN_IF_FAILED(m_contentWebView->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [this, browserWindow](ICoreWebView2* webview, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabNewWindowRequested(m_tabId, args), L"Can't open a new tab");
            return S_OK;
        }).Get(), &m_newWindowToken));

        // A page window.open() opened may close itself, e.g. a sign-in page
        // once the user is in: its tab closes.
        RETURN_IF_FAILED(m_contentWebView->add_WindowCloseRequested(Callback<ICoreWebView2WindowCloseRequestedEventHandler>(
            [this, browserWindow](ICoreWebView2* webview, IUnknown* args) -> HRESULT
        {
            BrowserWindow::CheckFailure(browserWindow->HandleTabCloseRequested(m_tabId), L"Can't close tab");
            return S_OK;
        }).Get(), &m_windowCloseToken));

        SetTrackingPrevention();

        if (newWindow.args)
        {
            // The page loads here, and window.open() gives the page that
            // asked this tab's window.
            RETURN_IF_FAILED(newWindow.args->put_NewWindow(m_contentWebView.Get()));
        }
        else
        {
            // A new tab shows a page of the browser's own, which loads at
            // once and keeps little memory, until the user says where to go.
            // A web page there would load, and keep a renderer busy, for
            // nothing.
            std::wstring newTabPath = browserWindow->GetFullPathFor(L"wvbrowser_ui\\content_ui\\newtab.html");
            RETURN_IF_FAILED(m_contentWebView->Navigate(newTabPath.c_str()));
        }
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

// The tab is likely to show soon, e.g. the pointer is on it in the tab strip:
// it gets its memory back and resumes now rather than as it shows. It is
// trimmed again if it stays hidden.
void Tab::Wake()
{
    if (!IsTrimmed())
    {
        return;
    }

    // Also for a TrySuspend still under way. Resuming a suspended tab sets the
    // level back to NORMAL; a tab that is only low is set back here.
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

    // Its renderer ended: there's no page to trim until it shows.
    if (m_reloadWhenShown)
    {
        return;
    }

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
        // WebView2's documentation advises using TrySuspend and Resume or the
        // memory level, not both. A tab uses the level only while it isn't
        // suspended, when WebView2 would ignore it: TrySuspend sets LOW
        // itself, and showing or resuming the tab sets NORMAL at once. Each
        // alone keeps more: with LOW only, the pages run on and take memory
        // back; suspended only, a tab keeps all of it for five minutes.
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

// The tab's renderer process ended: it crashed, ran out of memory or was
// ended from outside, and the page is gone. Once there are enough renderers
// that tabs of a site share one (see BrowserWindow::GetBrowserArguments), each
// of those tabs gets this. The tab in front loads its page again now, a hidden
// one as it shows, so they don't all load at once in the background.
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

void Tab::ReloadAfterCrash()
{
    m_reloadWhenShown = false;
    m_reloadedAfterCrashAt = GetTickCount64();
    m_contentWebView->Reload();
}

HRESULT Tab::ResizeWebView()
{
    RECT bounds;
    GetClientRect(m_parentHWnd, &bounds);

    BrowserWindow* browserWindow = reinterpret_cast<BrowserWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
    bounds.top += browserWindow->GetDPIAwareBound(BrowserWindow::c_uiBarHeight);

    return m_contentController->put_Bounds(bounds);
}
