// MB-Studio service interface - swath sonar file inspection.
#pragma once

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

namespace mbstudio {

struct TrackPoint {
  double lon = 0.0;
  double lat = 0.0;
  double time = 0.0;   // seconds since 1970-01-01 UTC
  double depth = 0.0;  // center beam depth (m), 0 if unknown
};

struct SwathSummary {
  bool ok = false;
  std::string error;
  std::string path;
  int format = 0;
  std::string formatName;
  long records = 0;          // survey (ping) records
  int maxBeams = 0;
  long beamsTotal = 0;
  long beamsGood = 0;
  long beamsFlagged = 0;
  double timeStart = 0.0;
  double timeEnd = 0.0;
  std::string timeStartIso;
  std::string timeEndIso;
  double lonMin = 0.0, lonMax = 0.0, latMin = 0.0, latMax = 0.0;
  double depthMin = 0.0, depthMax = 0.0;
  double trackLengthKm = 0.0;
  std::vector<TrackPoint> track;  // decimated navigation
};

class ISwathService {
 public:
  static constexpr const char *kServiceId = "mbsystem.swath";
  virtual ~ISwathService() = default;
  // Read a swath file (format 0: identify from the file name). At most
  // maxTrackPoints navigation points are returned.
  virtual SwathSummary inspect(const std::string &path, int format, std::size_t maxTrackPoints,
                               const std::atomic<bool> *cancel = nullptr) const = 0;
  // Navigation from a ".fnv" fast navigation file (written by mbdatalist -O
  // or mbprocess), decimated to at most maxPoints; empty if unreadable.
  virtual std::vector<TrackPoint> readNavigation(const std::string &fnvPath, std::size_t maxPoints) const = 0;
};

}  // namespace mbstudio
