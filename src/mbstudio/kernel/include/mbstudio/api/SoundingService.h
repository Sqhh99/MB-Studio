// MB-Studio service interface - the soundings of swath files for
// interactive bathymetry editing.
//
// load() reads every ping of a file with MBIO and applies the edits saved
// for it (the edit save file, ".esf", that mbclean, mbeditviz, mbedit and
// MB-Studio write), so the beam flags are the ones mbprocess will apply.
// Each sounding keeps its identity: the file, the ping time stamp, the ping
// multiplicity (pings sharing a time stamp) and the beam number - exactly
// what an edit event of the edit save file refers to.
//
// saveEdits() appends edit events to the edit save file and switches the
// mbprocess parameter file to use it (EDITSAVEMODE / EDITSAVEFILE), as
// described in MB-System's bathymetry editor integration guide, so that the
// next mbprocess run applies them. Only the edit save file and the parameter
// file are written; the swath file is never changed.
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mbstudio {

// MBIO beam flags (mb_status.h).
namespace beamflag {
constexpr std::uint8_t kNull = 0x01;      // no data
constexpr std::uint8_t kFlag = 0x01;      // bit: not usable
constexpr std::uint8_t kManual = 0x04;    // flagged by an editor
constexpr std::uint8_t kFilter = 0x08;    // flagged by an automatic filter
constexpr std::uint8_t kSonar = 0x80;     // flagged by the sonar
inline bool isNull(std::uint8_t f) { return f == kNull; }
inline bool isGood(std::uint8_t f) { return (f & kFlag) == 0; }
inline bool isFlagged(std::uint8_t f) { return (f & kFlag) != 0 && f != kNull; }
}  // namespace beamflag

// Edit actions of the edit save file (mb_process.h: MBP_EDIT_*).
enum class BeamAction : int { Flag = 1, Unflag = 2, Zero = 3, Filter = 4, Sonar = 5 };

struct SoundingPing {
  double time = 0.0;          // MBIO time stamp (epoch seconds)
  int multiplicity = 0;       // earlier pings with the same time stamp
  double navLon = 0.0, navLat = 0.0, heading = 0.0;
  std::uint32_t first = 0;    // index of its first beam in the arrays of the set
  std::uint32_t beams = 0;    // beams of the ping (including null beams)
};

struct SoundingSet {
  bool ok = false;
  std::string error;
  std::string path;
  int format = 0;
  std::string esfPath;        // edit save file that was applied ("" if none)
  long savedEdits = 0;        // edit events read from it
  bool truncated = false;     // stopped at maxSoundings
  std::vector<SoundingPing> pings;
  // one entry per beam of every ping, in ping order
  std::vector<double> lon, lat;
  std::vector<float> depth;          // metres, positive down
  std::vector<float> acrossTrack;    // metres, starboard positive
  std::vector<std::uint8_t> flag;    // MBIO beam flag after the saved edits
  std::size_t size() const { return flag.size(); }
};

struct BeamEdit {
  double time = 0.0;
  int multiplicity = 0;
  int beam = 0;
  BeamAction action = BeamAction::Flag;
};

class ISoundingService {
 public:
  static constexpr const char *kServiceId = "mbsystem.soundings";
  virtual ~ISoundingService() = default;
  // format 0: identify from the file name. maxSoundings 0: no limit.
  virtual SoundingSet load(const std::string &path, int format, std::size_t maxSoundings,
                           const std::atomic<bool> *cancel = nullptr) const = 0;
  // Append edit events for a swath file and set its mbprocess parameters.
  virtual bool saveEdits(const std::string &path, int format, const std::vector<BeamEdit> &edits, std::string &error,
                         std::string *esfPath = nullptr) const = 0;
};

}  // namespace mbstudio
