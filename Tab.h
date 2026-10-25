// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "framework.h"

class Tab
{
public:
    // A window a page asked for, which opens in a new tab (see
    // BrowserWindow::HandleTabNewWindowRequested).
    struct NewWindow
    {
        Microsoft::WRL::ComPtr<ICoreWebView2NewWindowRequestedEventArgs> args;
        Microsoft::WRL::ComPtr<ICoreWebView2Deferral> deferral;
    };

    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_contentController;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_contentWebView;

    static std::unique_ptr<Tab> CreateNewTab(HWND hWnd, ICoreWebView2Environment* env, size_t id, bool shouldBeActive, NewWindow newWindow = {});
    ~Tab();
    HRESULT ResizeWebView();
    HRESULT SetVisible(bool visible);
    void Wake();
    void OnHiddenTimer();
protected:
    // How long a tab stays hidden before WebView2 is asked to keep its memory
    // low. Showing a low tab again costs a few milliseconds, so tabs the user
    // flips between needn't keep all of theirs.
    static const UINT c_trimAfterMs = 10 * 1000;
    // How long it then stays low before it is trimmed again: a page that just
    // loaded is still fetching and running scripts, and takes much of its
    // memory back after the first trim.
    static const UINT c_retrimAfterMs = 20 * 1000;
    // How long it then stays low before it is suspended, five minutes after
    // it was hidden.
    static const UINT c_suspendAfterMs = 270 * 1000;

    // How much memory WebView2 may keep for a tab's page.
    enum class Memory
    {
        Normal,     // The tab shows.
        Hidden,     // Hidden; it goes Low after c_trimAfterMs.
        Low,        // Caches and garbage freed, the rest may be paged out.
        Settled,    // Low, and trimmed again c_retrimAfterMs later.
        Suspended,  // Scripts and timers paused, renderer memory given back.
    };
    bool IsTrimmed() const;
    // Shared with a pending TrySuspend, which may finish after the tab closed.
    std::shared_ptr<Memory> m_memory = std::make_shared<Memory>(Memory::Normal);

    // A page in front whose renderer ends again this soon after it was loaded
    // again for the last crash isn't loaded again: the user can reload it.
    static const ULONGLONG c_crashLoopMs = 30 * 1000;
    // The renderer ended while the tab was hidden: it loads as it shows.
    bool m_reloadWhenShown = false;
    ULONGLONG m_reloadedAfterCrashAt = 0;

    HWND m_parentHWnd = nullptr;
    size_t m_tabId = INVALID_TAB_ID;
    EventRegistrationToken m_historyUpdateForwarderToken = {};
    EventRegistrationToken m_uriUpdateForwarderToken = {};
    EventRegistrationToken m_navStartingToken = {};
    EventRegistrationToken m_navCompletedToken = {};
    EventRegistrationToken m_titleChangedToken = {};
    EventRegistrationToken m_faviconChangedToken = {};
    EventRegistrationToken m_certificateErrorToken = {};
    EventRegistrationToken m_processFailedToken = {};
    EventRegistrationToken m_newWindowToken = {};
    EventRegistrationToken m_windowCloseToken = {};
    EventRegistrationToken m_messageBrokerToken = {};  // Message broker for browser pages loaded in a tab
    Microsoft::WRL::ComPtr<ICoreWebView2WebMessageReceivedEventHandler> m_messageBroker;

    HRESULT Init(ICoreWebView2Environment* env, bool shouldBeActive, NewWindow newWindow);
    void SetMessageBroker();
    void SetTrackingPrevention();
    void OnRendererExited();
    void ReloadAfterCrash();
};
