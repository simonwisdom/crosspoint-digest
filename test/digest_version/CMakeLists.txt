add_executable(DigestVersionTest
  DigestVersionTest.cpp
)

target_include_directories(DigestVersionTest PRIVATE
  ${REPO_ROOT}/src
)

target_link_libraries(DigestVersionTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(DigestVersionTest)
