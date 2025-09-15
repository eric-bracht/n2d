#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <tl/expected.hpp>

#include "n2d/core/error.hpp"

namespace n2d {

enum class NormalizationMode { Auto, XYZ, XY, None };

struct BakeParams {
  std::filesystem::path mesh_path;
  std::string material_selector;      // name or index string
  std::string uv_set;                 // name or index string
  std::string normal_pattern;         // supports <UDIM>
  std::string output_pattern;         // supports <UDIM>
  bool y_is_down = false;             // +Y default
  bool export_sidecars = false;
  std::optional<std::filesystem::path> cache_dir;
  int threads = static_cast<int>(std::thread::hardware_concurrency());
  float amplitude_mm = 1.0f;
  float max_slope = 10.0f;
  NormalizationMode normalization = NormalizationMode::Auto;
  bool deterministic = false;
  std::optional<std::filesystem::path> debug_dumps_dir;
};

struct Result {
  std::vector<std::filesystem::path> outputs;  // EXR paths
  std::vector<std::string> log_lines;
};

tl::expected<Result, N2DError> Bake(const BakeParams& p);

}  // namespace n2d

