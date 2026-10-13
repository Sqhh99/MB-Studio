#include "mbstudio/kernel/ServiceRegistry.h"

namespace mbstudio {

bool ServiceRegistry::provideRaw(const std::string &id, std::shared_ptr<void> service, const std::string &provider) {
  if (id.empty() || !service)
    return false;
  std::lock_guard<std::mutex> lock(mutex_);
  return services_.emplace(id, Entry{std::move(service), provider}).second;
}

std::shared_ptr<void> ServiceRegistry::getRaw(const std::string &id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = services_.find(id);
  return it == services_.end() ? nullptr : it->second.service;
}

bool ServiceRegistry::withdraw(const std::string &id) {
  std::lock_guard<std::mutex> lock(mutex_);
  return services_.erase(id) > 0;
}

std::size_t ServiceRegistry::withdrawProvidedBy(const std::string &provider) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::size_t removed = 0;
  for (auto it = services_.begin(); it != services_.end();) {
    if (it->second.provider == provider) {
      it = services_.erase(it);
      removed++;
    } else {
      ++it;
    }
  }
  return removed;
}

std::vector<ServiceEntry> ServiceRegistry::list() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<ServiceEntry> out;
  for (const auto &kv : services_)
    out.push_back(ServiceEntry{kv.first, kv.second.provider});
  return out;
}

}  // namespace mbstudio
