#pragma once

#include <string>

// Looks up a web asset (e.g. "/index.html") that was embedded into the
// executable at build time. Returns true and fills out_data when found.
bool web_asset_get(const std::string& route, std::string& out_data);
