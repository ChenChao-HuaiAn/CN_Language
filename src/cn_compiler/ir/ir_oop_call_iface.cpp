// CN-IR生成器——阶段3 接口分派调用发射（552 行数整改·自 ir_oop_call.cpp 按族拆出）
//   拆分理由（§3 先拆再加纪律）：ir_oop_call.cpp 999 行已超 900 警戒线——本轮
//   新增「接口方法 128 实参定标」（552 双侧同案）前先按族拆分。
//   族 = 接口对象方法调用发射（tryInterfaceMethodCall：P3-19 B1 全局槽位运行时分派
//   + D3A 去虚拟化直调 + P3/D4 CFI 校验 emitCfiCheck——接口域单一归属）；
//   纯搬迁零行为变更（成员函数实现搬迁——声明仍在 ir.hpp）。
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// P3-19 接口对象方法调用族（原 handleClassCallExpr 接口段整体迁移）：
//   返回 true=已处理；false=非接口方法形态（fall-through 构造/成员路径，与原
bool IRGenerator::tryInterfaceMethodCall(CallExpr* node) {
    // ---- P3-19：接口对象方法调用（图形.方法(实参)）——B1 全局槽位运行时分派 ----
    // 接口分派区在对象首固定偏差（首个 8 字节为强制虚表指针，region 紧随其后）：
    //   偏移 = 8 + 全局槽*8；LoadPtr 取实现方法地址 -> CallIndirect(方法, [this, 实参])。
    if (node->callee->getType() == NodeType::MemberExpr) {
        MemberExpr* mem = static_cast<MemberExpr*>(node->callee.get());
        std::string objSrc = exprSrcType(mem->object.get());
        std::string ifaceName = types::canonical(objSrc);
        if (types::isPointer(ifaceName)) {
            ifaceName = types::canonical(types::pointeeOf(ifaceName));
        }
        if (!ifaceName.empty() && semantic_->isInterfaceType(ifaceName)) {
            const InterfaceInfo* iface = semantic_->findInterface(ifaceName);
            if (iface != nullptr) {
                const auto imit = iface->methods.find(mem->memberName);
                if (imit != iface->methods.end()) {
                    // P3/D3A（去虚拟化）：接口全局唯一实现类 → 编译期直接调用该实现
                    //   方法（跳过 LoadPtr+CallIndirect，热路径省 1 次间接跳转；
                    //   多实现接口回退既有间接路径，语义不变）
                    const std::vector<std::string> impls =
                        semantic_->interfaceImplClasses(ifaceName);
                    if (impls.size() == 1) {
                        std::string owner;
                        const ClassMemberInfo* m =
                            semantic_->lookupClassMember(impls[0], mem->memberName, owner);
                        if (m != nullptr && !m->isStatic) {
                            ir::IRValue objVal = genExpr(mem->object.get());
                            // 552（接口方法 × 128 形参双侧同案·宿主侧）：接口调用
                            //   实参定标——原逐 genExpr 直压栈（无 128 定标）→ 窄实参
                            //   按 i64 值直传，被调 prologue 按双槽地址 rep movsb
                            //   解引用 SIGSEGV（探针 552pe2：引用.取(窄32) rc=139）。
                            //   判据源=接口方法签名形参表（imit->second.paramTypes·
                            //   与间接路径同源），定标本体=widenI128Args 单一归属
                            //   （与实例/静态/构造/虚调用四路径同款 331-a 家系）。
                            std::vector<ir::IRValue> userArgs =
                                buildCallArgsOop(node->arguments, node->location);
                            widenI128Args(userArgs, imit->second.paramTypes,
                                          node->location);
                            std::vector<ir::IRValue> args;
                            args.push_back(objVal);  // this = 对象指针
                            for (auto& a : userArgs) args.push_back(a);
                            const std::string retIr =
                                mapType(types::canonical(imit->second.type));
                            const std::string ownerSym = owner.empty() ? impls[0] : owner;
                            lastExpr_ = emitResult(
                                ir::Opcode::Call, args, retIr,
                                methodSymbolKey(ownerSym, m->sigKey), node->location);
                            return true;
                        }
                    }
                    const int slot = semantic_->interfaceSlot(ifaceName, mem->memberName);
                    if (slot >= 0) {
                        ir::IRValue objVal = genExpr(mem->object.get());
                        const int dispOffset = 8 + slot * 8;  // 8=强制虚表指针
                        ir::IRValue regionAddr =
                            emitResult(ir::Opcode::FieldAddr, {objVal}, "ptr",
                                       std::to_string(dispOffset), node->location);
                        ir::IRValue meth =
                            emitResult(ir::Opcode::LoadPtr, {regionAddr}, "ptr", "",
                                       node->location);
                        // P3/D4（CFI）：--cfi 开启时校验 分派目标 ∈ 该接口已知实现集合
                        if (cfiEnabled_) {
                            emitCfiCheck(meth, ifaceName, mem->memberName,
                                         node->location);
                        }
                        // 552（接口方法 × 128 形参双侧同案·宿主侧）：间接分派路径
                        //   同补实参定标——判据源与直调分支同一（接口方法签名形参表
                        //   imit->second.paramTypes），载体同一（widenI128Args）；
                        //   原缺 → 窄实参照崩（探针 552pe5：引用.取(窄32) rc=139）。
                        std::vector<ir::IRValue> userArgs =
                            buildCallArgsOop(node->arguments, node->location);
                        widenI128Args(userArgs, imit->second.paramTypes,
                                      node->location);
                        std::vector<ir::IRValue> args;
                        args.push_back(objVal);  // this = 对象指针
                        for (auto& a : userArgs) args.push_back(a);
                        const std::string retIr =
                            mapType(types::canonical(imit->second.type));
                        args.insert(args.begin(), meth);  // operand[0]=方法地址
                        lastExpr_ = emitResult(ir::Opcode::CallIndirect, args, retIr,
                                               "", node->location);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void IRGenerator::emitCfiCheck(const ir::IRValue& target,
                               const std::string& ifaceName,
                               const std::string& methodName,
                               const SourceLocation& loc) {
    // P3/D4（2026-08）：接口间接调用 CFI——分派目标 ∈ 该接口已知实现集合 且 非空。
    //   目标表约束：编译期该接口的全部非抽象实现类的方法符号（继承链并入）。
    //   bad = (target==0) || (target 不属于任何已知实现) → __cn_runtime_error(3)。
    if (semantic_ == nullptr || currentBlock_ == nullptr) return;
    const std::vector<std::string> impls = semantic_->interfaceImplClasses(ifaceName);
    ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
    ir::IRValue isNull = emitResult(ir::Opcode::Eq, {target, zero}, "i1", "", loc);
    // valid = 命中任一已知实现
    ir::IRValue valid = emitResult(ir::Opcode::ConstBool, {}, "i1", "假", loc);
    for (const auto& cls : impls) {
        std::string owner;
        const ClassMemberInfo* m = semantic_->lookupClassMember(cls, methodName, owner);
        if (m == nullptr) continue;
        const std::string ownerSym = owner.empty() ? cls : owner;
        ir::IRValue implAddr = emitResult(
            ir::Opcode::FuncAddr, {}, "ptr",
            methodSymbolKey(ownerSym, m->sigKey), loc);
        ir::IRValue isMatch = emitResult(ir::Opcode::Eq, {target, implAddr}, "i1", "",
                                         loc);
        valid = emitResult(ir::Opcode::Or, {valid, isMatch}, "i1", "", loc);
    }
    ir::IRValue notValid = emitResult(ir::Opcode::Not, {valid}, "i1", "", loc);
    ir::IRValue bad = emitResult(ir::Opcode::Or, {isNull, notValid}, "i1", "", loc);
    // 错误块 + 继续块（与 emitBoundsCheck 同模式；newBlock 内部推进 blockCounter_）
    const std::string errLabel = "bb" + std::to_string(blockCounter_);
    const std::string okLabel = "bb" + std::to_string(blockCounter_ + 1);
    endBranch(bad.toString(), errLabel, okLabel);
    setCurrentBlock(newBlock(errLabel));
    ir::IRValue errCode = emitResult(ir::Opcode::ConstInt, {}, "i64", "3", loc);
    emitResult(ir::Opcode::Call, {errCode}, "i32", "__cn_runtime_error", loc);
    endReturn("");
    setCurrentBlock(newBlock(okLabel));
}

} // namespace cn_compiler
