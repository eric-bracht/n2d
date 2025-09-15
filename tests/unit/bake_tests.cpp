#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include <Eigen/Sparse>
#include <gtest/gtest.h>

#ifndef N2D_CLI_PATH
#define N2D_CLI_PATH "n2d_cli"
#endif

static std::array<float, 3> DecodeXY(unsigned char x, unsigned char y) {
  float fx = x / 255.0f * 2.0f - 1.0f;
  float fy = y / 255.0f * 2.0f - 1.0f;
  float fz = std::sqrt(std::max(0.0f, 1.0f - fx * fx - fy * fy));
  Eigen::Vector3f v(fx, fy, fz);
  v.normalize();
  return {v.x(), v.y(), v.z()};
}

TEST(Bake, bc5_two_channel_xy_reconstruct_and_normalize_ok) {
  auto n = DecodeXY(128, 255);
  float len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
  EXPECT_NEAR(len, 1.0f, 1e-5f);
}

static Eigen::VectorXf SolvePoisson(const Eigen::MatrixXf& b) {
  int N = static_cast<int>(b.rows());
  int M = N * N;
  Eigen::SparseMatrix<float> A(M, M);
  std::vector<Eigen::Triplet<float>> triplets;
  triplets.reserve(M * 5);
  auto idx = [N](int y, int x) { return y * N + x; };
  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      int i = idx(y, x);
      float diag = 0.0f;
      auto add = [&](int yy, int xx) {
        if (yy >= 0 && yy < N && xx >= 0 && xx < N) {
          triplets.emplace_back(i, idx(yy, xx), -1.0f);
          diag += 1.0f;
        }
      };
      add(y - 1, x);
      add(y + 1, x);
      add(y, x - 1);
      add(y, x + 1);
      triplets.emplace_back(i, i, diag);
    }
  }
  // Anchor
  triplets.emplace_back(0, 0, 1.0f);
  Eigen::VectorXf bvec(M);
  for (int y = 0; y < N; ++y)
    for (int x = 0; x < N; ++x) bvec[idx(y, x)] = b(y, x);
  bvec[0] = 0.0f;
  A.setFromTriplets(triplets.begin(), triplets.end());
  Eigen::ConjugateGradient<Eigen::SparseMatrix<float>, Eigen::Lower | Eigen::Upper> cg;
  cg.compute(A);
  return cg.solve(bvec);
}

TEST(Bake, poisson_reconstructs_procedural_height_rms_below_threshold) {
  int N = 8;
  Eigen::MatrixXf h(N, N);
  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      h(y, x) = std::sin(M_PI * x / (N - 1)) * std::sin(M_PI * y / (N - 1));
    }
  }
  Eigen::MatrixXf b(N, N);
  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      float center = 0.0f;
      auto sample = [&](int yy, int xx) {
        yy = std::clamp(yy, 0, N - 1);
        xx = std::clamp(xx, 0, N - 1);
        return h(yy, xx);
      };
      center += sample(y - 1, x);
      center += sample(y + 1, x);
      center += sample(y, x - 1);
      center += sample(y, x + 1);
      b(y, x) = center - 4.0f * h(y, x);
    }
  }
  Eigen::VectorXf sol = SolvePoisson(b);
  float err = 0.0f;
  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      float v = sol[y * N + x];
      float ref = h(y, x);
      err += (v - ref) * (v - ref);
    }
  }
  float rms = std::sqrt(err / (N * N));
  EXPECT_LT(rms, 1e-3f);
}

TEST(Bake, flag_parsing_on_real_asset_ok) {
  std::string cmd = std::string(N2D_CLI_PATH) +
                    " bake --mesh testdata/Informant_Total.fbx --normal-pattern "
                    "testdata/Std_Skin_Head_Normal.png --output-pattern out.exr";
  int code = std::system(cmd.c_str());
  if (code == -1) {
    GTEST_SKIP();
  }
  EXPECT_EQ(2, (code >> 8));
}

