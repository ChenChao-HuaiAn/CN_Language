// AST 二进制序列化器（任务 1142·模块级预编译缓存第一台阶）
// 职责：把语法分析产物（Program AST）序列化为紧凑二进制缓冲，并支持无损
//   反序列化重建——供 driver 层模块缓存（driver_ast_cache）跨进程复用
//   「词法+语法」阶段结果，免去重复解析。
// 设计要点：
//   1. 写读对称：本头声明对外两个入口（serializeProgram/deserializeProgram），
//      内部 Writer/Reader 原语 + 逐节点写读函数族（44 种节点的字段一一对应）。
//   2. 字段零遗漏纪律：每节点类的**全部**字段（含语义层回填字段——缓存来自
//      解析后状态，回填字段为默认值，照存照还原，防未来消费面漂移）。
//   3. 损坏防御：Reader 全程越界检查 + 长度上限；未知节点/枚举值即失败
//      （返回 false 由调用方回退重解析——不产生默认值继续，648 纪律）。
//   4. 格式版本：CACHE_FORMAT_VERSION 随字段布局变更递增；调用方（driver）
//      把版本并入缓存键，版本不符=未命中（显式失效）。
// 单文件 <=1000 行、单函数 <=100 行约束（写/读分文件承载）。
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/model/ast.hpp"

namespace cn_compiler {
namespace ast_serial {

// 缓存格式版本：节点字段布局/编码规则变更时必须 +1。
//   （1 = 首版，任务 1142）
//   （2 = #1554：CaseLabel 增写 decValue（1137 规范十进制文本）——布局变更，
//     旧条目经版本不符自动失效回退，无迁移）
constexpr std::uint32_t CACHE_FORMAT_VERSION = 2;

// 序列化 Program AST 为二进制缓冲（out 先清空；失败返回 false 并写 error）
bool serializeProgram(const Program* program, std::vector<char>& out, std::string& error);

// 反序列化（data/size 为 serializeProgram 产物）：
//   成功返回非空 Program；任何异常（截断/未知节点类型/枚举越界/长度非法）
//   返回 nullptr 并写 error——调用方必须回退重编译（禁默认值继续）。
std::unique_ptr<Program> deserializeProgram(const char* data, std::size_t size,
                                            std::string& error);

}  // namespace ast_serial
}  // namespace cn_compiler
