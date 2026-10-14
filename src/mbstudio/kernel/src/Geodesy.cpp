#include "mbstudio/platform/Geodesy.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace mbstudio::platform {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg = kPi / 180.0;
// WGS 84
constexpr double kA = 6378137.0;
constexpr double kF = 1.0 / 298.257223563;
constexpr double kE2 = kF * (2.0 - kF);
constexpr double kEp2 = kE2 / (1.0 - kE2);

double meridionalArc(double phi) {
  const double e4 = kE2 * kE2, e6 = e4 * kE2;
  return kA * ((1.0 - kE2 / 4.0 - 3.0 * e4 / 64.0 - 5.0 * e6 / 256.0) * phi -
               (3.0 * kE2 / 8.0 + 3.0 * e4 / 32.0 + 45.0 * e6 / 1024.0) * std::sin(2.0 * phi) +
               (15.0 * e4 / 256.0 + 45.0 * e6 / 1024.0) * std::sin(4.0 * phi) -
               (35.0 * e6 / 3072.0) * std::sin(6.0 * phi));
}

std::string upper(std::string s) {
  for (auto &c : s)
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return s;
}

std::string trim(const std::string &s) {
  const auto b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos)
    return std::string();
  const auto e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}

}  // namespace

int MapProjection::utmZone(double longitude) {
  double lon = std::fmod(longitude + 180.0, 360.0);
  if (lon < 0.0)
    lon += 360.0;
  const int zone = static_cast<int>(lon / 6.0) + 1;
  return zone > 60 ? 60 : zone;
}

MapProjection MapProjection::parse(const std::string &text) {
  MapProjection p;
  const std::string id = trim(text);
  const std::string u = upper(id);
  p.id_ = id;
  if (u.empty() || u == "GEOGRAPHIC" || u == "GEOGRAPHIC WGS84" || u == "EPSG:4326" || u == "WGS84") {
    p.kind_ = Kind::Geographic;
    return p;
  }
  auto utm = [&p](int zone, bool south) {
    if (zone < 1 || zone > 60)
      return;
    p.kind_ = Kind::TransverseMercator;
    p.zone_ = zone;
    p.south_ = south;
    p.lon0_ = -183.0 + 6.0 * zone;
    p.lat0_ = 0.0;
    p.k0_ = 0.9996;
    p.falseEasting_ = 500000.0;
    p.falseNorthing_ = south ? 10000000.0 : 0.0;
  };
  if (u.rfind("UTM", 0) == 0) {
    // "UTM10N", "UTM10S", "UTM10" (north), "UTM 10 N"
    std::string rest;
    for (char c : u.substr(3))
      if (c != ' ')
        rest += c;
    char *end = nullptr;
    const long zone = std::strtol(rest.c_str(), &end, 10);
    if (end == rest.c_str())
      return p;
    const std::string hemi(end);
    if (hemi.empty() || hemi == "N")
      utm(static_cast<int>(zone), false);
    else if (hemi == "S")
      utm(static_cast<int>(zone), true);
    return p;
  }
  if (u.rfind("EPSG:", 0) == 0) {
    const long code = std::strtol(u.c_str() + 5, nullptr, 10);
    if (code >= 32601 && code <= 32660)
      utm(static_cast<int>(code - 32600), false);
    else if (code >= 32701 && code <= 32760)
      utm(static_cast<int>(code - 32700), true);
    return p;
  }
  if (u.rfind("LTM", 0) == 0) {
    double lon = 0.0, lat = 0.0;
    if (std::sscanf(u.c_str() + 3, "%lf/%lf", &lon, &lat) == 2 && std::fabs(lat) < 90.0) {
      p.kind_ = Kind::TransverseMercator;
      p.lon0_ = lon;
      p.lat0_ = lat;
      p.k0_ = 1.0;
    }
    return p;
  }
  return p;
}

std::string MapProjection::name() const {
  switch (kind_) {
    case Kind::Geographic: return "WGS 84 geographic";
    case Kind::TransverseMercator:
      if (zone_ > 0)
        return "UTM zone " + std::to_string(zone_) + (south_ ? " S" : " N");
      return "Transverse Mercator " + id_;
    case Kind::Invalid: break;
  }
  return "unknown (" + id_ + ")";
}

bool MapProjection::forward(double lon, double lat, double &x, double &y) const {
  if (kind_ == Kind::Geographic) {
    x = lon;
    y = lat;
    return true;
  }
  if (kind_ != Kind::TransverseMercator || std::fabs(lat) > 89.9)
    return false;
  double dlon = lon - lon0_;
  while (dlon > 180.0)
    dlon -= 360.0;
  while (dlon < -180.0)
    dlon += 360.0;
  if (std::fabs(dlon) > 30.0)
    return false;
  const double phi = lat * kDeg;
  const double s = std::sin(phi), c = std::cos(phi), t = std::tan(phi);
  const double n = kA / std::sqrt(1.0 - kE2 * s * s);
  const double tt = t * t;
  const double cc = kEp2 * c * c;
  const double a = c * dlon * kDeg;
  const double a2 = a * a, a3 = a2 * a, a4 = a3 * a, a5 = a4 * a, a6 = a5 * a;
  const double m = meridionalArc(phi), m0 = meridionalArc(lat0_ * kDeg);
  x = falseEasting_ +
      k0_ * n * (a + (1.0 - tt + cc) * a3 / 6.0 + (5.0 - 18.0 * tt + tt * tt + 72.0 * cc - 58.0 * kEp2) * a5 / 120.0);
  y = falseNorthing_ +
      k0_ * (m - m0 +
             n * t *
                 (a2 / 2.0 + (5.0 - tt + 9.0 * cc + 4.0 * cc * cc) * a4 / 24.0 +
                  (61.0 - 58.0 * tt + tt * tt + 600.0 * cc - 330.0 * kEp2) * a6 / 720.0));
  return std::isfinite(x) && std::isfinite(y);
}

bool MapProjection::inverse(double x, double y, double &lon, double &lat) const {
  if (kind_ == Kind::Geographic) {
    lon = x;
    lat = y;
    return true;
  }
  if (kind_ != Kind::TransverseMercator)
    return false;
  const double e4 = kE2 * kE2, e6 = e4 * kE2;
  const double m = meridionalArc(lat0_ * kDeg) + (y - falseNorthing_) / k0_;
  const double mu = m / (kA * (1.0 - kE2 / 4.0 - 3.0 * e4 / 64.0 - 5.0 * e6 / 256.0));
  const double e1 = (1.0 - std::sqrt(1.0 - kE2)) / (1.0 + std::sqrt(1.0 - kE2));
  const double e12 = e1 * e1, e13 = e12 * e1, e14 = e13 * e1;
  const double phi1 = mu + (3.0 * e1 / 2.0 - 27.0 * e13 / 32.0) * std::sin(2.0 * mu) +
                      (21.0 * e12 / 16.0 - 55.0 * e14 / 32.0) * std::sin(4.0 * mu) +
                      (151.0 * e13 / 96.0) * std::sin(6.0 * mu) + (1097.0 * e14 / 512.0) * std::sin(8.0 * mu);
  const double s1 = std::sin(phi1), c1 = std::cos(phi1), t1 = std::tan(phi1);
  if (std::fabs(c1) < 1e-12)
    return false;
  const double cc1 = kEp2 * c1 * c1;
  const double tt1 = t1 * t1;
  const double n1 = kA / std::sqrt(1.0 - kE2 * s1 * s1);
  const double r1 = kA * (1.0 - kE2) / std::pow(1.0 - kE2 * s1 * s1, 1.5);
  const double d = (x - falseEasting_) / (n1 * k0_);
  const double d2 = d * d, d3 = d2 * d, d4 = d3 * d, d5 = d4 * d, d6 = d5 * d;
  const double phi =
      phi1 - (n1 * t1 / r1) *
                 (d2 / 2.0 - (5.0 + 3.0 * tt1 + 10.0 * cc1 - 4.0 * cc1 * cc1 - 9.0 * kEp2) * d4 / 24.0 +
                  (61.0 + 90.0 * tt1 + 298.0 * cc1 + 45.0 * tt1 * tt1 - 252.0 * kEp2 - 3.0 * cc1 * cc1) * d6 / 720.0);
  const double dlon =
      (d - (1.0 + 2.0 * tt1 + cc1) * d3 / 6.0 +
       (5.0 - 2.0 * cc1 + 28.0 * tt1 - 3.0 * cc1 * cc1 + 8.0 * kEp2 + 24.0 * tt1 * tt1) * d5 / 120.0) /
      c1;
  lat = phi / kDeg;
  lon = lon0_ + dlon / kDeg;
  if (lon > 180.0)
    lon -= 360.0;
  if (lon < -180.0)
    lon += 360.0;
  return std::isfinite(lat) && std::isfinite(lon) && std::fabs(lat) <= 90.0;
}

double geodesicDistance(double lon1, double lat1, double lon2, double lat2) {
  const double dlat = (lat2 - lat1) * kDeg;
  const double dlon = (lon2 - lon1) * kDeg;
  const double a = std::sin(dlat / 2) * std::sin(dlat / 2) +
                   std::cos(lat1 * kDeg) * std::cos(lat2 * kDeg) * std::sin(dlon / 2) * std::sin(dlon / 2);
  return 6371008.8 * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

}  // namespace mbstudio::platform
