// MB-Studio kernel - publish/subscribe event bus.
//
// Topics are dot separated ("job.output", "log.info"). A subscription to
// "job.*" receives every topic starting with "job.", "*" receives everything.
// Handlers run synchronously on the publishing thread; a front end that needs
// its own thread (e.g. the Qt GUI thread) marshals the event itself.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "mbstudio/kernel/Export.h"
#include "mbstudio/kernel/Value.h"

namespace mbstudio {

struct Event {
  std::string topic;
  ValueMap data;
};

class MBSTUDIO_KERNEL_API EventBus {
 public:
  using Handler = std::function<void(const Event &)>;
  using SubscriptionId = std::uint64_t;

  EventBus();
  ~EventBus();
  EventBus(const EventBus &) = delete;
  EventBus &operator=(const EventBus &) = delete;

  SubscriptionId subscribe(const std::string &pattern, Handler handler);
  bool unsubscribe(SubscriptionId id);
  // Returns the number of handlers the event was delivered to.
  std::size_t publish(const Event &event) const;
  std::size_t publish(const std::string &topic, ValueMap data = {}) const;
  std::size_t subscriberCount() const;

  static bool matches(const std::string &pattern, const std::string &topic);

 private:
  struct Subscription {
    std::string pattern;
    std::shared_ptr<Handler> handler;
  };
  mutable std::mutex mutex_;
  std::map<SubscriptionId, Subscription> subscriptions_;
  SubscriptionId next_ = 1;
};

}  // namespace mbstudio
