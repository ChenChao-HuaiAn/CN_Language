// CN-IR生成器实现：闭包调用族（954 拆族：自 ir_call.cpp visitCallExpr 巨函数
//   纯机械搬移·逐字保留零语义变化；函数≤100 行纪律）
//   族 = lambda 闭包变量调用速径展开（genClosureCallExpr + 捕获装载
//   buildClosureCaptureArgs482）：Task 2.10 闭包展开 / 482 载体快照槽 / 337-a
//   用户实参 ABI 定标 / 485 聚合返回 retbuf 预插。成员函数声明仍在 ir_generator.hpp。
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_generator.hpp"
#include "cn_compiler/model/ast_expr.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// Task 2.10 lambda 闭包调用：`加倍(21)`——callee 是登记过的闭包变量，
//   展开为：捕获实参（前置，定义处已固化的值快照/引用指针，缺陷修复——
//   原实现在此 genExpr(变量) 于调用点取值，[=] 值捕获读到最新值而非快照）
//           + 显式实参 + Call 匿名函数
// 954（visitCallExpr 拆族·机械搬移）：族体自 visitCallExpr 逐字迁出;单点裁剪变换=
//   「if(closureIt 命中){...}」->「if(miss) return false; ...return true;」+缩进左移。
bool IRGenerator::genClosureCallExpr(CallExpr* node, const std::string& calleeName) {
    auto closureIt = closureInfo_.find(calleeName);
    if (closureIt == closureInfo_.end()) return false;
    std::vector<ir::IRValue> closureArgs;
    buildClosureCaptureArgs482(closureIt->second, node, closureArgs);
    // 337-a（T53 家系·闭包调用路径）：用户实参按闭包形参类型 ABI 定标
    //   （i128 形参的窄整实参宽化）——展开式 `Call(捕获实参..., 用户实参...)`
    //   中原缺此步：字面量实参物理 i64 直传、被调方按 i128 指针 ABI 解引用
    //   SIGSEGV（探针 f_closure〔对象.方法 闭包〕rc=139）。捕获实参不入
    //   定标面（其形态由捕获机制自身决定）；paramTypes 空=零行为变更。
    std::vector<ir::IRValue> userArgs;
    userArgs.reserve(node->arguments.size());
    for (auto& arg : node->arguments) {
        userArgs.push_back(genExpr(arg.get()));
    }
    widenI128Args(userArgs, closureIt->second.paramTypes, node->location);
    closureArgs.insert(closureArgs.end(), userArgs.begin(), userArgs.end());
    // 485（结构体/数组返回闭包·同帧展开通道形态A）：?lambdaN 已置
    //   structReturn（ir_lambda 合成函数同普通函数声明通道·被调方隐藏
    //   返回指针接收/回写），调用点须同步预插 retbuf 为首实参（rcx）——
    //   原按 rax 取返回值=被调方栈上临时地址（同帧悬垂读·间接调用
    //   C0000005 同族）。判定用源码返回类型（returnIrType 已塌缩 ptr）。
    if (semantic_ != nullptr && !closureIt->second.srcReturnType.empty() &&
        (semantic_->isStructType(
             types::canonical(closureIt->second.srcReturnType)) ||
         types::isArray(closureIt->second.srcReturnType))) {
        const std::string temp = "__retbuf" + std::to_string(varCounter_++);
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
             temp, "ptr", node->location);
        // 返回缓冲区按 64 字节（8 槽）分配——与被调方 epilogue 拷贝
        //   上限一致（与直接调用形态A同款）
        function_->varSlots[temp] = 8;
        registerVarSlots(temp, closureIt->second.srcReturnType);
        ir::IRValue buf = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(temp, "i64")},
                                     "ptr", temp, node->location);
        // 隐藏返回指针作为第一个参数（rcx）
        closureArgs.insert(closureArgs.begin(), buf);
        emit(ir::Opcode::Call, closureArgs, ir::IRValue(),
             closureIt->second.lambdaName, "void", node->location);
        lastExpr_ = buf;
        return true;
    }
    // 结果类型：匿名函数返回 IR 类型（非空）；空（如 空类型）用 void
    const std::string retType = closureIt->second.returnIrType.empty()
                                    ? "void" : closureIt->second.returnIrType;
    lastExpr_ = emitResult(ir::Opcode::Call, closureArgs, retType,
                           closureIt->second.lambdaName, node->location);
    return true;
}

// 482（#482 多实例独立快照）捕获实参装载（954 拆族子段·原 visitCallExpr L104-148
//   逐字迁出;closureIt->second. -> info. 同对象引用重命名（防与循环变量 ci 撞名·
//   零语义）。
void IRGenerator::buildClosureCaptureArgs482(const ClosureInfo& info, CallExpr* node,
                                             std::vector<ir::IRValue>& closureArgs) {
    // 482（#482 多实例独立快照）：槽式载体登记优先——从载体槽装载捕获
    //   实参（每实例独立快照；captureArgs 为定义点寄存器/共享通道=同
    //   定义点多次求值后写覆盖）。空=旧寄存器式（276/816 既有语义）。
    if (!info.carrierCaptures.empty()) {
        const auto metaIt =
            lambdaCaptureMeta482_.find(info.lambdaName);
        for (std::size_t ci = 0;
             ci < info.carrierCaptures.size(); ++ci) {
            const auto& cc = info.carrierCaptures[ci];
            const bool byRef =
                metaIt != lambdaCaptureMeta482_.end() &&
                ci < metaIt->second.refs.size() &&
                metaIt->second.refs[ci] != 0;
            const std::string capCore = types::stripRef(cc.srcType);
            const std::string capIr =
                mapType(capCore.empty() ? "整64" : capCore);
            if (byRef) {
                // [&] 引用捕获：载体槽值=被捕获变量地址（ptr 直传——
                //   ?lambdaN 的引用捕获参数按地址收）
                closureArgs.push_back(emitResult(
                    ir::Opcode::Load,
                    {ir::IRValue::var(cc.unique, "ptr")}, "ptr",
                    cc.unique, node->location));
            } else if (semantic_ != nullptr && !capCore.empty() &&
                       semantic_->isStructType(
                           types::canonical(capCore))) {
                // 结构体值捕获：实参=载体槽地址（?lambdaN 按值形参
                //   prologue 自该地址整块拷贝——与定义点固化同协议）
                closureArgs.push_back(emitResult(
                    ir::Opcode::AddrOf,
                    {ir::IRValue::var(cc.unique, "i64")}, "ptr",
                    cc.unique, node->location));
            } else {
                // 标量/指针/字符串/i128：值装载（宽度按捕获 IR 类型）
                closureArgs.push_back(emitResult(
                    ir::Opcode::Load,
                    {ir::IRValue::var(cc.unique, capIr)}, capIr,
                    cc.unique, node->location));
            }
        }
    } else {
        for (const auto& capArg : info.captureArgs) {
            closureArgs.push_back(capArg);
        }
    }
}


} // namespace cn_compiler