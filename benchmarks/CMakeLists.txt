add_executable(bench_micro micro/bench_micro.cpp)
target_link_libraries(bench_micro PRIVATE aetherq benchmark::benchmark aetherq_warnings)

add_executable(bench_end_to_end end_to_end/bench_end_to_end.cpp)
target_link_libraries(bench_end_to_end PRIVATE aetherq benchmark::benchmark aetherq_warnings)

add_executable(bench_baselines baselines/bench_baselines.cpp)
target_link_libraries(bench_baselines PRIVATE aetherq benchmark::benchmark Eigen3::Eigen aetherq_warnings)
get_target_property(aetherq_eigen_includes Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
if(aetherq_eigen_includes)
  set_target_properties(Eigen3::Eigen PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${aetherq_eigen_includes}")
endif()
set_source_files_properties(baselines/bench_baselines.cpp PROPERTIES
  COMPILE_OPTIONS "-Wno-null-dereference")
