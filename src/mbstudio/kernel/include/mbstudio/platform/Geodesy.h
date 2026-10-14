// MB-Studio platform layer - map projections of gridded products.
//
// MB-System writes projected grids in the transverse Mercator projections it
// knows by name: "UTM<zone>N|S", "EPSG:326<zone>|327<zone>" and the local
// "LTM<lon>/<lat>" (see mb_readwritegrd.c). The workbench needs them on the
// WGS 84 (EPSG:4326) map, so this is a small, dependency free implementation
// of the ellipsoidal transverse Mercator (Snyder, USGS Professional Paper
// 1395, p. 61), accurate to millimetres within a UTM zone.
#pragma once

#include <string>

#include "mbstudio/kernel/Export.h"

namespace mbstudio::platform {

class MBSTUDIO_KERNEL_API MapProjection {
 public:
  enum class Kind { Invalid, Geographic, TransverseMercator };

  // "", "Geographic", "EPSG:4326": geographic; "UTM10N", "UTM10S",
  // "EPSG:32610", "EPSG:32710", "LTM-122.5/36.8": transverse Mercator.
  static MapProjection parse(const std::string &id);
  // UTM zone (1..60) that contains a longitude.
  static int utmZone(double longitude);

  Kind kind() const { return kind_; }
  bool valid() const { return kind_ != Kind::Invalid; }
  bool geographic() const { return kind_ == Kind::Geographic; }
  const std::string &id() const { return id_; }
  // Human readable name ("UTM zone 10 N", "WGS 84 geographic").
  std::string name() const;

  // Geographic (degrees) to projected (metres) and back; false outside the
  // domain of the projection (more than 30 degrees from the central meridian).
  bool forward(double lon, double lat, double &x, double &y) const;
  bool inverse(double x, double y, double &lon, double &lat) const;

 private:
  Kind kind_ = Kind::Invalid;
  std::string id_;
  double lon0_ = 0.0, lat0_ = 0.0, k0_ = 1.0, falseEasting_ = 0.0, falseNorthing_ = 0.0;
  int zone_ = 0;
  bool south_ = false;
};

// Great circle distance in metres on the WGS 84 mean sphere.
MBSTUDIO_KERNEL_API double geodesicDistance(double lon1, double lat1, double lon2, double lat2);

}  // namespace mbstudio::platform
