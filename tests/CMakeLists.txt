find_package(GTest CONFIG REQUIRED)

add_executable(n2d_tests
  unit/inspect_tests.cpp
  unit/bake_tests.cpp
)

target_link_libraries(n2d_tests PRIVATE n2d GTest::gtest_main)

target_compile_definitions(n2d_tests PRIVATE N2D_CLI_PATH="$<TARGET_FILE:n2d_cli>")

if(MSVC)
  target_compile_options(n2d_tests PRIVATE /W4 /WX)
else()
  target_compile_options(n2d_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

include(GoogleTest)
gtest_discover_tests(n2d_tests)
