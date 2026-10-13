#include "mbstudio/kernel/Value.h"

#include <cmath>
#include <cstdio>
#include <sstream>

namespace mbstudio {

std::string jsonEscape(const std::string &text) {
  std::string out;
  out.reserve(text.size() + 2);
  for (unsigned char c : text) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          char buffer[8];
          std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
          out += buffer;
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  return out;
}

namespace {

std::string formatDouble(double v) {
  if (!std::isfinite(v))
    return "null";
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%.10g", v);
  return buffer;
}

std::string toJson(const Value &value) {
  struct Visitor {
    std::string operator()(std::monostate) const { return "null"; }
    std::string operator()(bool b) const { return b ? "true" : "false"; }
    std::string operator()(std::int64_t i) const { return std::to_string(i); }
    std::string operator()(double d) const { return formatDouble(d); }
    std::string operator()(const std::string &s) const { return "\"" + jsonEscape(s) + "\""; }
  };
  return std::visit(Visitor{}, value);
}

}  // namespace

std::string valueToString(const Value &value) {
  struct Visitor {
    std::string operator()(std::monostate) const { return std::string(); }
    std::string operator()(bool b) const { return b ? "true" : "false"; }
    std::string operator()(std::int64_t i) const { return std::to_string(i); }
    std::string operator()(double d) const { return formatDouble(d); }
    std::string operator()(const std::string &s) const { return s; }
  };
  return std::visit(Visitor{}, value);
}

std::string valueMapToJson(const ValueMap &values) {
  std::string out = "{";
  bool first = true;
  for (const auto &kv : values) {
    if (!first)
      out += ", ";
    first = false;
    out += "\"" + jsonEscape(kv.first) + "\": " + toJson(kv.second);
  }
  out += "}";
  return out;
}

std::string getString(const ValueMap &values, const std::string &key, const std::string &fallback) {
  const auto it = values.find(key);
  if (it == values.end() || std::holds_alternative<std::monostate>(it->second))
    return fallback;
  return valueToString(it->second);
}

std::int64_t getInt(const ValueMap &values, const std::string &key, std::int64_t fallback) {
  const auto it = values.find(key);
  if (it == values.end())
    return fallback;
  if (const auto *i = std::get_if<std::int64_t>(&it->second))
    return *i;
  if (const auto *d = std::get_if<double>(&it->second))
    return static_cast<std::int64_t>(*d);
  if (const auto *b = std::get_if<bool>(&it->second))
    return *b ? 1 : 0;
  if (const auto *s = std::get_if<std::string>(&it->second)) {
    try {
      return std::stoll(*s);
    } catch (...) {
      return fallback;
    }
  }
  return fallback;
}

double getDouble(const ValueMap &values, const std::string &key, double fallback) {
  const auto it = values.find(key);
  if (it == values.end())
    return fallback;
  if (const auto *d = std::get_if<double>(&it->second))
    return *d;
  if (const auto *i = std::get_if<std::int64_t>(&it->second))
    return static_cast<double>(*i);
  if (const auto *s = std::get_if<std::string>(&it->second)) {
    try {
      return std::stod(*s);
    } catch (...) {
      return fallback;
    }
  }
  return fallback;
}

bool getBool(const ValueMap &values, const std::string &key, bool fallback) {
  const auto it = values.find(key);
  if (it == values.end())
    return fallback;
  if (const auto *b = std::get_if<bool>(&it->second))
    return *b;
  if (const auto *i = std::get_if<std::int64_t>(&it->second))
    return *i != 0;
  if (const auto *s = std::get_if<std::string>(&it->second))
    return *s == "1" || *s == "true" || *s == "yes" || *s == "on";
  return fallback;
}

}  // namespace mbstudio
