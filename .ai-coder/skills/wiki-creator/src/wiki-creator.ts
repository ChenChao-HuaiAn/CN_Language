/**
 * WikiCreator 主类
 * 
 * 提供 Wiki 创建技能的主要接口，整合所有模块功能
 */

import * as path from 'path';
import { ConfigManager, WikiCreatorConfig } from './config-manager';
import { FileWatcher, FileChangeEvent, FileChangeCallback } from './file-watcher';
import { ContentGenerator, ContentGenerationRequest, GeneratedContent } from './content-generator';
import { MetadataManager } from './metadata-manager';
import { SyncCoordinator, SyncResult, SyncStatus } from './sync-coordinator';

/**
 * WikiCreator 配置接口
 */
export interface WikiCreatorOptions {
  wikiRoot: string;
  sourceRoot: string;
  configFile?: string;
  syncStrategy?: 'realtime' | 'manual' | 'scheduled';
}

/**
 * WikiCreator 类
 * 
 * 主要功能：
 * - 文件监听：监控源代码文件变更
 * - 内容生成：自动生成 Wiki 内容
 * - 同步管理：保持 Wiki 与源代码同步
 * - 元数据管理：跟踪代码片段引用
 */
export class WikiCreator {
  private configManager: ConfigManager;
  private syncCoordinator: SyncCoordinator | null = null;
  private sourceRoot: string;
  private isInitialized: boolean = false;
  private isWatching: boolean = false;

  /**
   * 创建 WikiCreator 实例
   * @param options 配置选项
   */
  constructor(options: WikiCreatorOptions) {
    this.configManager = new ConfigManager();
    this.sourceRoot = path.resolve(options.sourceRoot);

    // 如果提供了配置文件，则加载配置
    if (options.configFile) {
      this.configManager.loadConfig(options.configFile);
    }
  }

  /**
   * 初始化 WikiCreator
   */
  async initialize(): Promise<void> {
    if (this.isInitialized) {
      console.warn('WikiCreator 已初始化');
      return;
    }

    const config = this.configManager.getConfig();
    this.syncCoordinator = new SyncCoordinator(config, this.sourceRoot);
    await this.syncCoordinator.initialize();

    this.isInitialized = true;
    console.log('WikiCreator 已初始化');
  }

  /**
   * 释放资源
   */
  async dispose(): Promise<void> {
    if (this.isWatching) {
      await this.stopWatching();
    }
    this.syncCoordinator = null;
    this.isInitialized = false;
    console.log('WikiCreator 已释放');
  }

  /**
   * 获取当前配置
   */
  getConfig(): WikiCreatorConfig {
    return this.configManager.getConfig();
  }

  /**
   * 开始监听文件变更
   * @param callback 文件变更回调函数
   */
  async watchFiles(callback: FileChangeCallback): Promise<void> {
    if (!this.isInitialized) {
      throw new Error('WikiCreator 未初始化，请先调用 initialize()');
    }

    if (this.isWatching) {
      console.warn('文件监听已在运行');
      return;
    }

    await this.syncCoordinator!.startRealtimeSync();
    this.isWatching = true;
  }

  /**
   * 停止监听文件变更
   */
  async stopWatching(): Promise<void> {
    if (!this.isWatching || !this.syncCoordinator) {
      return;
    }

    await this.syncCoordinator.stopRealtimeSync();
    this.isWatching = false;
  }

  /**
   * 检查是否正在监听
   */
  isCurrentlyWatching(): boolean {
    return this.isWatching;
  }

  /**
   * 生成内容
   * @param request 内容生成请求
   */
  async generateContent(request: ContentGenerationRequest): Promise<GeneratedContent> {
    if (!this.isInitialized || !this.syncCoordinator) {
      throw new Error('WikiCreator 未初始化');
    }

    const config = this.configManager.getConfig();
    const generator = new ContentGenerator(config);
    
    // 加载模板
    for (const [name, templatePath] of Object.entries(config.contentTemplates)) {
      try {
        await generator.loadTemplate(name, templatePath);
      } catch (error) {
        console.warn(`加载模板失败：${name}`, error);
      }
    }

    return generator.generateContent(request);
  }

  /**
   * 为文件生成内容
   * @param sourceFile 源文件路径
   */
  async generateContentForFile(sourceFile: string): Promise<GeneratedContent[]> {
    if (!this.isInitialized || !this.syncCoordinator) {
      throw new Error('WikiCreator 未初始化');
    }

    const config = this.configManager.getConfig();
    const generator = new ContentGenerator(config);
    
    // 加载模板
    for (const [name, templatePath] of Object.entries(config.contentTemplates)) {
      try {
        await generator.loadTemplate(name, templatePath);
      } catch (error) {
        console.warn(`加载模板失败：${name}`, error);
      }
    }

    return generator.generateContentForFile(sourceFile);
  }

  /**
   * 同步所有文件
   */
  async syncAll(): Promise<SyncResult> {
    if (!this.isInitialized || !this.syncCoordinator) {
      throw new Error('WikiCreator 未初始化');
    }

    return this.syncCoordinator.syncAll();
  }

  /**
   * 同步单个文件
   * @param sourceFile 源文件路径
   */
  async syncFile(sourceFile: string): Promise<SyncResult> {
    if (!this.isInitialized || !this.syncCoordinator) {
      throw new Error('WikiCreator 未初始化');
    }

    return this.syncCoordinator.syncFile(sourceFile);
  }

  /**
   * 获取同步状态
   */
  async getSyncStatus(): Promise<SyncStatus> {
    if (!this.isInitialized || !this.syncCoordinator) {
      throw new Error('WikiCreator 未初始化');
    }

    return this.syncCoordinator.getSyncStatus();
  }

  /**
   * 更新配置
   * @param options 配置更新项
   */
  async updateConfig(options: Partial<WikiCreatorOptions>): Promise<void> {
    const config = this.configManager.getConfig();
    
    if (options.wikiRoot) {
      config.wikiRoot = options.wikiRoot;
    }
    if (options.syncStrategy) {
      config.syncStrategy = options.syncStrategy;
    }

    await this.configManager.updateConfig(config);
  }

  /**
   * 验证配置
   */
  async validateConfig(): Promise<{ valid: boolean; errors: string[]; warnings: string[] }> {
    return this.configManager.validateConfig();
  }
}
