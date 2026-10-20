cmake_minimum_required(VERSION 3.20)
project(ArcanaWorkerTests LANGUAGES CXX)
enable_testing()
set(MariaDBRoot "" CACHE PATH "MariaDB client development package root")
add_executable(worker_tests worker_tests.cpp
    ../projectMagicWar/DBWorker.cpp ../projectMagicWar/DBWorkerInstance.cpp)
target_compile_features(worker_tests PRIVATE cxx_std_14)
target_include_directories(worker_tests PRIVATE "${MariaDBRoot}/include")
target_link_directories(worker_tests PRIVATE "${MariaDBRoot}/lib")
target_link_libraries(worker_tests PRIVATE libmariadb ws2_32)
# Assertions are part of the regression tests even in Release.
target_compile_options(worker_tests PRIVATE /UNDEBUG)
add_test(NAME worker_regressions COMMAND worker_tests)
set_tests_properties(worker_regressions PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${MariaDBRoot}/lib;PATH=path_list_prepend:${MariaDBRoot}/bin")
