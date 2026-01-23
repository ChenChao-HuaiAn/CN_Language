# CMake generated Testfile for 
# Source directory: C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration
# Build directory: C:/Users/ChenChao/Documents/gitcode/CN_Language/build/tests/integration
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(integration_parse_success_test "C:/Users/ChenChao/Documents/gitcode/CN_Language/build/tests/integration/integration_parse_success_test.exe")
set_tests_properties(integration_parse_success_test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/CMakeLists.txt;22;add_test;C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/CMakeLists.txt;0;")
add_test(integration_parse_failure_test "C:/Users/ChenChao/Documents/gitcode/CN_Language/build/tests/integration/integration_parse_failure_test.exe")
set_tests_properties(integration_parse_failure_test PROPERTIES  WORKING_DIRECTORY "C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration" _BACKTRACE_TRIPLES "C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/CMakeLists.txt;35;add_test;C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/CMakeLists.txt;0;")
add_test(integration_semantic_error_test "C:/Users/ChenChao/Documents/gitcode/CN_Language/build/tests/integration/integration_semantic_error_test.exe")
set_tests_properties(integration_semantic_error_test PROPERTIES  WORKING_DIRECTORY "C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration" _BACKTRACE_TRIPLES "C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/CMakeLists.txt;49;add_test;C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/CMakeLists.txt;0;")
add_test(integration_full_frontend_test "C:/Users/ChenChao/Documents/gitcode/CN_Language/build/tests/integration/integration_full_frontend_test.exe")
set_tests_properties(integration_full_frontend_test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/CMakeLists.txt;63;add_test;C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/CMakeLists.txt;0;")
add_test(integration_compile_full_test "C:/Users/ChenChao/Documents/gitcode/CN_Language/build/tests/integration/integration_compile_full_test.exe" "C:/Users/ChenChao/Documents/gitcode/CN_Language/build/src/cnc.exe")
set_tests_properties(integration_compile_full_test PROPERTIES  WORKING_DIRECTORY "C:/Users/ChenChao/Documents/gitcode/CN_Language/build/tests/integration" _BACKTRACE_TRIPLES "C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/CMakeLists.txt;70;add_test;C:/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/CMakeLists.txt;0;")
