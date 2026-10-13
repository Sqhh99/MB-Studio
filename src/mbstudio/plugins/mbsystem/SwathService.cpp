#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "MbioServices.h"

extern "C" {
#include "mb_define.h"
#include "mb_status.h"
}

namespace mbstudio::mbsystem {

namespace {

std::string isoTime(double t) {
  if (t <= 0.0)
    return std::string();
  int time_i[7];
  mb_get_date(0, t, time_i);
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", time_i[0], time_i[1], time_i[2],
                time_i[3], time_i[4], time_i[5], time_i[6] / 1000);
  return buffer;
}

double haversineKm(double lon1, double lat1, double lon2, double lat2) {
  constexpr double kDeg = 3.14159265358979323846 / 180.0;
  const double dlat = (lat2 - lat1) * kDeg;
  const double dlon = (lon2 - lon1) * kDeg;
  const double a = std::sin(dlat / 2) * std::sin(dlat / 2) +
                   std::cos(lat1 * kDeg) * std::cos(lat2 * kDeg) * std::sin(dlon / 2) * std::sin(dlon / 2);
  return 6371.0088 * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

}  // namespace

SwathSummary SwathService::inspect(const std::string &path, int format, std::size_t maxTrackPoints,
                                   const std::atomic<bool> *cancel) const {
  SwathSummary summary;
  summary.path = path;
  if (path.empty() || path.size() >= MB_PATH_MAXLINE) {
    summary.error = "invalid file name";
    return summary;
  }
  if (format == 0 && formats_)
    format = formats_->identify(path);
  if (format == 0) {
    summary.error = "unable to identify the MB-System format of " + path;
    return summary;
  }
  summary.format = format;
  FormatInfo info;
  if (formats_ && formats_->format(format, info))
    summary.formatName = info.name;

  std::lock_guard<std::mutex> lock(mbioMutex());

  char file[MB_PATH_MAXLINE];
  std::strncpy(file, path.c_str(), sizeof(file) - 1);
  file[sizeof(file) - 1] = '\0';

  const int verbose = 0;
  const int pings = 1;
  const int lonflip = 0;
  double bounds[4] = {-360.0, 360.0, -90.0, 90.0};
  int btime_i[7] = {1962, 2, 21, 10, 30, 0, 0};
  int etime_i[7] = {2100, 1, 1, 0, 0, 0, 0};
  const double speedmin = 0.0;
  const double timegap = 1000000000.0;
  void *mbio_ptr = nullptr;
  double btime_d = 0.0, etime_d = 0.0;
  int beams_bath = 0, beams_amp = 0, pixels_ss = 0;
  int error = MB_ERROR_NO_ERROR;

  if (mb_read_init(verbose, file, format, pings, lonflip, bounds, btime_i, etime_i, speedmin, timegap, &mbio_ptr,
                   &btime_d, &etime_d, &beams_bath, &beams_amp, &pixels_ss, &error) != MB_SUCCESS) {
    char *message = nullptr;
    mb_error(verbose, error, &message);
    summary.error = std::string("MBIO initialisation failed: ") + (message != nullptr ? message : "unknown error");
    return summary;
  }

  char *beamflag = nullptr;
  double *bath = nullptr, *amp = nullptr, *bathlon = nullptr, *bathlat = nullptr;
  double *ss = nullptr, *sslon = nullptr, *sslat = nullptr;
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_BATHYMETRY, sizeof(char), (void **)&beamflag, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_BATHYMETRY, sizeof(double), (void **)&bath, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_AMPLITUDE, sizeof(double), (void **)&amp, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_BATHYMETRY, sizeof(double), (void **)&bathlon, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_BATHYMETRY, sizeof(double), (void **)&bathlat, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_SIDESCAN, sizeof(double), (void **)&ss, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_SIDESCAN, sizeof(double), (void **)&sslon, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_SIDESCAN, sizeof(double), (void **)&sslat, &error);

  std::vector<TrackPoint> track;
  bool haveBounds = false;
  bool haveDepth = false;
  double lastLon = 0.0, lastLat = 0.0;
  bool haveLast = false;
  char comment[MB_COMMENT_MAXLINE];

  error = MB_ERROR_NO_ERROR;
  while (error <= MB_ERROR_NO_ERROR) {
    if (cancel != nullptr && cancel->load()) {
      summary.error = "cancelled";
      break;
    }
    int kind = 0;
    int npings = 0;
    int time_i[7];
    double time_d = 0.0, navlon = 0.0, navlat = 0.0, speed = 0.0, heading = 0.0, distance = 0.0;
    double altitude = 0.0, sensordepth = 0.0;
    int nbath = 0, namp = 0, nss = 0;
    error = MB_ERROR_NO_ERROR;
    mb_read(verbose, mbio_ptr, &kind, &npings, time_i, &time_d, &navlon, &navlat, &speed, &heading, &distance,
            &altitude, &sensordepth, &nbath, &namp, &nss, beamflag, bath, amp, bathlon, bathlat, ss, sslon, sslat,
            comment, &error);
    if (error > MB_ERROR_NO_ERROR)
      break;  // end of file or fatal error
    if (!(error == MB_ERROR_NO_ERROR || error == MB_ERROR_TIME_GAP) || kind != MB_DATA_DATA)
      continue;

    summary.records++;
    summary.maxBeams = std::max(summary.maxBeams, nbath);
    if (summary.timeStart == 0.0 || time_d < summary.timeStart)
      summary.timeStart = time_d;
    summary.timeEnd = std::max(summary.timeEnd, time_d);

    double centerDepth = 0.0;
    for (int i = 0; i < nbath; i++) {
      summary.beamsTotal++;
      if (mb_beam_ok(beamflag[i])) {
        summary.beamsGood++;
        if (!haveDepth) {
          summary.depthMin = summary.depthMax = bath[i];
          haveDepth = true;
        }
        summary.depthMin = std::min(summary.depthMin, bath[i]);
        summary.depthMax = std::max(summary.depthMax, bath[i]);
      } else if (!mb_beam_check_flag_null(beamflag[i])) {
        summary.beamsFlagged++;
      }
    }
    if (nbath > 0 && mb_beam_ok(beamflag[nbath / 2]))
      centerDepth = bath[nbath / 2];

    if (navlon != 0.0 || navlat != 0.0) {
      if (!haveBounds) {
        summary.lonMin = summary.lonMax = navlon;
        summary.latMin = summary.latMax = navlat;
        haveBounds = true;
      }
      summary.lonMin = std::min(summary.lonMin, navlon);
      summary.lonMax = std::max(summary.lonMax, navlon);
      summary.latMin = std::min(summary.latMin, navlat);
      summary.latMax = std::max(summary.latMax, navlat);
      if (haveLast)
        summary.trackLengthKm += haversineKm(lastLon, lastLat, navlon, navlat);
      lastLon = navlon;
      lastLat = navlat;
      haveLast = true;
      track.push_back(TrackPoint{navlon, navlat, time_d, centerDepth});
    }
  }
  mb_close(verbose, &mbio_ptr, &error);

  // Decimate the navigation to at most maxTrackPoints (keeping the ends).
  if (maxTrackPoints >= 2 && track.size() > maxTrackPoints) {
    std::vector<TrackPoint> decimated;
    decimated.reserve(maxTrackPoints);
    const double step = static_cast<double>(track.size() - 1) / static_cast<double>(maxTrackPoints - 1);
    for (std::size_t i = 0; i < maxTrackPoints; i++)
      decimated.push_back(track[static_cast<std::size_t>(std::lround(i * step))]);
    track.swap(decimated);
  }
  summary.track = std::move(track);
  summary.timeStartIso = isoTime(summary.timeStart);
  summary.timeEndIso = isoTime(summary.timeEnd);
  if (summary.error.empty())
    summary.ok = true;
  return summary;
}

}  // namespace mbstudio::mbsystem
