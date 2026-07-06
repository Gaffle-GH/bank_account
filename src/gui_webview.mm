/*
 * Native GUI — embeds web/index.html in a WebKit view for a true 1:1 match.
 */

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <thread>

#include "web_runtime.h"

@interface NativeBridge : NSObject <WKScriptMessageHandler>
@property(assign) NSApplication* app;
@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, WKNavigationDelegate>
@property(strong) NSWindow* window;
@property(strong) WKWebView* webView;
@property(strong) NativeBridge* bridge;
- (void)setLockedContentSize:(NSSize)size;
@end

@implementation NativeBridge
- (void)userContentController:(WKUserContentController*)controller
      didReceiveScriptMessage:(WKScriptMessage*)message
{
    (void)controller;

    if (![message.body isKindOfClass:[NSString class]])
    {
        return;
    }

    NSString* body = message.body;

    if ([body isEqualToString:@"quit"])
    {
        web_stop_server();
        [self.app terminate:nil];
        return;
    }

    if ([body hasPrefix:@"resize:"])
    {
        NSArray<NSString*>* parts = [[body substringFromIndex:7] componentsSeparatedByString:@","];
        if (parts.count == 2)
        {
            const CGFloat width = [parts[0] doubleValue];
            const CGFloat height = [parts[1] doubleValue];
            if (width > 0 && height > 0)
            {
                AppDelegate* delegate = (AppDelegate*)self.app.delegate;
                [delegate setLockedContentSize:NSMakeSize(width, height)];
            }
        }
    }
}
@end

@implementation AppDelegate

static constexpr CGFloat kLoginWidth = 360;
static constexpr CGFloat kLoginHeight = 110;

- (void)setLockedContentSize:(NSSize)size
{
    if (self.window == nil)
    {
        return;
    }

    [self.window setContentSize:size];
    [self.window setMinSize:size];
    [self.window setMaxSize:size];
    [self.window center];
}

- (void)focusWebView
{
    if (self.webView == nil || self.window == nil)
    {
        return;
    }
    [NSApp activateIgnoringOtherApps:YES];
    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:self.webView];
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    std::string webRoot = "web";
    if (!web_resolve_root(webRoot))
    {
        NSAlert* alert = [[NSAlert alloc] init];
        alert.messageText = @"Could not find web UI";
        alert.informativeText = @"Run from the project folder so web/index.html is available.";
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }

    const std::string rootCopy = webRoot;
    std::thread([rootCopy]() { web_start_server(rootCopy, kWebPort); }).detach();

    for (int i = 0; i < 50; ++i)
    {
        int probe = socket(AF_INET, SOCK_STREAM, 0);
        if (probe >= 0)
        {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(kWebPort);
            inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
            if (connect(probe, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0)
            {
                close(probe);
                break;
            }
            close(probe);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    const NSSize lockedSize = NSMakeSize(kLoginWidth, kLoginHeight);
    NSRect frame = NSMakeRect(0, 0, kLoginWidth, kLoginHeight);

    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                                        NSWindowStyleMaskMiniaturizable
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"Account Program"];
    [self.window setContentSize:lockedSize];
    [self.window setMinSize:lockedSize];
    [self.window setMaxSize:lockedSize];
    [self.window setDelegate:self];

    WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
    self.bridge = [[NativeBridge alloc] init];
    self.bridge.app = NSApp;
    [config.userContentController addScriptMessageHandler:self.bridge name:@"native"];

    self.webView = [[WKWebView alloc] initWithFrame:self.window.contentView.bounds configuration:config];
    self.webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.webView.navigationDelegate = self;
    [self.window.contentView addSubview:self.webView];

    if (NSScrollView* scrollView = self.webView.enclosingScrollView)
    {
        scrollView.hasVerticalScroller = NO;
        scrollView.hasHorizontalScroller = NO;
        scrollView.verticalScrollElasticity = NSScrollElasticityNone;
        scrollView.horizontalScrollElasticity = NSScrollElasticityNone;
        scrollView.borderType = NSNoBorder;
        scrollView.drawsBackground = NO;
    }

    [self.webView setValue:@NO forKey:@"drawsBackground"];

    NSURL* url = [NSURL URLWithString:[NSString stringWithFormat:@"http://127.0.0.1:%d/", kWebPort]];
    [self.webView loadRequest:[NSURLRequest requestWithURL:url]];

    [self.window center];
    [self focusWebView];
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
    (void)notification;
    [self focusWebView];
}

- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation*)navigation
{
    (void)webView;
    (void)navigation;
    [self focusWebView];
    [self.webView evaluateJavaScript:@"document.getElementById('login-name')?.focus()"
                   completionHandler:nil];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    (void)sender;
    web_stop_server();
    return YES;
}

@end

int main(int /*argc*/, char* /*argv*/[])
{
    @autoreleasepool
    {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        AppDelegate* delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
