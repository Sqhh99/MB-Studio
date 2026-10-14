#include "RequestJson.h"

#include <QJsonArray>
#include <QString>

namespace mbstudio::project {

namespace {

class Writer {
 public:
  explicit Writer(QJsonObject &o) : o_(o) {}
  void operator()(const char *key, const std::string &v) { o_[QLatin1String(key)] = QString::fromStdString(v); }
  void operator()(const char *key, const char *v) { o_[QLatin1String(key)] = QString::fromUtf8(v); }
  void operator()(const char *key, bool v) { o_[QLatin1String(key)] = v; }
  void operator()(const char *key, int v) { o_[QLatin1String(key)] = v; }
  void operator()(const char *key, double v) { o_[QLatin1String(key)] = v; }
  void operator()(const char *key, const GeoBounds &b) {
    if (b.valid())
      o_[QLatin1String(key)] = QJsonArray{b.west, b.east, b.south, b.north};
  }
  void operator()(const char *key, const std::vector<std::string> &list) {
    QJsonArray a;
    for (const auto &s : list)
      a.append(QString::fromStdString(s));
    o_[QLatin1String(key)] = a;
  }

 private:
  QJsonObject &o_;
};

class Reader {
 public:
  explicit Reader(const QJsonObject &o) : o_(o) {}
  void operator()(const char *key, std::string &v) const {
    if (o_.contains(QLatin1String(key)))
      v = o_.value(QLatin1String(key)).toString().toStdString();
  }
  void operator()(const char *key, bool &v) const {
    if (o_.value(QLatin1String(key)).isBool())
      v = o_.value(QLatin1String(key)).toBool();
  }
  void operator()(const char *key, int &v) const {
    if (o_.value(QLatin1String(key)).isDouble())
      v = o_.value(QLatin1String(key)).toInt();
  }
  void operator()(const char *key, double &v) const {
    if (o_.value(QLatin1String(key)).isDouble())
      v = o_.value(QLatin1String(key)).toDouble();
  }
  void operator()(const char *key, GeoBounds &b) const {
    const QJsonArray a = o_.value(QLatin1String(key)).toArray();
    if (a.size() == 4)
      b = GeoBounds{a[0].toDouble(), a[1].toDouble(), a[2].toDouble(), a[3].toDouble()};
  }
  void operator()(const char *key, std::vector<std::string> &list) const {
    if (!o_.value(QLatin1String(key)).isArray())
      return;
    list.clear();
    for (const auto &item : o_.value(QLatin1String(key)).toArray())
      list.push_back(item.toString().toStdString());
  }

 private:
  const QJsonObject &o_;
};

// One list of fields per parameter structure, used for both directions.
template <typename F, typename P>
void preprocessFields(F &&f, P &p) {
  f("navigationFile", p.navigationFile);
  f("navigationFormat", p.navigationFormat);
  f("platformFile", p.platformFile);
  f("skipExisting", p.skipExisting);
  f("recalculateBathymetry", p.recalculateBathymetry);
  f("ignoreWaterColumn", p.ignoreWaterColumn);
}

template <typename F, typename P>
void cleanFields(F &&f, P &p) {
  f("useSlope", p.useSlope);
  f("slope", p.slope);
  f("slopeMode", p.slopeMode);
  f("useMedianFraction", p.useMedianFraction);
  f("fractionLow", p.fractionLow);
  f("fractionHigh", p.fractionHigh);
  f("useDepthRange", p.useDepthRange);
  f("depthMin", p.depthMin);
  f("depthMax", p.depthMax);
  f("outerBeams", p.outerBeams);
  f("spikes", p.spikes);
  f("spikeSlope", p.spikeSlope);
  f("spikeMode", p.spikeMode);
  f("pingRms", p.pingRms);
  f("areaClean", p.areaClean);
  f("areaBinSize", p.areaBinSize);
  f("areaStdDev", p.areaStdDev);
  f("areaMinSoundings", p.areaMinSoundings);
}

template <typename F, typename P>
void correctionFields(F &&f, P &p) {
  f("svpMode", p.svpMode);
  f("svpFile", p.svpFile);
  f("longitude", p.longitude);
  f("latitude", p.latitude);
  f("useRollBias", p.useRollBias);
  f("rollBias", p.rollBias);
  f("usePitchBias", p.usePitchBias);
  f("pitchBias", p.pitchBias);
  f("useDraft", p.useDraft);
  f("draft", p.draft);
  f("tideMode", p.tideMode);
  f("tideFile", p.tideFile);
  f("tideFormat", p.tideFormat);
}

template <typename F, typename P>
void processFields(F &&f, P &p) {
  f("threads", p.threads);
  f("force", p.force);
}

template <typename F, typename P>
void gridFields(F &&f, P &p) {
  f("dataType", p.dataType);
  f("spacing", p.spacing);
  f("units", p.units);
  f("exactSpacing", p.exactSpacing);
  f("bounds", p.bounds);
  f("boundsFactor", p.boundsFactor);
  f("algorithm", p.algorithm);
  f("threshold", p.threshold);
  f("clip", p.clip);
  f("clipMode", p.clipMode);
  f("tension", p.tension);
  f("gaussianScale", p.gaussianScale);
  f("extraGrids", p.extraGrids);
  f("projection", p.projection);
  f("timeDifference", p.timeDifference);
}

template <typename F, typename P>
void mosaicFields(F &&f, P &p) {
  f("dataType", p.dataType);
  f("spacing", p.spacing);
  f("units", p.units);
  f("bounds", p.bounds);
  f("prioritySource", p.prioritySource);
  f("priorityRange", p.priorityRange);
  f("clip", p.clip);
  f("extraGrids", p.extraGrids);
  f("projection", p.projection);
  f("angleCorrection", p.angleCorrection);
}

template <typename F, typename P>
void exportFields(F &&f, P &p) {
  f("kind", p.kind);
  f("projection", p.projection);
  f("exaggeration", p.exaggeration);
}

template <typename F, typename P>
void figureFields(F &&f, P &p) {
  f("kind", p.kind);
  f("region", p.region);
  f("showTracks", p.showTracks);
  f("widthCm", p.widthCm);
  f("palette", p.palette);
  f("reversePalette", p.reversePalette);
  f("contourInterval", p.contourInterval);
  f("annotationInterval", p.annotationInterval);
  f("illumination", p.illumination);
  f("lightAzimuth", p.lightAzimuth);
  f("viewAzimuth", p.viewAzimuth);
  f("viewElevation", p.viewElevation);
  f("reliefHeightCm", p.reliefHeightCm);
  f("colorbar", p.colorbar);
  f("scaleBar", p.scaleBar);
  f("title", p.title);
  f("zLabel", p.zLabel);
  f("formats", p.formats);
  f("dpi", p.dpi);
}

template <typename Fields, typename P>
QJsonObject writeFields(Fields fields, const P &p) {
  QJsonObject o;
  fields(Writer(o), p);
  return o;
}

template <typename Fields, typename P>
void readFields(Fields fields, const QJsonObject &o, P &p) {
  fields(Reader(o), p);
}

}  // namespace

QJsonObject requestToJson(const ProcessingRequest &r) {
  QJsonObject o;
  Writer w(o);
  w("kind", r.kind);
  w("lines", r.lines);
  w("asset", r.asset);
  w("name", r.name);
  const std::string &k = r.kind;
  auto writeAs = [&o](const char *key, const QJsonObject &value) { o[QLatin1String(key)] = value; };
  if (k == "preprocess")
    writeAs("preprocess", writeFields([](auto &&f, auto &p) { preprocessFields(f, p); }, r.preprocess));
  else if (k == "clean")
    writeAs("clean", writeFields([](auto &&f, auto &p) { cleanFields(f, p); }, r.clean));
  else if (k == "corrections")
    writeAs("corrections", writeFields([](auto &&f, auto &p) { correctionFields(f, p); }, r.corrections));
  else if (k == "process")
    writeAs("process", writeFields([](auto &&f, auto &p) { processFields(f, p); }, r.process));
  else if (k == "grid")
    writeAs("grid", writeFields([](auto &&f, auto &p) { gridFields(f, p); }, r.grid));
  else if (k == "mosaic")
    writeAs("mosaic", writeFields([](auto &&f, auto &p) { mosaicFields(f, p); }, r.mosaic));
  else if (k == "export-grid" || k == "export-soundings")
    writeAs("export", writeFields([](auto &&f, auto &p) { exportFields(f, p); }, r.exporting));
  else if (k == "figure")
    writeAs("figure", writeFields([](auto &&f, auto &p) { figureFields(f, p); }, r.figure));
  return o;
}

bool requestFromJson(const QJsonObject &o, ProcessingRequest &r) {
  if (!o.value(QLatin1String("kind")).isString())
    return false;
  const Reader in(o);
  in("kind", r.kind);
  in("lines", r.lines);
  in("asset", r.asset);
  in("name", r.name);
  auto part = [&o](const char *key) { return o.value(QLatin1String(key)).toObject(); };
  readFields([](auto &&f, auto &p) { preprocessFields(f, p); }, part("preprocess"), r.preprocess);
  readFields([](auto &&f, auto &p) { cleanFields(f, p); }, part("clean"), r.clean);
  readFields([](auto &&f, auto &p) { correctionFields(f, p); }, part("corrections"), r.corrections);
  readFields([](auto &&f, auto &p) { processFields(f, p); }, part("process"), r.process);
  readFields([](auto &&f, auto &p) { gridFields(f, p); }, part("grid"), r.grid);
  readFields([](auto &&f, auto &p) { mosaicFields(f, p); }, part("mosaic"), r.mosaic);
  readFields([](auto &&f, auto &p) { exportFields(f, p); }, part("export"), r.exporting);
  readFields([](auto &&f, auto &p) { figureFields(f, p); }, part("figure"), r.figure);
  return !r.kind.empty();
}

}  // namespace mbstudio::project
