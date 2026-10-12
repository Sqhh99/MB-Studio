#pragma once
#include <string>

namespace mbstudio::test {
class IGreeter {
 public:
  static constexpr const char *kServiceId = "test.greeter";
  virtual ~IGreeter() = default;
  virtual std::string greet(const std::string &name) const = 0;
};
}  // namespace mbstudio::test
