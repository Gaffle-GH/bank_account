#pragma once

#include <string>

constexpr int kWebPort = 8765;

bool web_resolve_root(std::string& root_out);
bool web_start_server(const std::string& web_root, int port);
void web_stop_server();
