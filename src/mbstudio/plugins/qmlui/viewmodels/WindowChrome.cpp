#include "WindowChrome.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <algorithm>

#ifdef Q_OS_WIN
#include <QAbstractNativeEventFilter>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#endif

namespace mbstudio::ui {

#ifdef Q_OS_WIN

// Native hit testing and frame handling of the main window.
class WindowChrome::NativeFilter : public QAbstractNativeEventFilter {
 public:
  explicit NativeFilter(WindowChrome *chrome) : chrome_(chrome) {}

  bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override {
    // Never call QWindow::winId() here: messages arrive while the native
    // window is being created, and winId() would start a nested creation
    // that fails (CreateWindowEx: "The parameter is incorrect"), after which
    // Qt retries without end. The handle is stored once it exists.
    if (eventType != "windows_generic_MSG" || chrome_->hwnd_ == nullptr)
      return false;
    MSG *msg = static_cast<MSG *>(message);
    const HWND hwnd = static_cast<HWND>(chrome_->hwnd_);
    if (msg->hwnd != hwnd)
      return false;
    switch (msg->message) {
      case WM_NCCALCSIZE:
        if (msg->wParam == TRUE) {
          // the whole window is client area; a maximised window would
          // otherwise extend beyond the monitor by its invisible frame
          if (IsZoomed(hwnd)) {
            auto *params = reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam);
            MONITORINFO info;
            info.cbSize = sizeof(info);
            if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &info))
              params->rgrc[0] = info.rcWork;
          }
          *result = 0;
          return true;
        }
        return false;
      case WM_NCHITTEST: {
        POINT pt{GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
        ScreenToClient(hwnd, &pt);
        const double dpr = chrome_->window_->devicePixelRatio();
        switch (chrome_->hitAt(QPointF(pt.x / dpr, pt.y / dpr))) {
          case Caption: *result = HTCAPTION; break;
          case Left: *result = HTLEFT; break;
          case Right: *result = HTRIGHT; break;
          case Top: *result = HTTOP; break;
          case Bottom: *result = HTBOTTOM; break;
          case TopLeft: *result = HTTOPLEFT; break;
          case TopRight: *result = HTTOPRIGHT; break;
          case BottomLeft: *result = HTBOTTOMLEFT; break;
          case BottomRight: *result = HTBOTTOMRIGHT; break;
          default: *result = HTCLIENT; break;
        }
        return true;
      }
      case WM_NCACTIVATE:
        // keep the system from painting its frame when the activation changes
        *result = DefWindowProcW(hwnd, msg->message, msg->wParam, -1);
        return true;
      default:
        return false;
    }
  }

 private:
  WindowChrome *chrome_;
};

#else

class WindowChrome::NativeFilter {};

#endif

WindowChrome::WindowChrome(QObject *parent) : QObject(parent) {}

WindowChrome::~WindowChrome() {
#ifdef Q_OS_WIN
  if (filter_ && QCoreApplication::instance() != nullptr)
    QCoreApplication::instance()->removeNativeEventFilter(filter_.get());
#endif
}

QObject *WindowChrome::window() const { return window_.data(); }

void WindowChrome::setWindow(QObject *window) {
  auto *w = qobject_cast<QQuickWindow *>(window);
  if (w == window_.data())
    return;
  window_ = w;
  if (window_) {
    connect(window_.data(), &QWindow::windowStateChanged, this, [this]() { emit windowStateChanged(); });
    connect(window_.data(), &QWindow::activeChanged, this, [this]() { emit windowActiveChanged(); });
    auto track = [this]() {
      if (window_ && window_->windowState() == Qt::WindowNoState && window_->isVisible())
        normalGeometry_ = window_->geometry();
    };
    connect(window_.data(), &QWindow::xChanged, this, track);
    connect(window_.data(), &QWindow::yChanged, this, track);
    connect(window_.data(), &QWindow::widthChanged, this, track);
    connect(window_.data(), &QWindow::heightChanged, this, track);
    install();
  }
  emit windowChanged();
}

void WindowChrome::setTitleBar(QQuickItem *item) {
  if (item == titleBar_.data())
    return;
  titleBar_ = item;
  emit titleBarChanged();
}

void WindowChrome::install() {
  // The system title bar is the default. The title bar drawn by MB-Studio is
  // opt-in (MBSTUDIO_CUSTOM_TITLEBAR=1 or the "ui/customTitleBar" setting).
  // In 0.3.0 it was the default and the main window could not be created on
  // Windows (see NativeFilter), so MB-Studio did not start.
  const QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("MB-Studio"),
                           QStringLiteral("mbstudio"));
  const bool system = qEnvironmentVariableIntValue("MBSTUDIO_NATIVE_TITLEBAR") == 1;
  const bool custom = qEnvironmentVariableIntValue("MBSTUDIO_CUSTOM_TITLEBAR") == 1 ||
                      settings.value(QStringLiteral("ui/customTitleBar"), false).toBool();
  active_ = custom && !system;
  if (active_) {
#ifdef Q_OS_WIN
    if (QGuiApplication::platformName() == QLatin1String("windows")) {
      // An ordinary window: it keeps its frame styles for Snap, the minimise
      // and maximise animations and the system menu, and WM_NCCALCSIZE makes
      // the whole window client area, which removes the system title bar.
      // The native window is created first, then its messages are filtered.
      const HWND hwnd = reinterpret_cast<HWND>(window_->winId());
      if (hwnd != nullptr) {
        hwnd_ = hwnd;
        filter_ = std::make_unique<NativeFilter>(this);
        QCoreApplication::instance()->installNativeEventFilter(filter_.get());
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
        nativeFrame_ = true;
      } else {
        // no native window: keep the system title bar
        active_ = false;
      }
    } else
#endif
    {
      window_->setFlags(window_->flags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint |
                        Qt::WindowSystemMenuHint);
    }
  }
  emit activeChanged();
}

bool WindowChrome::maximized() const {
  return window_ && (window_->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen));
}

bool WindowChrome::windowActive() const { return window_ && window_->isActive(); }

void WindowChrome::minimize() {
  if (window_)
    window_->showMinimized();
}

void WindowChrome::toggleMaximize() {
  if (!window_)
    return;
  if (maximized())
    window_->showNormal();
  else
    window_->showMaximized();
}

void WindowChrome::close() {
  if (window_)
    window_->close();
}

void WindowChrome::startMove() {
  if (window_)
    window_->startSystemMove();
}

void WindowChrome::startResize(int edges) {
  if (window_ && !maximized())
    window_->startSystemResize(Qt::Edges(edges));
}

WindowChrome::Hit WindowChrome::hitTest(const QPointF &pos, const QSizeF &size, double border, bool maximized,
                                        bool inTitleBar, bool onControl) {
  if (!maximized) {
    // corners are easier to grab than the edges
    const double corner = 2.0 * border;
    const bool left = pos.x() < border, right = pos.x() >= size.width() - border;
    const bool top = pos.y() < border, bottom = pos.y() >= size.height() - border;
    const bool nearLeft = pos.x() < corner, nearRight = pos.x() >= size.width() - corner;
    const bool nearTop = pos.y() < corner, nearBottom = pos.y() >= size.height() - corner;
    if ((top && nearLeft) || (left && nearTop))
      return TopLeft;
    if ((top && nearRight) || (right && nearTop))
      return TopRight;
    if ((bottom && nearLeft) || (left && nearBottom))
      return BottomLeft;
    if ((bottom && nearRight) || (right && nearBottom))
      return BottomRight;
    if (left)
      return Left;
    if (right)
      return Right;
    if (top)
      return Top;
    if (bottom)
      return Bottom;
  }
  return inTitleBar && !onControl ? Caption : Client;
}

bool WindowChrome::onControl(QQuickItem *titleBar, const QPointF &scenePos) {
  if (titleBar == nullptr)
    return false;
  QQuickItem *item = titleBar;
  for (int depth = 0; depth < 64; depth++) {
    const QPointF local = item->mapFromScene(scenePos);
    QQuickItem *child = item->childAt(local.x(), local.y());
    if (child == nullptr)
      return false;
    // buttons, menus and fields take the mouse; labels and backgrounds do
    // not (Text accepts the left button only for links)
    if (child->isEnabled() && child->acceptedMouseButtons() != Qt::NoButton && !child->inherits("QQuickText"))
      return true;
    if (child->property("chromeInteractive").toBool())
      return true;
    item = child;
  }
  return false;
}

WindowChrome::Hit WindowChrome::hitAt(const QPointF &pos) const {
  if (!window_)
    return Client;
  bool inTitleBar = false, control = false;
  if (titleBar_ && titleBar_->isVisible()) {
    const QPointF local = titleBar_->mapFromScene(pos);
    inTitleBar = titleBar_->contains(local);
    control = inTitleBar && onControl(titleBar_.data(), pos);
  }
  return hitTest(pos, QSizeF(window_->width(), window_->height()), resizeBorder(), maximized(), inTitleBar, control);
}

QRect WindowChrome::fitToScreens(const QRect &rect, const QList<QRect> &screens, const QRect &primary) {
  QRect r = rect;
  if (!r.isValid() || r.width() < 200 || r.height() < 150)
    r = QRect(0, 0, std::min(1440, primary.width()), std::min(900, primary.height()));
  for (const QRect &screen : screens) {
    const QRect common = screen.intersected(r);
    // enough of the window, and its title bar, on this screen
    if (common.width() >= 200 && common.height() >= 100 && r.top() >= screen.top() && r.top() < screen.bottom() - 40) {
      if (r.width() > screen.width())
        r.setWidth(screen.width());
      if (r.height() > screen.height())
        r.setHeight(screen.height());
      return r;
    }
  }
  r.setWidth(std::min(r.width(), primary.width()));
  r.setHeight(std::min(r.height(), primary.height()));
  r.moveCenter(primary.center());
  return r;
}

void WindowChrome::saveGeometry(const QString &key) {
  if (!window_)
    return;
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("MB-Studio"), QStringLiteral("mbstudio"));
  const QRect normal = normalGeometry_.isValid() ? normalGeometry_ : window_->geometry();
  settings.setValue(QStringLiteral("window/%1/geometry").arg(key), normal);
  settings.setValue(QStringLiteral("window/%1/maximized").arg(key), maximized());
}

bool WindowChrome::restoreGeometry(const QString &key, int defaultWidth, int defaultHeight) {
  if (!window_)
    return false;
  const QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("MB-Studio"),
                           QStringLiteral("mbstudio"));
  const QRect saved = settings.value(QStringLiteral("window/%1/geometry").arg(key)).toRect();
  const bool wasMaximized = settings.value(QStringLiteral("window/%1/maximized").arg(key), false).toBool();
  QList<QRect> screens;
  for (QScreen *s : QGuiApplication::screens())
    screens << s->availableGeometry();
  const QRect primary = QGuiApplication::primaryScreen() != nullptr ? QGuiApplication::primaryScreen()->availableGeometry()
                                                                     : QRect(0, 0, defaultWidth, defaultHeight);
  QRect wanted = saved;
  if (!wanted.isValid()) {
    wanted = QRect(0, 0, std::min(defaultWidth, primary.width()), std::min(defaultHeight, primary.height()));
    wanted.moveCenter(primary.center());
  }
  const QRect r = screens.isEmpty() ? wanted : fitToScreens(wanted, screens, primary);
  window_->setGeometry(r);
  normalGeometry_ = r;
  if (wasMaximized && saved.isValid())
    window_->showMaximized();
  else
    window_->show();
  return saved.isValid();
}

}  // namespace mbstudio::ui
