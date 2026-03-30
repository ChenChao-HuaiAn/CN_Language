#!/usr/bin/env node

/**
 * lessons.md 权重计算脚本
 * 
 * 用法：
 *   node calculate-weight.js <lessons-file-path>
 *   node calculate-weight.js --entry "设计缺陷|描述|原因|解决|预防"
 * 
 * 输出：JSON格式的权重计算结果
 */

const fs = require('fs');
const path = require('path');

// 加载配置
const configPath = path.join(__dirname, '../config/weight-config.json');
let config;

try {
  config = JSON.parse(fs.readFileSync(configPath, 'utf-8'));
} catch (e) {
  console.error('无法加载配置文件:', e.message);
  process.exit(1);
}

/**
 * 计算单条记录的权重
 * @param {Object} entry - 错误记录对象
 * @returns {number} - 计算后的权重
 */
function calculateWeight(entry) {
  // 1. 基础权重
  const baseWeight = config.baseWeights[entry.type] || config.baseWeights['其他'];
  
  // 2. 类型系数
  let typeCoefficient = 1.0;
  
  if (entry.hasDetailedAnalysis || entry.rootCause) {
    typeCoefficient *= config.typeCoefficients.hasDetailedAnalysis;
  }
  if (entry.hasSolution || entry.solution) {
    typeCoefficient *= config.typeCoefficients.hasSolution;
  }
  if (entry.hasPrevention || entry.prevention) {
    typeCoefficient *= config.typeCoefficients.hasPrevention;
  }
  if (entry.isTemplate) {
    typeCoefficient *= config.typeCoefficients.isTemplate;
  }
  if (!entry.rootCause && !entry.solution && !entry.isTemplate) {
    typeCoefficient *= config.typeCoefficients.noAnalysis;
  }
  
  // 3. 新鲜度系数
  let freshness = 1.0;
  if (entry.createdAt || entry.timestamp) {
    const createdDate = new Date(entry.createdAt || entry.timestamp);
    const daysSinceCreation = (Date.now() - createdDate.getTime()) / (1000 * 60 * 60 * 24);
    freshness = 1 / (1 + Math.log10(daysSinceCreation + 1));
  }
  
  // 4. 解决状态系数
  const resolutionMultiplier = config.resolutionMultipliers[entry.resolutionStatus] || 1.0;
  
  // 5. 计算最终权重
  const weight = Math.round(baseWeight * typeCoefficient * freshness * resolutionMultiplier);
  
  return Math.max(1, weight); // 最小权重为1
}

/**
 * 解析命令行参数中的单条记录
 * @param {string} entryStr - 格式: "类型|描述|原因|解决|预防"
 * @returns {Object} - 解析后的记录对象
 */
function parseEntryFromArg(entryStr) {
  const parts = entryStr.split('|');
  return {
    type: parts[0] || '其他',
    description: parts[1] || '',
    rootCause: parts[2] || '',
    solution: parts[3] || '',
    prevention: parts[4] || '',
    hasDetailedAnalysis: !!parts[2],
    hasSolution: !!parts[3],
    hasPrevention: !!parts[4],
    isTemplate: false,
    createdAt: new Date(),
    resolutionStatus: 'unresolved'
  };
}

/**
 * 解析 lessons.md 文件内容
 * @param {string} content - 文件内容
 * @returns {Array} - 解析后的记录数组
 */
function parseLessonsContent(content) {
  const entries = [];
  const lines = content.split('\n');
  
  let currentEntry = null;
  
  for (const line of lines) {
    // 匹配错误记录起始行
    const match = line.match(/-\s*\[(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2})\]\s*\*\*问题类型\*\*:\s*(.+)/);
    if (match) {
      if (currentEntry) {
        currentEntry.weight = calculateWeight(currentEntry);
        entries.push(currentEntry);
      }
      currentEntry = {
        timestamp: match[1],
        type: match[2].trim(),
        description: '',
        rootCause: '',
        solution: '',
        prevention: '',
        hasDetailedAnalysis: false,
        hasSolution: false,
        hasPrevention: false,
        isTemplate: false,
        createdAt: new Date(match[1]),
        resolutionStatus: 'unresolved'
      };
      continue;
    }
    
    // 匹配描述
    const descMatch = line.match(/\*\*描述\*\*:\s*(.+)/);
    if (descMatch && currentEntry) {
      currentEntry.description = descMatch[1].trim();
    }
    
    // 匹配原因
    const reasonMatch = line.match(/\*\*原因\*\*:\s*(.+)/);
    if (reasonMatch && currentEntry) {
      currentEntry.rootCause = reasonMatch[1].trim();
      currentEntry.hasDetailedAnalysis = true;
    }
    
    // 匹配解决方案
    const solutionMatch = line.match(/\*\*解决\*\*:\s*(.+)/);
    if (solutionMatch && currentEntry) {
      currentEntry.solution = solutionMatch[1].trim();
      currentEntry.hasSolution = true;
    }
    
    // 匹配预防措施
    const preventionMatch = line.match(/\*\*预防\*\*:\s*(.+)/);
    if (preventionMatch && currentEntry) {
      currentEntry.prevention = preventionMatch[1].trim();
      currentEntry.hasPrevention = true;
    }
    
    // 检测模板化内容
    if (line.includes('[RooCode#say]') || line.includes('aborted')) {
      if (currentEntry) {
        currentEntry.isTemplate = true;
      }
    }
  }
  
  // 添加最后一条记录
  if (currentEntry) {
    currentEntry.weight = calculateWeight(currentEntry);
    entries.push(currentEntry);
  }
  
  return entries;
}

/**
 * 生成权重摘要
 * @param {Array} entries - 记录数组
 * @returns {Object} - 摘要对象
 */
function generateSummary(entries) {
  const summary = {
    totalEntries: entries.length,
    typeDistribution: {},
    weightDistribution: {
      mustRead: 0,      // ≥ 15
      recommendedRead: 0, // 10-14
      optionalRead: 0,   // 5-9
      lowPriority: 0     // < 5
    },
    highWeightIssues: []
  };
  
  // 统计类型分布
  for (const type of Object.keys(config.baseWeights)) {
    summary.typeDistribution[type] = 0;
  }
  
  // 遍历记录
  for (const entry of entries) {
    // 类型统计
    summary.typeDistribution[entry.type] = (summary.typeDistribution[entry.type] || 0) + 1;
    
    // 权重分布
    if (entry.weight >= config.thresholds.mustRead) {
      summary.weightDistribution.mustRead++;
      summary.highWeightIssues.push({
        weight: entry.weight,
        type: entry.type,
        description: entry.description,
        timestamp: entry.timestamp
      });
    } else if (entry.weight >= config.thresholds.recommendedRead) {
      summary.weightDistribution.recommendedRead++;
    } else if (entry.weight >= config.thresholds.optionalRead) {
      summary.weightDistribution.optionalRead++;
    } else {
      summary.weightDistribution.lowPriority++;
    }
  }
  
  // 按权重排序高权重问题
  summary.highWeightIssues.sort((a, b) => b.weight - a.weight);
  
  return summary;
}

/**
 * 主函数
 */
function main() {
  const args = process.argv.slice(2);
  
  if (args.length === 0) {
    console.error('用法:');
    console.error('  node calculate-weight.js <lessons-file-path>');
    console.error('  node calculate-weight.js --entry "类型|描述|原因|解决|预防"');
    process.exit(1);
  }
  
  // 单条记录计算
  if (args[0] === '--entry') {
    const entry = parseEntryFromArg(args[1]);
    const weight = calculateWeight(entry);
    console.log(JSON.stringify({
      entry: entry,
      weight: weight,
      level: weight >= 15 ? 'mustRead' : weight >= 10 ? 'recommendedRead' : weight >= 5 ? 'optionalRead' : 'lowPriority'
    }, null, 2));
    return;
  }
  
  // 文件解析
  const lessonsPath = args[0];
  if (!fs.existsSync(lessonsPath)) {
    console.error('文件不存在:', lessonsPath);
    process.exit(1);
  }
  
  const content = fs.readFileSync(lessonsPath, 'utf-8');
  const entries = parseLessonsContent(content);
  const summary = generateSummary(entries);
  
  console.log(JSON.stringify({
    summary: summary,
    entries: entries.sort((a, b) => b.weight - a.weight)
  }, null, 2));
}

main();