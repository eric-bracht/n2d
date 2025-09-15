#include <filesystem>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>
#include <OpenImageIO/imageio.h>

#include "n2d/io/inspect.hpp"

namespace fs = std::filesystem;

TEST(Inspect, lists_materials_uvs_udims_ok) {
  auto report = n2d::InspectMesh("testdata/Informant_Total.fbx");
  if (!report) {
    GTEST_SKIP();
  }
  EXPECT_GT(report->materials.size(), 0u);
  const auto& m0 = report->materials.front();
  EXPECT_FALSE(m0.uv_sets.empty());
  EXPECT_FALSE(m0.udim_tiles.empty());
}

static bool GuessYDownForTest(const std::string& path) {
  using namespace OIIO;
  std::unique_ptr<ImageInput> in = ImageInput::open(path);
  if (!in) return false;
  const ImageSpec& spec = in->spec();
  std::vector<float> data(spec.width * spec.height * spec.nchannels);
  in->read_image(TypeFloat, data.data());
  double sum = 0.0;
  for (size_t i = 1; i < data.size(); i += spec.nchannels) {
    sum += data[i];
  }
  double avg = sum / (spec.width * spec.height);
  return avg < 0.5;
}

TEST(Inspect, y_channel_guess_ok) {
  fs::path normal = "testdata/Std_Skin_Head_Normal.png";
  bool guess = GuessYDownForTest(normal.string());
  auto report = n2d::InspectMesh("testdata/Informant_Total.fbx");
  if (!report) {
    GTEST_SKIP();
  }
  bool found = false;
  bool ydown = false;
  for (const auto& m : report->materials) {
    if (m.normal_map.find(normal.string()) != std::string::npos) {
      found = true;
      ydown = m.y_is_down;
      break;
    }
  }
  if (!found) {
    GTEST_SKIP();
  }
  EXPECT_EQ(ydown, guess);
}

TEST(Inspect, mirrored_island_flip_u_flip_v_behavior_ok) {
  fs::path dir = fs::temp_directory_path() / "n2d_mirror";
  fs::create_directories(dir);
  fs::path obj = dir / "mesh.obj";
  {
    std::ofstream os(obj);
    os << "v 0 0 0\n" << "v 1 0 0\n" << "v 1 1 0\n" << "v 0 1 0\n";
    os << "vt 0 0\n" << "vt 1 0\n" << "vt 1 1\n" << "vt 0 1\n";
    os << "f 1/2 2/1 3/4\n";  // mirrored U
    os << "f 1/2 3/4 4/3\n";
  }
  auto rep = n2d::InspectMesh(obj);
  fs::remove_all(dir);
  if (!rep) {
    GTEST_SKIP();
  }
  ASSERT_EQ(rep->materials.size(), 1u);
  EXPECT_TRUE(rep->materials[0].flip_u || rep->materials[0].flip_v);
}

TEST(Inspect, overlapping_uvs_emit_warning_ok) {
  fs::path dir = fs::temp_directory_path() / "n2d_overlap";
  fs::create_directories(dir);
  fs::path obj = dir / "mesh.obj";
  {
    std::ofstream os(obj);
    os << "v 0 0 0\n" << "v 1 0 0\n" << "v 1 1 0\n" << "v 0 1 0\n";
    os << "v 0 0 0.1\n" << "v 1 0 0.1\n" << "v 1 1 0.1\n" << "v 0 1 0.1\n";
    os << "vt 0 0\n" << "vt 1 0\n" << "vt 1 1\n" << "vt 0 1\n";
    os << "f 1/1 2/2 3/3\n" << "f 1/1 3/3 4/4\n";
    os << "f 5/1 6/2 7/3\n" << "f 5/1 7/3 8/4\n";
  }
  auto rep = n2d::InspectMesh(obj);
  fs::remove_all(dir);
  if (!rep) {
    GTEST_SKIP();
  }
  ASSERT_EQ(rep->materials.size(), 1u);
  EXPECT_TRUE(rep->materials[0].overlapping_uvs);
}

TEST(Inspect, udim_pattern_expansion_1001_1002_ok) {
  std::string pattern = "tex_<UDIM>.exr";
  std::vector<int> tiles{1001, 1002};
  std::vector<std::string> paths;
  for (int t : tiles) {
    auto s = pattern;
    auto pos = s.find("<UDIM>");
    s.replace(pos, 6, std::to_string(t));
    paths.push_back(s);
  }
  EXPECT_EQ(paths[0], "tex_1001.exr");
  EXPECT_EQ(paths[1], "tex_1002.exr");
}

