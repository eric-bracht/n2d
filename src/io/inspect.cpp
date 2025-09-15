#include "n2d/io/inspect.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <unordered_set>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <nlohmann/json.hpp>
#include <OpenImageIO/imageio.h>

namespace n2d {
namespace {

bool GuessYDown(const std::string& path) {
  using namespace OIIO;
  std::unique_ptr<ImageInput> in = ImageInput::open(path);
  if (!in) {
    return false;
  }
  const ImageSpec& spec = in->spec();
  std::vector<float> pixels(spec.width * spec.height * spec.nchannels);
  in->read_image(TypeFloat, pixels.data());
  double sum = 0.0;
  for (size_t i = 1; i < pixels.size(); i += spec.nchannels) {
    sum += pixels[i];
  }
  double avg = sum / (spec.width * spec.height);
  return avg < 0.5;
}

struct OrientationCounts {
  int flip_u = 0;
  int flip_v = 0;
  int total = 0;
};

void AccumulateOrientation(const aiMesh* mesh, OrientationCounts& counts) {
  if (!mesh->HasTextureCoords(0)) {
    return;
  }
  for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
    const aiFace& face = mesh->mFaces[f];
    if (face.mNumIndices != 3) continue;
    const aiVector3D& p0 = mesh->mVertices[face.mIndices[0]];
    const aiVector3D& p1 = mesh->mVertices[face.mIndices[1]];
    const aiVector3D& p2 = mesh->mVertices[face.mIndices[2]];
    const aiVector3D& uv0 = mesh->mTextureCoords[0][face.mIndices[0]];
    const aiVector3D& uv1 = mesh->mTextureCoords[0][face.mIndices[1]];
    const aiVector3D& uv2 = mesh->mTextureCoords[0][face.mIndices[2]];

    aiVector3D e1 = p1 - p0;
    aiVector3D e2 = p2 - p0;
    aiVector3D n = e1 ^ e2;

    aiVector3D duv1 = uv1 - uv0;
    aiVector3D duv2 = uv2 - uv0;
    float r = duv1.x * duv2.y - duv1.y * duv2.x;
    if (std::abs(r) < 1e-8f) {
      continue;
    }
    float inv = 1.0f / r;
    aiVector3D t = (e1 * duv2.y - e2 * duv1.y) * inv;
    aiVector3D b = (e2 * duv1.x - e1 * duv2.x) * inv;

    aiVector3D cross_tb = t ^ b;
    bool mirrored = cross_tb * n < 0;
    if (mirrored) {
      if ((-t ^ b) * n > 0) {
        counts.flip_u++;
      }
      if ((t ^ -b) * n > 0) {
        counts.flip_v++;
      }
    }
    counts.total++;
  }
}

}  // namespace

tl::expected<InspectReport, N2DError> InspectMesh(const std::filesystem::path& mesh_path) {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(mesh_path.string(),
                                          aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);
  if (!scene) {
    return tl::unexpected(N2DError{ErrorCode::MeshParseError, importer.GetErrorString()});
  }

  InspectReport report;
  for (unsigned m = 0; m < scene->mNumMaterials; ++m) {
    MaterialInfo info;
    info.index = static_cast<int>(m);
    aiString name;
    if (scene->mMaterials[m]->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
      info.name = name.C_Str();
    }

    aiString tex;
    if (scene->mMaterials[m]->GetTexture(aiTextureType_NORMALS, 0, &tex) == AI_SUCCESS ||
        scene->mMaterials[m]->GetTexture(aiTextureType_HEIGHT, 0, &tex) == AI_SUCCESS) {
      info.normal_map = tex.C_Str();
      info.y_is_down = GuessYDown(info.normal_map);
    }

    std::unordered_set<int> tiles;
    std::unordered_set<uint64_t> seen_cells;
    OrientationCounts counts;
    for (unsigned mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx) {
      const aiMesh* mesh = scene->mMeshes[mesh_idx];
      if (mesh->mMaterialIndex != m) continue;
      for (unsigned uv = 0; uv < mesh->GetNumUVChannels(); ++uv) {
        if (info.uv_sets.size() <= uv) {
          info.uv_sets.resize(uv + 1);
          info.uv_sets[uv] = "UV" + std::to_string(uv);
        }
      }
      if (!mesh->HasTextureCoords(0)) continue;
      AccumulateOrientation(mesh, counts);
      for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        if (face.mNumIndices != 3) continue;
        float u_sum = 0.0f;
        float v_sum = 0.0f;
        for (int k = 0; k < 3; ++k) {
          const aiVector3D& uv = mesh->mTextureCoords[0][face.mIndices[k]];
          int tile_u = static_cast<int>(std::floor(uv.x));
          int tile_v = static_cast<int>(std::floor(uv.y));
          int tile = tile_u + tile_v * 10 + 1001;
          tiles.insert(tile);
          u_sum += uv.x;
          v_sum += uv.y;
        }
        const int grid = 1024;
        int cu = static_cast<int>(std::floor((u_sum / 3.0f) * grid));
        int cv = static_cast<int>(std::floor((v_sum / 3.0f) * grid));
        uint64_t key = (static_cast<uint64_t>(cu) << 32) ^ static_cast<uint32_t>(cv);
        if (!seen_cells.insert(key).second) {
          info.overlapping_uvs = true;
        }
      }
    }
    info.udim_tiles.assign(tiles.begin(), tiles.end());
    std::sort(info.udim_tiles.begin(), info.udim_tiles.end());
    if (counts.total > 0) {
      info.flip_u = counts.flip_u > counts.total / 2;
      info.flip_v = counts.flip_v > counts.total / 2;
    }
    report.materials.push_back(std::move(info));
  }
  return report;
}

tl::expected<void, N2DError> WriteInspectJson(const InspectReport& report,
                                              const std::filesystem::path& json_path) {
  nlohmann::json j;
  for (const auto& mat : report.materials) {
    nlohmann::json jm;
    jm["index"] = mat.index;
    jm["name"] = mat.name;
    jm["uv_sets"] = mat.uv_sets;
    jm["udim_tiles"] = mat.udim_tiles;
    jm["normal_map"] = mat.normal_map;
    jm["y_is_down"] = mat.y_is_down;
    jm["flip_u"] = mat.flip_u;
    jm["flip_v"] = mat.flip_v;
    jm["overlapping_uvs"] = mat.overlapping_uvs;
    j["materials"].push_back(jm);
  }
  std::ofstream os(json_path);
  if (!os) {
    return tl::unexpected(N2DError{ErrorCode::IoError, "failed to write inspect json"});
  }
  os << j.dump(2);
  return {};
}

}  // namespace n2d
