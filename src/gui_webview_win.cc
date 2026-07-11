/*
 * Native GUI (Windows) — embeds the web UI in WebView2.
 */

#ifdef _WIN32

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <WebView2.h>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#include "web_runtime.h"

namespace
{
constexpr int kLoginWidth = 360;
constexpr int kLoginHeight = 400;
constexpr int kMinWidth = 320;
constexpr int kMinHeight = 300;

HWND g_hwnd = nullptr;
ICoreWebView2Controller* g_controller = nullptr;
ICoreWebView2* g_webview = nullptr;
bool g_userDidResizeWindow = false;
bool g_programmaticResize = false;

using CreateCoreWebView2EnvironmentWithOptionsFn = HRESULT(STDMETHODCALLTYPE*)(
    PCWSTR browserExecutableFolder, PCWSTR userDataFolder,
    ICoreWebView2EnvironmentOptions* environmentOptions,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler);

CreateCoreWebView2EnvironmentWithOptionsFn g_createEnvironment = nullptr;

template <typename T>
void release(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}

void resizeWebView()
{
    if (g_controller == nullptr || g_hwnd == nullptr)
    {
        return;
    }

    RECT bounds{};
    GetClientRect(g_hwnd, &bounds);
    g_controller->put_Bounds(bounds);
}

void runScript(const std::wstring& script)
{
    if (g_webview != nullptr)
    {
        g_webview->ExecuteScript(script.c_str(), nullptr);
    }
}

void resetWebViewScrollPosition()
{
    runScript(L"window.scrollTo(0, 0);"
              L"document.documentElement.scrollTop = 0;"
              L"document.body.scrollTop = 0;"
              L"document.documentElement.style.overflow = 'hidden';"
              L"document.body.style.overflow = 'hidden';");
}

void setPreferredWindowSize(int width, int height, bool fit)
{
    if (g_hwnd == nullptr || width <= 0 || height <= 0)
    {
        return;
    }

    if (!fit && g_userDidResizeWindow)
    {
        return;
    }

    RECT windowRect{};
    RECT clientRect{};
    GetWindowRect(g_hwnd, &windowRect);
    GetClientRect(g_hwnd, &clientRect);
    const int chromeW = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
    const int chromeH = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);

    g_programmaticResize = true;
    SetWindowPos(g_hwnd, nullptr, 0, 0, width + chromeW, height + chromeH,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    resizeWebView();
    resetWebViewScrollPosition();

    if (fit)
    {
        g_userDidResizeWindow = false;
    }
}

std::wstring expandTilde(const std::wstring& path)
{
    if (path.empty())
    {
        return path;
    }
    if (path[0] == L'~')
    {
        wchar_t home[MAX_PATH]{};
        if (SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, SHGFP_TYPE_CURRENT, home) == S_OK)
        {
            if (path.size() == 1 || path[1] == L'\\' || path[1] == L'/')
            {
                return std::wstring(home) + path.substr(1);
            }
        }
    }
    return path;
}

std::wstring jsonStringLiteral(const std::wstring& value)
{
    std::wstring out = L"\"";
    for (wchar_t ch : value)
    {
        switch (ch)
        {
        case L'\\':
            out += L"\\\\";
            break;
        case L'"':
            out += L"\\\"";
            break;
        case L'\n':
            out += L"\\n";
            break;
        case L'\r':
            out += L"\\r";
            break;
        case L'\t':
            out += L"\\t";
            break;
        default:
            if (ch < 0x20)
            {
                wchar_t buf[8]{};
                swprintf(buf, 8, L"\\u%04x", static_cast<unsigned>(ch));
                out += buf;
            }
            else
            {
                out += ch;
            }
            break;
        }
    }
    out += L"\"";
    return out;
}

bool setFolderDialogStart(IFileOpenDialog* dialog, const std::wstring& startPath)
{
    std::wstring resolved = startPath.empty() ? expandTilde(L"~") : expandTilde(startPath);
    std::wstring target = resolved;

    if (!PathIsDirectoryW(target.c_str()))
    {
        wchar_t parent[MAX_PATH]{};
        wcsncpy_s(parent, target.c_str(), _TRUNCATE);
        if (PathRemoveFileSpecW(parent) && PathIsDirectoryW(parent))
        {
            target = parent;
        }
        else
        {
            return false;
        }
    }

    IShellItem* item = nullptr;
    if (FAILED(SHCreateItemFromParsingName(target.c_str(), nullptr, IID_PPV_ARGS(&item))))
    {
        return false;
    }

    dialog->SetFolder(item);
    item->Release();
    return true;
}

void focusWebView()
{
    if (g_hwnd == nullptr)
    {
        return;
    }

    SetForegroundWindow(g_hwnd);
    if (g_controller != nullptr)
    {
        g_controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
    }
}

void onNavigationCompleted()
{
    focusWebView();
    resetWebViewScrollPosition();
    runScript(L"document.getElementById('login-name')?.focus();");
    runScript(L"window.syncNativeWindowSize && window.syncNativeWindowSize({ fit: true });");
    resetWebViewScrollPosition();
}

void handleNativeMessage(const std::wstring& body)
{
    if (body == L"quit")
    {
        web_stop_server();
        PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
        return;
    }

    if (body.rfind(L"resize:", 0) == 0)
    {
        const std::wstring sizePart = body.substr(7);
        const size_t comma = sizePart.find(L',');
        if (comma != std::wstring::npos)
        {
            const size_t comma2 = sizePart.find(L',', comma + 1);
            const std::wstring heightPart = comma2 == std::wstring::npos
                                                ? sizePart.substr(comma + 1)
                                                : sizePart.substr(comma + 1, comma2 - comma - 1);
            const int width = _wtoi(sizePart.substr(0, comma).c_str());
            const int height = _wtoi(heightPart.c_str());
            const bool fit = comma2 != std::wstring::npos &&
                             sizePart.substr(comma2 + 1) == L"fit";
            setPreferredWindowSize(width, height, fit);
        }
        return;
    }

    if (body.rfind(L"openFile:", 0) == 0)
    {
        const std::wstring path = body.substr(9);
        if (PathFileExistsW(path.c_str()))
        {
            const std::wstring args = L"/select," + path;
            ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
        }
        else
        {
            const size_t slash = path.find_last_of(L"\\/");
            if (slash != std::wstring::npos)
            {
                const std::wstring dir = path.substr(0, slash);
                ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
        }
        return;
    }

    if (body.rfind(L"openDir:", 0) == 0)
    {
        std::wstring dir = expandTilde(body.substr(8));
        if (dir.empty())
        {
            wchar_t home[MAX_PATH]{};
            if (SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, SHGFP_TYPE_CURRENT, home) == S_OK)
            {
                dir = home;
            }
        }
        if (!dir.empty())
        {
            ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        return;
    }

    if (body == L"pickDir" || body.rfind(L"pickDir:", 0) == 0)
    {
        std::wstring start = body.rfind(L"pickDir:", 0) == 0 ? expandTilde(body.substr(8)) : L"";
        IFileOpenDialog* dialog = nullptr;
        if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&dialog))))
        {
            return;
        }

        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        dialog->SetTitle(L"Choose the folder where the account file will be saved.");

        setFolderDialogStart(dialog, start);

        if (dialog->Show(g_hwnd) != S_OK)
        {
            dialog->Release();
            return;
        }

        IShellItem* result = nullptr;
        if (FAILED(dialog->GetResult(&result)))
        {
            dialog->Release();
            return;
        }

        PWSTR chosen = nullptr;
        if (FAILED(result->GetDisplayName(SIGDN_FILESYSPATH, &chosen)) || chosen == nullptr)
        {
            result->Release();
            dialog->Release();
            return;
        }

        std::wstring path = chosen;
        CoTaskMemFree(chosen);
        result->Release();
        dialog->Release();

        std::wstring script = L"window.__setLoginDir(" + jsonStringLiteral(path) +
                              L"); window.syncNativeWindowSize && window.syncNativeWindowSize({ fit: true });";
        runScript(script);
    }
}

class WebMessageHandler final : public ICoreWebView2WebMessageReceivedEventHandler
{
public:
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2WebMessageReceivedEventHandler)
        {
            *ppv = this;
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender,
                                   ICoreWebView2WebMessageReceivedEventArgs* args) override
    {
        (void)sender;
        LPWSTR message = nullptr;
        if (SUCCEEDED(args->TryGetWebMessageAsString(&message)) && message != nullptr)
        {
            handleNativeMessage(message);
            CoTaskMemFree(message);
        }
        return S_OK;
    }
};

class NavigationCompletedHandler final : public ICoreWebView2NavigationCompletedEventHandler
{
public:
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2NavigationCompletedEventHandler)
        {
            *ppv = this;
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender,
                                   ICoreWebView2NavigationCompletedEventArgs* args) override
    {
        (void)sender;
        (void)args;
        onNavigationCompleted();
        return S_OK;
    }
};

WebMessageHandler g_webMessageHandler;
NavigationCompletedHandler g_navigationCompletedHandler;

class ControllerCompletedHandler final
    : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
{
public:
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)
        {
            *ppv = this;
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override
    {
        if (FAILED(result) || controller == nullptr)
        {
            MessageBoxW(g_hwnd, L"Could not create WebView2 controller.", L"Account Program",
                        MB_ICONERROR | MB_OK);
            return result;
        }

        g_controller = controller;
        g_controller->AddRef();
        g_controller->get_CoreWebView2(&g_webview);
        resizeWebView();

        ICoreWebView2Controller2* controller2 = nullptr;
        if (SUCCEEDED(g_controller->QueryInterface(IID_PPV_ARGS(&controller2))) && controller2 != nullptr)
        {
            COREWEBVIEW2_COLOR transparent{};
            controller2->put_DefaultBackgroundColor(transparent);
            controller2->Release();
        }

        ICoreWebView2Settings* settings = nullptr;
        if (SUCCEEDED(g_webview->get_Settings(&settings)) && settings != nullptr)
        {
            settings->put_AreDefaultScriptDialogsEnabled(TRUE);
            settings->put_IsStatusBarEnabled(FALSE);
            settings->put_IsWebMessageEnabled(TRUE);
            settings->Release();
        }

        g_webview->add_WebMessageReceived(&g_webMessageHandler, nullptr);

        const std::wstring url = L"http://127.0.0.1:" + std::to_wstring(kWebPort) + L"/";
        g_webview->Navigate(url.c_str());
        g_webview->add_NavigationCompleted(&g_navigationCompletedHandler, nullptr);
        return S_OK;
    }
};

class EnvironmentCompletedHandler final
    : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
{
public:
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IUnknown ||
            riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)
        {
            *ppv = this;
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* env) override
    {
        if (FAILED(result) || env == nullptr)
        {
            MessageBoxW(g_hwnd, L"Could not create WebView2 environment.", L"Account Program",
                        MB_ICONERROR | MB_OK);
            return result;
        }

        ControllerCompletedHandler controllerHandler;
        return env->CreateCoreWebView2Controller(g_hwnd, &controllerHandler);
    }
};

ControllerCompletedHandler g_controllerHandler;
EnvironmentCompletedHandler g_environmentHandler;

bool loadWebView2Loader()
{
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    const std::wstring loaderPath = std::wstring(exePath) + L"\\WebView2Loader.dll";

    HMODULE loader = LoadLibraryW(loaderPath.c_str());
    if (loader == nullptr)
    {
        loader = LoadLibraryW(L"WebView2Loader.dll");
    }
    if (loader == nullptr)
    {
        return false;
    }

    g_createEnvironment = reinterpret_cast<CreateCoreWebView2EnvironmentWithOptionsFn>(
        GetProcAddress(loader, "CreateCoreWebView2EnvironmentWithOptions"));
    return g_createEnvironment != nullptr;
}

bool waitForServer()
{
    for (int i = 0; i < 50; ++i)
    {
        WSADATA wsa{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        {
            return false;
        }

        SOCKET probe = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        bool ready = false;
        if (probe != INVALID_SOCKET)
        {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(static_cast<u_short>(kWebPort));
            InetPtonA(AF_INET, "127.0.0.1", &addr.sin_addr);
            ready = connect(probe, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
            closesocket(probe);
        }
        WSACleanup();
        if (ready)
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

void initWebView()
{
    if (!loadWebView2Loader())
    {
        MessageBoxW(g_hwnd,
                    L"WebView2Loader.dll was not found next to BankAccount.exe.\n"
                    L"Rebuild the app or install the WebView2 runtime.",
                    L"Account Program", MB_ICONERROR | MB_OK);
        return;
    }

    const HRESULT hr = g_createEnvironment(nullptr, nullptr, nullptr, &g_environmentHandler);
    if (FAILED(hr))
    {
        MessageBoxW(g_hwnd, L"Could not initialize WebView2.", L"Account Program",
                    MB_ICONERROR | MB_OK);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_GETMINMAXINFO:
    {
        auto* info = reinterpret_cast<MINMAXINFO*>(lparam);
        RECT frame{0, 0, kMinWidth, kMinHeight};
        AdjustWindowRect(&frame, WS_OVERLAPPEDWINDOW, FALSE);
        info->ptMinTrackSize.x = frame.right - frame.left;
        info->ptMinTrackSize.y = frame.bottom - frame.top;
        return 0;
    }
    case WM_ACTIVATE:
        if (LOWORD(wparam) != WA_INACTIVE)
        {
            focusWebView();
        }
        return 0;
    case WM_SIZE:
        if (!g_programmaticResize)
        {
            g_userDidResizeWindow = true;
        }
        g_programmaticResize = false;
        resizeWebView();
        resetWebViewScrollPosition();
        return 0;
    case WM_DESTROY:
        release(g_webview);
        release(g_controller);
        web_stop_server();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    std::string webRoot = "web";
    if (!web_resolve_root(webRoot))
    {
        MessageBoxW(nullptr, L"Run from the project folder so web/index.html is available.",
                    L"Account Program", MB_ICONERROR | MB_OK);
        CoUninitialize();
        return 1;
    }

    const std::string rootCopy = webRoot;
    std::thread([rootCopy]() { web_start_server(rootCopy, kWebPort); }).detach();
    if (!waitForServer())
    {
        MessageBoxW(nullptr, L"Could not start the embedded web server.", L"Account Program",
                    MB_ICONERROR | MB_OK);
        web_stop_server();
        CoUninitialize();
        return 1;
    }

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"BankAccountWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    const DWORD style = WS_OVERLAPPEDWINDOW;
    RECT frame{0, 0, kLoginWidth, kLoginHeight};
    AdjustWindowRect(&frame, style, FALSE);

    g_hwnd = CreateWindowExW(0, wc.lpszClassName, L"Account Program", style, CW_USEDEFAULT,
                             CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top,
                             nullptr, nullptr, instance, nullptr);

    if (g_hwnd == nullptr)
    {
        web_stop_server();
        CoUninitialize();
        return 1;
    }

    ShowWindow(g_hwnd, show);
    UpdateWindow(g_hwnd);
    initWebView();

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    CoUninitialize();
    return static_cast<int>(message.wParam);
}

#endif
