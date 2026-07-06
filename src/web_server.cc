/*
 * Minimal local web server — serves web/ (HTML/CSS/JS) and talks to account.cc
 */

#include <cstdlib>
#include <iostream>
#include <string>

#include "web_runtime.h"

using namespace std;

namespace
{
void openBrowser()
{
    string cmd = "open http://127.0.0.1:" + to_string(kWebPort);
    system(cmd.c_str());
}
} // namespace

int main(int argc, char* argv[])
{
    bool openBrowserOnStart = true;
    string webRoot = "web";

    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "--no-browser")
        {
            openBrowserOnStart = false;
        }
        else if (arg[0] != '-')
        {
            webRoot = arg;
        }
    }

    if (!web_resolve_root(webRoot))
    {
        cerr << "Could not find web UI in " << webRoot << '\n';
        cerr << "Run from the project folder, or pass a path: ./bin/account_web path/to/web\n";
        return 1;
    }

    cout << "Bank Account UI: http://127.0.0.1:" << kWebPort << '\n';
    cout << "Serving UI from: " << webRoot << '\n';
    cout << "Edit web/index.html and web/styles.css, then refresh the browser.\n";

    if (openBrowserOnStart)
    {
        openBrowser();
    }

    if (!web_start_server(webRoot, kWebPort))
    {
        cerr << "Failed to bind port " << kWebPort << '\n';
        return 1;
    }

    return 0;
}
