#include "command_system_info.hpp"

#include <iostream>

#include <nlohmann/json.hpp>

#include "aetherq/kernels/dispatch.hpp"
#include "aetherq/measurement/environment.hpp"
#include "aetherq/runtime/version.hpp"

namespace aetherq::cli {

int command_system_info(bool json_output) {
  const EnvironmentInfo environment = capture_environment();
  const CpuFeatures features = detect_cpu_features();

  if (json_output) {
    nlohmann::json document;
    document["version"] = std::string(version());
    document["git_commit"] = std::string(git_commit());
    document["git_dirty"] = std::string(git_dirty());
    document["build_type"] = std::string(build_type());
    document["cpu_model"] = environment.cpu_model;
    document["logical_cores"] = environment.logical_cores;
    document["physical_cores"] = environment.physical_cores;
    document["l1_cache_bytes"] = environment.l1_cache_bytes;
    document["l2_cache_bytes"] = environment.l2_cache_bytes;
    document["l3_cache_bytes"] = environment.l3_cache_bytes;
    document["frequency_governor"] = environment.frequency_governor;
    document["os_name"] = environment.os_name;
    document["kernel_version"] = environment.kernel_version;
    document["compiler"] = environment.compiler;
    document["compiler_version"] = environment.compiler_version;
    document["cpu_features"]["sse42"] = features.sse42;
    document["cpu_features"]["avx2"] = features.avx2;
    document["cpu_features"]["avx512f"] = features.avx512f;
    document["cpu_features"]["neon"] = features.neon;
    document["best_backend"] = std::string(best_backend_name(features));
    std::cout << document.dump(2) << '\n';
    return 0;
  }

  std::cout << "AetherQ " << version() << " (" << build_type() << ", " << git_commit() << ")\n";
  std::cout << "CPU: " << environment.cpu_model << "\n";
  std::cout << "Cores: " << environment.logical_cores << " logical, " << environment.physical_cores
            << " physical\n";
  std::cout << "Caches: L1=" << environment.l1_cache_bytes << " L2=" << environment.l2_cache_bytes
            << " L3=" << environment.l3_cache_bytes << " bytes\n";
  std::cout << "Governor: " << environment.frequency_governor << "\n";
  std::cout << "OS: " << environment.os_name << " " << environment.kernel_version << "\n";
  std::cout << "Compiler: " << environment.compiler << " " << environment.compiler_version << "\n";
  std::cout << "Features: sse42=" << (features.sse42 ? "yes" : "no")
            << " avx2=" << (features.avx2 ? "yes" : "no")
            << " avx512f=" << (features.avx512f ? "yes" : "no")
            << " neon=" << (features.neon ? "yes" : "no") << "\n";
  std::cout << "Best backend: " << best_backend_name(features) << "\n";
  return 0;
}

}
