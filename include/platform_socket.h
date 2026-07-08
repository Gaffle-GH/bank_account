#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using socket_t = SOCKET;
inline constexpr socket_t kInvalidSocket = INVALID_SOCKET;

inline void platform_socket_init()
{
    static bool initialized = false;
    if (!initialized)
    {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
        initialized = true;
    }
}

inline void platform_socket_shutdown()
{
    WSACleanup();
}

inline void socket_close(socket_t fd)
{
    if (fd != kInvalidSocket)
    {
        closesocket(fd);
    }
}

inline int socket_shutdown(socket_t fd)
{
    return shutdown(fd, SD_BOTH);
}

inline int socket_read(socket_t fd, char* buffer, int length)
{
    return recv(fd, buffer, length, 0);
}

inline int socket_write(socket_t fd, const char* buffer, int length)
{
    return send(fd, buffer, length, 0);
}

inline int socket_last_error()
{
    return WSAGetLastError();
}

#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using socket_t = int;
inline constexpr socket_t kInvalidSocket = -1;

inline void platform_socket_init() {}
inline void platform_socket_shutdown() {}

inline void socket_close(socket_t fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}

inline int socket_shutdown(socket_t fd)
{
    return ::shutdown(fd, SHUT_RDWR);
}

inline int socket_read(socket_t fd, char* buffer, int length)
{
    return static_cast<int>(read(fd, buffer, static_cast<size_t>(length)));
}

inline int socket_write(socket_t fd, const char* buffer, int length)
{
    return static_cast<int>(write(fd, buffer, static_cast<size_t>(length)));
}

inline int socket_last_error()
{
    return errno;
}
#endif
