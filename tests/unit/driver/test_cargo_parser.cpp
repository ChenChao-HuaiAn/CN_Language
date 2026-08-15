// 货舱.toml 解析器单元测试（模块系统 v2.0 第 5 层，规格书09-二）
// 覆盖：完整配置 / 缺段（无 [依赖]）/ 注释（整行+行尾）/ 多依赖 / 空文件 / 错误路径。
#include <gtest/gtest.h>
#include <string>

#include "cn_compiler/driver/cargo_parser.hpp"

using cn_compiler::driver::CargoConfig;
using cn_compiler::driver::parseCargoToml;

namespace {

// 解析并断言成功（error 应为空）
CargoConfig parseOk(const std::string& text) {
    CargoConfig config;
    std::string error;
    EXPECT_TRUE(parseCargoToml(text, config, error)) << "解析失败: " << error;
    EXPECT_TRUE(error.empty());
    return config;
}

} // namespace

// 完整配置：名称/版本 + 核心 内置 + 网络库 本地版本
TEST(CargoParserTest, ParseFullConfig) {
    const std::string toml =
        "[货舱]\n"
        "名称 = \"我的程序\"\n"
        "版本 = \"0.1.0\"\n"
        "\n"
        "[依赖]\n"
        "核心 = \"内置\"\n"
        "网络库 = \"1.2\"\n";
    const CargoConfig config = parseOk(toml);
    EXPECT_EQ(config.name, "我的程序");
    EXPECT_EQ(config.version, "0.1.0");
    ASSERT_EQ(config.deps.size(), 2u);
    EXPECT_EQ(config.deps[0].name, "核心");
    EXPECT_EQ(config.deps[0].version, "内置");
    EXPECT_EQ(config.deps[1].name, "网络库");
    EXPECT_EQ(config.deps[1].version, "1.2");
}

// 缺 [依赖] 节：仅 [货舱]（可选节缺失正常）
TEST(CargoParserTest, ParseCargoOnly) {
    const CargoConfig config = parseOk("[货舱]\n名称 = \"最小程序\"\n版本 = \"1.0.0\"\n");
    EXPECT_EQ(config.name, "最小程序");
    EXPECT_EQ(config.version, "1.0.0");
    EXPECT_TRUE(config.deps.empty());
}

// 注释：整行注释 + 行尾注释（字符串内的 # 不应被当作注释）
TEST(CargoParserTest, ParseComments) {
    const std::string toml =
        "# 货舱.toml 包元数据与依赖\n"
        "[货舱]  # 节注释\n"
        "名称 = \"我的程序\"\n"
        "版本 = \"0.1.0\"  # 行尾注释\n"
        "\n"
        "[依赖]\n"
        "核心 = \"内置\"        # 内置核心包\n"
        "工具库 = \"0.3\"\n";
    const CargoConfig config = parseOk(toml);
    EXPECT_EQ(config.name, "我的程序");
    EXPECT_EQ(config.version, "0.1.0");
    ASSERT_EQ(config.deps.size(), 2u);
    EXPECT_EQ(config.deps[0].name, "核心");
    EXPECT_EQ(config.deps[0].version, "内置");
    EXPECT_EQ(config.deps[1].name, "工具库");
    EXPECT_EQ(config.deps[1].version, "0.3");
}

// 多依赖：三个依赖按行顺序保留
TEST(CargoParserTest, ParseMultipleDeps) {
    const std::string toml =
        "[货舱]\n名称 = \"多依赖\"\n版本 = \"2.0.0\"\n"
        "[依赖]\n"
        "核心 = \"内置\"\n"
        "网络库 = \"1.2\"\n"
        "工具库 = \"0.3\"\n";
    const CargoConfig config = parseOk(toml);
    ASSERT_EQ(config.deps.size(), 3u);
    EXPECT_EQ(config.deps[0].name, "核心");
    EXPECT_EQ(config.deps[1].name, "网络库");
    EXPECT_EQ(config.deps[2].name, "工具库");
}

// 空文件 / 纯注释：解析成功，配置为空（自动发现场景允许）
TEST(CargoParserTest, ParseEmptyFile) {
    const CargoConfig config = parseOk("");
    EXPECT_TRUE(config.name.empty());
    EXPECT_TRUE(config.version.empty());
    EXPECT_TRUE(config.deps.empty());
}

// 纯注释文件：同样为空配置
TEST(CargoParserTest, ParseCommentOnlyFile) {
    const CargoConfig config = parseOk("# 只有注释\n# 第二行\n");
    EXPECT_TRUE(config.deps.empty());
}

// findDependency：按名查找命中/未命中
TEST(CargoParserTest, FindDependency) {
    const CargoConfig config = parseOk(
        "[货舱]\n名称 = \"x\"\n版本 = \"1\"\n"
        "[依赖]\n核心 = \"内置\"\n网络库 = \"1.2\"\n");
    const cn_compiler::driver::CargoDependency* core = config.findDependency("核心");
    ASSERT_NE(core, nullptr);
    EXPECT_EQ(core->version, "内置");
    const cn_compiler::driver::CargoDependency* net = config.findDependency("网络库");
    ASSERT_NE(net, nullptr);
    EXPECT_EQ(net->version, "1.2");
    EXPECT_EQ(config.findDependency("不存在"), nullptr);
}

// 错误路径：缺右引号 -> 解析失败且 error 非空
TEST(CargoParserTest, ParseMissingQuoteFails) {
    CargoConfig config;
    std::string error;
    EXPECT_FALSE(parseCargoToml("[依赖]\n网络库 = \"1.2\n", config, error));
    EXPECT_FALSE(error.empty());
}

// 错误路径：值非字符串 -> 解析失败
TEST(CargoParserTest, ParseNonStringValueFails) {
    CargoConfig config;
    std::string error;
    EXPECT_FALSE(parseCargoToml("[货舱]\n名称 = 123\n", config, error));
    EXPECT_FALSE(error.empty());
}

// 宽容扩展：未知节/未知键忽略，不影响已识别配置
TEST(CargoParserTest, ParseUnknownSectionIgnored) {
    const std::string toml =
        "[货舱]\n名称 = \"程序\"\n版本 = \"0.1.0\"\n"
        "[构建]\n优化 = \"true\"\n未知键 = \"x\"\n"  // 未知节整体忽略
        "[依赖]\n核心 = \"内置\"\n";
    const CargoConfig config = parseOk(toml);
    EXPECT_EQ(config.name, "程序");
    ASSERT_EQ(config.deps.size(), 1u);  // 只有 [依赖] 节中的 核心
    EXPECT_EQ(config.deps[0].name, "核心");
}
