#include "SceneRenderer.h"

#include <algorithm>
#include <limits>

namespace mbstudio::ui::scene {

namespace {

constexpr double kDeg = 3.14159265358979323846 / 180.0;

inline std::uint32_t channel(std::uint32_t c, int shift) { return (c >> shift) & 0xFFu; }

}  // namespace

View::View(const Camera &camera, int width, int height, double sceneRadius) {
  const double yaw = camera.yaw * kDeg;
  const double pitch = std::clamp(camera.pitch, -89.9, 89.9) * kDeg;
  // camera position relative to the target
  const Vec3 offset{std::sin(yaw) * std::cos(pitch), -std::cos(yaw) * std::cos(pitch), std::sin(pitch)};
  forward_ = normalized(offset * -1.0);
  right_ = Vec3{std::cos(yaw), std::sin(yaw), 0.0};
  up_ = normalized(cross(right_, forward_));
  cx_ = 0.5 * width;
  cy_ = 0.5 * height;
  perspective_ = camera.perspective;
  const double half = std::tan(std::clamp(camera.fov, 5.0, 120.0) * 0.5 * kDeg);
  const double distance = std::max(camera.distance, 1e-6);
  focal_ = 0.5 * std::max(1, height) / half;
  if (perspective_) {
    eye_ = camera.target + offset * distance;
    near_ = distance * 1e-4;
  } else {
    // the same framing of the target as the perspective view; the eye
    // stays outside the scene so that every depth is positive
    scale_ = 0.5 * std::max(1, height) / (distance * half);
    eye_ = camera.target + offset * (distance + 4.0 * std::max(sceneRadius, distance));
    near_ = -std::numeric_limits<double>::infinity();
  }
}

bool View::project(const Vec3 &world, double &sx, double &sy, double &depth) const {
  const Vec3 v = world - eye_;
  const double xc = dot(v, right_), yc = dot(v, up_), zc = dot(v, forward_);
  depth = zc;
  if (perspective_) {
    if (zc <= near_)
      return false;
    sx = cx_ + focal_ * xc / zc;
    sy = cy_ - focal_ * yc / zc;
  } else {
    sx = cx_ + scale_ * xc;
    sy = cy_ - scale_ * yc;
  }
  return std::isfinite(sx) && std::isfinite(sy);
}

Vec3 View::unproject(double sx, double sy, double depth) const {
  double xc, yc;
  if (perspective_) {
    xc = (sx - cx_) * depth / focal_;
    yc = -(sy - cy_) * depth / focal_;
  } else {
    xc = (sx - cx_) / scale_;
    yc = -(sy - cy_) / scale_;
  }
  return eye_ + right_ * xc + up_ * yc + forward_ * depth;
}

double View::pixelsPerUnit(double depth) const { return perspective_ ? focal_ / std::max(depth, 1e-9) : scale_; }

void Frame::resize(int w, int h) {
  w = std::max(1, w);
  h = std::max(1, h);
  if (w == width && h == height)
    return;
  width = w;
  height = h;
  const std::size_t n = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
  color.assign(n, 0);
  depth.assign(n, std::numeric_limits<float>::infinity());
  id.assign(n, -1);
}

void Frame::clear(std::uint32_t background) {
  std::fill(color.begin(), color.end(), background);
  std::fill(depth.begin(), depth.end(), std::numeric_limits<float>::infinity());
  std::fill(id.begin(), id.end(), -1);
}

void drawTriangle(Frame &frame, const double sx[3], const double sy[3], const double depth[3],
                  const std::uint32_t color[3], std::int32_t id) {
  const double minX = std::min({sx[0], sx[1], sx[2]}), maxX = std::max({sx[0], sx[1], sx[2]});
  const double minY = std::min({sy[0], sy[1], sy[2]}), maxY = std::max({sy[0], sy[1], sy[2]});
  if (maxX < 0.0 || maxY < 0.0 || minX >= frame.width || minY >= frame.height)
    return;
  const double area = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (sy[1] - sy[0]);
  if (std::fabs(area) < 1e-12)
    return;
  const int x0 = std::max(0, static_cast<int>(std::floor(minX)));
  const int x1 = std::min(frame.width - 1, static_cast<int>(std::ceil(maxX)));
  const int y0 = std::max(0, static_cast<int>(std::floor(minY)));
  const int y1 = std::min(frame.height - 1, static_cast<int>(std::ceil(maxY)));
  const double inv = 1.0 / area;
  const bool flat = color[0] == color[1] && color[1] == color[2];
  // edge functions, evaluated at pixel centres
  for (int y = y0; y <= y1; y++) {
    const double py = y + 0.5;
    std::size_t index = static_cast<std::size_t>(y) * frame.width + x0;
    for (int x = x0; x <= x1; x++, index++) {
      const double px = x + 0.5;
      const double w0 = ((sx[1] - px) * (sy[2] - py) - (sx[2] - px) * (sy[1] - py)) * inv;
      const double w1 = ((sx[2] - px) * (sy[0] - py) - (sx[0] - px) * (sy[2] - py)) * inv;
      const double w2 = 1.0 - w0 - w1;
      constexpr double eps = -1e-7;
      if (w0 < eps || w1 < eps || w2 < eps)
        continue;
      const float z = static_cast<float>(w0 * depth[0] + w1 * depth[1] + w2 * depth[2]);
      if (!(z < frame.depth[index]))
        continue;
      frame.depth[index] = z;
      frame.id[index] = id;
      if (flat) {
        frame.color[index] = color[0];
        continue;
      }
      auto lerp = [&](int shift) {
        return static_cast<std::uint32_t>(std::clamp(
            w0 * channel(color[0], shift) + w1 * channel(color[1], shift) + w2 * channel(color[2], shift) + 0.5, 0.0,
            255.0));
      };
      frame.color[index] = 0xFF000000u | (lerp(16) << 16) | (lerp(8) << 8) | lerp(0);
    }
  }
}

void drawPoint(Frame &frame, double sx, double sy, double depth, std::uint32_t color, int size, std::int32_t id) {
  if (!(sx > -size && sy > -size && sx < frame.width + size && sy < frame.height + size))
    return;
  const int half = std::max(1, size) / 2;
  const int cx = static_cast<int>(std::floor(sx)), cy = static_cast<int>(std::floor(sy));
  const float z = static_cast<float>(depth);
  const int x0 = std::max(0, cx - half), x1 = std::min(frame.width - 1, cx - half + std::max(1, size) - 1);
  const int y0 = std::max(0, cy - half), y1 = std::min(frame.height - 1, cy - half + std::max(1, size) - 1);
  for (int y = y0; y <= y1; y++) {
    std::size_t index = static_cast<std::size_t>(y) * frame.width + x0;
    for (int x = x0; x <= x1; x++, index++) {
      if (!(z < frame.depth[index]))
        continue;
      frame.depth[index] = z;
      frame.color[index] = color;
      frame.id[index] = id;
    }
  }
}

std::uint32_t shade(std::uint32_t color, const Vec3 &normal, const Vec3 &light, double ambient) {
  const double lambert = std::max(0.0, dot(normal, light));
  const double f = ambient + (1.0 - ambient) * lambert;
  auto c = [&](int shift) {
    return static_cast<std::uint32_t>(std::clamp(channel(color, shift) * f + 0.5, 0.0, 255.0));
  };
  return 0xFF000000u | (c(16) << 16) | (c(8) << 8) | c(0);
}

std::uint32_t mix(std::uint32_t a, std::uint32_t b, double t) {
  auto c = [&](int shift) {
    return static_cast<std::uint32_t>(
        std::clamp(channel(a, shift) * (1.0 - t) + channel(b, shift) * t + 0.5, 0.0, 255.0));
  };
  return 0xFF000000u | (c(16) << 16) | (c(8) << 8) | c(0);
}

Vec3 lightDirection(double azimuth, double elevation) {
  const double az = azimuth * kDeg, el = elevation * kDeg;
  // towards the light: from the azimuth, above the horizon
  return normalized(Vec3{std::sin(az) * std::cos(el), std::cos(az) * std::cos(el), std::sin(el)});
}

double niceExaggeration(double horizontalExtent, double verticalRange) {
  if (!(horizontalExtent > 0.0) || !(verticalRange > 0.0))
    return 1.0;
  // relief of about a fifth of the horizontal extent
  const double wanted = 0.2 * horizontalExtent / verticalRange;
  if (wanted <= 1.0)
    return 1.0;
  const double steps[] = {1, 2, 3, 5, 10, 20, 30, 50, 100};
  double best = 1.0;
  for (const double s : steps)
    if (s <= wanted)
      best = s;
  return best;
}

}  // namespace mbstudio::ui::scene
