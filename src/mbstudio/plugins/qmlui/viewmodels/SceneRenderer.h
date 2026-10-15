// MB-Studio QML UI - a small software renderer for the 3D views (terrain
// surface and sounding point clouds).
//
// It draws Gouraud shaded triangles and square points with a depth buffer
// and an identifier buffer, so that every pixel can be traced back to the
// grid cell or sounding it shows (picking without guessing from colours).
// It works with every Qt Quick scene graph backend, including the software
// backend of the automated tests, and needs no GPU or shader tool chain;
// grids are decimated for interaction and drawn at full resolution of the
// level of detail when the view is still.
//
// World coordinates: x east, y north, z up, in metres.
#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

namespace mbstudio::ui::scene {

struct Vec3 {
  double x = 0.0, y = 0.0, z = 0.0;
};
inline Vec3 operator+(const Vec3 &a, const Vec3 &b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3 &a, const Vec3 &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(const Vec3 &a, double s) { return {a.x * s, a.y * s, a.z * s}; }
inline double dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(const Vec3 &a, const Vec3 &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double length(const Vec3 &a) { return std::sqrt(dot(a, a)); }
inline Vec3 normalized(const Vec3 &a) {
  const double l = length(a);
  return l > 0.0 ? a * (1.0 / l) : Vec3{0.0, 0.0, 1.0};
}

// An orbit camera looking at a target.
struct Camera {
  Vec3 target;
  double yaw = 20.0;      // degrees: 0 views from the south (looking north), 90 from the east
  double pitch = 35.0;    // degrees above the horizontal plane; 90 looks straight down
  double distance = 1.0;  // from the target (also sets the scale of orthographic views)
  bool perspective = true;
  double fov = 40.0;      // vertical field of view, degrees
};

// The transformation of a camera for a viewport of width x height pixels.
class View {
 public:
  View(const Camera &camera, int width, int height, double sceneRadius);
  // Screen position (pixels, y down) and depth along the viewing direction;
  // false behind the camera.
  bool project(const Vec3 &world, double &sx, double &sy, double &depth) const;
  // The world point on the viewing ray through a pixel at a depth.
  Vec3 unproject(double sx, double sy, double depth) const;
  // Screen pixels per world unit at a depth (scale bars, point sizes).
  double pixelsPerUnit(double depth) const;
  const Vec3 &right() const { return right_; }
  const Vec3 &up() const { return up_; }
  const Vec3 &forward() const { return forward_; }
  const Vec3 &eye() const { return eye_; }

 private:
  Vec3 eye_, right_, up_, forward_;
  double cx_ = 0.0, cy_ = 0.0;
  double focal_ = 1.0;  // perspective: pixels at unit depth
  double scale_ = 1.0;  // orthographic: pixels per world unit
  bool perspective_ = true;
  double near_ = 1e-3;
};

// Colour (0xAARRGGBB), depth and identifier per pixel.
struct Frame {
  int width = 0, height = 0;
  std::vector<std::uint32_t> color;
  std::vector<float> depth;
  std::vector<std::int32_t> id;
  void resize(int w, int h);
  void clear(std::uint32_t background);
  bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
};

// A Gouraud shaded triangle with depth test (no back face culling: a
// surface is seen from above and from below).
void drawTriangle(Frame &frame, const double sx[3], const double sy[3], const double depth[3],
                  const std::uint32_t color[3], std::int32_t id);
// A square point of size pixels centred on (sx, sy), with depth test.
void drawPoint(Frame &frame, double sx, double sy, double depth, std::uint32_t color, int size, std::int32_t id);

// Lambert shading of a colour for a unit normal and light direction.
std::uint32_t shade(std::uint32_t color, const Vec3 &normal, const Vec3 &light, double ambient = 0.35);
// Mix two colours (t = 0: a, 1: b).
std::uint32_t mix(std::uint32_t a, std::uint32_t b, double t);
// A light coming from an azimuth (degrees clockwise from north) and elevation.
Vec3 lightDirection(double azimuth, double elevation);
// A "nice" vertical exaggeration so that relief is visible: 1, 2, 3, 5, 10, ...
double niceExaggeration(double horizontalExtent, double verticalRange);

}  // namespace mbstudio::ui::scene
