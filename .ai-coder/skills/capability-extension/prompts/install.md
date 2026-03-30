# 安装指导提示模板

此模板用于指导技能的安装、验证和使用。

## 使用说明

在评估并决定安装某个技能后，使用此模板指导安装过程。

## 安装前准备

### 环境检查

```markdown
## 安装前环境检查

### 系统要求
- [ ] Node.js 版本: >= 16.0.0
- [ ] npm/npx 可用
- [ ] 网络连接正常
- [ ] 磁盘空间充足

### 权限检查
- [ ] 有写入权限
- [ ] 有执行权限

### 依赖检查
- [ ] 必要依赖已安装
- [ ] 版本兼容性确认

### 冲突检查
- [ ] 与已安装技能无冲突
- [ ] 与项目配置无冲突
```

### 安装决策

```yaml
# 安装决策记录
install_decision:
  skill_id: "react-i18n-guide"
  source: "vercel-labs/agent-skills@react-i18n-guide"
  
  decision: "install"  # install|skip|defer
  
  reasons:
    - "与当前任务需求高度相关"
    - "评估得分4.4，推荐安装"
    - "无冲突和风险"
    
  scope: "project"  # global|project
  
  options:
    overwrite: false
    dependencies: true
```

## 安装流程

### 第一步：执行安装命令

```bash
# 基本安装命令
npx skills add <source>

# 安装示例
npx skills add vercel-labs/agent-skills@react-i18n-guide

# 常用选项
npx skills add <source> [options]

选项说明:
  -g, --global          安装到用户目录（全局）
  -a, --agent <agents>  指定目标Agent
  -s, --skill <skills>  指定要安装的技能
  -l, --list            只列出可用技能，不安装
  --copy                复制文件而非符号链接
  -y, --yes             跳过确认提示
  --all                 安装所有技能到所有Agent
```

### 第二步：确认安装

```markdown
## 安装确认

### 安装信息
- **技能名称**: react-i18n-guide
- **来源**: vercel-labs/agent-skills
- **安装范围**: 项目级
- **目标位置**: ./<agent>/skills/

### 将要安装的文件
```
skills/react-i18n-guide/
├── SKILL.md
├── prompts/
│   └── ...
└── templates/
    └── ...
```

### 确认安装？
- 输入 `y` 确认安装
- 输入 `n` 取消安装
```

### 第三步：安装过程

```markdown
## 安装过程

### 安装步骤

1. **下载技能**
   ```
   正在从 GitHub 下载技能...
   ✓ 下载完成
   ```

2. **验证内容**
   ```
   正在验证技能内容...
   ✓ SKILL.md 存在
   ✓ 格式正确
   ✓ 内容完整
   ```

3. **安装文件**
   ```
   正在安装文件...
   ✓ 创建目录: ./claude-code/skills/react-i18n-guide
   ✓ 链接/复制文件
   ```

4. **更新配置**
   ```
   正在更新配置...
   ✓ 更新 skills-lock.json
   ```

5. **验证安装**
   ```
   正在验证安装...
   ✓ 技能可用
   ```

### 安装日志
```
[2024-01-15 10:30:00] 开始安装 react-i18n-guide
[2024-01-15 10:30:01] 下载完成
[2024-01-15 10:30:02] 验证通过
[2024-01-15 10:30:03] 安装完成
[2024-01-15 10:30:04] 验证成功
```
```

### 第四步：安装验证

```markdown
## 安装验证

### 验证检查清单

#### 文件检查
- [ ] 技能目录存在
- [ ] SKILL.md 文件存在
- [ ] 文件内容正确

#### 配置检查
- [ ] skills-lock.json 已更新
- [ ] 技能已注册

#### 功能检查
- [ ] 技能可以被识别
- [ ] 技能内容可访问

### 验证命令

```bash
# 列出已安装技能
npx skills list

# 预期输出
已安装技能:
  react-i18n-guide (v1.2.0)
  typescript-guide (v1.0.0)
```
```

## 安装后操作

### 学习新技能

```markdown
## 学习新安装的技能

### 学习步骤

1. **阅读技能文档**
   - 打开 SKILL.md
   - 理解技能用途
   - 了解使用场景

2. **查看示例**
   - 阅读提供的示例
   - 理解代码模板
   - 尝试运行示例

3. **实践应用**
   - 在实际任务中应用
   - 验证效果
   - 记录心得

### 学习记录

```yaml
learning_record:
  skill_id: "react-i18n-guide"
  learned_at: "2024-01-15T10:35:00Z"
  
  understanding:
    purpose: "清晰"
    usage: "基本掌握"
    examples: "已实践"
    
  notes:
    - "i18next是主要的国际化库"
    - "使用useTranslation Hook获取翻译函数"
    - "需要注意命名空间的使用"
    
  practical_experience:
    - task: "实现语言切换"
      result: "成功"
      notes: "使用i18n.changeLanguage()方法"
```

### 触发反思

安装完成后，触发 self-reflection 技能进行学习反思：

```yaml
reflection_request:
  trigger: "new_skill_installed"
  
  skill_info:
    id: "react-i18n-guide"
    name: "React国际化指南"
    
  reflection_tasks:
    - "评估技能对当前任务的帮助程度"
    - "识别需要进一步学习的部分"
    - "记录学到的关键知识"
    - "发现可能的改进点"
```
```

## 批量安装

### 批量安装配置

```yaml
# 批量安装配置文件
batch_install:
  # 安装范围
  scope: "project"
  
  # 技能列表
  skills:
    - source: "vercel-labs/agent-skills@react-i18n-guide"
      required: true
      
    - source: "vercel-labs/agent-skills@typescript-guide"
      required: true
      
    - source: "vercel-labs/agent-skills@testing-guide"
      required: false
      
  # 安装选项
  options:
    parallel: true  # 并行安装
    continue_on_error: true  # 失败继续
    skip_existing: true  # 跳过已安装
```

### 批量安装命令

```bash
# 从配置文件安装
npx skills add -f install-config.yaml

# 安装多个技能
npx skills add owner/repo --skill skill1 --skill skill2

# 安装所有技能
npx skills add owner/repo --all
```

## 安装问题排查

### 常见问题

```markdown
## 常见安装问题

### 问题1: 网络连接失败

**症状**:
```
Error: Failed to fetch skill from GitHub
```

**解决方案**:
1. 检查网络连接
2. 检查代理设置
3. 重试安装命令

### 问题2: 权限不足

**症状**:
```
Error: EACCES: permission denied
```

**解决方案**:
1. 检查目录权限
2. 使用管理员权限运行
3. 修改目标目录权限

### 问题3: 技能已存在

**症状**:
```
Warning: Skill already installed
```

**解决方案**:
1. 使用 --overwrite 选项覆盖安装
2. 或使用 --skip-existing 跳过

### 问题4: 版本冲突

**症状**:
```
Error: Version conflict detected
```

**解决方案**:
1. 检查依赖版本
2. 更新相关依赖
3. 选择兼容的技能版本
```

## 卸载和更新

### 卸载技能

```bash
# 卸载技能
npx skills remove react-i18n-guide

# 卸载多个技能
npx skills remove skill1 skill2
```

### 更新技能

```bash
# 检查更新
npx skills check

# 更新所有技能
npx skills update

# 更新特定技能
npx skills update react-i18n-guide
```

## 安装记录

### 安装记录格式

```yaml
# 安装历史记录
install_history:
  - skill_id: "react-i18n-guide"
    source: "vercel-labs/agent-skills@react-i18n-guide"
    version: "1.2.0"
    installed_at: "2024-01-15T10:30:00Z"
    scope: "project"
    status: "success"
    
  - skill_id: "typescript-guide"
    source: "vercel-labs/agent-skills@typescript-guide"
    version: "1.0.0"
    installed_at: "2024-01-15T10:31:00Z"
    scope: "project"
    status: "success"
```

## 注意事项

1. **安装范围**: 
   - 项目级安装到 `./<agent>/skills/`
   - 全局安装到 `~/<agent>/skills/`

2. **符号链接**:
   - 默认使用符号链接，便于更新
   - 使用 `--copy` 可改为复制文件

3. **版本管理**:
   - 锁定版本可确保一致性
   - 定期检查更新获取改进

4. **清理管理**:
   - 定期清理不用的技能
   - 避免安装过多技能影响性能

5. **安全考虑**:
   - 只从可信来源安装技能
   - 安装前检查技能内容