// 1143（2026-10-19）：函数名分桶索引——resolveOverload / hasFunctionName /
//   hasCurrentModuleDefinition / funcFirstSigKey 四查询的共享加速设施。
//   背景（主排 perf 采样·宿主编译 v2 全树）：四查询逐次全表扫描 functions_
//   （每次函数调用 × 全表签名条目），每条目现场 find('#')/find('$')/substr
//   构造 3~5 个临时 std::string——memchr/find 16% + 分配族 21% 的主要来源。
//   方案：functions_ 只增不减（全树零 erase/clear 核验——写入皆为新键插入或
//   同键覆盖）且单次编译单实例 ⇒ 以 size 变化为重建判据：size 变化时按
//   functions_ 遍历顺序整表构建一次桶索引（O(条目数)·每 key 一次性剥离），
//   查询按 base 名直达同名额（O(同名额)）；size 不变期间零重建、零临时串。
//   桶内条目顺序 = 构建时 functions_ 遍历顺序 = 该时刻原全表扫描的同名子序列
//   顺序 ⇒ 决议遍历顺序/歧义诊断文本与原实现逐字节一致。
//   剥离规则精确复刻原实现（overloadEntryMatch / hasFunctionName 家族）：
//   两形态（查询名含/不含 '$'）——仅「含 '$' 且无 '#'」的 key 两形态不同。
#pragma once
#include <cstddef>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cn_compiler/model/symbols.hpp"

namespace cn_compiler {

// 单条 functions_ 条目的预解析视图（string_view 指向 key 存储）——functions_
//   节点指针/rehash 稳定、key 恒 const 不变（覆盖赋值=false 键不变，新键插入=
//   新节点）⇒ 视图在索引下次重建前恒有效。
struct FuncEntryView {
    const std::pair<const std::string, FunctionInfo>* entry = nullptr;  // functions_ 节点
    // 查询名不含 '$' 形态的 base：'$' 前缀剥离条件 =「含 '$' 且（无 '#' 或
    //   '$' 在 '#' 前）」；剥后取到 '#' 前（无 '#' 取到串尾）。
    std::string_view baseNoDollar;
    // 查询名含 '$' 形态的 base：'$' 前缀剥离条件 =「含 '$' 且含 '#' 且
    //   '$' 在 '#' 前」（原 hasFunctionName 第四层口径——泛型实例名
    //   排序$整32 无 '#' 不剥，保持整键匹配泛型调用）。
    std::string_view baseWithDollar;
    // 形态 A 剥出的模块名前缀（空=未剥——原实现以空串表达"未剥"并回退
    //   info.moduleName，且"剥出空模块名"与"未剥"行为一致，故无需区分）。
    std::string_view moduleNoDollar;
    // 形态 B 剥出的模块名前缀（空=未剥）。
    std::string_view moduleWithDollar;
};

// 函数名分桶索引：builtSize = 构建时 functions_.size()；size 相等 ⇒ 无插入
//   （functions_ 无删除路径）⇒ 索引与表一致。views 用 deque 持有稳定地址
//   （unordered_map 扩容会使自身元素搬家，deque 追加则地址恒定）。
struct FuncNameIndex {
    // 未构建哨兵（functions_.size() 永不为 SIZE_MAX，首查必重建）
    std::size_t builtSize = static_cast<std::size_t>(-1);
    std::deque<FuncEntryView> views;
    std::unordered_map<std::string_view, std::vector<const FuncEntryView*>> byBase;
    std::unordered_map<std::string_view, std::vector<const FuncEntryView*>> byBaseDollar;
};

// 1155（2026-10-19）：结构体/枚举/类名查询索引——isStructType/findStruct/
//   isEnumType/findEnum/findClass 五查询的共享加速设施。
//   背景（perf 语义面自身耗时 top：isStructType 18% / findClass 12% /
//   findStruct 7%）：三查询逐次线性扫描 program_->structs/enums/classes_，
//   每条目现场 std::string 比较/substr 构造临时串；classes_ 含全部泛型实例
//   （向量$整32 …），裸名未命中当前模块时全表回退代价随实例数线性增长。
//   方案（与 FuncNameIndex 同构）：三表只增不减（structs 唯一运行期插入=
//   error_analysis 合成体降级、classes_=注册/泛型实例化、enums=解析期合并），
//   以三表 size 变化为重建判据惰性整表构建；查询 O(1) 且零临时串。
//   首命中语义：索引按容器遍历顺序 emplace（先到先得），与原循环「首个匹配
//   即返回」逐字节一致（classes_ 裸名回退/结构体/枚举向量同序）。
//   string_view 键指向容器内恒定存储：unordered_map 节点 rehash 不搬家、
//   StructDecl/EnumDecl 对象经 unique_ptr 间接持有（vector 扩容只搬指针）。
struct TypeLookupIndex {
    const Program* builtProgram = nullptr;                      // 换程序即重建
    std::size_t structsSize = static_cast<std::size_t>(-1);     // 未构建哨兵
    std::size_t enumsSize = static_cast<std::size_t>(-1);
    std::size_t classesSize = static_cast<std::size_t>(-1);
    std::unordered_map<std::string_view, const StructDecl*> structByName;
    std::unordered_map<std::string_view, std::unordered_map<std::string_view,
                                                      const StructDecl*>> structByModule;
    std::unordered_map<std::string_view, const EnumDecl*> enumByName;
    std::unordered_map<std::string_view, std::unordered_map<std::string_view,
                                                      const EnumDecl*>> enumByModule;
    // 类索引三面：全键（限定名直查）/ 基名（原裸名回退首命中）/ 模块+基名
    //   （当前模块精确命中；键=模块名，即类键最后一个 "::" 之前的前缀）。
    std::unordered_map<std::string_view, const ClassInfo*> classByFullKey;
    std::unordered_map<std::string_view, const ClassInfo*> classByBase;
    std::unordered_map<std::string_view, std::unordered_map<std::string_view,
                                                       const ClassInfo*>> classByModule;
};

}  // namespace cn_compiler
