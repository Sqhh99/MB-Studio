// MB-Studio QML UI - window chrome: the workbench draws its own title bar
// (application and project identity, the menus, the window buttons) and
// the system title bar is hidden.
//
// On Windows the window keeps its native frame styles and answers the
// window manager's hit test itself: the empty parts of the title bar are
// the caption (dragging, double click to maximise, Snap, the system menu
// with Alt+Space or a right click), the edges and corners resize, and the
// controls inside the title bar stay ordinary controls. Elsewhere the title
// bar asks the window system to move or resize the window
// (QWindow::startSystemMove / startSystemResize).
//
// The system title bar is the default; the title bar drawn by MB-Studio is
// opt-in with MBSTUDIO_CUSTOM_TITLEBAR=1 or the "ui/customTitleBar" setting
// (MBSTUDIO_NATIVE_TITLEBAR=1 always keeps the system title bar). Window
// geometry is saved per key and restored onto a screen that still exists.
#pragma once

#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRect>
#include <memory>

namespace mbstudio::ui {

class WindowChrome : public QObject {
  Q_OBJECT
  Q_PROPERTY(QObject *window READ window WRITE setWindow NOTIFY windowChanged)
  Q_PROPERTY(QQuickItem *titleBar READ titleBar WRITE setTitleBar NOTIFY titleBarChanged)
  // the application draws the title bar (false: the system title bar is used)
  Q_PROPERTY(bool active READ active NOTIFY activeChanged)
  // the window system handles moving, resizing and Snap through hit testing
  Q_PROPERTY(bool nativeFrame READ nativeFrame NOTIFY activeChanged)
  Q_PROPERTY(bool maximized READ maximized NOTIFY windowStateChanged)
  Q_PROPERTY(bool windowActive READ windowActive NOTIFY windowActiveChanged)
  Q_PROPERTY(int resizeBorder READ resizeBorder CONSTANT)

 public:
  enum Hit { Client, Caption, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };
  Q_ENUM(Hit)

  explicit WindowChrome(QObject *parent = nullptr);
  ~WindowChrome() override;

  QObject *window() const;
  void setWindow(QObject *window);
  QQuickItem *titleBar() const { return titleBar_.data(); }
  void setTitleBar(QQuickItem *item);
  bool active() const { return active_; }
  bool nativeFrame() const { return nativeFrame_; }
  bool maximized() const;
  bool windowActive() const;
  int resizeBorder() const { return 6; }

  Q_INVOKABLE void minimize();
  Q_INVOKABLE void toggleMaximize();
  Q_INVOKABLE void close();
  // Drag the window from the title bar, or resize it from an edge
  // (Qt::Edges flags), for platforms without native hit testing.
  Q_INVOKABLE void startMove();
  Q_INVOKABLE void startResize(int edges);
  // Window geometry kept in the user settings under a key.
  Q_INVOKABLE void saveGeometry(const QString &key);
  // Restore it (or centre a default size on the primary screen) and show
  // the window; false when nothing usable was saved.
  Q_INVOKABLE bool restoreGeometry(const QString &key, int defaultWidth = 1440, int defaultHeight = 900);

  // What a window point (logical pixels) is: an edge or corner (resizing),
  // the caption (moving) or the client area. Pure function for tests.
  static Hit hitTest(const QPointF &pos, const QSizeF &size, double border, bool maximized, bool inTitleBar,
                     bool onControl);
  // Whether the deepest item of the title bar under a scene point takes
  // mouse input (a button, a menu); such points must not move the window.
  static bool onControl(QQuickItem *titleBar, const QPointF &scenePos);
  // A rectangle moved onto the available area of the screens (restoring a
  // window after the monitor arrangement changed).
  static QRect fitToScreens(const QRect &rect, const QList<QRect> &screens, const QRect &primary);

 signals:
  void windowChanged();
  void titleBarChanged();
  void activeChanged();
  void windowStateChanged();
  void windowActiveChanged();

 private:
  class NativeFilter;
  void *hwnd_ = nullptr;  // the native window (Windows), once it exists
  void install();
  Hit hitAt(const QPointF &pos) const;
  QPointer<QQuickWindow> window_;
  QPointer<QQuickItem> titleBar_;
  bool active_ = false;
  bool nativeFrame_ = false;
  QRect normalGeometry_;
  std::unique_ptr<NativeFilter> filter_;
};

}  // namespace mbstudio::ui
