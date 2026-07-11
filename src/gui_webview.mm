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
@property(assign) WKWebView* webView;
@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, WKNavigationDelegate>
@property(strong) NSWindow* window;
@property(strong) WKWebView* webView;
@property(strong) NativeBridge* bridge;
@property(assign) BOOL userDidResizeWindow;
- (void)setPreferredContentSize:(NSSize)size fit:(BOOL)fit;
- (void)showFolderPickerStartingAt:(NSString*)startPath;
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
        if (parts.count >= 2)
        {
            const CGFloat width = [parts[0] doubleValue];
            const CGFloat height = [parts[1] doubleValue];
            if (width > 0 && height > 0)
            {
                const BOOL forceFit = parts.count >= 3 && [parts[2] isEqualToString:@"fit"];
                AppDelegate* delegate = (AppDelegate*)self.app.delegate;
                [delegate setPreferredContentSize:NSMakeSize(width, height) fit:forceFit];
            }
        }
        return;
    }

    if ([body hasPrefix:@"openFile:"])
    {
        NSString* path = [body substringFromIndex:9];
        dispatch_async(dispatch_get_main_queue(), ^{
            NSFileManager* fm = [NSFileManager defaultManager];
            if ([fm fileExistsAtPath:path])
            {
                [[NSWorkspace sharedWorkspace] selectFile:path
                                 inFileViewerRootedAtPath:[path stringByDeletingLastPathComponent]];
            }
            else
            {
                NSString* dir = [path stringByDeletingLastPathComponent];
                NSURL* url = [NSURL fileURLWithPath:dir isDirectory:YES];
                [[NSWorkspace sharedWorkspace] openURL:url];
            }
        });
        return;
    }

    if ([body hasPrefix:@"openDir:"])
    {
        NSString* path = [body substringFromIndex:8];
        dispatch_async(dispatch_get_main_queue(), ^{
            NSString* resolved = path.length > 0 ? [path stringByExpandingTildeInPath] : NSHomeDirectory();
            NSURL* url = [NSURL fileURLWithPath:resolved isDirectory:YES];
            [[NSWorkspace sharedWorkspace] openURL:url];
        });
        return;
    }

    if ([body isEqualToString:@"pickDir"] || [body hasPrefix:@"pickDir:"])
    {
        NSString* startPath = [body hasPrefix:@"pickDir:"] ? [body substringFromIndex:8] : @"";
        dispatch_async(dispatch_get_main_queue(), ^{
            AppDelegate* delegate = (AppDelegate*)self.app.delegate;
            [delegate showFolderPickerStartingAt:startPath];
        });
    }
}
@end

@implementation AppDelegate

static constexpr CGFloat kLoginWidth = 360;
static constexpr CGFloat kLoginHeight = 400;
static constexpr CGFloat kMinWidth = 320;
static constexpr CGFloat kMinHeight = 300;

- (void)setPreferredContentSize:(NSSize)size fit:(BOOL)fit
{
    if (self.window == nil)
    {
        return;
    }

    if (fit || !self.userDidResizeWindow)
    {
        [self.window setContentSize:size];
        [self resetWebViewScrollPosition];
        if (fit)
        {
            self.userDidResizeWindow = NO;
        }
    }
    [self.window setMinSize:NSMakeSize(kMinWidth, kMinHeight)];
}

- (void)resetWebViewScrollPosition
{
    if (self.webView == nil)
    {
        return;
    }

    if (NSScrollView* scrollView = self.webView.enclosingScrollView)
    {
        [scrollView.contentView scrollToPoint:NSMakePoint(0, 0)];
        [scrollView reflectScrolledClipView:scrollView.contentView];
        scrollView.contentInsets = NSEdgeInsetsZero;
        scrollView.scrollerInsets = NSEdgeInsetsZero;
    }

    [self.webView evaluateJavaScript:
        @"window.scrollTo(0, 0);"
        "document.documentElement.scrollTop = 0;"
        "document.body.scrollTop = 0;"
        "document.documentElement.style.overflow = 'hidden';"
        "document.body.style.overflow = 'hidden';"
                   completionHandler:nil];
}

- (void)showFolderPickerStartingAt:(NSString*)startPath
{
    if (self.webView == nil)
    {
        return;
    }

    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.canChooseDirectories = YES;
    panel.canChooseFiles = NO;
    panel.allowsMultipleSelection = NO;
    panel.canCreateDirectories = YES;
    panel.prompt = @"Choose";
    panel.message = @"Choose the folder where the account file will be saved.";

    NSString* resolved = startPath.length > 0 ? [startPath stringByExpandingTildeInPath] : NSHomeDirectory();
    NSFileManager* fm = [NSFileManager defaultManager];
    BOOL isDir = NO;
    if ([fm fileExistsAtPath:resolved isDirectory:&isDir] && isDir)
    {
        panel.directoryURL = [NSURL fileURLWithPath:resolved isDirectory:YES];
    }
    else
    {
        NSString* parent = [resolved stringByDeletingLastPathComponent];
        if (parent.length > 0 && [fm fileExistsAtPath:parent isDirectory:&isDir] && isDir)
        {
            panel.directoryURL = [NSURL fileURLWithPath:parent isDirectory:YES];
        }
    }

    [NSApp activateIgnoringOtherApps:YES];
    if ([panel runModal] != NSModalResponseOK)
    {
        return;
    }

    NSString* chosen = panel.URL.path;
    if (chosen.length == 0)
    {
        return;
    }

    NSError* error = nil;
    NSData* jsonData = [NSJSONSerialization dataWithJSONObject:chosen
                                                       options:NSJSONWritingFragmentsAllowed
                                                         error:&error];
    if (jsonData == nil || error != nil)
    {
        return;
    }

    NSString* jsonPath = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    NSString* script = [NSString stringWithFormat:@"window.__setLoginDir(%@)", jsonPath];
    [self.webView evaluateJavaScript:script completionHandler:^(id, NSError*) {
        [self.webView evaluateJavaScript:@"window.syncNativeWindowSize && window.syncNativeWindowSize({ fit: true })"
                       completionHandler:nil];
    }];
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

    const NSSize initialSize = NSMakeSize(kLoginWidth, kLoginHeight);
    NSRect frame = NSMakeRect(0, 0, kLoginWidth, kLoginHeight);

    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                                        NSWindowStyleMaskMiniaturizable |
                                                        NSWindowStyleMaskResizable |
                                                        NSWindowStyleMaskFullScreen
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"Account Program"];
    [self.window setContentSize:initialSize];
    [self.window setMinSize:NSMakeSize(kMinWidth, kMinHeight)];
    [self.window setDelegate:self];
    self.userDidResizeWindow = NO;

    WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
    self.bridge = [[NativeBridge alloc] init];
    self.bridge.app = NSApp;
    [config.userContentController addScriptMessageHandler:self.bridge name:@"native"];

    self.webView = [[WKWebView alloc] initWithFrame:self.window.contentView.bounds configuration:config];
    self.bridge.webView = self.webView;
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
        scrollView.autohidesScrollers = YES;
        scrollView.contentInsets = NSEdgeInsetsZero;
        scrollView.scrollerInsets = NSEdgeInsetsZero;
    }

    [self.webView setValue:@NO forKey:@"drawsBackground"];

    NSURL* url = [NSURL URLWithString:[NSString stringWithFormat:@"http://127.0.0.1:%d/", kWebPort]];
    [self.webView loadRequest:[NSURLRequest requestWithURL:url]];

    [self.window center];
    [self focusWebView];
}

- (void)windowDidResize:(NSNotification*)notification
{
    (void)notification;
    self.userDidResizeWindow = YES;
    [self resetWebViewScrollPosition];
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
    [self resetWebViewScrollPosition];
    [self.webView evaluateJavaScript:@"document.getElementById('login-name')?.focus()"
                   completionHandler:nil];
    [self.webView evaluateJavaScript:@"window.syncNativeWindowSize && window.syncNativeWindowSize({ fit: true })"
                   completionHandler:^(id, NSError*) {
                       [self resetWebViewScrollPosition];
                   }];
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
