#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <tl/expected.hpp>

#include "n2d/core/error.hpp"

namespace n2d {

struct MaterialInfo {
  int index = 0;
  std::string name;
  std::vector<std::string> uv_sets;
  std::vector<int> udim_tiles;
  std::string normal_map;
  bool y_is_down = false;
  bool flip_u = false;
  bool flip_v = false;
  bool overlapping_uvs = false;
};

struct InspectReport {
  std::vector<MaterialInfo> materials;
};

tl::expected<InspectReport, N2DError> InspectMesh(const std::filesystem::path& mesh_path);
tl::expected<void, N2DError> WriteInspectJson(const InspectReport& report,
                                              const std::filesystem::path& json_path);

}  // namespace n2d

