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

#include "cartographer/io/pcd_intensity_writing_points_processor.h"

#include <iomanip>
#include <sstream>
#include <iostream>
#include <string>
#include <algorithm>

#include "absl/memory/memory.h"
#include "cartographer/common/lua_parameter_dictionary.h"
#include "cartographer/io/points_batch.h"
#include "glog/logging.h"

namespace cartographer {
namespace io {

namespace {

// Writes the PCD header claiming 'num_points' will follow it into
// 'output_file'.
// https://pointclouds.org/documentation/tutorials/pcd_file_format.html
void WriteBinaryPcdIntensityHeader(const bool has_color, const bool has_intensity, const bool has_reflectivity,
                            const bool has_ambient, const bool has_range, const bool has_ring, const int64 num_points,
                          FileWriter* const file_writer) {
  std::cerr << "has_color " << has_color << std::endl;
  std::cerr << "has_intensity " << has_intensity << std::endl;
  std::cerr << "has_reflectivity " << has_reflectivity << std::endl;
  std::cerr << "has_ambient " << has_ambient << std::endl;
  std::cerr << "has_range " << has_range << std::endl;
  std::cerr << "has_ring " << has_ring << std::endl;


  std::string color_header_field = !has_color ? "" : " rgb";
  std::string color_header_type = !has_color ? "" : " U";
  std::string color_header_size = !has_color ? "" : " 4";
  std::string color_header_count = !has_color ? "" : " 1";

  std::string intensity_header_field = !has_intensity ? "" : " intensity";
  std::string intensity_header_type = !has_intensity ? "" : " U";
  std::string intensity_header_size = !has_intensity ? "" : " 4";
  std::string intensity_header_count = !has_intensity ? "" : " 1";

  std::string reflectivity_header_field = !has_reflectivity ? "" : " reflectivity";
  std::string reflectivity_header_type = !has_reflectivity ? "" : " U";
  std::string reflectivity_header_size = !has_reflectivity ? "" : " 2";
  std::string reflectivity_header_count = !has_reflectivity ? "" : " 1";

  std::string ambient_header_field = !has_ambient ? "" : " ambient";
  std::string ambient_header_type = !has_ambient ? "" : " U";
  std::string ambient_header_size = !has_ambient ? "" : " 4";
  std::string ambient_header_count = !has_ambient ? "" : " 1";

  std::string range_header_field = !has_range ? "" : " range";
  std::string range_header_type = !has_range ? "" : " U";
  std::string range_header_size = !has_range ? "" : " 4";
  std::string range_header_count = !has_range ? "" : " 1";

  std::string ring_header_field = !has_ring ? "" : " ring";
  std::string ring_header_type = !has_ring ? "" : " U";
  std::string ring_header_size = !has_ring ? "" : " 2";
  std::string ring_header_count = !has_ring ? "" : " 1";

  std::ostringstream stream;
  stream << "# generated by Cartographer\n"
         << "VERSION .7\n"
         << "FIELDS x y z"  << color_header_field << intensity_header_field << reflectivity_header_field << ambient_header_field << range_header_field << ring_header_field << " frame" << "\n"
         << "SIZE 4 4 4"    << color_header_size  << intensity_header_size  << reflectivity_header_size  << ambient_header_size  << range_header_size  << ring_header_size  << " 4" << "\n"
         << "TYPE F F F"    << color_header_type  << intensity_header_type  << reflectivity_header_type  << ambient_header_type  << range_header_type  << ring_header_type  << " F"  << "\n"
         << "COUNT 1 1 1"   << color_header_count << intensity_header_count << reflectivity_header_count << ambient_header_count << range_header_count << ring_header_count << " 1" << "\n"
         << "WIDTH " << std::setw(15) << std::setfill('0') << num_points << "\n"
         << "HEIGHT 1\n"
         << "VIEWPOINT 0 0 0 1 0 0 0\n"
         << "POINTS " << std::setw(15) << std::setfill('0') << num_points
         << "\n"
         << "DATA binary\n";
  const std::string out = stream.str();
  file_writer->WriteHeader(out.data(), out.size());
}

void WriteBinaryPcdIntensityPointCoordinate(const Eigen::Vector3f& point,
                                   FileWriter* const file_writer) {
//  unsigned int u_intensity = (unsigned int)intensity;
  char buffer[12];
  memcpy(buffer, &point[0], sizeof(float));
  memcpy(buffer + 4, &point[1], sizeof(float));
  memcpy(buffer + 8, &point[2], sizeof(float));
//  memcpy(buffer + 12, &u_intensity, sizeof(unsigned int));
  CHECK(file_writer->Write(buffer, 12));
}

void WriteBinaryFloat(const float& value,
                                   FileWriter* const file_writer) {
  char buffer[4];
  memcpy(buffer, &value, sizeof(float));
  CHECK(file_writer->Write(buffer, 4));
}

void WriteUInt16_fieldsize_2(const uint16_t& value,
                                   FileWriter* const file_writer) {
  char buffer[2];
  memcpy(buffer, &value, sizeof(uint16_t));
  CHECK(file_writer->Write(buffer, 2));
}

void WriteUInt8_fieldsize_2(const uint8_t& value,
                                   FileWriter* const file_writer) {
  char buffer[2];
  memcpy(buffer, &value, sizeof(uint8_t));
  CHECK(file_writer->Write(buffer, 2));
}

void WriteUInt16_fieldsize_4(const uint16_t& value,
                                   FileWriter* const file_writer) {
  char buffer[4];
  memcpy(buffer, &value, sizeof(uint16_t));
  CHECK(file_writer->Write(buffer, 4));
}

void WriteUInt32_with_fieldsize_4(const uint32_t& value,
                                   FileWriter* const file_writer) {
  char buffer[4];
  memcpy(buffer, &value, sizeof(uint32_t));
  CHECK(file_writer->Write(buffer, 4));
}

void WriteBinaryFloatAsUnsignedInt(const float& value,
                                   FileWriter* const file_writer) {
  unsigned int u_value = (unsigned int)value;
  char buffer[4];
  memcpy(buffer, &u_value, sizeof(unsigned int));
  CHECK(file_writer->Write(buffer, 4));
}

void WriteBinaryInteger(const int& value,
                                   FileWriter* const file_writer) {
  char buffer[4];
  memcpy(buffer, &value, sizeof(int));
  CHECK(file_writer->Write(buffer, 4));
}
void WriteBinaryChar(const char& value,
                                   FileWriter* const file_writer) {
  char buffer[1];
  memcpy(buffer, &value, sizeof(char));
  CHECK(file_writer->Write(buffer, 1));
}

void WriteBinaryPcdPointColor(const Uint8Color& color,
                              FileWriter* const file_writer) {
  char buffer[4];
  buffer[0] = color[2];
  buffer[1] = color[1];
  buffer[2] = color[0];
  buffer[3] = 0;
  CHECK(file_writer->Write(buffer, 4));
}

}  // namespace

std::unique_ptr<PcdIntensityWritingPointsProcessor>
PcdIntensityWritingPointsProcessor::FromDictionary(
    FileWriterFactory file_writer_factory,
    common::LuaParameterDictionary* const dictionary,
    PointsProcessor* const next) {
  return absl::make_unique<PcdIntensityWritingPointsProcessor>(
      file_writer_factory(dictionary->GetString("filename")), dictionary->GetString("export_fields"), next);
}

PcdIntensityWritingPointsProcessor::PcdIntensityWritingPointsProcessor(
    std::unique_ptr<FileWriter> file_writer, std::string export_fields, PointsProcessor* const next)
    : next_(next),
      num_points_(0),
      has_colors_(false),
      has_intensity_(false),
      has_reflectivity_(false),
      has_ambient_(false),
      has_range_(false),
      has_ring_(false),
      export_fields_(export_fields),
      file_writer_(std::move(file_writer)) {
      std::cerr << "export_fields_: " << export_fields_ << std::endl;
      if (!export_fields_.empty()) {
      export_reflectivity_ = export_fields_.find("reflectivity") != std::string::npos;
      export_ambient_ = export_fields_.find("ambient") != std::string::npos;
      export_range_ = export_fields_.find("range") != std::string::npos;
      export_ring_ = export_fields_.find("ring") != std::string::npos;
      } else {
         export_reflectivity_ = true;
         export_ambient_ = true;
         export_range_ = true;
         export_ring_ = true;
      }
      std::cerr << "export reflectivity: " << export_reflectivity_ << std::endl;
      std::cerr << "export ambient: " << export_ambient_ << std::endl;
      std::cerr << "export range: " << export_range_ << std::endl;
      std::cerr << "export ring: " << export_ring_ << std::endl;
      }

PointsProcessor::FlushResult PcdIntensityWritingPointsProcessor::Flush() {
  WriteBinaryPcdIntensityHeader(
    has_colors_,
    has_intensity_,
    has_reflectivity_ && export_reflectivity_,
    has_ambient_ && export_ambient_,
    has_range_ && export_range_,
    has_ring_ && export_ring_,
    num_points_,
    file_writer_.get()
  );
  CHECK(file_writer_->Close());

  switch (next_->Flush()) {
    case FlushResult::kFinished:
      return FlushResult::kFinished;

    case FlushResult::kRestartStream:
      LOG(FATAL) << "PCD generation must be configured to occur after any "
                    "stages that require multiple passes.";
  }
  LOG(FATAL);
}

void PcdIntensityWritingPointsProcessor::Process(std::unique_ptr<PointsBatch> batch) {
  if (batch->points.empty()) {
    next_->Process(std::move(batch));
    return;
  }

  auto it = std::find(registered_frame_ids_.begin(), registered_frame_ids_.end(), batch->frame_id);
  if (it == registered_frame_ids_.end()) {
    registered_frame_ids_.push_back(batch->frame_id);
    it = std::find(registered_frame_ids_.begin(), registered_frame_ids_.end(), batch->frame_id);
  }
  float internal_frame_id = static_cast<float>(it - registered_frame_ids_.begin());

  if (num_points_ == 0) {
    has_colors_ = !batch->colors.empty();
    has_intensity_ = !batch->intensities.empty();
    has_reflectivity_ = !batch->reflectivities.empty();
    has_ambient_ = !batch->ambients.empty();
    has_range_ = !batch->ranges.empty();
    has_ring_ = !batch->rings.empty();
    WriteBinaryPcdIntensityHeader(
       has_colors_,
       has_intensity_,
       has_reflectivity_ && export_reflectivity_,
       has_ambient_ && export_ambient_,
       has_range_ && export_range_,
       has_ring_ && export_ring_,
       0,
       file_writer_.get());
  }
//<< color_header_field << intensity_header_field << reflectivity_header_field << ambient_header_field << range_header_field << ring_header_field
  for (size_t i = 0; i < batch->points.size(); ++i) {
    WriteBinaryPcdIntensityPointCoordinate(batch->points[i].position,
                                  file_writer_.get());
    if (!batch->colors.empty()) {
      WriteBinaryPcdPointColor(ToUint8Color(batch->colors[i]),
                               file_writer_.get());
    }
    if (!batch->intensities.empty()) {
      WriteBinaryFloatAsUnsignedInt(batch->intensities[i], file_writer_.get());
    }
    if (!batch->reflectivities.empty() && export_reflectivity_) {
      uint16_t reflectivity_value = (uint16_t)batch->reflectivities[i];
      WriteUInt16_fieldsize_2(reflectivity_value, file_writer_.get());
    }
    if (!batch->ambients.empty() && export_ambient_) {
      uint16_t ambient_value = (uint16_t)batch->ambients[i];
      WriteUInt16_fieldsize_4(ambient_value, file_writer_.get());
    }
    if (!batch->ranges.empty() && export_range_) {
      uint32_t range_value = (uint32_t)batch->ranges[i];
      std::cerr << "###" << std::endl;
      if (i < 10) {
        std::cerr << "range " << i << ": " << range_value << std::endl;
      }
      WriteUInt32_with_fieldsize_4(range_value, file_writer_.get());
    }
    if (!batch->rings.empty() && export_ring_) {
      uint8_t ring_value = (uint8_t)batch->rings[i];
      WriteBinaryFloat(ring_value, file_writer_.get());
    }
    // write the internal frame_id of the given view
//    WriteBinaryChar(internal_frame_id, file_writer_.get());
   // std::cout << "[" << batch->frame_id << "] " << "internal frame id: " << internal_frame_id << " as integer: " << (int)internal_frame_id << " as float: " << (float)internal_frame_id << " frame_id: " << batch->frame_id << std::endl;
    WriteBinaryFloat((float)internal_frame_id, file_writer_.get());
 //    std::cout << "############################################################" << std::endl;
    ++num_points_;
  }
  next_->Process(std::move(batch));
}

}  // namespace io
}  // namespace cartographer
