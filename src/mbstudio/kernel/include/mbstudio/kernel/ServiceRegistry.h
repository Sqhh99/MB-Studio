// MB-Studio kernel - service registry.
//
// Services are interfaces (see include/mbstudio/api) identified by a string
// id (T::kServiceId) rather than by RTTI, so lookups work reliably across
// DLL boundaries. A plugin provides an implementation; any other plugin or
// front end looks it up by interface.
#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "mbstudio/kernel/Export.h"

namespace mbstudio {

struct ServiceEntry {
  std::string id;
  std::string provider;
};

class MBSTUDIO_KERNEL_API ServiceRegistry {
 public:
  template <class T>
  bool provide(std::shared_ptr<T> service, const std::string &provider) {
    return provideRaw(T::kServiceId, std::static_pointer_cast<void>(std::move(service)), provider);
  }

  template <class T>
  std::shared_ptr<T> get() const {
    return std::static_pointer_cast<T>(getRaw(T::kServiceId));
  }

  template <class T>
  bool has() const {
    return getRaw(T::kServiceId) != nullptr;
  }

  bool provideRaw(const std::string &id, std::shared_ptr<void> service, const std::string &provider);
  std::shared_ptr<void> getRaw(const std::string &id) const;
  bool withdraw(const std::string &id);
  std::size_t withdrawProvidedBy(const std::string &provider);
  std::vector<ServiceEntry> list() const;

 private:
  struct Entry {
    std::shared_ptr<void> service;
    std::string provider;
  };
  mutable std::mutex mutex_;
  std::map<std::string, Entry> services_;
};

}  // namespace mbstudio
