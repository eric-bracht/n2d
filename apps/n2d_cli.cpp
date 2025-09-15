#include <filesystem>
#include <iostream>
#include <string>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "n2d/api.hpp"
#include "n2d/io/inspect.hpp"

int n2d_main(int argc, char** argv) {
  CLI::App app{"normal2disp"};
  bool deterministic = false;
  app.add_flag("--deterministic", deterministic, "Enable deterministic behaviour");
  bool interactive = false;
  app.add_flag("--interactive", interactive, "Enable interactive prompts");

  app.add_flag_callback("--version", []() {
    std::cout << "n2d 0.1.0" << std::endl;
    std::exit(0);
  });

  // Inspect subcommand
  auto* inspect = app.add_subcommand("inspect", "Inspect mesh and normal maps");
  std::string mesh_path;
  std::string inspect_json;
  inspect->add_option("--mesh", mesh_path, "Path to mesh") ->required();
  inspect->add_option("--inspect-json", inspect_json, "Write inspection report to JSON");
  inspect->callback([&]() {
    auto report = n2d::InspectMesh(mesh_path);
    if (!report) {
      spdlog::error(report.error().message);
      std::exit(1);
    }
    for (const auto& mat : report->materials) {
      std::cout << "material " << mat.index << ": " << mat.name << std::endl;
      std::cout << "  uv_sets: ";
      for (const auto& uv : mat.uv_sets) std::cout << uv << ' ';
      std::cout << std::endl;
      std::cout << "  udims: ";
      for (int t : mat.udim_tiles) std::cout << t << ' ';
      std::cout << std::endl;
      if (!mat.normal_map.empty()) {
        std::cout << "  normal_map: " << mat.normal_map << std::endl;
        std::cout << "  y_is_down: " << (mat.y_is_down ? "true" : "false") << std::endl;
      }
      if (mat.overlapping_uvs) {
        std::cout << "  warning: overlapping UVs detected" << std::endl;
      }
    }
    if (!inspect_json.empty()) {
      auto w = n2d::WriteInspectJson(*report, inspect_json);
      if (!w) {
        spdlog::error(w.error().message);
        std::exit(1);
      }
    }
  });

  // Bake subcommand (stub)
  auto* bake = app.add_subcommand("bake", "Bake displacement from normal maps");
  n2d::BakeParams params;
  bake->add_option("--mesh", params.mesh_path, "Path to mesh")->required();
  bake->add_option("--material", params.material_selector, "Material name or index");
  bake->add_option("--uv-set", params.uv_set, "UV set name or index");
  bake->add_option("--normal-pattern", params.normal_pattern, "Normal map pattern")
      ->required();
  bake->add_option("--output-pattern", params.output_pattern, "Output pattern")
      ->required();
  bake->add_flag("--y-is-down", params.y_is_down, "Normals use +Y down convention");
  bake->add_flag("--export-sidecars", params.export_sidecars, "Export sidecar files");
  bake->add_option("--cache-dir", params.cache_dir, "Cache directory");
  bake->add_option("--threads", params.threads, "Thread count");
  bake->add_option("--amplitude-mm", params.amplitude_mm, "Amplitude in millimeters");
  bake->add_option("--max-slope", params.max_slope, "Maximum slope");
  std::string norm_mode;
  bake->add_option("--normalization", norm_mode, "Normalization mode");
  bake->add_flag("--deterministic", params.deterministic, "Deterministic execution");
  bake->add_option("--debug-dumps-dir", params.debug_dumps_dir,
                   "Directory for debug dumps");
  bake->callback([&]() {
    (void)params;
    std::cout << "bake not yet implemented; try 'inspect' or run with --help" << std::endl;
    std::exit(2);
  });

  try {
    app.require_subcommand();
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }
  return 0;
}

int main(int argc, char** argv) { return n2d_main(argc, argv); }

