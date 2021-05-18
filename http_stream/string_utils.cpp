#include "string_utils.h"

#include <cctype>
#include <string_view>

static const std::string_view kWhiteSpaceChars = " \t\r\n";

void trimLeft(std::string_view* s) {
  s->remove_prefix(s->find_first_not_of(kWhiteSpaceChars));
}

void trimRight(std::string_view* s) {
  *s = s->substr(0, s->find_last_not_of(kWhiteSpaceChars) + 1);
}

bool strcasecmp(const std::string_view a, const std::string_view b) {
  return std::equal(
      a.begin(), a.end(),
      b.begin(), b.end(),
      [] (const char& a, const char& b)
      {
          return std::tolower(a) == std::tolower(b);
      });
}

bool splitTagValue(const std::string_view line,
                   std::string_view* tag,
                   std::string_view* value) {
  *tag = std::string_view();
  *value = std::string_view();

  auto p = line.find(':');
  if (p == std::string_view::npos) {
    return false;
  }

  *tag = line.substr(0, p);
  trimRight(tag);

  *value = line.substr(p + 1);
  trimLeft(value);
  trimRight(value);

  return true;
}
