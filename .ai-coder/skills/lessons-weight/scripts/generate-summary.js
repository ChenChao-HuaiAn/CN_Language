#!/usr/bin/env node

/**
 * lessons.md 摘要生成脚本
 * 
 * 用法：
 *   node generate-summary.js <lessons-file-path> [--output <output-path>]
 * 
 * 输出：Markdown格式的权重摘要
 */

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

/**
 * 生成Markdown格式的权重摘要
 * @param {Object} summary - 摘要对象
 * @param {Array} entries - 记录数组
 * @returns {string} - Markdown内容
 */
function generateMarkdownSummary(summary, entries) {
  let md = `## ⚠️ 高权重问题（AI 重点阅读）

`;
  
  // 必须关注的问题 (≥15)
  const mustReadIssues = entries.filter(e => e.weight >= 15);
  if (mustReadIssues.length > 0) {
    md += `### 🔴 权重 ≥ 15：必须关注\n\n`;
    for (const issue of mustReadIssues) {
      md += `> [权重: ${issue.weight}] ${issue.timestamp} | ${issue.type} | ${issue.description.substring(0, 30)}...\n`;
      md += `> - 问题：${issue.description}\n`;
      if (issue.solution) {
        md += `> - 解决：${issue.solution.substring(0, 50)}...\n`;
      }
      md += `\n`;
    }
  }
  
  // 重要问题 (10-14)
  const recommendedIssues = entries.filter(e => e.weight >= 10 && e.weight < 15);
  if (recommendedIssues.length > 0) {
    md += `### 🟡 权重 10-14：重要问题\n\n`;
    for (const issue of recommendedIssues) {
      md += `> [权重: ${issue.weight}] ${issue.timestamp} | ${issue.type} | ${issue.description.substring(0, 30)}...\n`;
      md += `\n`;
    }
  }
  
  // 统计表
  md += `---

## 问题类型统计

| 问题类型 | 出现次数 | 平均权重 | 最高权重 |
|---------|---------|---------|---------|
`;
  
  const typeStats = {};
  for (const entry of entries) {
    if (!typeStats[entry.type]) {
      typeStats[entry.type] = { count: 0, totalWeight: 0, maxWeight: 0 };
    }
    typeStats[entry.type].count++;
    typeStats[entry.type].totalWeight += entry.weight;
    typeStats[entry.type].maxWeight = Math.max(typeStats[entry.type].maxWeight, entry.weight);
  }
  
  for (const [type, stats] of Object.entries(typeStats)) {
    const avgWeight = (stats.totalWeight / stats.count).toFixed(1);
    md += `| ${type} | ${stats.count} | ${avgWeight} | ${stats.maxWeight} |\n`;
  }
  
  // 权重分布
  md += `
---

## 权重分布

\`\`\`
权重 ≥ 15：${summary.weightDistribution.mustRead} 个（必须关注）
权重 10-14：${summary.weightDistribution.recommendedRead} 个（重要问题）
权重 5-9：${summary.weightDistribution.optionalRead} 个
权重 < 5：${summary.weightDistribution.lowPriority} 个（可压缩）
\`\`\`
`;
  
  return md;
}

/**
 * 主函数
 */
function main() {
  const args = process.argv.slice(2);
  
  if (args.length === 0) {
    console.error('用法: node generate-summary.js <lessons-file-path> [--output <output-path>]');
    process.exit(1);
  }
  
  const lessonsPath = args[0];
  const outputIndex = args.indexOf('--output');
  const outputPath = outputIndex !== -1 ? args[outputIndex + 1] : null;
  
  if (!fs.existsSync(lessonsPath)) {
    console.error('文件不存在:', lessonsPath);
    process.exit(1);
  }
  
  // 调用计算脚本获取数据
  const scriptDir = __dirname;
  const result = execSync(`node "${scriptDir}/calculate-weight.js" "${lessonsPath}"`, {
    encoding: 'utf-8'
  });
  
  const data = JSON.parse(result);
  const markdown = generateMarkdownSummary(data.summary, data.entries);
  
  if (outputPath) {
    fs.writeFileSync(outputPath, markdown, 'utf-8');
    console.log('摘要已写入:', outputPath);
  } else {
    console.log(markdown);
  }
}

main();