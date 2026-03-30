/**
 * 配置管理器模块
 * 
 * 负责管理 Wiki 生成的配置参数，包括：
 * - 加载和验证配置文件
 * - 提供配置访问接口
 * - 支持动态更新配置
 */

import * as fs from 'fs-extra';
import * as path from 'path';

/**
 * 同步策略类型
 */
export type SyncStrategy = 'realtime' | 'manual' | 'scheduled';

/**
 * 内容模板配置
 */
export interface ContentTemplates {
  default: string;
  architecture?: string;
  api?: string;
  [key: string]: string | undefined;
}

/**
 * Wiki 创建器配置接口
 */
export interface WikiCreatorConfig {
  /** Wiki 根目录路径 */
  wikiRoot: string;
  /** 源文件匹配模式 */
  sourcePatterns: string[];
  /** 排除文件模式 */
  exclusionPatterns: string[];
  /** 内容模板配置 */
  contentTemplates: ContentTemplates;
  /** 同步策略 */
  syncStrategy: SyncStrategy;
  /** 定时同步间隔（毫秒），仅当 syncStrategy 为'scheduled'时使用 */
  scheduledInterval?: number;
  /** 元数据文件路径 */
  metadataFile: string;
  /** 是否生成引用 */
  generateCitations: boolean;
  /** 是否生成 Mermaid 图表 */
  generateMermaidDiagrams: boolean;
  /** 最大行长度 */
  maxLineLength: number;
  /** 缩进大小 */
  indentSize: number;
}

/**
 * 配置验证结果
 */
export interface ConfigValidationResult {
  valid: boolean;
  errors: string[];
  warnings: string[];
}

/**
 * 默认配置
 */
const DEFAULT_CONFIG: Partial<WikiCreatorConfig> = {
  syncStrategy: 'realtime',
  scheduledInterval: 3600000, // 1 小时
  generateCitations: true,
  generateMermaidDiagrams: true,
  maxLineLength: 100,
  indentSize: 2
};

/**
 * 配置管理器类
 */
export class ConfigManager {
  private config: WikiCreatorConfig | null = null;
  private configPath: string | null = null;

  /**
   * 从文件加载配置
   * @param configPath 配置文件路径
   */
  async loadConfig(configPath: string): Promise<WikiCreatorConfig> {
    this.configPath = configPath;
    
    try {
      const configData = await fs.readJson(configPath);
      this.config = { ...DEFAULT_CONFIG, ...configData } as WikiCreatorConfig;
      
      // 验证配置
      const validation = await this.validateConfig();
      if (!validation.valid) {
        console.warn('配置验证警告:', validation.warnings);
        console.error('配置验证错误:', validation.errors);
      }
      
      return this.config;
    } catch (error) {
      console.error('加载配置文件失败:', error);
      throw new Error(`无法加载配置文件：${configPath}`);
    }
  }

  /**
   * 获取当前配置
   */
  getConfig(): WikiCreatorConfig {
    if (!this.config) {
      throw new Error('配置尚未加载，请先调用 loadConfig()');
    }
    return this.config;
  }

  /**
   * 更新配置
   * @param updates 配置更新项
   */
  async updateConfig(updates: Partial<WikiCreatorConfig>): Promise<void> {
    if (!this.config) {
      throw new Error('配置尚未加载');
    }

    this.config = { ...this.config, ...updates };

    // 如果配置文件存在，则保存更新
    if (this.configPath) {
      await fs.writeJson(this.configPath, this.config, { spaces: 2 });
    }
  }

  /**
   * 验证配置
   */
  async validateConfig(): Promise<ConfigValidationResult> {
    const errors: string[] = [];
    const warnings: string[] = [];

    if (!this.config) {
      errors.push('配置尚未加载');
      return { valid: false, errors, warnings };
    }

    // 验证 wikiRoot
    if (!this.config.wikiRoot) {
      errors.push('wikiRoot 不能为空');
    } else {
      try {
        await fs.access(this.config.wikiRoot);
      } catch {
        warnings.push(`Wiki 根目录不存在：${this.config.wikiRoot}`);
      }
    }

    // 验证 sourcePatterns
    if (!this.config.sourcePatterns || this.config.sourcePatterns.length === 0) {
      errors.push('sourcePatterns 不能为空');
    }

    // 验证 syncStrategy
    const validStrategies: SyncStrategy[] = ['realtime', 'manual', 'scheduled'];
    if (!validStrategies.includes(this.config.syncStrategy)) {
      errors.push(`无效的同步策略：${this.config.syncStrategy}`);
    }

    // 验证 scheduledInterval
    if (this.config.syncStrategy === 'scheduled') {
      if (!this.config.scheduledInterval || this.config.scheduledInterval <= 0) {
        errors.push('定时同步间隔必须为正数');
      }
    }

    // 验证 contentTemplates
    if (!this.config.contentTemplates || !this.config.contentTemplates.default) {
      errors.push('必须指定默认模板');
    }

    return {
      valid: errors.length === 0,
      errors,
      warnings
    };
  }

  /**
   * 获取 Wiki 根目录的绝对路径
   */
  getWikiRootPath(): string {
    if (!this.config) {
      throw new Error('配置尚未加载');
    }
    return path.resolve(this.config.wikiRoot);
  }

  /**
   * 获取元数据文件的绝对路径
   */
  getMetadataFilePath(): string {
    if (!this.config) {
      throw new Error('配置尚未加载');
    }
    return path.join(this.getWikiRootPath(), this.config.metadataFile);
  }

  /**
   * 获取模板文件的绝对路径
   * @param templateName 模板名称
   */
  getTemplatePath(templateName: string): string {
    if (!this.config) {
      throw new Error('配置尚未加载');
    }
    
    const templatePath = this.config.contentTemplates[templateName] || 
                         this.config.contentTemplates.default;
    
    if (!templatePath) {
      throw new Error(`模板不存在：${templateName}`);
    }
    
    return path.resolve(templatePath);
  }

  /**
   * 重置配置为默认值
   */
  resetToDefaults(): void {
    this.config = { ...DEFAULT_CONFIG } as WikiCreatorConfig;
  }
}

/**
 * 创建默认配置文件
 * @param outputPath 输出文件路径
 */
export async function createDefaultConfig(outputPath: string): Promise<void> {
  const defaultConfig = {
    wikiRoot: '.qoder/repowiki/zh',
    sourcePatterns: ['src/**/*.c', 'src/**/*.h', 'include/**/*.h'],
    exclusionPatterns: ['**/build/**', '**/*.o', '**/*.exe', '**/.back', '**/.gitkeep'],
    contentTemplates: {
      default: './templates/default.md',
      architecture: './templates/architecture.md',
      api: './templates/api.md'
    },
    syncStrategy: 'realtime' as SyncStrategy,
    scheduledInterval: 3600000,
    metadataFile: 'meta/repowiki-metadata.json',
    generateCitations: true,
    generateMermaidDiagrams: true,
    maxLineLength: 100,
    indentSize: 2
  };

  await fs.writeJson(outputPath, defaultConfig, { spaces: 2 });
}
