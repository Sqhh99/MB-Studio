#include "ToolsViewModel.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>

#include "mbstudio/platform/Process.h"

namespace mbstudio::ui {

namespace {
constexpr int kMaxOutput = 2 * 1024 * 1024;  // characters kept in the console
}

ToolsViewModel::ToolsViewModel(std::shared_ptr<IToolService> service, QObject *parent)
    : QAbstractListModel(parent), service_(std::move(service)) {
  if (service_)
    for (const auto &t : service_->tools())
      tools_.push_back(t);
  workingDirectory_ = QDir::homePath();
  rebuild();
  connect(&watcher_, &QFutureWatcher<platform::ProcessResult>::finished, this, [this]() {
    const platform::ProcessResult result = watcher_.result();
    running_ = false;
    exitCode_ = result.exitCode;
    lastError_ = QString::fromStdString(result.error);
    if (result.cancelled)
      appendOutput(tr("\n[stopped by user]\n"));
    else if (!result.started)
      appendOutput(QStringLiteral("\n[%1]\n").arg(lastError_));
    else
      appendOutput(tr("\n[exit code %1]\n").arg(result.exitCode));
    emit runningChanged();
    emit finished(exitCode_);
  });
}

ToolsViewModel::~ToolsViewModel() {
  stop();
  watcher_.waitForFinished();
}

int ToolsViewModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : count(); }

QVariant ToolsViewModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= visible_.size())
    return {};
  const ToolInfo &t = tools_[visible_[index.row()]];
  switch (role) {
    case NameRole: return QString::fromStdString(t.name);
    case CategoryRole: return QString::fromStdString(t.category);
    case SummaryRole: return QString::fromStdString(t.summary);
    case ExampleRole: return QString::fromStdString(t.example);
    case AvailableRole: return t.available;
    case PathRole: return QString::fromStdString(t.path);
    default: return {};
  }
}

QHash<int, QByteArray> ToolsViewModel::roleNames() const {
  return {{NameRole, "name"},       {CategoryRole, "category"},   {SummaryRole, "summary"},
          {ExampleRole, "example"}, {AvailableRole, "available"}, {PathRole, "path"}};
}

void ToolsViewModel::setFilter(const QString &filter) {
  if (filter == filter_)
    return;
  filter_ = filter;
  rebuild();
  emit filterChanged();
}

void ToolsViewModel::rebuild() {
  beginResetModel();
  visible_.clear();
  const QString needle = filter_.trimmed();
  for (int i = 0; i < tools_.size(); i++) {
    const ToolInfo &t = tools_[i];
    if (needle.isEmpty() || QString::fromStdString(t.name).contains(needle, Qt::CaseInsensitive) ||
        QString::fromStdString(t.summary).contains(needle, Qt::CaseInsensitive) ||
        QString::fromStdString(t.category).contains(needle, Qt::CaseInsensitive))
      visible_.push_back(i);
  }
  endResetModel();
  emit countChanged();
}

int ToolsViewModel::availableCount() const {
  int n = 0;
  for (const auto &t : tools_)
    n += t.available ? 1 : 0;
  return n;
}

void ToolsViewModel::setCurrentIndex(int index) {
  // index is a row of the (filtered) model
  const int toolIndex = (index >= 0 && index < visible_.size()) ? visible_[index] : -1;
  if (toolIndex == current_)
    return;
  current_ = toolIndex;
  emit currentChanged();
  emit argumentsChanged();
}

QVariantMap ToolsViewModel::current() const {
  QVariantMap out;
  if (current_ < 0)
    return out;
  const ToolInfo &t = tools_[current_];
  out[QStringLiteral("name")] = QString::fromStdString(t.name);
  out[QStringLiteral("category")] = QString::fromStdString(t.category);
  out[QStringLiteral("summary")] = QString::fromStdString(t.summary);
  out[QStringLiteral("example")] = QString::fromStdString(t.example);
  out[QStringLiteral("available")] = t.available;
  out[QStringLiteral("path")] = QString::fromStdString(t.path);
  return out;
}

bool ToolsViewModel::selectTool(const QString &name) {
  for (int i = 0; i < tools_.size(); i++) {
    if (QString::fromStdString(tools_[i].name) == name) {
      if (current_ != i) {
        current_ = i;
        emit currentChanged();
        emit argumentsChanged();
      }
      return true;
    }
  }
  return false;
}

void ToolsViewModel::setArguments(const QString &arguments) {
  if (arguments == arguments_)
    return;
  arguments_ = arguments;
  emit argumentsChanged();
}

void ToolsViewModel::setWorkingDirectory(const QString &directory) {
  if (directory == workingDirectory_)
    return;
  workingDirectory_ = directory;
  emit argumentsChanged();
}

void ToolsViewModel::useExample() {
  if (current_ >= 0)
    setArguments(QString::fromStdString(tools_[current_].example));
}

QString ToolsViewModel::commandPreview() const {
  if (current_ < 0)
    return QString();
  return QString::fromStdString(
      platform::formatCommand(tools_[current_].name, platform::splitArguments(arguments_.toStdString())));
}

void ToolsViewModel::run() {
  if (current_ < 0)
    return;
  QStringList args;
  for (const auto &a : platform::splitArguments(arguments_.toStdString()))
    args << QString::fromStdString(a);
  start(QString::fromStdString(tools_[current_].name), args);
}

void ToolsViewModel::showHelp() {
  if (current_ >= 0)
    start(QString::fromStdString(tools_[current_].name), {QStringLiteral("-h")});
}

void ToolsViewModel::start(const QString &tool, const QStringList &arguments) {
  if (!service_ || running_)
    return;
  running_ = true;
  exitCode_ = 0;
  lastError_.clear();
  std::vector<std::string> args;
  for (const auto &a : arguments)
    args.push_back(a.toStdString());
  appendOutput(QStringLiteral("$ %1\n").arg(QString::fromStdString(platform::formatCommand(tool.toStdString(), args))));
  emit runningChanged();

  cancel_ = std::make_shared<std::atomic<bool>>(false);
  auto service = service_;
  auto cancel = cancel_;
  const std::string name = tool.toStdString();
  const std::string cwd = workingDirectory_.toStdString();
  QPointer<ToolsViewModel> self(this);
  watcher_.setFuture(QtConcurrent::run([service, cancel, name, args, cwd, self]() {
    return service->run(
        name, args, cwd,
        [self](const std::string &chunk) {
          const QString text = QString::fromUtf8(chunk.data(), static_cast<int>(chunk.size()));
          if (!self)
            return;
          QMetaObject::invokeMethod(
              self.data(),
              [self, text]() {
                if (self)
                  self->appendOutput(text);
              },
              Qt::QueuedConnection);
        },
        cancel.get());
  }));
}

void ToolsViewModel::stop() {
  if (cancel_)
    cancel_->store(true);
}

void ToolsViewModel::appendOutput(const QString &text) {
  QString cleaned = text;
  cleaned.remove(QLatin1Char('\r'));
  output_ += cleaned;
  if (output_.size() > kMaxOutput)
    output_ = output_.right(kMaxOutput);
  emit outputChanged();
}

void ToolsViewModel::clearOutput() {
  output_.clear();
  emit outputChanged();
}

bool ToolsViewModel::waitForFinished(int timeoutMs) {
  QElapsedTimer timer;
  timer.start();
  while (running_ && timer.elapsed() < timeoutMs)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  QCoreApplication::processEvents();  // deliver queued output
  return !running_;
}

}  // namespace mbstudio::ui
