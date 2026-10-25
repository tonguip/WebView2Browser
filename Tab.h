// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "framework.h"

class Tab
{
public:
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_contentController;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_contentWebView;
    Microsoft::WRL::ComPtr<ICoreWebView2DevToolsProtocolEventReceiver> m_securityStateChangedReceiver;

    static std::unique_ptr<Tab> CreateNewTab(HWND hWnd, ICoreWebView2Environment* env, size_t id, bool shouldBeActive);
    ~Tab();
    HRESULT ResizeWebView();
    HRESULT SetVisible(bool visible);
    void Wake();
    void OnHiddenTimer();
protected:
    // How long a tab stays hidden before WebView2 is asked to keep its memory
    // low: long enough that flipping between tabs finds them as they were.
    static const UINT c_trimAfterMs = 30 * 1000;
    // How long it then stays low before it is suspended, five minutes after
    // it was hidden.
    static const UINT c_suspendAfterMs = 270 * 1000;

    // How much memory WebView2 may keep for a tab's page.
    enum class Memory
    {
        Normal,     // The tab shows.
        Hidden,     // Hidden; it goes Low after c_trimAfterMs.
        Low,        // Caches and garbage freed, the rest may be paged out.
        Suspended,  // Scripts and timers paused, renderer memory given back.
    };
    // Shared with a pending TrySuspend, which may finish after the tab closed.
    std::shared_ptr<Memory> m_memory = std::make_shared<Memory>(Memory::Normal);

    HWND m_parentHWnd = nullptr;
    size_t m_tabId = INVALID_TAB_ID;
    EventRegistrationToken m_historyUpdateForwarderToken = {};
    EventRegistrationToken m_uriUpdateForwarderToken = {};
    EventRegistrationToken m_navStartingToken = {};
    EventRegistrationToken m_navCompletedToken = {};
    EventRegistrationToken m_securityUpdateToken = {};
    EventRegistrationToken m_messageBrokerToken = {};  // Message broker for browser pages loaded in a tab
    Microsoft::WRL::ComPtr<ICoreWebView2WebMessageReceivedEventHandler> m_messageBroker;

    HRESULT Init(ICoreWebView2Environment* env, bool shouldBeActive);
    void SetMessageBroker();
};
