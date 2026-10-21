// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Minimal stand-in for Bazel's C++ runfiles library (see
// utils/pyshim/bazel_tools/tools/python/runfiles for the Python twin).
// `Rlocation("coralnpu_hw/<path>")` is looked up in each directory of
// $CORALNPU_RUNFILES (colon separated) and then in $CORALNPU_ROOT.
#ifndef CORALNPU_CPPSHIM_TOOLS_CPP_RUNFILES_RUNFILES_H_
#define CORALNPU_CPPSHIM_TOOLS_CPP_RUNFILES_RUNFILES_H_

#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace bazel {
namespace tools {
namespace cpp {
namespace runfiles {

class Runfiles {
 public:
  static Runfiles* Create(const std::string& /*argv0*/, std::string* /*error*/ = nullptr) {
    return new Runfiles();
  }
  static Runfiles* CreateForTest(std::string* /*error*/ = nullptr) { return new Runfiles(); }

  // Returns the first existing candidate, else the path under the source tree
  // (like a directory-based runfiles tree, callers probe for existence).
  std::string Rlocation(const std::string& path) const {
    std::vector<std::string> candidates = {path};
    auto slash = path.find('/');
    if (slash != std::string::npos) candidates.push_back(path.substr(slash + 1));
    std::vector<std::string> roots;
    if (const char* env = std::getenv("CORALNPU_RUNFILES")) {
      std::stringstream ss(env);
      std::string item;
      while (std::getline(ss, item, ':'))
        if (!item.empty()) roots.push_back(item);
    }
    const char* root_env = std::getenv("CORALNPU_ROOT");
    const std::string root = root_env ? root_env : ".";
    roots.push_back(root);
    for (const auto& r : roots)
      for (const auto& c : candidates)
        if (std::ifstream(r + "/" + c).good()) return r + "/" + c;
    return root + "/" + candidates.back();
  }
};

}  // namespace runfiles
}  // namespace cpp
}  // namespace tools
}  // namespace bazel

#endif  // CORALNPU_CPPSHIM_TOOLS_CPP_RUNFILES_RUNFILES_H_
