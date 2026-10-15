// Soundings of swath files for interactive editing, with the edits of the
// edit save file applied, and saving of new edit events (see
// mbstudio/api/SoundingService.h). Follows the bathymetry editor
// integration of mbeditviz: mb_esf_load / mb_esf_apply when reading,
// mb_esf_load in append mode / mb_esf_save and the parameter file update
// when saving.
#include <algorithm>
#include <cmath>
#include <cstring>

#include "MbioServices.h"
#include "mbstudio/platform/Paths.h"

extern "C" {
#include "mb_define.h"
#include "mb_process.h"
#include "mb_status.h"
}

namespace mbstudio::mbsystem {

namespace {

constexpr const char *kProgram = "MB-Studio";
constexpr double kDegToRad = 3.14159265358979323846 / 180.0;

std::string mbioError(int error) {
  char *message = nullptr;
  mb_error(0, error, &message);
  return message != nullptr ? std::string(message) : std::string("MBIO error ") + std::to_string(error);
}

}  // namespace

SoundingSet SoundingService::load(const std::string &path, int format, std::size_t maxSoundings,
                                  const std::atomic<bool> *cancel) const {
  SoundingSet set;
  set.path = path;
  if (path.empty() || path.size() >= MB_PATH_MAXLINE) {
    set.error = "invalid file name";
    return set;
  }
  if (format == 0 && formats_)
    format = formats_->identify(path);
  if (format <= 0) {
    set.error = "unable to identify the MB-System format of " + path;
    return set;
  }
  set.format = format;

  std::lock_guard<std::mutex> lock(mbioMutex());
  char file[MB_PATH_MAXLINE];
  std::strncpy(file, path.c_str(), sizeof(file) - 1);
  file[sizeof(file) - 1] = '\0';

  const int verbose = 0;
  double bounds[4] = {-360.0, 360.0, -90.0, 90.0};
  int btime_i[7] = {1962, 2, 21, 10, 30, 0, 0};
  int etime_i[7] = {2100, 1, 1, 0, 0, 0, 0};
  void *mbio_ptr = nullptr;
  double btime_d = 0.0, etime_d = 0.0;
  int beams_bath = 0, beams_amp = 0, pixels_ss = 0;
  int error = MB_ERROR_NO_ERROR;
  if (mb_read_init(verbose, file, format, 1, 0, bounds, btime_i, etime_i, 0.0, 1000000000.0, &mbio_ptr, &btime_d,
                   &etime_d, &beams_bath, &beams_amp, &pixels_ss, &error) != MB_SUCCESS) {
    set.error = "MBIO initialisation failed: " + mbioError(error);
    return set;
  }

  char *beamflag = nullptr;
  double *bath = nullptr, *amp = nullptr, *acrosstrack = nullptr, *alongtrack = nullptr;
  double *ss = nullptr, *ssacross = nullptr, *ssalong = nullptr;
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_BATHYMETRY, sizeof(char), (void **)&beamflag, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_BATHYMETRY, sizeof(double), (void **)&bath, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_AMPLITUDE, sizeof(double), (void **)&amp, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_BATHYMETRY, sizeof(double), (void **)&acrosstrack, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_BATHYMETRY, sizeof(double), (void **)&alongtrack, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_SIDESCAN, sizeof(double), (void **)&ss, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_SIDESCAN, sizeof(double), (void **)&ssacross, &error);
  mb_register_array(verbose, mbio_ptr, MB_MEM_TYPE_SIDESCAN, sizeof(double), (void **)&ssalong, &error);

  // the saved edits of the file (the edit save file named in its mbprocess
  // parameter file, or "<file>.esf")
  struct mb_esf_struct esf;
  std::memset(&esf, 0, sizeof(esf));
  char esffile[MB_PATH_MAXLINE] = "";
  error = MB_ERROR_NO_ERROR;
  bool esfOpen = mb_esf_load(verbose, kProgram, file, true, MBP_ESF_NOWRITE, esffile, &esf, &error) == MB_SUCCESS;
  if (esfOpen) {
    set.esfPath = esffile;
    set.savedEdits = esf.nedit;
  }

  char comment[MB_COMMENT_MAXLINE];
  double lastTime = -1.0;
  int multiplicity = 0;
  error = MB_ERROR_NO_ERROR;
  while (error <= MB_ERROR_NO_ERROR) {
    if (cancel != nullptr && cancel->load()) {
      set.error = "cancelled";
      break;
    }
    int kind = 0, pings = 0, time_i[7];
    double time_d = 0.0, navlon = 0.0, navlat = 0.0, speed = 0.0, heading = 0.0, distance = 0.0;
    double altitude = 0.0, sensordepth = 0.0;
    int nbath = 0, namp = 0, nss = 0;
    error = MB_ERROR_NO_ERROR;
    mb_get(verbose, mbio_ptr, &kind, &pings, time_i, &time_d, &navlon, &navlat, &speed, &heading, &distance,
           &altitude, &sensordepth, &nbath, &namp, &nss, beamflag, bath, amp, acrosstrack, alongtrack, ss, ssacross,
           ssalong, comment, &error);
    if (error > MB_ERROR_NO_ERROR)
      break;
    if (!(error == MB_ERROR_NO_ERROR || error == MB_ERROR_TIME_GAP) || kind != MB_DATA_DATA)
      continue;
    // pings sharing a time stamp (dual heads) are told apart by their order
    multiplicity = time_d == lastTime ? multiplicity + 1 : 0;
    lastTime = time_d;
    if (esfOpen && nbath > 0)
      mb_esf_apply(verbose, &esf, time_d, multiplicity, nbath, beamflag, &error);
    error = MB_ERROR_NO_ERROR;

    if (maxSoundings > 0 && set.flag.size() + static_cast<std::size_t>(nbath) > maxSoundings) {
      set.truncated = true;
      break;
    }
    SoundingPing ping;
    ping.time = time_d;
    ping.multiplicity = multiplicity;
    ping.navLon = navlon;
    ping.navLat = navlat;
    ping.heading = heading;
    ping.first = static_cast<std::uint32_t>(set.flag.size());
    ping.beams = static_cast<std::uint32_t>(std::max(0, nbath));
    double mtodeglon = 0.0, mtodeglat = 0.0;
    mb_coor_scale(verbose, navlat, &mtodeglon, &mtodeglat);
    const double hx = std::sin(heading * kDegToRad), hy = std::cos(heading * kDegToRad);
    for (int i = 0; i < nbath; i++) {
      set.lon.push_back(navlon + hy * mtodeglon * acrosstrack[i] + hx * mtodeglon * alongtrack[i]);
      set.lat.push_back(navlat - hx * mtodeglat * acrosstrack[i] + hy * mtodeglat * alongtrack[i]);
      set.depth.push_back(static_cast<float>(bath[i]));
      set.acrossTrack.push_back(static_cast<float>(acrosstrack[i]));
      set.flag.push_back(static_cast<std::uint8_t>(beamflag[i]));
    }
    set.pings.push_back(ping);
  }
  if (esfOpen)
    mb_esf_close(verbose, &esf, &error);
  mb_close(verbose, &mbio_ptr, &error);
  if (set.error.empty())
    set.ok = true;
  return set;
}

bool SoundingService::saveEdits(const std::string &path, int format, const std::vector<BeamEdit> &edits,
                                std::string &error, std::string *esfPath) const {
  if (path.empty() || path.size() >= MB_PATH_MAXLINE) {
    error = "invalid file name";
    return false;
  }
  if (format == 0 && formats_)
    format = formats_->identify(path);
  if (format <= 0) {
    error = "unable to identify the MB-System format of " + path;
    return false;
  }
  std::lock_guard<std::mutex> lock(mbioMutex());
  char file[MB_PATH_MAXLINE];
  std::strncpy(file, path.c_str(), sizeof(file) - 1);
  file[sizeof(file) - 1] = '\0';
  const int verbose = 0;
  int status = MB_ERROR_NO_ERROR;

  // other MB-System editors respect this lock
  if (mb_pr_lockswathfile(verbose, file, MBP_LOCK_EDITBATHY, kProgram, &status) != MB_SUCCESS) {
    error = "the file is locked by another program: " + mbioError(status);
    return false;
  }
  struct mb_esf_struct esf;
  std::memset(&esf, 0, sizeof(esf));
  char esffile[MB_PATH_MAXLINE] = "";
  status = MB_ERROR_NO_ERROR;
  // append to the existing edits (later events override earlier ones)
  if (mb_esf_load(verbose, kProgram, file, false, MBP_ESF_APPEND, esffile, &esf, &status) != MB_SUCCESS ||
      esf.esffp == nullptr) {
    error = "cannot open the edit save file " + std::string(esffile) + ": " + mbioError(status);
    mb_esf_close(verbose, &esf, &status);
    mb_pr_unlockswathfile(verbose, file, MBP_LOCK_EDITBATHY, kProgram, &status);
    return false;
  }
  bool ok = true;
  for (const auto &e : edits) {
    status = MB_ERROR_NO_ERROR;
    if (mb_esf_save(verbose, &esf, e.time, e.beam + e.multiplicity * MB_ESF_MULTIPLICITY_FACTOR,
                    static_cast<int>(e.action), &status) != MB_SUCCESS) {
      error = "cannot write the edit save file: " + mbioError(status);
      ok = false;
      break;
    }
  }
  mb_esf_close(verbose, &esf, &status);
  if (ok) {
    // mbprocess applies the edits of this file from now on
    status = MB_ERROR_NO_ERROR;
    mb_pr_update_format(verbose, file, true, format, &status);
    status = MB_ERROR_NO_ERROR;
    if (mb_pr_update_edit(verbose, file, MBP_EDIT_ON, esffile, &status) != MB_SUCCESS) {
      error = "cannot update the mbprocess parameter file: " + mbioError(status);
      ok = false;
    }
  }
  mb_pr_unlockswathfile(verbose, file, MBP_LOCK_EDITBATHY, kProgram, &status);
  if (ok && esfPath != nullptr)
    *esfPath = esffile;
  return ok;
}

}  // namespace mbstudio::mbsystem
