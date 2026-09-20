# T92 place 键修复 patch（2026-09-21 持久化·源=target/t90.patch 易失·402-a 教训预防）

## 应用方式
认领 T92 修复轮后：`git apply 该 patch 内容`（或按下方全文手改）→ 构建 → t528b/t528c 探针
（复现=target/p499s1/d11/t528*.cn 会话易失·重造见复现说明）→ 反证（checkout HEAD~1 复现误报）→ E2E 上架。

## 复现说明（探针易失重建口径）
```cn
// t528b：误报复现形态（修复前=「访问 结果.错误 前必须检查」误报；修复后=PASS）
函数 主() -> 整32 {
    结果<整32, 整32>[2] 表;
    表[1] = 错误(9);
    如果 (表[1].正常) {
        打印(0 - 2);
    } 否则 {
        打印(表[1].错误);
    }
    返回 0;
}
```
t528c=成员链同构（结构体 盒{结果 内}·b.内.正常/否则 b.内.错误）。

## patch 全文（src/cn_compiler/semantic/error_analysis.cpp·objectVarName 推广）
```diff
--- a/src/cn_compiler/semantic/error_analysis.cpp
+++ b/src/cn_compiler/semantic/error_analysis.cpp
@@ -376,10 +376,44 @@
-// 变量名提取（成员访问对象为标识符时返回变量名；否则空串）
-std::string SemanticAnalyzer::objectVarName(Expr* object) {
-    if (object == nullptr) return "";
-    if (object->getType() == NodeType::IdentifierExpr) {
-        return static_cast<IdentifierExpr*>(object)->name;
-    }
-    return "";
-}
+// 目标 place 键提取（505-b T90 推广）：强制检查分支绑定原只认简单变量名——
+//   「如果 (表[1].正常) { } 否则 { 表[1].错误 }」的下标/成员链目标进不了
+//   errorCheckState_（键=简单名）→ 正确代码被误报「必须检查 结果.正常」。
+//   推广为 place 规范键：
+//   - 简单名          -> "r"            （现行行为·零回归）
+//   - 成员链          -> "b.内"（递归）
+//   - 字面量下标      -> "表[1]"（精确槽 place）
+//   - 非字面量下标    -> ""（不确定 place·保守维持误报——Rust 对不确定索引
+//     place 同样保守拒绝；键含循环变量会跨轮残留标记=防线弱化·不做）
+std::string SemanticAnalyzer::objectVarName(Expr* object) {
+    if (object == nullptr) return "";
+    if (object->getType() == NodeType::IdentifierExpr) {
+        return static_cast<IdentifierExpr*>(object)->name;
+    }
+    if (object->getType() == NodeType::MemberExpr) {
+        auto* mem = static_cast<MemberExpr*>(object);
+        const std::string base = objectVarName(mem->object.get());
+        if (base.empty()) return "";
+        return base + "." + mem->memberName;
+    }
+    if (object->getType() == NodeType::IndexExpr) {
+        auto* idx = static_cast<IndexExpr*>(object);
+        const std::string base = objectVarName(idx->object.get());
+        if (base.empty()) return "";
+        // 仅字面量索引生成精确键（非字面量=不确定 place·保守返回空串）
+        if (idx->index == nullptr ||
+            idx->index->getType() != NodeType::IntegerLiteral) {
+            return "";
+        }
+        return base + "[" +
+               std::to_string(static_cast<IntegerLiteral*>(idx->index)->value) + "]";
+    }
+    return "";
+}
```
注意：函数签名不变（objectVarName）——调用点（trackIfCheck/checkResultMember）零改动。
