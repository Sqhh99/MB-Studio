#include "ProcessingViewModel.h"

#include <QDir>
#include <QLocale>
#include <QUrl>
#include <cmath>

#include "ProjectViewModel.h"

namespace mbstudio::ui {

namespace {

// Field of a parameter schema.
struct Field {
  QVariantMap m;
  Field(const QString &key, const QString &type, const QString &label) {
    m[QStringLiteral("key")] = key;
    m[QStringLiteral("type")] = type;
    m[QStringLiteral("label")] = label;
  }
  Field &unit(const QString &u) {
    m[QStringLiteral("unit")] = u;
    return *this;
  }
  Field &range(double lo, double hi, double step) {
    m[QStringLiteral("min")] = lo;
    m[QStringLiteral("max")] = hi;
    m[QStringLiteral("step")] = step;
    return *this;
  }
  Field &help(const QString &h) {
    m[QStringLiteral("help")] = h;
    return *this;
  }
  Field &option(const QVariant &value, const QString &label) {
    QVariantList list = m.value(QStringLiteral("options")).toList();
    list.push_back(QVariantMap{{QStringLiteral("value"), value}, {QStringLiteral("label"), label}});
    m[QStringLiteral("options")] = list;
    return *this;
  }
  // visible only when the value of another field compares as given
  Field &when(const QString &key, const QString &op, const QVariant &value) {
    m[QStringLiteral("whenKey")] = key;
    m[QStringLiteral("whenOp")] = op;
    m[QStringLiteral("whenValue")] = value;
    return *this;
  }
  operator QVariant() const { return m; }
};

QVariantMap section(const QString &label) {
  return {{QStringLiteral("key"), QStringLiteral("section:") + label},
          {QStringLiteral("type"), QStringLiteral("section")},
          {QStringLiteral("label"), label}};
}

QString localPath(const QVariant &v) {
  const QString text = v.toString();
  if (text.startsWith(QLatin1String("file:")))
    return QUrl(text).toLocalFile();
  return text;
}

std::string str(const QVariantMap &m, const char *key) { return localPath(m.value(QLatin1String(key))).toStdString(); }
double num(const QVariantMap &m, const char *key) { return m.value(QLatin1String(key)).toDouble(); }
int integer(const QVariantMap &m, const char *key) { return static_cast<int>(std::lround(m.value(QLatin1String(key)).toDouble())); }
bool flag(const QVariantMap &m, const char *key) { return m.value(QLatin1String(key)).toBool(); }

}  // namespace

ProcessingViewModel::ProcessingViewModel(std::shared_ptr<IProjectService> project,
                                         std::shared_ptr<IProcessingService> processing, ProjectViewModel *projectModel,
                                         QObject *parent)
    : QObject(parent), project_(std::move(project)), processing_(std::move(processing)), projectModel_(projectModel) {
  for (const auto &k : kinds())
    values_[k] = defaults(k);
  previewTimer_.setSingleShot(true);
  previewTimer_.setInterval(120);
  connect(&previewTimer_, &QTimer::timeout, this, &ProcessingViewModel::updatePreview);
  if (projectModel_ != nullptr) {
    connect(projectModel_, &ProjectViewModel::selectionChanged, &previewTimer_, qOverload<>(&QTimer::start));
    connect(projectModel_, &ProjectViewModel::linesChanged, &previewTimer_, qOverload<>(&QTimer::start));
    connect(projectModel_, &ProjectViewModel::projectChanged, &previewTimer_, qOverload<>(&QTimer::start));
  }
  updatePreview();
}

QStringList ProcessingViewModel::kinds() {
  return {QStringLiteral("preprocess"), QStringLiteral("clean"), QStringLiteral("corrections"),
          QStringLiteral("process"),    QStringLiteral("grid"),  QStringLiteral("mosaic"),
          QStringLiteral("export"),     QStringLiteral("figure")};
}

void ProcessingViewModel::setKind(const QString &kind) {
  if (kind == kind_ || !kinds().contains(kind))
    return;
  kind_ = kind;
  emit kindChanged();
  revision_++;
  emit parametersChanged();
  updatePreview();
}

QString ProcessingViewModel::title() const {
  if (kind_ == QLatin1String("preprocess"))
    return tr("Preprocess");
  if (kind_ == QLatin1String("clean"))
    return tr("Clean bathymetry");
  if (kind_ == QLatin1String("corrections"))
    return tr("Corrections");
  if (kind_ == QLatin1String("process"))
    return tr("Process survey lines");
  if (kind_ == QLatin1String("grid"))
    return tr("Grid bathymetry");
  if (kind_ == QLatin1String("mosaic"))
    return tr("Backscatter mosaic");
  if (kind_ == QLatin1String("figure"))
    return tr("Map figure (GMT)");
  return tr("Export");
}

QString ProcessingViewModel::description() const {
  if (kind_ == QLatin1String("preprocess"))
    return tr("Converts raw vendor files (for example Kongsberg .all) into the MB-System processing format, merging navigation and attitude. Recommended before cleaning and processing.");
  if (kind_ == QLatin1String("clean"))
    return tr("Flags outlier soundings automatically (mbclean, mbareaclean). Flags are stored in edit files and applied when the lines are processed; the data files are not changed.");
  if (kind_ == QLatin1String("corrections"))
    return tr("Sets sound velocity, attitude biases, draft and tide for processing (mbset). They are applied when the lines are processed.");
  if (kind_ == QLatin1String("process"))
    return tr("Applies the edits and corrections and writes the processed files (mbprocess). Gridding uses processed files automatically where they exist.");
  if (kind_ == QLatin1String("grid"))
    return tr("Makes a digital terrain model from the survey lines (mbgrid). The grid is added to the project and shown on the map.");
  if (kind_ == QLatin1String("mosaic"))
    return tr("Makes an amplitude or sidescan mosaic (mbmosaic), optionally after a grazing angle correction.");
  if (kind_ == QLatin1String("figure"))
    return tr("Composes a publication figure with GMT: a bathymetric map with contours, a survey overview, a quality map or a perspective view, with colour scale and scale bar, as PNG, PDF or PostScript. The figure and its recipe are kept in the project.");
  return tr("Writes a grid as GeoTIFF, ESRI ASCII, XYZ or a 3D model, or the soundings as a LAS point cloud or XYZ text.");
}

QVariantMap ProcessingViewModel::defaults(const QString &kind) const {
  QVariantMap d;
  if (kind == QLatin1String("preprocess")) {
    d[QStringLiteral("skipExisting")] = true;
    d[QStringLiteral("recalculateBathymetry")] = false;
    d[QStringLiteral("ignoreWaterColumn")] = true;
    d[QStringLiteral("navigationFile")] = QString();
    d[QStringLiteral("navigationFormat")] = 9;
    d[QStringLiteral("platformFile")] = QString();
  } else if (kind == QLatin1String("clean")) {
    const CleanParameters p;
    d[QStringLiteral("useSlope")] = p.useSlope;
    d[QStringLiteral("slope")] = p.slope;
    d[QStringLiteral("slopeMode")] = p.slopeMode;
    d[QStringLiteral("outerBeams")] = p.outerBeams;
    d[QStringLiteral("useDepthRange")] = p.useDepthRange;
    d[QStringLiteral("depthMin")] = 10.0;
    d[QStringLiteral("depthMax")] = 8000.0;
    d[QStringLiteral("useMedianFraction")] = p.useMedianFraction;
    d[QStringLiteral("fractionLow")] = p.fractionLow;
    d[QStringLiteral("fractionHigh")] = p.fractionHigh;
    d[QStringLiteral("spikes")] = p.spikes;
    d[QStringLiteral("spikeSlope")] = p.spikeSlope;
    d[QStringLiteral("pingRms")] = p.pingRms;
    d[QStringLiteral("areaClean")] = p.areaClean;
    d[QStringLiteral("areaBinSize")] = p.areaBinSize;
    d[QStringLiteral("areaStdDev")] = p.areaStdDev;
  } else if (kind == QLatin1String("corrections")) {
    d[QStringLiteral("svpMode")] = 0;
    d[QStringLiteral("svpFile")] = QString();
    d[QStringLiteral("useRollBias")] = false;
    d[QStringLiteral("rollBias")] = 0.0;
    d[QStringLiteral("usePitchBias")] = false;
    d[QStringLiteral("pitchBias")] = 0.0;
    d[QStringLiteral("useDraft")] = false;
    d[QStringLiteral("draft")] = 0.0;
    d[QStringLiteral("tideMode")] = 0;
    d[QStringLiteral("tideFile")] = QString();
    d[QStringLiteral("tideFormat")] = 2;
  } else if (kind == QLatin1String("process")) {
    d[QStringLiteral("threads")] = 2;
    d[QStringLiteral("force")] = false;
  } else if (kind == QLatin1String("grid")) {
    d[QStringLiteral("name")] = QStringLiteral("bathymetry");
    d[QStringLiteral("dataType")] = 2;
    d[QStringLiteral("spacing")] = 25.0;
    d[QStringLiteral("units")] = QStringLiteral("meters");
    d[QStringLiteral("algorithm")] = 1;
    d[QStringLiteral("threshold")] = 0.0;
    d[QStringLiteral("clip")] = 0;
    d[QStringLiteral("clipMode")] = 1;
    d[QStringLiteral("tension")] = 0.0;
    d[QStringLiteral("extraGrids")] = true;
    d[QStringLiteral("projection")] = QString();
    d[QStringLiteral("boundsFactor")] = 0.0;
  } else if (kind == QLatin1String("mosaic")) {
    d[QStringLiteral("name")] = QStringLiteral("backscatter");
    d[QStringLiteral("dataType")] = 3;
    d[QStringLiteral("spacing")] = 10.0;
    d[QStringLiteral("prioritySource")] = 0;
    d[QStringLiteral("priorityRange")] = 0.0;
    d[QStringLiteral("clip")] = 0;
    d[QStringLiteral("extraGrids")] = false;
    d[QStringLiteral("projection")] = QString();
    d[QStringLiteral("angleCorrection")] = false;
  } else if (kind == QLatin1String("figure")) {
    const FigureParameters p;
    d[QStringLiteral("template")] = QStringLiteral("bathymetry");
    d[QStringLiteral("source")] = QString();  // the newest grid
    d[QStringLiteral("name")] = QString();
    d[QStringLiteral("formats")] = QStringLiteral("png,pdf");
    d[QStringLiteral("widthCm")] = p.widthCm;
    d[QStringLiteral("dpi")] = p.dpi;
    d[QStringLiteral("palette")] = QString::fromStdString(p.palette);
    d[QStringLiteral("reversePalette")] = p.reversePalette;
    d[QStringLiteral("contourInterval")] = p.contourInterval;
    d[QStringLiteral("annotationInterval")] = p.annotationInterval;
    d[QStringLiteral("illumination")] = p.illumination;
    d[QStringLiteral("showTracks")] = p.showTracks;
    d[QStringLiteral("colorbar")] = p.colorbar;
    d[QStringLiteral("scaleBar")] = p.scaleBar;
    d[QStringLiteral("title")] = QString();
    d[QStringLiteral("zLabel")] = QString::fromStdString(p.zLabel);
    d[QStringLiteral("viewAzimuth")] = p.viewAzimuth;
    d[QStringLiteral("viewElevation")] = p.viewElevation;
    d[QStringLiteral("reliefHeightCm")] = p.reliefHeightCm;
  } else if (kind == QLatin1String("export")) {
    d[QStringLiteral("source")] = QStringLiteral("soundings");
    d[QStringLiteral("gridFormat")] = QStringLiteral("geotiff");
    d[QStringLiteral("soundingFormat")] = QStringLiteral("las");
    d[QStringLiteral("projection")] = QString();
    d[QStringLiteral("exaggeration")] = 1.0;
    d[QStringLiteral("name")] = QString();
  }
  return d;
}

QVariantList ProcessingViewModel::schema(const QString &kind) const {
  QVariantList f;
  const QString m = tr("m");
  if (kind == QLatin1String("preprocess")) {
    f << Field(QStringLiteral("skipExisting"), QStringLiteral("bool"), tr("Skip lines that are already preprocessed"));
    f << Field(QStringLiteral("ignoreWaterColumn"), QStringLiteral("bool"), tr("Leave out water column records"))
             .help(tr("Makes the processing files much smaller (Reson and Teledyne 7k data)."));
    f << Field(QStringLiteral("recalculateBathymetry"), QStringLiteral("bool"), tr("Recalculate bathymetry"))
             .help(tr("Raytrace again with the merged attitude (only some formats)."));
    f << section(tr("External navigation (optional)"));
    f << Field(QStringLiteral("navigationFile"), QStringLiteral("file"), tr("Navigation file"))
             .help(tr("Replaces the navigation stored in the data, e.g. post-processed positions."));
    f << Field(QStringLiteral("navigationFormat"), QStringLiteral("enum"), tr("Navigation format"))
             .option(9, tr("MB-System fast navigation (.fnv)"))
             .option(1, tr("Epoch seconds, longitude, latitude"))
             .option(2, tr("Date and time, longitude, latitude"))
             .option(6, tr("NMEA 0183"))
             .when(QStringLiteral("navigationFile"), QStringLiteral("set"), QVariant());
    f << Field(QStringLiteral("platformFile"), QStringLiteral("file"), tr("Platform file (lever arms)"));
  } else if (kind == QLatin1String("clean")) {
    f << section(tr("Ping by ping (mbclean)"));
    f << Field(QStringLiteral("useSlope"), QStringLiteral("bool"), tr("Flag steep slopes between beams"));
    f << Field(QStringLiteral("slope"), QStringLiteral("number"), tr("Maximum slope"))
             .range(0.05, 10, 0.05)
             .help(tr("Tangent of the slope angle: 1.0 is 45°."))
             .when(QStringLiteral("useSlope"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("slopeMode"), QStringLiteral("enum"), tr("On a steep slope"))
             .option(1, tr("Flag the beam farthest from the median depth"))
             .option(2, tr("Flag both beams"))
             .when(QStringLiteral("useSlope"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("outerBeams"), QStringLiteral("integer"), tr("Flag outermost beams on each side"))
             .range(0, 100, 1);
    f << Field(QStringLiteral("useDepthRange"), QStringLiteral("bool"), tr("Flag depths outside a range"));
    f << Field(QStringLiteral("depthMin"), QStringLiteral("number"), tr("Shallowest depth"))
             .unit(m)
             .range(0, 12000, 1)
             .when(QStringLiteral("useDepthRange"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("depthMax"), QStringLiteral("number"), tr("Deepest depth"))
             .unit(m)
             .range(0, 12000, 1)
             .when(QStringLiteral("useDepthRange"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("useMedianFraction"), QStringLiteral("bool"), tr("Flag depths far from the local median"));
    f << Field(QStringLiteral("fractionLow"), QStringLiteral("number"), tr("Lower limit (× median)"))
             .range(0.1, 1, 0.05)
             .when(QStringLiteral("useMedianFraction"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("fractionHigh"), QStringLiteral("number"), tr("Upper limit (× median)"))
             .range(1, 5, 0.05)
             .when(QStringLiteral("useMedianFraction"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("spikes"), QStringLiteral("bool"), tr("Flag spikes"));
    f << Field(QStringLiteral("spikeSlope"), QStringLiteral("number"), tr("Spike slope"))
             .range(0.05, 10, 0.05)
             .when(QStringLiteral("spikes"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("pingRms"), QStringLiteral("number"), tr("Flag noisy pings (rms deviation, 0 = off)"))
             .unit(m)
             .range(0, 1000, 0.5);
    f << section(tr("By area (mbareaclean)"));
    f << Field(QStringLiteral("areaClean"), QStringLiteral("bool"), tr("Flag outliers by area statistics"));
    f << Field(QStringLiteral("areaBinSize"), QStringLiteral("number"), tr("Bin size (0 = automatic)"))
             .unit(m)
             .range(0, 10000, 1)
             .when(QStringLiteral("areaClean"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("areaStdDev"), QStringLiteral("number"), tr("Threshold (standard deviations)"))
             .range(0.5, 10, 0.1)
             .when(QStringLiteral("areaClean"), QStringLiteral("=="), true);
  } else if (kind == QLatin1String("corrections")) {
    f << section(tr("Sound velocity"));
    f << Field(QStringLiteral("svpMode"), QStringLiteral("enum"), tr("Sound velocity profile"))
             .option(0, tr("Keep the bathymetry as recorded"))
             .option(1, tr("Raytrace with a profile file"))
             .option(2, tr("Raytrace with the Levitus climatology"));
    f << Field(QStringLiteral("svpFile"), QStringLiteral("file"), tr("Profile file (.svp)"))
             .when(QStringLiteral("svpMode"), QStringLiteral("=="), 1);
    f << section(tr("Attitude and draft"));
    f << Field(QStringLiteral("useRollBias"), QStringLiteral("bool"), tr("Roll bias"));
    f << Field(QStringLiteral("rollBias"), QStringLiteral("number"), tr("Roll bias"))
             .unit(QStringLiteral("°"))
             .range(-10, 10, 0.01)
             .when(QStringLiteral("useRollBias"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("usePitchBias"), QStringLiteral("bool"), tr("Pitch bias"));
    f << Field(QStringLiteral("pitchBias"), QStringLiteral("number"), tr("Pitch bias"))
             .unit(QStringLiteral("°"))
             .range(-10, 10, 0.01)
             .when(QStringLiteral("usePitchBias"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("useDraft"), QStringLiteral("bool"), tr("Constant draft"));
    f << Field(QStringLiteral("draft"), QStringLiteral("number"), tr("Draft"))
             .unit(m)
             .range(-100, 1000, 0.01)
             .when(QStringLiteral("useDraft"), QStringLiteral("=="), true);
    f << section(tr("Tide"));
    f << Field(QStringLiteral("tideMode"), QStringLiteral("enum"), tr("Tide correction"))
             .option(0, tr("None"))
             .option(1, tr("Tide file"))
             .option(2, tr("Predicted (OTPS tide model)"));
    f << Field(QStringLiteral("tideFile"), QStringLiteral("file"), tr("Tide file"))
             .when(QStringLiteral("tideMode"), QStringLiteral("=="), 1);
    f << Field(QStringLiteral("tideFormat"), QStringLiteral("enum"), tr("Tide file format"))
             .option(1, tr("Epoch seconds, tide"))
             .option(2, tr("Year month day hour minute second, tide"))
             .option(3, tr("Year day-of-year hour minute second, tide"))
             .when(QStringLiteral("tideMode"), QStringLiteral("=="), 1);
  } else if (kind == QLatin1String("process")) {
    f << Field(QStringLiteral("threads"), QStringLiteral("integer"), tr("Lines processed in parallel")).range(1, 32, 1);
    f << Field(QStringLiteral("force"), QStringLiteral("bool"), tr("Process lines that are up to date again"));
  } else if (kind == QLatin1String("grid")) {
    f << Field(QStringLiteral("name"), QStringLiteral("text"), tr("Product name"));
    f << Field(QStringLiteral("dataType"), QStringLiteral("enum"), tr("Data"))
             .option(2, tr("Topography (elevation, negative below sea level)"))
             .option(1, tr("Bathymetry (depth, positive down)"))
             .option(3, tr("Beam amplitude"))
             .option(4, tr("Sidescan"));
    f << Field(QStringLiteral("spacing"), QStringLiteral("number"), tr("Cell size")).range(0.01, 100000, 1);
    f << Field(QStringLiteral("units"), QStringLiteral("enum"), tr("Cell size unit"))
             .option(QStringLiteral("meters"), tr("metres"))
             .option(QStringLiteral("km"), tr("kilometres"))
             .option(QStringLiteral("arcsec"), tr("arc seconds"));
    f << Field(QStringLiteral("algorithm"), QStringLiteral("enum"), tr("Gridding algorithm"))
             .option(1, tr("Gaussian weighted mean"))
             .option(2, tr("Median filter"))
             .option(3, tr("Minimum (shoalest)"))
             .option(4, tr("Maximum (deepest)"))
             .option(5, tr("Beam footprint with slope"))
             .option(6, tr("Beam footprint"));
    f << Field(QStringLiteral("threshold"), QStringLiteral("number"), tr("Average soundings within"))
             .unit(m)
             .range(0, 1000, 0.1)
             .help(tr("0 keeps only the extreme sounding of each cell."))
             .when(QStringLiteral("algorithm"), QStringLiteral("in"), QVariantList{3, 4});
    f << section(tr("Gaps"));
    f << Field(QStringLiteral("clip"), QStringLiteral("integer"), tr("Interpolate gaps up to"))
             .unit(tr("cells"))
             .range(0, 1000, 1)
             .help(tr("0 leaves cells without data empty. Interpolated cells are not measured data."));
    f << Field(QStringLiteral("clipMode"), QStringLiteral("enum"), tr("Interpolate"))
             .option(1, tr("Only gaps between data"))
             .option(2, tr("All cells near data"))
             .option(3, tr("The whole grid"))
             .when(QStringLiteral("clip"), QStringLiteral(">"), 0);
    f << Field(QStringLiteral("tension"), QStringLiteral("number"), tr("Spline tension"))
             .range(0, 1, 0.05)
             .when(QStringLiteral("clip"), QStringLiteral(">"), 0);
    f << section(tr("Output"));
    f << Field(QStringLiteral("extraGrids"), QStringLiteral("bool"), tr("Quality grids (data count, standard deviation)"));
    f << Field(QStringLiteral("projection"), QStringLiteral("enum"), tr("Coordinates"))
             .option(QString(), tr("Geographic (WGS 84)"))
             .option(QStringLiteral("U"), tr("UTM (zone chosen automatically)"));
    f << Field(QStringLiteral("boundsFactor"), QStringLiteral("enum"), tr("Extent"))
             .option(0.0, tr("Data extent"))
             .option(1.1, tr("Data extent + 10 %"));
  } else if (kind == QLatin1String("mosaic")) {
    f << Field(QStringLiteral("name"), QStringLiteral("text"), tr("Product name"));
    f << Field(QStringLiteral("dataType"), QStringLiteral("enum"), tr("Data"))
             .option(3, tr("Beam amplitude"))
             .option(4, tr("Sidescan"));
    f << Field(QStringLiteral("spacing"), QStringLiteral("number"), tr("Cell size")).unit(m).range(0.01, 100000, 1);
    f << Field(QStringLiteral("angleCorrection"), QStringLiteral("bool"), tr("Correct the grazing angle first"))
             .help(tr("Runs mbbackangle and mbprocess so that the mosaic is not darker at the swath edges."));
    f << Field(QStringLiteral("prioritySource"), QStringLiteral("enum"), tr("Preferred part of the swath"))
             .option(0, tr("No preference"))
             .option(1, tr("Outer swath (up to ±60°)"))
             .option(3, tr("Outer swath (up to ±75°)"))
             .option(5, tr("Near nadir"));
    f << Field(QStringLiteral("priorityRange"), QStringLiteral("number"), tr("Blend samples within priority"))
             .range(0, 1, 0.05);
    f << Field(QStringLiteral("clip"), QStringLiteral("integer"), tr("Interpolate gaps up to"))
             .unit(tr("cells"))
             .range(0, 100, 1);
    f << Field(QStringLiteral("extraGrids"), QStringLiteral("bool"), tr("Quality grids (data count, standard deviation)"))
             .when(QStringLiteral("priorityRange"), QStringLiteral(">"), 0);
    f << Field(QStringLiteral("projection"), QStringLiteral("enum"), tr("Coordinates"))
             .option(QString(), tr("Geographic (WGS 84)"))
             .option(QStringLiteral("U"), tr("UTM (zone chosen automatically)"));
  } else if (kind == QLatin1String("export")) {
    f << Field(QStringLiteral("source"), QStringLiteral("enum"), tr("Export"));
    f << Field(QStringLiteral("gridFormat"), QStringLiteral("enum"), tr("Format"))
             .option(QStringLiteral("geotiff"), tr("GeoTIFF (GIS)"))
             .option(QStringLiteral("esri"), tr("ESRI ASCII grid"))
             .option(QStringLiteral("xyz"), tr("XYZ text (grid nodes)"))
             .option(QStringLiteral("gltf"), tr("3D model (glTF binary)"))
             .when(QStringLiteral("source"), QStringLiteral("!="), QStringLiteral("soundings"));
    f << Field(QStringLiteral("exaggeration"), QStringLiteral("number"), tr("Vertical exaggeration"))
             .range(0.1, 100, 0.5)
             .when(QStringLiteral("gridFormat"), QStringLiteral("=="), QStringLiteral("gltf"));
    f << Field(QStringLiteral("soundingFormat"), QStringLiteral("enum"), tr("Format"))
             .option(QStringLiteral("las"), tr("LAS point cloud"))
             .option(QStringLiteral("xyz"), tr("XYZ text (longitude, latitude, depth)"))
             .when(QStringLiteral("source"), QStringLiteral("=="), QStringLiteral("soundings"));
    f << Field(QStringLiteral("projection"), QStringLiteral("enum"), tr("Coordinates"))
             .option(QString(), tr("Geographic (WGS 84)"))
             .option(QStringLiteral("U"), tr("UTM (zone chosen automatically)"))
             .when(QStringLiteral("soundingFormat"), QStringLiteral("=="), QStringLiteral("las"));
    f << Field(QStringLiteral("name"), QStringLiteral("text"), tr("File name (optional)"));
  } else if (kind == QLatin1String("figure")) {
    f << Field(QStringLiteral("template"), QStringLiteral("enum"), tr("Figure"))
             .option(QStringLiteral("bathymetry"), tr("Bathymetric map with contours"))
             .option(QStringLiteral("overview"), tr("Survey overview (survey lines)"))
             .option(QStringLiteral("quality"), tr("Grid quality map"))
             .option(QStringLiteral("perspective"), tr("Perspective view"));
    f << Field(QStringLiteral("source"), QStringLiteral("enum"), tr("Grid"))
             .help(tr("A quality map uses a data count or standard deviation grid."));
    f << Field(QStringLiteral("name"), QStringLiteral("text"), tr("Product name"));
    f << Field(QStringLiteral("title"), QStringLiteral("text"), tr("Title on the figure"))
             .help(tr("GMT's standard fonts show Latin letters only."));
    f << section(tr("Map"));
    f << Field(QStringLiteral("palette"), QStringLiteral("enum"), tr("Colour table"))
             .option(QStringLiteral("haxby"), QStringLiteral("haxby"))
             .option(QStringLiteral("geo"), QStringLiteral("geo"))
             .option(QStringLiteral("globe"), QStringLiteral("globe"))
             .option(QStringLiteral("ocean"), QStringLiteral("ocean"))
             .option(QStringLiteral("viridis"), QStringLiteral("viridis"))
             .option(QStringLiteral("turbo"), QStringLiteral("turbo"))
             .option(QStringLiteral("hot"), QStringLiteral("hot"))
             .option(QStringLiteral("gray"), QStringLiteral("gray"));
    f << Field(QStringLiteral("reversePalette"), QStringLiteral("bool"), tr("Reverse the colour table"));
    f << Field(QStringLiteral("illumination"), QStringLiteral("bool"), tr("Illuminate the relief"))
             .when(QStringLiteral("template"), QStringLiteral("!="), QStringLiteral("quality"));
    f << Field(QStringLiteral("contourInterval"), QStringLiteral("number"), tr("Contour interval (0 = none)"))
             .unit(m)
             .range(0, 100000, 1)
             .when(QStringLiteral("template"), QStringLiteral("in"), QVariantList{QStringLiteral("bathymetry"), QStringLiteral("overview")});
    f << Field(QStringLiteral("annotationInterval"), QStringLiteral("number"), tr("Labelled contours every (0 = none)"))
             .unit(m)
             .range(0, 100000, 1)
             .when(QStringLiteral("contourInterval"), QStringLiteral(">"), 0);
    f << Field(QStringLiteral("showTracks"), QStringLiteral("bool"), tr("Draw the survey lines"))
             .when(QStringLiteral("template"), QStringLiteral("in"), QVariantList{QStringLiteral("bathymetry"), QStringLiteral("quality")});
    f << Field(QStringLiteral("colorbar"), QStringLiteral("bool"), tr("Colour scale"));
    f << Field(QStringLiteral("zLabel"), QStringLiteral("text"), tr("Colour scale label"))
             .when(QStringLiteral("colorbar"), QStringLiteral("=="), true);
    f << Field(QStringLiteral("scaleBar"), QStringLiteral("bool"), tr("Scale bar"))
             .when(QStringLiteral("template"), QStringLiteral("!="), QStringLiteral("perspective"));
    f << section(tr("Perspective"));
    f << Field(QStringLiteral("viewAzimuth"), QStringLiteral("number"), tr("View from (azimuth)"))
             .unit(QStringLiteral("°"))
             .range(0, 360, 5)
             .when(QStringLiteral("template"), QStringLiteral("=="), QStringLiteral("perspective"));
    f << Field(QStringLiteral("viewElevation"), QStringLiteral("number"), tr("View elevation"))
             .unit(QStringLiteral("°"))
             .range(5, 90, 1)
             .when(QStringLiteral("template"), QStringLiteral("=="), QStringLiteral("perspective"));
    f << Field(QStringLiteral("reliefHeightCm"), QStringLiteral("number"), tr("Height of the relief"))
             .unit(QStringLiteral("cm"))
             .range(0.5, 50, 0.5)
             .when(QStringLiteral("template"), QStringLiteral("=="), QStringLiteral("perspective"));
    f << section(tr("Output"));
    f << Field(QStringLiteral("formats"), QStringLiteral("enum"), tr("Files"))
             .option(QStringLiteral("png,pdf"), tr("PNG and PDF"))
             .option(QStringLiteral("png"), tr("PNG"))
             .option(QStringLiteral("pdf"), tr("PDF"))
             .option(QStringLiteral("ps"), tr("PostScript (no Ghostscript needed)"));
    f << Field(QStringLiteral("widthCm"), QStringLiteral("number"), tr("Map width"))
             .unit(QStringLiteral("cm"))
             .range(2, 200, 0.5);
    f << Field(QStringLiteral("dpi"), QStringLiteral("integer"), tr("Raster resolution"))
             .unit(QStringLiteral("dpi"))
             .range(30, 2400, 1)
             .when(QStringLiteral("formats"), QStringLiteral("!="), QStringLiteral("ps"));
  }
  return f;
}

QVariantList ProcessingViewModel::fields() const { return schema(kind_); }

QVariant ProcessingViewModel::value(const QString &key) const { return values_.value(kind_).value(key); }

void ProcessingViewModel::setValue(const QString &key, const QVariant &value) {
  QVariantMap &v = values_[kind_];
  if (v.value(key) == value)
    return;
  v[key] = value;
  revision_++;
  emit parametersChanged();
  previewTimer_.start();
}

bool ProcessingViewModel::fieldVisible(const QString &key) const {
  for (const auto &item : schema(kind_)) {
    const QVariantMap f = item.toMap();
    if (f.value(QStringLiteral("key")).toString() != key)
      continue;
    const QString whenKey = f.value(QStringLiteral("whenKey")).toString();
    if (whenKey.isEmpty())
      return true;
    if (!fieldVisible(whenKey))
      return false;
    const QString op = f.value(QStringLiteral("whenOp")).toString();
    const QVariant actual = value(whenKey);
    const QVariant expected = f.value(QStringLiteral("whenValue"));
    if (op == QLatin1String("set"))
      return !actual.toString().isEmpty();
    if (op == QLatin1String(">"))
      return actual.toDouble() > expected.toDouble();
    if (op == QLatin1String("in")) {
      for (const auto &e : expected.toList()) {
        if (e.typeId() == QMetaType::QString ? actual.toString() == e.toString() : actual.toDouble() == e.toDouble())
          return true;
      }
      return false;
    }
    bool equal = false;
    if (expected.typeId() == QMetaType::Bool)
      equal = actual.toBool() == expected.toBool();
    else if (expected.typeId() == QMetaType::QString)
      equal = actual.toString() == expected.toString();
    else
      equal = actual.toDouble() == expected.toDouble();
    return op == QLatin1String("!=") ? !equal : equal;
  }
  return true;
}

QVariantList ProcessingViewModel::options(const QString &key) const {
  for (const auto &item : schema(kind_)) {
    const QVariantMap f = item.toMap();
    if (f.value(QStringLiteral("key")).toString() == key && f.contains(QStringLiteral("options")))
      return f.value(QStringLiteral("options")).toList();
  }
  QVariantList out;
  if (kind_ == QLatin1String("figure") && key == QLatin1String("source")) {
    out.push_back(QVariantMap{{QStringLiteral("value"), QString()}, {QStringLiteral("label"), tr("The newest grid")}});
    out.push_back(QVariantMap{{QStringLiteral("value"), QStringLiteral("none")},
                              {QStringLiteral("label"), tr("None (survey lines only)")}});
    if (project_ && project_->isOpen())
      for (const auto &a : project_->assets())
        if (a.type == "grid" || a.type == "mosaic" || a.type == "grid_num" || a.type == "grid_sd")
          out.push_back(QVariantMap{{QStringLiteral("value"), QString::fromStdString(a.id)},
                                    {QStringLiteral("label"), QString::fromStdString(a.label)}});
    return out;
  }
  if (kind_ == QLatin1String("export") && key == QLatin1String("source")) {
    out.push_back(QVariantMap{{QStringLiteral("value"), QStringLiteral("soundings")},
                              {QStringLiteral("label"), tr("Soundings of the selected survey lines")}});
    if (project_ && project_->isOpen())
      for (const auto &a : project_->assets())
        if (a.type == "grid" || a.type == "mosaic")
          out.push_back(QVariantMap{{QStringLiteral("value"), QString::fromStdString(a.id)},
                                    {QStringLiteral("label"), tr("Grid: %1").arg(QString::fromStdString(a.label))}});
  }
  return out;
}

void ProcessingViewModel::resetDefaults() {
  values_[kind_] = defaults(kind_);
  revision_++;
  emit parametersChanged();
  updatePreview();
}

ProcessingRequest ProcessingViewModel::request() const {
  ProcessingRequest r;
  const QVariantMap v = values_.value(kind_);
  r.kind = kind_.toStdString();
  if (projectModel_ != nullptr)
    r.lines = projectModel_->selection();
  r.name = v.value(QStringLiteral("name")).toString().trimmed().toStdString();
  if (kind_ == QLatin1String("preprocess")) {
    PreprocessParameters &p = r.preprocess;
    p.skipExisting = flag(v, "skipExisting");
    p.recalculateBathymetry = flag(v, "recalculateBathymetry");
    p.ignoreWaterColumn = flag(v, "ignoreWaterColumn");
    p.navigationFile = str(v, "navigationFile");
    p.navigationFormat = integer(v, "navigationFormat");
    p.platformFile = str(v, "platformFile");
  } else if (kind_ == QLatin1String("clean")) {
    CleanParameters &p = r.clean;
    p.useSlope = flag(v, "useSlope");
    p.slope = num(v, "slope");
    p.slopeMode = integer(v, "slopeMode");
    p.outerBeams = integer(v, "outerBeams");
    p.useDepthRange = flag(v, "useDepthRange");
    p.depthMin = num(v, "depthMin");
    p.depthMax = num(v, "depthMax");
    p.useMedianFraction = flag(v, "useMedianFraction");
    p.fractionLow = num(v, "fractionLow");
    p.fractionHigh = num(v, "fractionHigh");
    p.spikes = flag(v, "spikes");
    p.spikeSlope = num(v, "spikeSlope");
    p.pingRms = num(v, "pingRms");
    p.areaClean = flag(v, "areaClean");
    p.areaBinSize = num(v, "areaBinSize");
    p.areaStdDev = num(v, "areaStdDev");
  } else if (kind_ == QLatin1String("corrections")) {
    CorrectionParameters &p = r.corrections;
    p.svpMode = integer(v, "svpMode");
    p.svpFile = str(v, "svpFile");
    p.useRollBias = flag(v, "useRollBias");
    p.rollBias = num(v, "rollBias");
    p.usePitchBias = flag(v, "usePitchBias");
    p.pitchBias = num(v, "pitchBias");
    p.useDraft = flag(v, "useDraft");
    p.draft = num(v, "draft");
    p.tideMode = integer(v, "tideMode");
    p.tideFile = str(v, "tideFile");
    p.tideFormat = integer(v, "tideFormat");
  } else if (kind_ == QLatin1String("process")) {
    r.process.threads = integer(v, "threads");
    r.process.force = flag(v, "force");
  } else if (kind_ == QLatin1String("grid")) {
    GridParameters &p = r.grid;
    p.dataType = integer(v, "dataType");
    p.spacing = num(v, "spacing");
    p.units = v.value(QStringLiteral("units")).toString().toStdString();
    p.algorithm = integer(v, "algorithm");
    p.threshold = num(v, "threshold");
    p.clip = integer(v, "clip");
    p.clipMode = integer(v, "clipMode");
    p.tension = num(v, "tension");
    p.extraGrids = flag(v, "extraGrids");
    p.projection = v.value(QStringLiteral("projection")).toString().toStdString();
    p.boundsFactor = num(v, "boundsFactor");
  } else if (kind_ == QLatin1String("mosaic")) {
    MosaicParameters &p = r.mosaic;
    p.dataType = integer(v, "dataType");
    p.spacing = num(v, "spacing");
    p.prioritySource = integer(v, "prioritySource");
    p.priorityRange = num(v, "priorityRange");
    p.clip = integer(v, "clip");
    p.extraGrids = flag(v, "extraGrids");
    p.projection = v.value(QStringLiteral("projection")).toString().toStdString();
    p.angleCorrection = flag(v, "angleCorrection");
  } else if (kind_ == QLatin1String("figure")) {
    FigureParameters &p = r.figure;
    p.kind = v.value(QStringLiteral("template")).toString().toStdString();
    const QString source = v.value(QStringLiteral("source")).toString();
    if (source == QLatin1String("none") || p.kind == "overview") {
      r.asset.clear();
    } else if (source.isEmpty()) {
      // the newest grid (a quality grid for the quality map)
      if (project_ && project_->isOpen())
        for (const auto &a : project_->assets()) {
          const bool quality = a.type == "grid_num" || a.type == "grid_sd";
          if ((p.kind == "quality" ? quality : a.type == "grid" && a.parent.empty()))
            r.asset = a.id;
        }
    } else {
      r.asset = source.toStdString();
    }
    p.formats.clear();
    for (const auto &f : v.value(QStringLiteral("formats")).toString().split(QLatin1Char(','), Qt::SkipEmptyParts))
      p.formats.push_back(f.trimmed().toStdString());
    p.widthCm = num(v, "widthCm");
    p.dpi = integer(v, "dpi");
    p.palette = v.value(QStringLiteral("palette")).toString().toStdString();
    p.reversePalette = flag(v, "reversePalette");
    p.contourInterval = num(v, "contourInterval");
    p.annotationInterval = num(v, "annotationInterval");
    p.illumination = flag(v, "illumination");
    p.showTracks = flag(v, "showTracks");
    p.colorbar = flag(v, "colorbar");
    p.scaleBar = flag(v, "scaleBar");
    p.title = v.value(QStringLiteral("title")).toString().toStdString();
    p.zLabel = v.value(QStringLiteral("zLabel")).toString().toStdString();
    p.viewAzimuth = num(v, "viewAzimuth");
    p.viewElevation = num(v, "viewElevation");
    p.reliefHeightCm = num(v, "reliefHeightCm");
    if (p.kind == "quality" && p.zLabel == "Depth (m)")
      p.zLabel = "Soundings per cell";
  } else if (kind_ == QLatin1String("export")) {
    const QString source = v.value(QStringLiteral("source")).toString();
    if (source == QLatin1String("soundings")) {
      r.kind = "export-soundings";
      r.exporting.kind = v.value(QStringLiteral("soundingFormat")).toString().toStdString();
      r.exporting.projection = v.value(QStringLiteral("projection")).toString().toStdString();
    } else {
      r.kind = "export-grid";
      r.asset = source.toStdString();
      r.exporting.kind = v.value(QStringLiteral("gridFormat")).toString().toStdString();
      r.exporting.exaggeration = num(v, "exaggeration");
    }
  }
  return r;
}

bool ProcessingViewModel::loadRequest(qint64 sequence) {
  ProcessingRequest r;
  if (!project_ || !project_->request(sequence, r))
    return false;
  QString kind = QString::fromStdString(r.kind);
  if (kind == QLatin1String("export-grid") || kind == QLatin1String("export-soundings"))
    kind = QStringLiteral("export");
  if (!kinds().contains(kind))
    return false;
  QVariantMap v = defaults(kind);
  v[QStringLiteral("name")] = QString::fromStdString(r.name);
  if (kind == QLatin1String("preprocess")) {
    const PreprocessParameters &p = r.preprocess;
    v[QStringLiteral("skipExisting")] = p.skipExisting;
    v[QStringLiteral("recalculateBathymetry")] = p.recalculateBathymetry;
    v[QStringLiteral("ignoreWaterColumn")] = p.ignoreWaterColumn;
    v[QStringLiteral("navigationFile")] = QString::fromStdString(p.navigationFile);
    v[QStringLiteral("navigationFormat")] = p.navigationFormat;
    v[QStringLiteral("platformFile")] = QString::fromStdString(p.platformFile);
  } else if (kind == QLatin1String("clean")) {
    const CleanParameters &p = r.clean;
    v[QStringLiteral("useSlope")] = p.useSlope;
    v[QStringLiteral("slope")] = p.slope;
    v[QStringLiteral("slopeMode")] = p.slopeMode;
    v[QStringLiteral("outerBeams")] = p.outerBeams;
    v[QStringLiteral("useDepthRange")] = p.useDepthRange;
    v[QStringLiteral("depthMin")] = p.depthMin;
    v[QStringLiteral("depthMax")] = p.depthMax;
    v[QStringLiteral("useMedianFraction")] = p.useMedianFraction;
    v[QStringLiteral("fractionLow")] = p.fractionLow;
    v[QStringLiteral("fractionHigh")] = p.fractionHigh;
    v[QStringLiteral("spikes")] = p.spikes;
    v[QStringLiteral("spikeSlope")] = p.spikeSlope;
    v[QStringLiteral("pingRms")] = p.pingRms;
    v[QStringLiteral("areaClean")] = p.areaClean;
    v[QStringLiteral("areaBinSize")] = p.areaBinSize;
    v[QStringLiteral("areaStdDev")] = p.areaStdDev;
  } else if (kind == QLatin1String("corrections")) {
    const CorrectionParameters &p = r.corrections;
    v[QStringLiteral("svpMode")] = p.svpMode;
    v[QStringLiteral("svpFile")] = QString::fromStdString(p.svpFile);
    v[QStringLiteral("useRollBias")] = p.useRollBias;
    v[QStringLiteral("rollBias")] = p.rollBias;
    v[QStringLiteral("usePitchBias")] = p.usePitchBias;
    v[QStringLiteral("pitchBias")] = p.pitchBias;
    v[QStringLiteral("useDraft")] = p.useDraft;
    v[QStringLiteral("draft")] = p.draft;
    v[QStringLiteral("tideMode")] = p.tideMode;
    v[QStringLiteral("tideFile")] = QString::fromStdString(p.tideFile);
    v[QStringLiteral("tideFormat")] = p.tideFormat;
  } else if (kind == QLatin1String("process")) {
    v[QStringLiteral("threads")] = r.process.threads;
    v[QStringLiteral("force")] = r.process.force;
  } else if (kind == QLatin1String("grid")) {
    const GridParameters &p = r.grid;
    v[QStringLiteral("dataType")] = p.dataType;
    v[QStringLiteral("spacing")] = p.spacing;
    v[QStringLiteral("units")] = QString::fromStdString(p.units);
    v[QStringLiteral("algorithm")] = p.algorithm;
    v[QStringLiteral("threshold")] = p.threshold;
    v[QStringLiteral("clip")] = p.clip;
    v[QStringLiteral("clipMode")] = p.clipMode;
    v[QStringLiteral("tension")] = p.tension;
    v[QStringLiteral("extraGrids")] = p.extraGrids;
    v[QStringLiteral("projection")] = QString::fromStdString(p.projection);
    v[QStringLiteral("boundsFactor")] = p.boundsFactor;
  } else if (kind == QLatin1String("mosaic")) {
    const MosaicParameters &p = r.mosaic;
    v[QStringLiteral("dataType")] = p.dataType;
    v[QStringLiteral("spacing")] = p.spacing;
    v[QStringLiteral("prioritySource")] = p.prioritySource;
    v[QStringLiteral("priorityRange")] = p.priorityRange;
    v[QStringLiteral("clip")] = p.clip;
    v[QStringLiteral("extraGrids")] = p.extraGrids;
    v[QStringLiteral("projection")] = QString::fromStdString(p.projection);
    v[QStringLiteral("angleCorrection")] = p.angleCorrection;
  } else if (kind == QLatin1String("export")) {
    if (r.kind == "export-grid") {
      v[QStringLiteral("source")] = QString::fromStdString(r.asset);
      v[QStringLiteral("gridFormat")] = QString::fromStdString(r.exporting.kind);
      v[QStringLiteral("exaggeration")] = r.exporting.exaggeration;
    } else {
      v[QStringLiteral("source")] = QStringLiteral("soundings");
      v[QStringLiteral("soundingFormat")] = QString::fromStdString(r.exporting.kind);
      v[QStringLiteral("projection")] = QString::fromStdString(r.exporting.projection);
    }
  } else if (kind == QLatin1String("figure")) {
    const FigureParameters &p = r.figure;
    v[QStringLiteral("template")] = QString::fromStdString(p.kind);
    v[QStringLiteral("source")] = r.asset.empty() ? QStringLiteral("none") : QString::fromStdString(r.asset);
    QStringList formats;
    for (const auto &f : p.formats)
      formats << QString::fromStdString(f);
    v[QStringLiteral("formats")] = formats.join(QLatin1Char(','));
    v[QStringLiteral("widthCm")] = p.widthCm;
    v[QStringLiteral("dpi")] = p.dpi;
    v[QStringLiteral("palette")] = QString::fromStdString(p.palette);
    v[QStringLiteral("reversePalette")] = p.reversePalette;
    v[QStringLiteral("contourInterval")] = p.contourInterval;
    v[QStringLiteral("annotationInterval")] = p.annotationInterval;
    v[QStringLiteral("illumination")] = p.illumination;
    v[QStringLiteral("showTracks")] = p.showTracks;
    v[QStringLiteral("colorbar")] = p.colorbar;
    v[QStringLiteral("scaleBar")] = p.scaleBar;
    v[QStringLiteral("title")] = QString::fromStdString(p.title);
    v[QStringLiteral("zLabel")] = QString::fromStdString(p.zLabel);
    v[QStringLiteral("viewAzimuth")] = p.viewAzimuth;
    v[QStringLiteral("viewElevation")] = p.viewElevation;
    v[QStringLiteral("reliefHeightCm")] = p.reliefHeightCm;
  }
  values_[kind] = v;
  // the same survey lines
  if (projectModel_ != nullptr && !r.lines.empty()) {
    QStringList ids;
    for (const auto &id : r.lines)
      ids << QString::fromStdString(id);
    projectModel_->selectIds(ids);
  }
  if (kind_ != kind) {
    kind_ = kind;
    emit kindChanged();
  }
  revision_++;
  emit parametersChanged();
  updatePreview();
  return true;
}

void ProcessingViewModel::updatePreview() {
  preview_.clear();
  outputs_.clear();
  warnings_.clear();
  error_.clear();
  estimate_.clear();
  if (!project_) {
    error_ = tr("Projects are not available (the project plugin is not loaded)");
  } else if (!project_->isOpen()) {
    error_ = tr("Create or open a project first");
  } else {
    const ProcessingRequest r = request();
    const CommandPlan plan = project_->plan(r);
    if (!plan.ok) {
      error_ = QString::fromStdString(plan.error);
    } else {
      QStringList lines;
      for (const auto &c : plan.commandLines())
        lines << QStringLiteral("$ ") + QString::fromStdString(c);
      preview_ = lines.join(QLatin1Char('\n'));
      for (const auto &o : plan.outputs)
        outputs_ << QDir::toNativeSeparators(QString::fromStdString(o));
      for (const auto &w : plan.warnings)
        warnings_ << QString::fromStdString(w);
    }
    if (kind_ == QLatin1String("grid") && projectModel_ != nullptr && processing_) {
      // size of the grid before it is made: cells and working memory
      const GridEstimate e = processing_->estimateGrid(r.grid, projectModel_->selectionBounds());
      if (e.cells > 0) {
        const QLocale locale;
        estimate_ = tr("About %1 × %2 cells, %3 MB of memory")
                        .arg(locale.toString(e.nx), locale.toString(e.ny),
                             locale.toString(e.megabytes, 'f', e.megabytes < 10 ? 1 : 0));
        if (e.megabytes > 4096)
          warnings_ << tr("This grid needs a lot of memory; consider a larger cell size.");
      }
    }
  }
  emit previewChanged();
}

qint64 ProcessingViewModel::run() {
  updatePreview();
  if (!project_ || !error_.isEmpty()) {
    emit notify(error_);
    return 0;
  }
  std::string error;
  const qint64 id = project_->run(request(), error);
  if (id == 0) {
    error_ = QString::fromStdString(error);
    emit previewChanged();
    emit notify(tr("Could not start: %1").arg(error_));
    return 0;
  }
  emit started(id, title());
  emit notify(tr("Started: %1").arg(title()));
  // the next run gets a new job folder and product name
  previewTimer_.start();
  return id;
}

}  // namespace mbstudio::ui
