# Look for Catch2 (you can also vendor it instead)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2 # Or latest stable
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit_tests test_dummy.cpp)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)

# Enable discovery so you can run tests in IDEs or with ctest
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
