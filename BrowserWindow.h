// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "framework.h"
#include "Tab.h"

class BrowserWindow
{
public:
    static const int c_uiBarHeight = 70;
    static const int c_optionsDropdownHeight = 108;
    static const int c_optionsDropdownWidth = 200;

    static ATOM RegisterClass(_In_ HINSTANCE hInstance);
    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static BOOL LaunchWindow(_In_ HINSTANCE hInstance, _In_ int nCmdShow);
    static std::wstring GetAppDataDirectory();
    std::wstring GetFullPathFor(LPCWSTR relativePath);
    HRESULT HandleTabURIUpdate(size_t tabId, ICoreWebView2* webview);
    HRESULT HandleTabHistoryUpdate(size_t tabId, ICoreWebView2* webview);
    HRESULT HandleTabNavStarting(size_t tabId, ICoreWebView2* webview);
    HRESULT HandleTabNavCompleted(size_t tabId, ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args);
    HRESULT HandleTabSecurityUpdate(size_t tabId, ICoreWebView2* webview);
    HRESULT HandleTabCertificateError(size_t tabId, ICoreWebView2* webview, LPCWSTR uri);
    void HandleTabCreated(size_t tabId, bool shouldBeActive);
    HRESULT HandleTabMessageReceived(size_t tabId, ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* eventArgs);
    int GetDPIAwareBound(int bound);
    static void CheckFailure(HRESULT hr, LPCWSTR errorMessage);
    static void SetMemoryUsage(ICoreWebView2* webview, COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL level);
protected:
    // The browser UI's WebView2 profile: its own data, in the tabs' browser
    // process.
    static constexpr LPCWSTR c_uiProfileName = L"BrowserUI";
    // Where WebView2 keeps that profile's data in the user data folder.
    static constexpr LPCWSTR c_uiProfileFolder = L"EBWebView\\WV2Profile_browserui";
    // How many renderer processes, the browser UI's two included, there are
    // before tabs of one site share one (see GetBrowserArguments).
    static const int c_rendererProcessLimit = 8;

    HINSTANCE m_hInst = nullptr;  // Current app instance
    HWND m_hWnd = nullptr;

    static WCHAR s_windowClass[MAX_LOADSTRING];  // The window class name
    static WCHAR s_title[MAX_LOADSTRING];  // The title bar text

    int m_minWindowWidth = 0;
    int m_minWindowHeight = 0;

    Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_env;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controlsController;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_optionsController;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_controlsWebView;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_optionsWebView;
    std::map<size_t,std::unique_ptr<Tab>> m_tabs;
    size_t m_activeTabId = 0;
    bool m_minimized = false;
    // Hosts a tab met a certificate error on, see HandleTabSecurityUpdate.
    std::set<std::wstring> m_certificateErrorHosts;

    EventRegistrationToken m_controlsUIMessageBrokerToken = {};  // Token for the UI message handler in controls WebView
    EventRegistrationToken m_controlsZoomToken = {};
    EventRegistrationToken m_optionsUIMessageBrokerToken = {};  // Token for the UI message handler in options WebView
    EventRegistrationToken m_optionsZoomToken = {};
    EventRegistrationToken m_lostOptionsFocus = {};  // Token for the lost focus handler in options WebView
    Microsoft::WRL::ComPtr<ICoreWebView2WebMessageReceivedEventHandler> m_uiMessageBroker;

    BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
    HRESULT InitUIWebViews();
    HRESULT CreateUIController(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* handler);
    void MoveBrowserUIData(const std::wstring& userDataDirectory);
    static std::wstring GetBrowserArguments();
    static bool HasDiscreteGpu();
    HRESULT CreateBrowserControlsWebView();
    HRESULT CreateBrowserOptionsWebView();
    HRESULT ClearContentCache();
    HRESULT ClearControlsCache();
    HRESULT ClearContentCookies();
    HRESULT ClearControlsCookies();
    static HRESULT ClearBrowsingData(ICoreWebView2* webview, COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds);
    static std::wstring GetHost(LPCWSTR uri, DWORD* scheme);

    void SetUIMessageBroker();
    HRESULT ResizeUIWebViews();
    void UpdateMinWindowSize();
    HRESULT PostJsonToWebView(web::json::value jsonObj, ICoreWebView2* webview);
    HRESULT SwitchToTab(size_t tabId);
    void SetMinimized(bool minimized);
    std::wstring GetFilePathAsURI(std::wstring fullPath);
    std::wstring GetURIToShow(const std::wstring& uri);
};
