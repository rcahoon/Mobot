#pragma once

#include <string_view>

void trimLeft(std::string_view* s);
void trimRight(std::string_view* s);

bool strcasecmp(const std::string_view a, const std::string_view b);

bool splitTagValue(const std::string_view line,
                   std::string_view* tag,
                   std::string_view* value);
