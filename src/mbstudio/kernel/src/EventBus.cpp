#include "mbstudio/kernel/EventBus.h"

#include <vector>

namespace mbstudio {

EventBus::EventBus() = default;
EventBus::~EventBus() = default;

bool EventBus::matches(const std::string &pattern, const std::string &topic) {
  if (pattern == "*" || pattern == topic)
    return true;
  if (pattern.size() >= 2 && pattern.compare(pattern.size() - 2, 2, ".*") == 0) {
    const std::string prefix = pattern.substr(0, pattern.size() - 1);  // keep the '.'
    return topic.compare(0, prefix.size(), prefix) == 0;
  }
  return false;
}

EventBus::SubscriptionId EventBus::subscribe(const std::string &pattern, Handler handler) {
  std::lock_guard<std::mutex> lock(mutex_);
  const SubscriptionId id = next_++;
  subscriptions_[id] = Subscription{pattern, std::make_shared<Handler>(std::move(handler))};
  return id;
}

bool EventBus::unsubscribe(SubscriptionId id) {
  std::lock_guard<std::mutex> lock(mutex_);
  return subscriptions_.erase(id) > 0;
}

std::size_t EventBus::publish(const Event &event) const {
  // Copy the matching handlers so that handlers may (un)subscribe or publish
  // without dead-locking.
  std::vector<std::shared_ptr<Handler>> targets;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &kv : subscriptions_)
      if (matches(kv.second.pattern, event.topic))
        targets.push_back(kv.second.handler);
  }
  for (const auto &handler : targets)
    (*handler)(event);
  return targets.size();
}

std::size_t EventBus::publish(const std::string &topic, ValueMap data) const {
  return publish(Event{topic, std::move(data)});
}

std::size_t EventBus::subscriberCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return subscriptions_.size();
}

}  // namespace mbstudio
