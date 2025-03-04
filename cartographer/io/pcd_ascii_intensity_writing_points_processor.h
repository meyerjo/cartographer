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

#include <fstream>
#include <vector>
#include <string>

#include "cartographer/common/lua_parameter_dictionary.h"
#include "cartographer/io/file_writer.h"
#include "cartographer/io/points_processor.h"

namespace cartographer {
namespace io {

// Streams a PCD file to disk. The header is written in 'Flush'.
class PcdAsciiIntensityWritingPointsProcessor : public PointsProcessor {
 public:
  constexpr static const char* kConfigurationFileActionName = "write_pcd_ascii_intensity";
  PcdAsciiIntensityWritingPointsProcessor(std::unique_ptr<FileWriter> file_writer, std::string export_fields,
                            PointsProcessor* next);

  static std::unique_ptr<PcdAsciiIntensityWritingPointsProcessor> FromDictionary(
      FileWriterFactory file_writer_factory,
      common::LuaParameterDictionary* dictionary, PointsProcessor* next);

  ~PcdAsciiIntensityWritingPointsProcessor() override {}

  PcdAsciiIntensityWritingPointsProcessor(const PcdAsciiIntensityWritingPointsProcessor&) = delete;
  PcdAsciiIntensityWritingPointsProcessor& operator=(const PcdAsciiIntensityWritingPointsProcessor&) =
      delete;

  void Process(std::unique_ptr<PointsBatch> batch) override;
  FlushResult Flush() override;

 private:
  PointsProcessor* const next_;

  int64 num_points_;
  bool has_colors_;
  bool has_intensity_;
  bool has_reflectivity_;
  bool has_ambient_;
  bool has_range_;
  bool has_ring_;
  bool has_classification_;

  bool export_reflectivity_;
  bool export_ambient_;
  bool export_range_;
  bool export_ring_;
  bool export_classification_;

  std::string export_fields_;
  std::vector<std::string> registered_frame_ids_;
  std::unique_ptr<FileWriter> file_writer_;
};

}  // namespace io
}  // namespace cartographer
