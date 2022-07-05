/*
 * Copyright 2017 The Cartographer Authors
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

#ifndef CARTOGRAPHER_IO_COLOR_H_
#define CARTOGRAPHER_IO_COLOR_H_

#include <array>

#include "cartographer/common/math.h"
#include "cartographer/common/port.h"

namespace cartographer {
namespace io {

using Uint8Color = std::array<uint8, 3>;
using FloatColor = std::array<float, 3>;
using Uint8ColorWithAlpha = std::array<uint8, 4>;
using FloatColorWithAlpha = std::array<float, 4>;

// A function for on-demand generation of a color palette, with every two
// direct successors having large contrast.
FloatColor GetColor(int id);

inline uint8 FloatComponentToUint8(float c) {
  return static_cast<uint8>(cartographer::common::RoundToInt(
      cartographer::common::Clamp(c, 0.f, 1.f) * 255));
}

inline float Uint8ComponentToFloat(uint8 c) { return c / 255.f; }

inline Uint8Color ToUint8Color(const FloatColor& color) {
  return {{FloatComponentToUint8(color[0]), FloatComponentToUint8(color[1]),
           FloatComponentToUint8(color[2])}};
}

inline FloatColor ToFloatColor(const Uint8Color& color) {
  return {{Uint8ComponentToFloat(color[0]), Uint8ComponentToFloat(color[1]),
           Uint8ComponentToFloat(color[2])}};
}

inline Uint8ColorWithAlpha ToUint8ColorWithAlpha(const FloatColorWithAlpha& color) {
  return {{FloatComponentToUint8(color[0]), FloatComponentToUint8(color[1]),
           FloatComponentToUint8(color[2]), FloatComponentToUint8(color[3])}};
}

inline FloatColorWithAlpha ToFloatColorWithAlpha(const Uint8ColorWithAlpha& color) {
  return {{Uint8ComponentToFloat(color[0]), Uint8ComponentToFloat(color[1]),
           Uint8ComponentToFloat(color[2]), Uint8ComponentToFloat(color[3])}};
}

inline Uint8ColorWithAlpha ToUint8AppendAlphaChannel(const Uint8Color& color) {
  return {{color[0], color[1], color[2], 255}};
}

inline FloatColorWithAlpha ToUint8AppendAlphaChannel(const FloatColor& color) {
  return {{color[0], color[1], color[2], 1.f}};
}

}  // namespace io
}  // namespace cartographer

#endif  // CARTOGRAPHER_IO_COLOR_H_
