# 单元测试目标在此定义，由顶层 enable_testing() 统一管理

add_executable(sample_unit_test
    sample_unit_test.c
)

add_test(NAME sample_unit_test
         COMMAND sample_unit_test)
