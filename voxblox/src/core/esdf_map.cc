#include "voxblox/core/esdf_map.h"

namespace voxblox {

bool EsdfMap::getDistanceAtPosition(const Eigen::Vector3d& position,
                                    double* distance) const {
  constexpr bool interpolate = true;
  FloatingPoint distance_fp;
  bool success = interpolator_.getDistance(position, &distance_fp, interpolate);
  if (success) {
    *distance = static_cast<double>(distance_fp);
  }
  return success;
}

bool EsdfMap::getDistanceAndGradientAtPosition(
    const Eigen::Vector3d& position, double* distance,
    Eigen::Vector3d* gradient) const {
  FloatingPoint distance_fp;
  Point gradient_fp = Point::Zero();

  bool success = interpolator_.getAdaptiveDistanceAndGradient(
      position, &distance_fp, &gradient_fp);

  *distance = static_cast<double>(distance_fp);
  *gradient = gradient_fp.cast<double>();

  return success;
}

}  // namespace voxblox
