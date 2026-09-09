# HANDOFF 交接文档

**交接时间**: 2026-09-09 第四十二轮（**单位机 麒麟 ARM64**）——按第四十/四十一轮 HANDOFF 挂账1 拉取 c9945ca 执行跨机轮 arm64 动态收口：全量门禁复验零回归 + 字面量位模式 arm64 探针动态实锤（宿主 vs v2p 产物逐字节一致）+ v2self 字节级固定点 fix_p≡fix_s 复现（294888 行）+ 探针转正 E2E 176/177。纯验证轮零编译器源码改动。

---

## 一、本轮做了什么（写给无上下文的新会话）

1. **全量门禁（挂账1 本体）**：清场重建 GCC 零警告 + 单测 1251/1251 + 全量 E2E linux-arm64 177 用例 173 过/1 败/3 跳——败=78（已知 OOM 家族 4149MB，基线 4126MB 同形态零回归）、跳=62/69/79（既有平台限制）。第四十轮 intern_api 平台分流的 GCC 路径、第四十一轮宿主改动（win 独立目录）对 arm64 零影响的静态论证全部动态兑现。
2. **字面量位模式 arm64 动态复验**（第四十一轮挂账「静态论证，跨机轮动态复验」收口）：八形态探针（0xFF/0b1010/0o777/42L/7U/2^55/0x7FFFFFFFFFFFFFFF/-255，自身源码之外——lessons 权重10 预防①）——宿主与 v2p(arm64) 产物运行输出**逐字节一致**；v2 产物 asm 实证 `movz x9,#255`（修复前 arm64 本地解析把 0x 静默解析为 0 的潜伏缺陷确认根治）；X64L 静态旁证 `movabs` 大常量发射正确（动态运行留深度机）。
3. **v2self arm64 锚定复现**：v2p 重建（宿主编译 v2 全树）→ cn_self 一代链接（cn_self.o 在前 + v2p.o `-Wl,-z,muldefs` 供容器符号，nm 自检 cn_main 唯一）→ **字节级固定点 fix_p ≡ fix_s（294888 行裸逐字节一致）**；组件单文件入口冒烟 5/5 rc=0（词法分析/解析基础/IR容器/代码生成共用/货舱解析——包上下文恢复路径实证）。
4. **探针转正 E2E 176/177**：176_字面量进制形态（宿主侧，打印逐行 expected+程序内八断言双保险）、177_v2_字面量进制形态（v2 闭环注册，空 expected=rc 门控，对齐 E2E 解耦方向）；run_e2e.py `v2闭环用例们` 增量一行；单跑双绿。

## 二、验证链（下轮接手可复跑）

```bash
uname -m                                    # aarch64=单位机（本行）
rm -rf target/build && cmake -S . -B target/build -DCMAKE_BUILD_TYPE=Debug && cmake --build target/build -j 8
./target/cn_unit_tests                      # 1251/1251
python3 tests/e2e/run_e2e.py --target linux-arm64 --cn target/cn    # 全量（约 40 分钟）
# 字面量位模式探针（target/probe_lit/主.cn，不入库）：
./target/cn build target/probe_lit/主.cn --target linux-arm64 --output target/probe_lit/host.s   # host.s=可执行
./target/probe_lit/host.s && ./target/probe_lit/v2p.s target/probe_lit/主.cn linux-arm64 && as -o /tmp/p.o target/v2asm.s && g++ -no-pie -o /tmp/pv /tmp/p.o target/probe_lit/rt/*.o && /tmp/pv   # 两侧输出逐字节一致
# v2self 固定点（v2p 二进制=target/probe_lit/v2p.s，由宿主编译 v2/主.cn 产出；cn_self=fix_p.s 链接）：
./target/probe_lit/cn_self CN语言编译器v2/主.cn linux-arm64   # 产物 target/v2asm.s 应与 fix_p.s 逐字节一致
```

注意：`cn build --output X` 的 X 是**可执行文件**（不是汇编文本），.o 是 relocatable 副产品；`打印`=末尾换行、`打印行`=不换行（探针曾按直觉写反，行为二分定位后以 ir_call.cpp 注释为准）。

## 三、挂账（按优先级）

1. **深度机轮（linux-x86_64）**：拉取后跑本机全量门禁；重点=组件对拍 30/30 与固定点 fix_p≡fix_s 应保持（strData 编号随第四十一轮共用层新函数平移，跨轮对拍须规范化）；X64L 侧字面量位模式动态运行复验（本轮仅静态旁证）；176/177 在 X64L 首验。
2. **家机轮（win-x64）**：176/177 在 win 后端首验（ml64/link 路径）+ 全量门禁例行复验。
3. **呈报待用户裁决（三项，第四十一轮呈报未变，见更新日志第五节）**：①v2 位运算/移位全链路缺失（建议独立特性轮）；②v2 结构体构造字面量 `类型{字段=值}` 缺口（建议随语法覆盖轮）；③v2 主.cn 路径文本匹配反斜杠脆弱（建议路径身份归一 `/`）。
4. v2p 构建链脚本转正待用户定（target/ 不入库——本轮 target/probe_lit/ 有 v2p 二进制与 fix_p/fix_s 产物可复用）。

## 四、本轮踩坑（未入 lessons——单机环境差异类，权重低；两条备忘）

1. `cn build --output X` 产物语义：X=链接好的可执行文件，X.o=relocatable（`.s` 后缀会误导——as 一个 ELF 必炸「junk at end of line」）；v2p 二进制直接跑即可，无需自拼 as+g++。
2. cn_self 链接必须 `-Wl,-z,muldefs` 且 cn_self.o 在 v2p.o 之前（容器方法符号来自 v2p.o——E2E 执行v2闭环Linux 同款编排，漏 muldefs=成片 undefined reference `_E59091E9878F24*`）。

## 五、诚实边界

- 组件 30/30 全量逐字节对拍未在本机跑（深度机轮重点；本轮 5 入口抽样冒烟+全树固定点强锚定）；
- X64L 字面量位模式为静态旁证（本机 arm64 无 x86_64 运行能力）；
- 78/79 为 v1 旧架构固有 OOM/平台跳过，随 v2 重建完成自然解决；
- 呈报三项未经用户裁决不实施（缺陷裁决纪律）。
