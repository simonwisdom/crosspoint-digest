add_executable(DigestScheduleTest
  DigestScheduleTest.cpp
)

target_include_directories(DigestScheduleTest PRIVATE
  ${REPO_ROOT}/src
)

target_link_libraries(DigestScheduleTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(DigestScheduleTest)
