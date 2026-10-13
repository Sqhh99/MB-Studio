// Colour shaded relief rendering of bathymetry grids.
#include <algorithm>
#include <cmath>

#include "MbioServices.h"

namespace mbstudio::mbsystem {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kMetersPerDegree = 111319.49;

struct Stop {
  double t;
  int r, g, b;
};

// GMT "haxby" colour table (the MB-System default for bathymetry).
const Stop kHaxby[] = {{0.00, 10, 0, 121},   {0.10, 40, 0, 150},   {0.20, 20, 5, 175},   {0.30, 0, 10, 200},
                       {0.40, 0, 25, 212},   {0.50, 0, 40, 224},   {0.55, 26, 102, 240}, {0.60, 13, 129, 248},
                       {0.65, 25, 175, 255}, {0.70, 50, 190, 255}, {0.75, 68, 202, 255}, {0.80, 97, 225, 240},
                       {0.84, 106, 235, 225}, {0.87, 124, 235, 200}, {0.90, 138, 236, 174}, {0.93, 172, 245, 168},
                       {0.95, 205, 255, 162}, {0.97, 223, 245, 141}, {0.98, 240, 236, 121}, {0.99, 247, 215, 104},
                       {1.00, 255, 189, 87}};
// Deep blue to pale cyan, for bathymetry-only scenes.
const Stop kBathymetry[] = {{0.0, 8, 29, 88},     {0.25, 37, 52, 148},  {0.5, 34, 94, 168},
                            {0.75, 65, 182, 196}, {0.9, 161, 218, 180}, {1.0, 237, 248, 177}};
const Stop kGrayscale[] = {{0.0, 20, 20, 20}, {1.0, 245, 245, 245}};

template <size_t N>
std::uint32_t interpolate(const Stop (&stops)[N], double t) {
  t = std::clamp(t, 0.0, 1.0);
  for (size_t i = 1; i < N; i++) {
    if (t <= stops[i].t) {
      const Stop &a = stops[i - 1];
      const Stop &b = stops[i];
      const double f = b.t > a.t ? (t - a.t) / (b.t - a.t) : 0.0;
      const int r = static_cast<int>(std::lround(a.r + f * (b.r - a.r)));
      const int g = static_cast<int>(std::lround(a.g + f * (b.g - a.g)));
      const int bl = static_cast<int>(std::lround(a.b + f * (b.b - a.b)));
      return 0xFF000000u | (static_cast<std::uint32_t>(r) << 16) | (static_cast<std::uint32_t>(g) << 8) |
             static_cast<std::uint32_t>(bl);
    }
  }
  const Stop &l = stops[N - 1];
  return 0xFF000000u | (static_cast<std::uint32_t>(l.r) << 16) | (static_cast<std::uint32_t>(l.g) << 8) |
         static_cast<std::uint32_t>(l.b);
}

std::uint32_t rainbow(double t) {
  // Hue from blue (240 degrees) to red (0 degrees), full saturation.
  t = std::clamp(t, 0.0, 1.0);
  const double h = (1.0 - t) * 240.0 / 60.0;
  const int i = static_cast<int>(std::floor(h));
  const double f = h - i;
  double r = 0, g = 0, b = 0;
  switch (i) {
    case 0: r = 1; g = f; b = 0; break;
    case 1: r = 1 - f; g = 1; b = 0; break;
    case 2: r = 0; g = 1; b = f; break;
    case 3: r = 0; g = 1 - f; b = 1; break;
    default: r = 0; g = 0; b = 1; break;
  }
  return 0xFF000000u | (static_cast<std::uint32_t>(std::lround(r * 255)) << 16) |
         (static_cast<std::uint32_t>(std::lround(g * 255)) << 8) | static_cast<std::uint32_t>(std::lround(b * 255));
}

std::uint32_t shade(std::uint32_t argb, double intensity) {
  const double k = std::clamp(intensity, 0.0, 1.25);
  auto channel = [k](std::uint32_t c) {
    return static_cast<std::uint32_t>(std::clamp(static_cast<double>(c) * k, 0.0, 255.0));
  };
  return (argb & 0xFF000000u) | (channel((argb >> 16) & 0xFF) << 16) | (channel((argb >> 8) & 0xFF) << 8) |
         channel(argb & 0xFF);
}

}  // namespace

std::vector<std::string> GridRenderService::colorMapNames() const {
  return {"Haxby", "Bathymetry", "Rainbow", "Grayscale"};
}

std::uint32_t GridRenderService::color(ColorMap map, double t) const {
  switch (map) {
    case ColorMap::Haxby: return interpolate(kHaxby, t);
    case ColorMap::Bathymetry: return interpolate(kBathymetry, t);
    case ColorMap::Rainbow: return rainbow(t);
    case ColorMap::Grayscale: return interpolate(kGrayscale, t);
  }
  return interpolate(kHaxby, t);
}

Image GridRenderService::render(const Grid &grid, const RenderOptions &options) const {
  Image image;
  if (!grid.valid())
    return image;
  image.width = grid.nx;
  image.height = grid.ny;
  image.pixels.assign(static_cast<size_t>(grid.nx) * grid.ny, options.noDataColor);

  double zmin = options.autoRange ? grid.zmin : options.zmin;
  double zmax = options.autoRange ? grid.zmax : options.zmax;
  if (!(zmax > zmin))
    zmax = zmin + 1.0;

  // Grid spacing in metres for the slope computation.
  const double midLat = 0.5 * (grid.ymin + grid.ymax);
  double dxm = grid.dx();
  double dym = grid.dy();
  if (grid.geographic) {
    dxm *= kMetersPerDegree * std::max(0.01, std::cos(midLat * kPi / 180.0));
    dym *= kMetersPerDegree;
  }
  if (dxm <= 0.0)
    dxm = 1.0;
  if (dym <= 0.0)
    dym = 1.0;

  const double azimuth = options.azimuth * kPi / 180.0;
  const double zenith = (90.0 - options.elevation) * kPi / 180.0;
  const double exaggeration = options.exaggeration > 0.0 ? options.exaggeration : 1.0;

  auto value = [&](int ix, int iy, float fallback) {
    ix = std::clamp(ix, 0, grid.nx - 1);
    iy = std::clamp(iy, 0, grid.ny - 1);
    const float v = grid.at(ix, iy);
    return std::isnan(v) ? fallback : v;
  };

  for (int iy = 0; iy < grid.ny; iy++) {
    for (int ix = 0; ix < grid.nx; ix++) {
      const float z = grid.at(ix, iy);
      if (std::isnan(z))
        continue;
      std::uint32_t c = color(options.colorMap, (z - zmin) / (zmax - zmin));
      if (options.hillshade) {
        // Central differences; y increases to the north (row 0 is north).
        const double dzdx = (value(ix + 1, iy, z) - value(ix - 1, iy, z)) / (2.0 * dxm) * exaggeration;
        const double dzdy = (value(ix, iy - 1, z) - value(ix, iy + 1, z)) / (2.0 * dym) * exaggeration;
        const double slope = std::atan(std::sqrt(dzdx * dzdx + dzdy * dzdy));
        // Aspect measured clockwise from north, pointing downhill.
        const double aspect = std::atan2(-dzdx, -dzdy);
        double intensity = std::cos(zenith) * std::cos(slope) +
                           std::sin(zenith) * std::sin(slope) * std::cos(azimuth - aspect);
        intensity = std::clamp(intensity, 0.0, 1.0);
        // Normalise so that flat ground keeps its colour.
        const double flat = std::cos(zenith);
        c = shade(c, 0.35 + 0.65 * (flat > 0.0 ? intensity / flat : intensity));
      }
      image.pixels[static_cast<size_t>(iy) * grid.nx + ix] = c;
    }
  }
  return image;
}

}  // namespace mbstudio::mbsystem
