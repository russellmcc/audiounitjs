// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef _CLIENT_HANDLER_H
#define _CLIENT_HANDLER_H

#include "cef.h"
#include "util.h"
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>

// Define this value to redirect all popup URLs to the main application browser
// window.
//#define TEST_REDIRECT_POPUP_URLS


// ClientHandler implementation.
class ClientHandler : public CefClient,
public CefLifeSpanHandler,
public CefLoadHandler,
public CefRequestHandler,
public CefDisplayHandler,
public CefFocusHandler,
public CefKeyboardHandler,
public CefPrintHandler,
public CefV8ContextHandler,
public CefDragHandler
{
public:
    ClientHandler(AudioUnit au);
    virtual ~ClientHandler();
    
    // CefClient methods
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE
    { return this; }
    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE
    { return this; }
    virtual CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE
    { return this; }
    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE
    { return this; }
    virtual CefRefPtr<CefFocusHandler> GetFocusHandler() OVERRIDE
    { return this; }
    virtual CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() OVERRIDE
    { return this; }
    virtual CefRefPtr<CefPrintHandler> GetPrintHandler() OVERRIDE
    { return this; }
    virtual CefRefPtr<CefV8ContextHandler> GetV8ContextHandler() OVERRIDE
    { return this; }
    virtual CefRefPtr<CefDragHandler> GetDragHandler() OVERRIDE
    { return this; }
    
    // CefLifeSpanHandler methods
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
    virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
    
    // CefLoadHandler methods
    virtual void OnLoadStart(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame) OVERRIDE;
    virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           int httpStatusCode) OVERRIDE;
    virtual bool OnLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             ErrorCode errorCode,
                             const CefString& failedUrl,
                             CefString& errorText) OVERRIDE;
    
    // CefRequestHandler methods
    virtual bool GetDownloadHandler(CefRefPtr<CefBrowser> browser,
                                    const CefString& mimeType,
                                    const CefString& fileName,
                                    int64 contentLength,
                                    CefRefPtr<CefDownloadHandler>& handler) 
        {return false;}

    // CefDisplayHandler methods
    virtual void OnNavStateChange(CefRefPtr<CefBrowser> browser,
                                  bool canGoBack,
                                  bool canGoForward) OVERRIDE;
    virtual bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                  const CefString& message,
                                  const CefString& source,
                                  int line) OVERRIDE;
    
    // CefFocusHandler methods.
    virtual void OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefDOMNode> node) OVERRIDE;
    
    // CefKeyboardHandler methods.
    virtual bool OnKeyEvent(CefRefPtr<CefBrowser> browser,
                            KeyEventType type,
                            int code,
                            int modifiers,
                            bool isSystemKey,
                            bool isAfterJavaScript) OVERRIDE;
    
    // CefPrintHandler methods.
    virtual bool GetPrintHeaderFooter(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      const CefPrintInfo& printInfo,
                                      const CefString& url,
                                      const CefString& title,
                                      int currentPage,
                                      int maxPages,
                                      CefString& topLeft,
                                      CefString& topCenter,
                                      CefString& topRight,
                                      CefString& bottomLeft,
                                      CefString& bottomCenter,
                                      CefString& bottomRight) OVERRIDE;
    
    // CefV8ContextHandler methods
    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context) OVERRIDE;
    
    // CefDragHandler methods.
    virtual bool OnDragStart(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefDragData> dragData,
                             DragOperationsMask mask) OVERRIDE;
    virtual bool OnDragEnter(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefDragData> dragData,
                             DragOperationsMask mask) OVERRIDE;
    
    bool OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
                                             CefRefPtr<CefRequest> request,
                                             CefString& redirectUrl,
                                             CefRefPtr<CefStreamReader>& resourceStream,
                                             CefRefPtr<CefResponse> response,
                                             int loadFlags)
    {
        return false;
    }
    
    void ReceiveAUEvent(void *object, const AudioUnitEvent *event, UInt64 hostTime, Float32 value);
    
protected:
    // The child browser window
    CefRefPtr<CefBrowser> mBrowser;
    
    // the local AU
    AudioUnit mAU;
    
    // event listener for the AU.
    AUEventListenerRef		mAUEventListener;
	
    // the V8 value representing the AudioUnit object.
    CefRefPtr<CefV8Value> mV8Value;
    
    // the V8 context for the browser
    CefRefPtr<CefV8Context> mV8Context;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(ClientHandler);
    // Include the default locking implementation.
    IMPLEMENT_LOCKING(ClientHandler);
    
    // helper subroutine to setup per-Parameter stuff
    void SetupAUParams();
};

#endif // _CLIENT_HANDLER_H
