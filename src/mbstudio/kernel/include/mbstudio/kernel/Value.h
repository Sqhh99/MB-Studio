// MB-Studio kernel - dynamically typed values exchanged on the event bus and
// through commands. Keeping the payload types this simple lets any front end
// (QML, a command line, a test) talk to any plugin without sharing headers.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <variant>

#include "mbstudio/kernel/Export.h"

namespace mbstudio {

using Value = std::variant<std::monostate, bool, std::int64_t, double, std::string>;
using ValueMap = std::map<std::string, Value>;

MBSTUDIO_KERNEL_API std::string valueToString(const Value &value);
MBSTUDIO_KERNEL_API std::string valueMapToJson(const ValueMap &values);
MBSTUDIO_KERNEL_API std::string jsonEscape(const std::string &text);

// Typed access with a fallback when the key is missing or of another type
// (integers and doubles convert into each other).
MBSTUDIO_KERNEL_API std::string getString(const ValueMap &values, const std::string &key,
                                          const std::string &fallback = std::string());
MBSTUDIO_KERNEL_API std::int64_t getInt(const ValueMap &values, const std::string &key,
                                        std::int64_t fallback = 0);
MBSTUDIO_KERNEL_API double getDouble(const ValueMap &values, const std::string &key, double fallback = 0.0);
MBSTUDIO_KERNEL_API bool getBool(const ValueMap &values, const std::string &key, bool fallback = false);

}  // namespace mbstudio
