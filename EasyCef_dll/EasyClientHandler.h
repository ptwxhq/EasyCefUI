#pragma once

#include <list>
#include <unordered_map>
#include "WebViewControl.h"

//主进程


class EasyClientHandler :
    public CefClient,
    public CefLifeSpanHandler,
    public CefDisplayHandler,
    public CefLoadHandler,
    public CefContextMenuHandler,
    public CefRequestHandler,
    public CefDragHandler,
    public CefDownloadHandler
{
public:



    //////////////////////////////////////////////////////

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    //CefLifeSpanHandler

    bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        const CefString& target_url,
        const CefString& target_frame_name,
        CefLifeSpanHandler::WindowOpenDisposition target_disposition,
        bool user_gesture,
        const CefPopupFeatures& popupFeatures,
        CefWindowInfo& windowInfo,
        CefRefPtr<CefClient>& client,
        CefBrowserSettings& settings,
        CefRefPtr<CefDictionaryValue>& extra_info,
        bool* no_javascript_access) override;

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;

    bool DoClose(CefRefPtr<CefBrowser> browser) override;

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    ////////////////////////////////////////

    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    //GetDisplayHandler

    void OnAddressChange(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        const CefString& url) override;

    void OnTitleChange(CefRefPtr<CefBrowser> browser,
        const CefString& title) override;

    void OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
        const std::vector<CefString>& icon_urls) override;

    bool OnTooltip(CefRefPtr<CefBrowser> browser, CefString& text) override;

    bool OnCursorChange(CefRefPtr<CefBrowser> browser,
        CefCursorHandle cursor,
        cef_cursor_type_t type,
        const CefCursorInfo& custom_cursor_info) override;

    bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
        cef_log_severity_t level,
        const CefString& message,
        const CefString& source,
        int line) override;



    /////////////////////////////////////////

    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    //CefLoadHandler

    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
        bool isLoading,
        bool canGoBack,
        bool canGoForward) override;

    void OnLoadStart(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        TransitionType transition_type) override;

    void OnLoadEnd(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        int httpStatusCode) override;

    void OnLoadError(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        ErrorCode errorCode,
        const CefString& errorText,
        const CefString& failedUrl) override;


    //////////////////////////////////////////////
        //CefRenderHandler 这边主要提供给透明UI，其他情况不使用
    CefRefPtr<CefRenderHandler> GetRenderHandler() override;

    //////////////////////////////////////////


    //右键菜单
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }

    void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefContextMenuParams> params,
        CefRefPtr<CefMenuModel> model) override;

    bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefContextMenuParams> params,
        int command_id,
        EventFlags event_flags)  override;


    //////////////////////////////////////////

    ///
    // Return the handler for browser request events.
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }


    void OnPluginCrashed(CefRefPtr<CefBrowser> browser,
        const CefString& plugin_path) override;

    void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
        TerminationStatus status) override;

    void OnDocumentAvailableInMainFrame(CefRefPtr<CefBrowser> browser) override;

    bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        bool user_gesture,
        bool is_redirect) override;

    bool OnOpenURLFromTab(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        const CefString& target_url,
        CefRequestHandler::WindowOpenDisposition target_disposition,
        bool user_gesture) override;


    /////////////////
    //标题拖动

    CefRefPtr<CefDragHandler> GetDragHandler() override { return this; }

    bool OnDragEnter(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDragData> dragData,
        CefDragHandler::DragOperationsMask mask) override;

    void OnDraggableRegionsChanged(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        const std::vector<CefDraggableRegion>& regions) override;


    CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return this; }

    void OnBeforeDownload(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDownloadItem> download_item,
        const CefString& suggested_name,
        CefRefPtr<CefBeforeDownloadCallback> callback) override;

    void SetManualHandle(wvhandle handle) {
        m_hManualCreateHandle = handle;
    }

    void SetUIWindowInfo(CefRefPtr<WebViewUIControl> webui, bool bTransparent);

protected:
    bool m_bIsUIControl = false;
    bool m_bIsUITransparent = false;

    //typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
    //BrowserList m_popbrowsers;

    //共用handler的数量
    int m_BrowserCount = 0;

    CefRefPtr<CefBrowser> m_browser;

    wvhandle m_hManualCreateHandle = 0;
    std::list<wvhandle> m_preCreatePopHandle;

    CefRefPtr<WebViewUIControl> m_webuicontrol;

    IMPLEMENT_REFCOUNTING(EasyClientHandler);

    //Cefclient 自己的
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override;


};

