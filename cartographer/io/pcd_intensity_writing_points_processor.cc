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
  std::string color_header_field = !has_color ? "" : " rgb";
  std::string color_header_type = !has_color ? "" : " U";
  std::string color_header_size = !has_color ? "" : " 4";
  std::string color_header_count = !has_color ? "" : " 1";

  std::string intensity_header_field = !has_intensity ? "" : " intensity";
  std::string intensity_header_type = !has_intensity ? "" : " U";
  std::string intensity_header_size = !has_intensity ? "" : " 4";
  std::string intensity_header_count = !has_intensity ? "" : " 1";

  std::string reflectivity_header_field = !has_reflectivity ? "" : " reflectivity";
  std::string reflectivity_header_type = !has_reflectivity ? "" : " F";
  std::string reflectivity_header_size = !has_reflectivity ? "" : " 4";
  std::string reflectivity_header_count = !has_reflectivity ? "" : " 1";

  std::string ambient_header_field = !has_ambient ? "" : " ambient";
  std::string ambient_header_type = !has_ambient ? "" : " F";
  std::string ambient_header_size = !has_ambient ? "" : " 4";
  std::string ambient_header_count = !has_ambient ? "" : " 1";

  std::string range_header_field = !has_range ? "" : " range";
  std::string range_header_type = !has_range ? "" : " F";
  std::string range_header_size = !has_range ? "" : " 4";
  std::string range_header_count = !has_range ? "" : " 1";

  std::string ring_header_field = !has_ring ? "" : " ring";
  std::string ring_header_type = !has_ring ? "" : " F";
  std::string ring_header_size = !has_ring ? "" : " 4";
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
      file_writer_factory(dictionary->GetString("filename")), next);
}

PcdIntensityWritingPointsProcessor::PcdIntensityWritingPointsProcessor(
    std::unique_ptr<FileWriter> file_writer, PointsProcessor* const next)
    : next_(next),
      num_points_(0),
      has_colors_(false),
      has_intensity_(false),
      has_reflectivity_(false),
      has_ambient_(false),
      has_range_(false),
      has_ring_(false),
      file_writer_(std::move(file_writer)) {}

PointsProcessor::FlushResult PcdIntensityWritingPointsProcessor::Flush() {
  WriteBinaryPcdIntensityHeader(has_colors_, has_intensity_, has_reflectivity_, has_ambient_,
                               has_range_, has_ring_, num_points_, file_writer_.get());
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
  char internal_frame_id = (char)(it - registered_frame_ids_.begin());

  if (num_points_ == 0) {
    has_colors_ = !batch->colors.empty();
    has_intensity_ = !batch->intensities.empty();
    has_reflectivity_ = !batch->reflectivities.empty();
    has_ambient_ = !batch->ambients.empty();
    has_range_ = !batch->ranges.empty();
    has_ring_ = !batch->rings.empty();
    WriteBinaryPcdIntensityHeader(has_colors_, has_intensity_, has_reflectivity_, has_ambient_, has_range_, has_ring_, 0, file_writer_.get());
  }

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
    if (!batch->reflectivities.empty()) {
      std::cout << "[" << batch->frame_id << "] " << "reflectivity: " << batch->reflectivities[i] << " as float: " << (float)batch->reflectivities[i] << " frame_id": << batch->frame_id << std::endl;
//      WriteBinaryInteger(batch->reflectivities[i], file_writer_.get());
      WriteBinaryFloat((float)batch->reflectivities[i], file_writer_.get());
    }
    if (!batch->ambients.empty()) {
//      WriteBinaryInteger(batch->ambients[i], file_writer_.get());
      std::cout << "[" << batch->frame_id << "] " << "ambients: " << batch->ambients[i] << " as float: " << (float)batch->ambients[i] << " frame_id": << batch->frame_id << std::endl;
      WriteBinaryFloat((float)batch->ambients[i], file_writer_.get());
    }
    if (!batch->ranges.empty()) {
//      WriteBinaryInteger(batch->ranges[i], file_writer_.get());
      std::cout << "[" << batch->frame_id << "] " << "ranges: " << batch->ranges[i] << " as float: " << (float)batch->ranges[i] << " frame_id": << batch->frame_id << std::endl;
      WriteBinaryFloat((float)batch->ranges[i], file_writer_.get());
    }
    if (!batch->rings.empty()) {
//      WriteBinaryInteger(batch->rings[i], file_writer_.get());
      std::cout << "[" << batch->frame_id << "] " << "rings: " << batch->rings[i] << " as float: " << (float)batch->rings[i] << " frame_id": << batch->frame_id << std::endl;
      WriteBinaryFloat((float)batch->rings[i], file_writer_.get());
    }
    // write the internal frame_id of the given view
//    WriteBinaryChar(internal_frame_id, file_writer_.get());
    std::cout << "[" << batch->frame_id << "] " << "internal frame id: " << internal_frame_id << " as integer: " << (int)internal_frame_id << " as float: " << (float)internal_frame_id << " frame_id": << batch->frame_id << std::endl;
    WriteBinaryFloat((float)internal_frame_id, file_writer_.get());

    ++num_points_;
  }
  next_->Process(std::move(batch));
}

}  // namespace io
}  // namespace cartographer
