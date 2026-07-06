#include "web_runtime.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "class.h"

using namespace std;

namespace
{
atomic<bool> g_running{false};
int g_serverFd = -1;
string g_webRoot = "web";

unique_ptr<account> activeAccount;

bool fileExists(const string& path)
{
    ifstream file(path);
    return file.good();
}

string readFile(const string& path)
{
    ifstream file(path, ios::binary);
    if (!file.is_open())
    {
        return "";
    }
    ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

string jsonEscape(const string& value)
{
    string out;
    for (char c : value)
    {
        if (c == '"')
        {
            out += "\\\"";
        }
        else if (c == '\\')
        {
            out += "\\\\";
        }
        else if (c == '\n')
        {
            out += "\\n";
        }
        else
        {
            out += c;
        }
    }
    return out;
}

string extractJsonString(const string& body, const string& key)
{
    string token = "\"" + key + "\"";
    size_t pos = body.find(token);
    if (pos == string::npos)
    {
        return "";
    }
    pos = body.find(':', pos);
    if (pos == string::npos)
    {
        return "";
    }
    pos = body.find('"', pos);
    if (pos == string::npos)
    {
        return "";
    }
    size_t end = body.find('"', pos + 1);
    if (end == string::npos)
    {
        return "";
    }
    return body.substr(pos + 1, end - pos - 1);
}

double extractJsonNumber(const string& body, const string& key)
{
    string token = "\"" + key + "\"";
    size_t pos = body.find(token);
    if (pos == string::npos)
    {
        return -1.0;
    }
    pos = body.find(':', pos);
    if (pos == string::npos)
    {
        return -1.0;
    }

    ++pos;
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t'))
    {
        ++pos;
    }
    if (pos >= body.size() || body[pos] == 'n' || body[pos] == 'N')
    {
        return -1.0;
    }

    try
    {
        size_t consumed = 0;
        double value = stod(body.substr(pos), &consumed);
        return consumed == 0 ? -1.0 : value;
    }
    catch (...)
    {
        return -1.0;
    }
}

string accountJson(bool loaded)
{
    if (!activeAccount)
    {
        return "{\"error\":\"No account open\"}";
    }

    ostringstream json;
    json << fixed << setprecision(2);
    json << "{\"name\":\"" << jsonEscape(activeAccount->getName()) << "\""
         << ",\"balance\":" << activeAccount->getBalance() << ",\"loaded\":" << (loaded ? "true" : "false")
         << ",\"history\":[";
    const vector<string>& history = activeAccount->getHistory();
    for (size_t i = 0; i < history.size(); ++i)
    {
        if (i > 0)
        {
            json << ',';
        }
        json << "\"" << jsonEscape(history[i]) << "\"";
    }
    json << "]}";
    return json.str();
}

string contentType(const string& path)
{
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html")
    {
        return "text/html; charset=utf-8";
    }
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css")
    {
        return "text/css; charset=utf-8";
    }
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js")
    {
        return "application/javascript; charset=utf-8";
    }
    return "text/plain; charset=utf-8";
}

string httpResponse(int code, const string& status, const string& body, const string& type)
{
    ostringstream response;
    response << "HTTP/1.1 " << code << ' ' << status << "\r\n"
             << "Content-Type: " << type << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "Access-Control-Allow-Origin: *\r\n\r\n"
             << body;
    return response.str();
}

string handleRequest(const string& method, const string& path, const string& body)
{
    if (method == "OPTIONS")
    {
        return "HTTP/1.1 204 No Content\r\nAccess-Control-Allow-Origin: *\r\n"
               "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
               "Access-Control-Allow-Headers: Content-Type\r\nConnection: close\r\n\r\n";
    }

    if (method == "GET")
    {
        string filePath = path;
        if (filePath == "/" || filePath == "/index.html")
        {
            filePath = "/index.html";
        }

        string diskPath = g_webRoot + filePath;
        string fileBody = readFile(diskPath);
        if (fileBody.empty())
        {
            return httpResponse(404, "Not Found", "{\"error\":\"Not found\"}", "application/json");
        }
        return httpResponse(200, "OK", fileBody, contentType(filePath));
    }

    if (method == "POST" && path == "/api/open")
    {
        string name = extractJsonString(body, "name");
        if (name.empty())
        {
            return httpResponse(400, "Bad Request", "{\"error\":\"Name required\"}", "application/json");
        }

        activeAccount = make_unique<account>(name, 0);
        bool loaded = activeAccount->loadFile();
        return httpResponse(200, "OK", accountJson(loaded), "application/json");
    }

    if (!activeAccount)
    {
        return httpResponse(400, "Bad Request", "{\"error\":\"Open an account first\"}", "application/json");
    }

    if (method == "GET" && path == "/api/account")
    {
        return httpResponse(200, "OK", accountJson(true), "application/json");
    }

    if (method == "POST" && path == "/api/deposit")
    {
        double amount = extractJsonNumber(body, "amount");
        if (!activeAccount->deposit(amount))
        {
            return httpResponse(400, "Bad Request", "{\"error\":\"Invalid amount\"}", "application/json");
        }
        activeAccount->saveFile();
        return httpResponse(200, "OK", accountJson(true), "application/json");
    }

    if (method == "POST" && path == "/api/withdraw")
    {
        double amount = extractJsonNumber(body, "amount");
        string errorMsg;
        if (!activeAccount->withdraw(amount, errorMsg))
        {
            ostringstream json;
            json << "{\"error\":\"" << jsonEscape(errorMsg) << "\"}";
            return httpResponse(400, "Bad Request", json.str(), "application/json");
        }
        activeAccount->saveFile();
        return httpResponse(200, "OK", accountJson(true), "application/json");
    }

    if (method == "POST" && path == "/api/save")
    {
        if (!activeAccount->saveFile())
        {
            return httpResponse(500, "Server Error", "{\"error\":\"Save failed\"}", "application/json");
        }
        string msg = "Saved to " + activeAccount->getName() + "_accountINFOCARD.txt";
        ostringstream json;
        json << "{\"message\":\"" << jsonEscape(msg) << "\"}";
        return httpResponse(200, "OK", json.str(), "application/json");
    }

    if (method == "POST" && path == "/api/quit")
    {
        return httpResponse(200, "OK", "{\"ok\":true}", "application/json");
    }

    return httpResponse(404, "Not Found", "{\"error\":\"Not found\"}", "application/json");
}

size_t parseContentLength(const string& headers)
{
    size_t pos = 0;
    while (pos < headers.size())
    {
        size_t lineEnd = headers.find('\n', pos);
        if (lineEnd == string::npos)
        {
            lineEnd = headers.size();
        }
        string line = headers.substr(pos, lineEnd - pos);
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.rfind("Content-Length:", 0) == 0)
        {
            size_t value = 0;
            size_t i = 15;
            while (i < line.size() && line[i] == ' ')
            {
                ++i;
            }
            value = stoul(line.substr(i));
            return value;
        }
        pos = lineEnd + 1;
    }
    return 0;
}

bool isChunked(const string& headers)
{
    return headers.find("Transfer-Encoding: chunked") != string::npos ||
           headers.find("transfer-encoding: chunked") != string::npos;
}

string decodeChunkedBody(const string& chunked)
{
    string body;
    size_t pos = 0;
    while (pos < chunked.size())
    {
        size_t lineEnd = chunked.find("\r\n", pos);
        if (lineEnd == string::npos)
        {
            break;
        }
        size_t chunkSize = stoul(chunked.substr(pos, lineEnd - pos), nullptr, 16);
        pos = lineEnd + 2;
        if (chunkSize == 0)
        {
            break;
        }
        body.append(chunked, pos, chunkSize);
        pos += chunkSize + 2;
    }
    return body;
}

string readHttpRequest(int client)
{
    string raw;
    char buffer[4096];
    size_t contentLength = 0;
    bool chunked = false;

    while (true)
    {
        const ssize_t bytes = read(client, buffer, sizeof(buffer));
        if (bytes <= 0)
        {
            break;
        }
        raw.append(buffer, static_cast<size_t>(bytes));

        const size_t headerEnd = raw.find("\r\n\r\n");
        if (headerEnd == string::npos)
        {
            continue;
        }

        const string headers = raw.substr(0, headerEnd);
        if (contentLength == 0)
        {
            contentLength = parseContentLength(headers);
            chunked = isChunked(headers);
        }

        const size_t bodyStart = headerEnd + 4;
        if (chunked)
        {
            if (raw.find("0\r\n\r\n", bodyStart) != string::npos)
            {
                break;
            }
            continue;
        }

        if (contentLength == 0)
        {
            break;
        }

        if (raw.size() >= bodyStart + contentLength)
        {
            break;
        }
    }

    return raw;
}

void serveClient(int client)
{
    const string raw = readHttpRequest(client);
    if (raw.empty())
    {
        close(client);
        return;
    }

    istringstream request(raw);
    string method;
    string path;
    request >> method >> path;

    string body;
    const size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd != string::npos)
    {
        const string headers = raw.substr(0, headerEnd);
        const size_t bodyStart = headerEnd + 4;
        const size_t contentLength = parseContentLength(headers);
        if (contentLength > 0 && raw.size() >= bodyStart + contentLength)
        {
            body = raw.substr(bodyStart, contentLength);
        }
        else if (isChunked(headers))
        {
            body = decodeChunkedBody(raw.substr(bodyStart));
        }
    }

    string response = handleRequest(method, path, body);
    write(client, response.c_str(), response.size());
    close(client);
}
} // namespace

bool web_resolve_root(string& root_out)
{
    const string candidates[] = {"web", "../web", "../../web"};
    for (const string& candidate : candidates)
    {
        if (fileExists(candidate + "/index.html"))
        {
            root_out = candidate;
            return true;
        }
    }
    return fileExists(root_out + "/index.html");
}

bool web_start_server(const string& web_root, int port)
{
    g_webRoot = web_root;
    g_running = true;

    g_serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_serverFd < 0)
    {
        return false;
    }

    int opt = 1;
    setsockopt(g_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(g_serverFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
    {
        close(g_serverFd);
        g_serverFd = -1;
        return false;
    }

    if (listen(g_serverFd, 8) < 0)
    {
        close(g_serverFd);
        g_serverFd = -1;
        return false;
    }

    while (g_running)
    {
        int client = accept(g_serverFd, nullptr, nullptr);
        if (client < 0)
        {
            break;
        }
        serveClient(client);
    }

    if (g_serverFd >= 0)
    {
        close(g_serverFd);
        g_serverFd = -1;
    }

    return true;
}

void web_stop_server()
{
    g_running = false;
    if (g_serverFd >= 0)
    {
        shutdown(g_serverFd, SHUT_RDWR);
        close(g_serverFd);
        g_serverFd = -1;
    }
}
