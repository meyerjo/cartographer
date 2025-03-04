/*
 * Copyright 2016 The Cartographer Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cartographer/transform/transform_interpolation_buffer.h"

#include <algorithm>

#include "Eigen/Core"
#include "Eigen/Geometry"
#include "cartographer/transform/transform.h"
#include "glog/logging.h"

namespace cartographer {
namespace transform {

TransformInterpolationBuffer::TransformInterpolationBuffer(
    const mapping::proto::Trajectory& trajectory) {
    // TODO: check if we can skip stuff here
  for (const mapping::proto::Trajectory::Node& node : trajectory.node()) {
    Push(common::FromUniversal(node.timestamp()),
         transform::ToRigid3(node.pose()));
  }
}

TransformInterpolationBuffer::TransformInterpolationBuffer(
    const mapping::proto::Trajectory& trajectory, int64 timestamp_threshold) {
    // TODO: check if we can skip stuff here
  int kept = 0, dropped = 0;
  int64_t minimum_value = std::numeric_limits<int64_t>::max(), maximum_value = std::numeric_limits<int64_t>::min();
  for (const mapping::proto::Trajectory::Node& node : trajectory.node()) {
    if (node.timestamp() < minimum_value) {
        minimum_value = node.timestamp();
    }
    if (node.timestamp() > maximum_value) {
        maximum_value = node.timestamp();
    }

    if (node.timestamp() > timestamp_threshold) {
        dropped++;
        continue;
    }
    kept++;
    Push(common::FromUniversal(node.timestamp()),
         transform::ToRigid3(node.pose()));
  }
  std::cerr << "kept= " << kept << ", dropped= " << dropped << std::endl;
  std::cerr << "minimum timestamp: " << minimum_value << ", maximum timestamp: " << maximum_value << " threshold: " << timestamp_threshold << std::endl;
}

void TransformInterpolationBuffer::Push(const common::Time time,
                                        const transform::Rigid3d& transform) {
  if (!timestamped_transforms_.empty()) {
    CHECK_GE(time, latest_time()) << "New transform is older than latest.";
  }
  timestamped_transforms_.push_back(TimestampedTransform{time, transform});
  RemoveOldTransformsIfNeeded();
}

void TransformInterpolationBuffer::SetSizeLimit(
    const size_t buffer_size_limit) {
  buffer_size_limit_ = buffer_size_limit;
  RemoveOldTransformsIfNeeded();
}

void TransformInterpolationBuffer::Clear() { timestamped_transforms_.clear(); }

bool TransformInterpolationBuffer::Has(const common::Time time) const {
  if (timestamped_transforms_.empty()) {
    return false;
  }
  return earliest_time() <= time && time <= latest_time();
}

transform::Rigid3d TransformInterpolationBuffer::Lookup(
    const common::Time time) const {
  CHECK(Has(time)) << "Missing transform for: " << time;
  const auto end = std::lower_bound(
      timestamped_transforms_.begin(), timestamped_transforms_.end(), time,
      [](const TimestampedTransform& timestamped_transform,
         const common::Time time) {
        return timestamped_transform.time < time;
      });
  if (end->time == time) {
    return end->transform;
  }
  const auto start = std::prev(end);
  return Interpolate(*start, *end, time).transform;
}

void TransformInterpolationBuffer::RemoveOldTransformsIfNeeded() {
  while (timestamped_transforms_.size() > buffer_size_limit_) {
    timestamped_transforms_.pop_front();
  }
}

common::Time TransformInterpolationBuffer::earliest_time() const {
  CHECK(!empty()) << "Empty buffer.";
  return timestamped_transforms_.front().time;
}

common::Time TransformInterpolationBuffer::latest_time() const {
  CHECK(!empty()) << "Empty buffer.";
  return timestamped_transforms_.back().time;
}

bool TransformInterpolationBuffer::empty() const {
  return timestamped_transforms_.empty();
}

size_t TransformInterpolationBuffer::size_limit() const {
  return buffer_size_limit_;
}

size_t TransformInterpolationBuffer::size() const {
  return timestamped_transforms_.size();
}

}  // namespace transform
}  // namespace cartographer
