#pragma once

#include <string>

namespace n2d {

enum class ErrorCode {
  IoError,
  MeshParseError,
  InvalidAsset,
  InvalidArgs,
  IncompatibleTextures,
  AmbiguousInput,
  SolverFailed,
  UserCancelled
};

struct N2DError {
  ErrorCode code;
  std::string message;
};

}  // namespace n2d

