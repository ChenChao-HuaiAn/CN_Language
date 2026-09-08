# HANDOFF 交接文档

**交接时间**: 2026-09-09 第四十轮（**家机 win-x64，跨机轮 win 侧收官**）——按挂账1 拉取 1a12bc3（第三十一~三十九轮）跑全量门禁，**连拦四枚平台缺陷**（深度机/单位机侧不可见，跨机轮价值实证），四枚均按用户预授权准则（Rust 准绳：性能第一/安全第二）选方案A 当轮根治，门禁收官 177 用例 175 过/2 败/0 跳（唯一败面=78/79 已知 OOM 家族，与第三十轮基线一致零回归）。

---

## 一、本轮四枚缺陷（写给无上下文的新会话；详见 lessons.md 三条与 git 提交）

1. **intern_api.cpp GCC 内建不可移植**（第三十七轮插桩入库）：`__builtin_return_address(0)` MSVC 无此内建（error C3861）→ `#if defined(_MSC_VER)` 分流 `<intrin.h>` `_ReturnAddress()`，GCC 路径逐字节原样；编译期展开零开销，诊断 ra 能力全平台对等。
2. **同段 getenv 弃用**：C4996/WX=错误（首错遮蔽延迟暴露）→ `getenv_s` 空值查询（跟随 cn_main.cpp getEnvVar 安全 CRT 先例）。
3. **win E2E 40+ 用例齐爆 ml64 失败**（首发缺陷）：表象 73 个 A2005 bbN 重定义；首错实锤 27 个 A2043 identifier too long——**289 字符 mangled 符号 > MASM 247 硬上限**（第三十四/三十八轮包名前缀命名 × hex-mangle 每汉字 6 字符膨胀；GAS 无此限制故 linux/arm64 全绿掩盖；**A2043 失效形态=PROC 配对崩坏→A2005 雪崩，非单点报错**）。根治：x64_codegen.cpp nameMangle 单点包裹 shortenLongSymbol（>200 →「头段180$L<原长>H<FNV-1a64>」；哈希含参数串重载不冲突+嵌入原长碰撞面 2^-64；LLVM/rustc 超长 mangle 内容哈希截断同构；短符号逐字节不变零扰动）。**worktree HEAD 对拍实证**：HEAD 产物最长 231/超限 0/ml64 RC=0 vs 本轮 289/34/RC=1——严格对应前缀命名轮次。
4. **run_e2e.py win 负路径整树复制漏支**（第三十一轮四象限加三漏一）：168/169/170 停在 p6b 依赖缺文件 → 补同款 copytree 单跑转绿。

## 二、验证（门禁全绿口径）

- 构建零警告（MSVC /W4 /WX）+ 单测 **1251/1251** + E2E win-x64 **177 用例 175 过/2 败/0 跳**（败=78/79 已知 OOM 家族 4108/4130MB，v1 旧架构固有）。
- 修复分界验证：ci_full3（缺③④）172 过/5 败 → ci_full5（四修复齐）175 过/2 败。
- win 侧三处代码修复对 linux/arm64 零改动静态论证：①②#if 分流 GCC 原样/③nameMangle 短化仅 win 独立目录 x64_codegen.cpp/④copytree 平台中性。

## 三、挂账（按优先级）

1. **跨机轮 arm64 半程**：本轮三处代码修复（intern_api/x64_codegen/run_e2e）对 linux 零改动已静态论证，单位机同步后跑 arm64 全量门禁动态收口。
2. abi辅助.cn（v1 遗留探针 13 行）转正或删除（沿上轮）。
3. v2 结构体构造字面量 `记录{...}` 语法缺口（随语法覆盖轮，沿上轮）。
4. v2self 侧长符号面自查（本轮短化在宿主 win 后端；v2 自举编译器的 win 符号发射如有同款上限需同款收缩——v2 x64 后端 生成指令 段自查，暂无触发证据）。

## 四、验证链（下轮接手先跑）

```bash
uname -m                                    # 环境识别（x86_64=深度机/aarch64=单位机/本行=家机win）
powershell -ExecutionPolicy Bypass -File scripts/ci.ps1    # 家机全量门禁（构建+单测+E2E）
# 长符号回归自查（win）：
target/Debug/cn.exe compile tests/e2e/89_关键字穷举/主.cn --target win-x64 --output target/t.s
#   产物 PROC/ENDP 符号长度须全 ≤247（python 扫描 ' PROC'/' ENDP' 行尾）
# 负路径三例（win 分支 copytree）：
python tests/e2e/run_e2e.py --filter 169_v2_glob歧义
```

## 五、本轮踩坑（已入 lessons.md）

1. **MASM 247 字符标识符上限 + A2043 连锁失效形态**（权重10）：三后端共存时任何「使命名变长」的改动必须对三后端符号上限对表；汇编器错误必须看第一条（首错=根因，尾部数百条全是级联）。
2. **GCC 内建/POSIX 函数跨平台入库纪律**（权重8）：跨机轮拦截面价值所在；同段多平台问题被首错遮蔽，修首错后立即重编译。
3. **多分支对称改动四象限穷尽**（权重7）：平台×路径类型的正交改动逐格勾稽；未触发分支在当轮门禁不可见——合入时标注显式待验清单。
