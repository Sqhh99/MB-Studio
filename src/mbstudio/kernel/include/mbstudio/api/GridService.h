// MB-Studio service interface - gridded bathymetry (GMT netCDF grids) and
// their rendering to colour shaded relief images.
#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace mbstudio {

struct Grid {
  int nx = 0;
  int ny = 0;
  double xmin = 0.0, xmax = 0.0, ymin = 0.0, ymax = 0.0;  // node coordinates
  double zmin = 0.0, zmax = 0.0;
  bool geographic = false;
  // Map projection of a projected grid as MB-System names it in the grid
  // remark ("UTM10N", "EPSG:32610", "LTM-122.5/36.8"), empty for geographic
  // grids or when the file does not say (see platform::MapProjection).
  std::string projection;
  std::string title;
  std::string xName = "x", yName = "y", zName = "z", zUnits;
  // nx * ny values, row major; row 0 is the northernmost (ymax) row.
  // Missing values are NaN.
  std::vector<float> z;

  bool valid() const { return nx > 1 && ny > 1 && z.size() == static_cast<size_t>(nx) * ny; }
  float at(int ix, int iy) const { return z[static_cast<size_t>(iy) * nx + ix]; }
  double dx() const { return nx > 1 ? (xmax - xmin) / (nx - 1) : 0.0; }
  double dy() const { return ny > 1 ? (ymax - ymin) / (ny - 1) : 0.0; }
  double xOf(int ix) const { return xmin + ix * dx(); }
  double yOf(int iy) const { return ymax - iy * dy(); }
};

struct GridLoadResult {
  bool ok = false;
  std::string error;
  Grid grid;
};

struct GridStatistics {
  long long nodes = 0;
  long long valid = 0;     // finite values
  long long empty = 0;     // NaN (no data)
  double emptyRatio = 0.0; // empty / nodes
  double mean = 0.0;
  double stddev = 0.0;
};

class IGridService {
 public:
  static constexpr const char *kServiceId = "mbsystem.grids";
  virtual ~IGridService() = default;
  virtual GridLoadResult load(const std::string &path) const = 0;
  virtual GridStatistics statistics(const Grid &grid) const = 0;
};

enum class ColorMap { Haxby = 0, Bathymetry, Rainbow, Grayscale };

struct RenderOptions {
  ColorMap colorMap = ColorMap::Haxby;
  bool hillshade = true;
  double azimuth = 315.0;    // degrees clockwise from north
  double elevation = 45.0;   // degrees above the horizon
  double exaggeration = 1.0; // vertical exaggeration for shading
  bool autoRange = true;
  double zmin = 0.0, zmax = 0.0;
  std::uint32_t noDataColor = 0x00000000;  // ARGB, transparent
};

// 32-bit ARGB pixels (0xAARRGGBB), row major, row 0 at the top.
struct Image {
  int width = 0;
  int height = 0;
  std::vector<std::uint32_t> pixels;
};

class IGridRenderService {
 public:
  static constexpr const char *kServiceId = "mbsystem.render";
  virtual ~IGridRenderService() = default;
  virtual std::vector<std::string> colorMapNames() const = 0;
  virtual Image render(const Grid &grid, const RenderOptions &options) const = 0;
  // Colour of a normalised value t in [0, 1] (for legends).
  virtual std::uint32_t color(ColorMap map, double t) const = 0;
};

}  // namespace mbstudio
