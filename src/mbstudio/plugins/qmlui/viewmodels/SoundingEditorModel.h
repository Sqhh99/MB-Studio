// MB-Studio QML UI - a sounding editor document: the soundings of one or
// more survey lines as a 3D point cloud, with the edits saved for them
// applied, for selecting soundings (point, rectangle, lasso, brush),
// flagging and unflagging them with undo and redo, and saving the edits to
// the lines' edit save files through a project job (see ISoundingService
// and IProjectService::saveEdits). A ping view shows the beams of one ping
// across the track.
//
// Every point keeps its identity (survey line, ping time stamp and
// multiplicity, beam number), which is exactly what the saved edit refers
// to; the display only decimates which points are drawn while the camera
// moves, never which sounding a drawn point stands for.
#pragma once

#include <QFutureWatcher>
#include <QImage>
#include <QObject>
#include <QPointF>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <memory>

#include "SceneRenderer.h"
#include "mbstudio/api/GridService.h"
#include "mbstudio/api/ProjectService.h"
#include "mbstudio/api/SoundingService.h"

namespace mbstudio::ui {

class SoundingEditorModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
  Q_PROPERTY(bool loaded READ loaded NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
  Q_PROPERTY(QString warning READ warning NOTIFY stateChanged)
  Q_PROPERTY(QStringList lineIds READ lineIds NOTIFY stateChanged)
  // [{lineId, name, path, pings, soundings, color, esf, savedEdits}]
  Q_PROPERTY(QVariantList lines READ lines NOTIFY stateChanged)
  Q_PROPERTY(int soundingCount READ soundingCount NOTIFY stateChanged)
  Q_PROPERTY(int pingCount READ pingCount NOTIFY stateChanged)
  Q_PROPERTY(int flaggedCount READ flaggedCount NOTIFY countsChanged)
  Q_PROPERTY(int editCount READ editCount NOTIFY countsChanged)
  Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
  Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
  Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
  Q_PROPERTY(bool saving READ saving NOTIFY stateChanged)
  // 0 depth, 1 survey line, 2 flag state, 3 beam number
  Q_PROPERTY(int colorMode READ colorMode WRITE setColorMode NOTIFY sceneChanged)
  Q_PROPERTY(bool showFlagged READ showFlagged WRITE setShowFlagged NOTIFY sceneChanged)
  Q_PROPERTY(int pointSize READ pointSize WRITE setPointSize NOTIFY sceneChanged)
  Q_PROPERTY(double exaggeration READ exaggeration WRITE setExaggeration NOTIFY sceneChanged)
  Q_PROPERTY(bool perspective READ perspective WRITE setPerspective NOTIFY sceneChanged)
  // 0 navigate, 1 rectangle, 2 lasso, 3 brush, 4 point
  Q_PROPERTY(int tool READ tool WRITE setTool NOTIFY toolChanged)
  // 0 replace the selection, 1 add to it, 2 remove from it
  Q_PROPERTY(int selectionMode READ selectionMode WRITE setSelectionMode NOTIFY toolChanged)
  Q_PROPERTY(int brushRadius READ brushRadius WRITE setBrushRadius NOTIFY toolChanged)
  Q_PROPERTY(int currentPing READ currentPing WRITE setCurrentPing NOTIFY pingChanged)
  Q_PROPERTY(QVariantMap pingInfo READ pingInfo NOTIFY pingChanged)
  Q_PROPERTY(QVariantMap probe READ probe NOTIFY probeChanged)
  Q_PROPERTY(double depthMin READ depthMin NOTIFY stateChanged)
  Q_PROPERTY(double depthMax READ depthMax NOTIFY stateChanged)
  Q_PROPERTY(QVariantList legend READ legend NOTIFY sceneChanged)
  Q_PROPERTY(int revision READ revision NOTIFY sceneChanged)
  Q_PROPERTY(QString viewState READ viewState WRITE setViewState NOTIFY viewStateChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY probeChanged)

 public:
  SoundingEditorModel(std::shared_ptr<IProjectService> project, std::shared_ptr<ISoundingService> soundings,
                      std::shared_ptr<IGridRenderService> renderer, QObject *parent = nullptr);
  ~SoundingEditorModel() override;

  bool busy() const { return loading_; }
  bool loaded() const { return loaded_; }
  QString error() const { return error_; }
  QString warning() const { return warning_; }
  QStringList lineIds() const { return lineIds_; }
  QVariantList lines() const;
  int soundingCount() const { return static_cast<int>(flag_.size()); }
  int pingCount() const { return static_cast<int>(pings_.size()); }
  int flaggedCount() const;
  int editCount() const;
  int selectedCount() const { return selectedCount_; }
  bool canUndo() const { return undoPosition_ > 0; }
  bool canRedo() const { return undoPosition_ < static_cast<int>(undo_.size()); }
  bool saving() const { return saveJob_ != 0; }
  int colorMode() const { return colorMode_; }
  void setColorMode(int value);
  bool showFlagged() const { return showFlagged_; }
  void setShowFlagged(bool value);
  int pointSize() const { return pointSize_; }
  void setPointSize(int value);
  double exaggeration() const { return exaggeration_; }
  void setExaggeration(double value);
  bool perspective() const { return camera_.perspective; }
  void setPerspective(bool value);
  int tool() const { return tool_; }
  void setTool(int value);
  int selectionMode() const { return selectionMode_; }
  void setSelectionMode(int value);
  int brushRadius() const { return brushRadius_; }
  void setBrushRadius(int value);
  int currentPing() const { return currentPing_; }
  void setCurrentPing(int value);
  QVariantMap pingInfo() const;
  QVariantMap probe() const { return probe_; }
  double depthMin() const { return depthMin_; }
  double depthMax() const { return depthMax_; }
  QVariantList legend() const;
  int revision() const { return revision_; }
  QString viewState() const;
  void setViewState(const QString &json);
  QString statusText() const;

  Q_INVOKABLE void load(const QStringList &lineIds);
  Q_INVOKABLE void reload() { load(lineIds_); }
  Q_INVOKABLE bool waitForFinished(int timeoutMs = 60000);
  // camera (see TerrainViewModel)
  Q_INVOKABLE void fit();
  Q_INVOKABLE void standardView(int which);
  Q_INVOKABLE void orbit(double dx, double dy);
  Q_INVOKABLE void pan(double dx, double dy, double width, double height);
  Q_INVOKABLE void zoom(double factor);
  // Selection in the pixels of the last rendered frame; returns the
  // number of selected soundings.
  Q_INVOKABLE int selectRect(double x0, double y0, double x1, double y1);
  Q_INVOKABLE int selectLasso(const QVariantList &points);
  Q_INVOKABLE int selectBrush(double x, double y);
  Q_INVOKABLE int selectAt(double x, double y);
  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void clearSelection();
  // Flag or unflag the selected soundings (an undo step); returns how many changed.
  Q_INVOKABLE int flagSelected();
  Q_INVOKABLE int unflagSelected();
  Q_INVOKABLE bool undo();
  Q_INVOKABLE bool redo();
  // Discard the edits that are not saved.
  Q_INVOKABLE void revert();
  // Save the edits as a project job; returns its id (0: nothing to save or an error).
  Q_INVOKABLE qint64 save();
  // Called when a job finishes (the save job).
  Q_INVOKABLE void jobFinished(qint64 id, const QString &state, const QString &error);
  // Ping view: the beams of a ping [{x: across track, y: depth, flag, good, index, selected}].
  Q_INVOKABLE QVariantList pingBeams(int ping) const;
  Q_INVOKABLE void toggleBeam(int index);
  Q_INVOKABLE void stepPing(int delta);
  Q_INVOKABLE void hover(double x, double y);
  Q_INVOKABLE void leave();
  // The sounding under a pixel (-1: none).
  Q_INVOKABLE int soundingAt(double x, double y) const;
  Q_INVOKABLE QVariantMap soundingInfo(int index) const;

  QImage render(int width, int height, bool fast);
  // Edits not saved yet, per survey line (exposed for tests).
  std::map<std::string, std::vector<BeamEdit>> pendingEdits() const;
  std::uint8_t flagOf(int index) const { return flag_[static_cast<std::size_t>(index)]; }

 signals:
  void stateChanged();
  void countsChanged();
  void selectionChanged();
  void historyChanged();
  void sceneChanged();
  void toolChanged();
  void pingChanged();
  void probeChanged();
  void viewStateChanged();
  void notify(const QString &message);
  void saved();

 private:
  struct Line {
    QString id, name, path, esf;
    int format = 0;
    long savedEdits = 0;
    std::uint32_t firstPing = 0, pingCount = 0;
    std::uint32_t firstPoint = 0, pointCount = 0;
  };
  struct Ping {
    double time = 0.0;
    int multiplicity = 0;
    std::uint16_t line = 0;
    std::uint32_t first = 0, count = 0;  // its soundings
  };
  struct Data {
    QString error, warning;
    QVector<Line> lines;
    std::vector<Ping> pings;
    std::vector<float> x, y, z, across;
    std::vector<std::uint8_t> flag;
    std::vector<std::uint32_t> ping;
    std::vector<std::uint16_t> beam;
    double lon0 = 0.0, lat0 = 0.0, metersX = 1.0, metersY = 1.0;
  };
  struct Step {
    std::vector<std::uint32_t> indices;
    std::vector<std::uint8_t> before, after;
  };
  bool visible(std::size_t i) const { return showFlagged_ || beamflag::isGood(flag_[i]); }
  std::uint32_t colorOf(std::size_t i) const;
  scene::Vec3 world(std::size_t i) const { return scene::Vec3{x_[i], y_[i], z_[i] * exaggeration_}; }
  double sceneRadius() const;
  template <typename Inside>
  int select(Inside inside);
  int applyFlags(bool flag);
  void apply(const Step &step, bool forward);
  void changed();
  void countsUpdated();

  std::shared_ptr<IProjectService> project_;
  std::shared_ptr<ISoundingService> soundings_;
  std::shared_ptr<IGridRenderService> renderer_;
  QFutureWatcher<std::shared_ptr<Data>> watcher_;
  bool loading_ = false;
  bool loaded_ = false;
  QString error_, warning_;
  QStringList lineIds_;
  QVector<Line> lines_;
  std::vector<Ping> pings_;
  std::vector<float> x_, y_, z_, across_;
  std::vector<std::uint8_t> flag_, baseline_, selected_;
  std::vector<std::uint32_t> ping_;
  std::vector<std::uint16_t> beam_;
  double lon0_ = 0.0, lat0_ = 0.0, metersX_ = 1.0, metersY_ = 1.0;
  double depthMin_ = 0.0, depthMax_ = 0.0;
  int maxBeams_ = 1;
  int selectedCount_ = 0;
  std::vector<Step> undo_;
  int undoPosition_ = 0;
  qint64 saveJob_ = 0;
  std::vector<std::uint8_t> savingFlags_;
  int colorMode_ = 0;
  bool showFlagged_ = true;
  int pointSize_ = 2;
  double exaggeration_ = 1.0;
  int tool_ = 0;
  int selectionMode_ = 0;
  int brushRadius_ = 18;
  int currentPing_ = -1;
  QVariantMap probe_;
  scene::Camera camera_;
  scene::Frame frame_;
  std::unique_ptr<scene::View> view_;
  int revision_ = 0;
  QString pendingState_;
};

}  // namespace mbstudio::ui
