/**
 * 同步协调器模块
 * 
 * 负责协调 Wiki 内容与源代码的同步，支持：
 * - 增量更新：只更新变更的部分
 * - 冲突检测：处理手动修改与自动生成的冲突
 * - 版本控制：集成 Git 进行版本管理
 */

import * as fs from 'fs-extra';
import * as path from 'path';
import { WikiCreatorConfig } from './config-manager';
import { FileWatcher, FileChangeEvent } from './file-watcher';
import { ContentGenerator, GeneratedContent } from './content-generator';
import { MetadataManager, CodeSnippetMetadata } from './metadata-manager';

/**
 * 同步结果
 */
export interface SyncResult {
  /** 是否成功 */
  success: boolean;
  /** 处理的文件数量 */
  processedFiles: number;
  /** 错误列表 */
  errors: Array<{ file: string; error: string }>;
  /** 时间戳 */
  timestamp: Date;
  /** 新增的 Wiki 文件 */
  newFiles: string[];
  /** 更新的 Wiki 文件 */
  updatedFiles: string[];
  /** 删除的 Wiki 文件 */
  deletedFiles: string[];
}

/**
 * 同步状态
 */
export interface SyncStatus {
  /** 最后同步时间 */
  lastSync: Date;
  /** 待处理的变更数量 */
  pendingChanges: number;
  /** 同步中的文件 */
  syncingFiles: string[];
  /** 同步队列 */
  syncQueue: string[];
}

/**
 * 冲突信息
 */
export interface ConflictInfo {
  /** 文件路径 */
  filePath: string;
  /** 冲突类型 */
  conflictType: 'manual_edit' | 'auto_generated' | 'deleted';
  /** 冲突描述 */
  description: string;
  /** 解决建议 */
  suggestion: string;
}

/**
 * 同步协调器类
 */
export class SyncCoordinator {
  private config: WikiCreatorConfig;
  private fileWatcher: FileWatcher;
  private contentGenerator: ContentGenerator;
  private metadataManager: MetadataManager;
  
  private isSyncing: boolean = false;
  private syncQueue: string[] = [];
  private syncingFiles: Set<string> = new Set();
  private lastSyncTime: Date | null = null;
  private pendingChanges: Map<string, FileChangeEvent> = new Map();

  /**
   * 创建同步协调器
   * @param config Wiki 创建器配置
   * @param sourceRoot 源代码根目录
   */
  constructor(
    config: WikiCreatorConfig,
    sourceRoot: string
  ) {
    this.config = config;
    this.fileWatcher = new FileWatcher(config, sourceRoot);
    this.contentGenerator = new ContentGenerator(config);
    this.metadataManager = new MetadataManager();
  }

  /**
   * 初始化同步协调器
   */
  async initialize(): Promise<void> {
    // 初始化元数据管理器
    const metadataPath = path.join(
      this.config.wikiRoot,
      this.config.metadataFile
    );
    await this.metadataManager.initialize(metadataPath);

    // 加载模板
    for (const [name, templatePath] of Object.entries(this.config.contentTemplates)) {
      try {
        await this.contentGenerator.loadTemplate(name, templatePath);
      } catch (error) {
        console.warn(`加载模板失败：${name}`, error);
      }
    }

    console.log('同步协调器已初始化');
  }

  /**
   * 开始实时同步
   */
  async startRealtimeSync(): Promise<void> {
    await this.fileWatcher.watchFiles(async (event) => {
      this.pendingChanges.set(event.filePath, event);
      
      if (!this.isSyncing) {
        await this.processPendingChanges();
      }
    });

    console.log('实时同步已启动');
  }

  /**
   * 停止实时同步
   */
  async stopRealtimeSync(): Promise<void> {
    await this.fileWatcher.stopWatching();
    console.log('实时同步已停止');
  }

  /**
   * 同步所有文件
   */
  async syncAll(): Promise<SyncResult> {
    if (this.isSyncing) {
      return {
        success: false,
        processedFiles: 0,
        errors: [{ file: '', error: '同步正在进行中' }],
        timestamp: new Date(),
        newFiles: [],
        updatedFiles: [],
        deletedFiles: []
      };
    }

    this.isSyncing = true;
    const result: SyncResult = {
      success: true,
      processedFiles: 0,
      errors: [],
      timestamp: new Date(),
      newFiles: [],
      updatedFiles: [],
      deletedFiles: []
    };

    try {
      // 获取所有源文件
      const sourceFiles = await this.getSourceFiles();
      
      // 逐个处理文件
      for (const file of sourceFiles) {
        try {
          const processed = await this.processFile(file);
          if (processed) {
            result.processedFiles++;
            if (processed.isNew) {
              result.newFiles.push(processed.wikiFile);
            } else {
              result.updatedFiles.push(processed.wikiFile);
            }
          }
        } catch (error) {
          result.errors.push({
            file,
            error: error instanceof Error ? error.message : String(error)
          });
        }
      }

      this.lastSyncTime = new Date();
      result.success = result.errors.length === 0;
    } finally {
      this.isSyncing = false;
    }

    return result;
  }

  /**
   * 同步单个文件
   * @param sourceFile 源文件路径
   */
  async syncFile(sourceFile: string): Promise<SyncResult> {
    if (this.isSyncing) {
      return {
        success: false,
        processedFiles: 0,
        errors: [{ file: sourceFile, error: '同步正在进行中' }],
        timestamp: new Date(),
        newFiles: [],
        updatedFiles: [],
        deletedFiles: []
      };
    }

    this.isSyncing = true;
    this.syncingFiles.add(sourceFile);

    try {
      const processed = await this.processFile(sourceFile);
      
      return {
        success: true,
        processedFiles: processed ? 1 : 0,
        errors: [],
        timestamp: new Date(),
        newFiles: processed?.isNew ? [processed.wikiFile] : [],
        updatedFiles: processed?.isNew ? [] : [processed?.wikiFile || ''],
        deletedFiles: []
      };
    } catch (error) {
      return {
        success: false,
        processedFiles: 0,
        errors: [{ file: sourceFile, error: error instanceof Error ? error.message : String(error) }],
        timestamp: new Date(),
        newFiles: [],
        updatedFiles: [],
        deletedFiles: []
      };
    } finally {
      this.isSyncing = false;
      this.syncingFiles.delete(sourceFile);
    }
  }

  /**
   * 获取同步状态
   */
  async getSyncStatus(): Promise<SyncStatus> {
    return {
      lastSync: this.lastSyncTime || new Date(0),
      pendingChanges: this.pendingChanges.size,
      syncingFiles: Array.from(this.syncingFiles),
      syncQueue: [...this.syncQueue]
    };
  }

  /**
   * 处理待处理的变更
   */
  private async processPendingChanges(): Promise<void> {
    if (this.pendingChanges.size === 0) {
      return;
    }

    this.isSyncing = true;

    try {
      const changes = Array.from(this.pendingChanges.entries());
      this.pendingChanges.clear();

      for (const [filePath, event] of changes) {
        this.syncQueue.push(filePath);
      }

      while (this.syncQueue.length > 0) {
        const filePath = this.syncQueue.shift()!;
        this.syncingFiles.add(filePath);

        try {
          await this.processFile(filePath);
        } catch (error) {
          console.error(`处理文件失败：${filePath}`, error);
        } finally {
          this.syncingFiles.delete(filePath);
        }
      }

      this.lastSyncTime = new Date();
    } finally {
      this.isSyncing = false;
    }
  }

  /**
   * 处理单个文件
   * @param sourceFile 源文件路径
   * @returns 处理结果
   */
  private async processFile(sourceFile: string): Promise<{
    wikiFile: string;
    isNew: boolean;
  } | null> {
    // 检查文件是否匹配配置
    const minimatch = require('minimatch');
    const matches = this.config.sourcePatterns.some(pattern => 
      minimatch(sourceFile, pattern)
    );

    if (!matches) {
      return null;
    }

    // 生成内容
    const generatedContent = await this.contentGenerator.generateContent({
      sourceFile,
      templateName: 'default'
    });

    // 确定 Wiki 文件路径
    const wikiFilePath = await this.getWikiFilePath(sourceFile, generatedContent);

    // 检查文件是否存在
    const exists = await fs.pathExists(wikiFilePath);

    // 写入 Wiki 文件
    await fs.ensureDir(path.dirname(wikiFilePath));
    await fs.writeFile(wikiFilePath, generatedContent.content, 'utf-8');

    // 更新元数据
    await this.metadataManager.addSnippet({
      path: sourceFile,
      lineRange: generatedContent.metadata.lineRange,
      wikiFiles: [wikiFilePath],
      contentHash: this.contentGenerator.calculateContentHash(generatedContent.content)
    });

    return {
      wikiFile: wikiFilePath,
      isNew: !exists
    };
  }

  /**
   * 获取 Wiki 文件路径
   * @param sourceFile 源文件路径
   * @param generatedContent 生成的内容
   */
  private async getWikiFilePath(
    sourceFile: string,
    generatedContent: GeneratedContent
  ): Promise<string> {
    const relativePath = path.relative(process.cwd(), sourceFile);
    const baseName = path.basename(sourceFile, path.extname(sourceFile));
    
    // 根据源文件路径生成 Wiki 文件路径
    const dirName = path.dirname(relativePath).replace(/[/\\]/g, '-');
    const wikiFileName = `${dirName}-${baseName}.md`;
    
    return path.join(this.config.wikiRoot, 'content', 'auto-generated', wikiFileName);
  }

  /**
   * 获取所有源文件
   */
  private async getSourceFiles(): Promise<string[]> {
    const glob = require('glob');
    const files: string[] = [];

    for (const pattern of this.config.sourcePatterns) {
      const matched = await glob(pattern, {
        cwd: process.cwd(),
        ignore: this.config.exclusionPatterns
      });
      files.push(...matched);
    }

    return files;
  }

  /**
   * 检测冲突
   * @param wikiFilePath Wiki 文件路径
   */
  async detectConflict(wikiFilePath: string): Promise<ConflictInfo | null> {
    if (!await fs.pathExists(wikiFilePath)) {
      return null;
    }

    const content = await fs.readFile(wikiFilePath, 'utf-8');
    
    // 检查是否包含自动生成标记
    if (content.includes('<!-- AUTO-GENERATED -->')) {
      return {
        filePath: wikiFilePath,
        conflictType: 'auto_generated',
        description: '文件是自动生成的，手动修改可能会被覆盖',
        suggestion: '如需修改，请移除自动生成标记或修改源文件'
      };
    }

    // 检查是否有手动修改标记
    if (content.includes('<!-- MANUAL-EDIT -->')) {
      return {
        filePath: wikiFilePath,
        conflictType: 'manual_edit',
        description: '文件包含手动修改，自动更新可能会覆盖这些修改',
        suggestion: '请确认是否需要保留手动修改'
      };
    }

    return null;
  }

  /**
   * 清理同步状态
   */
  cleanup(): void {
    this.syncQueue = [];
    this.syncingFiles.clear();
    this.pendingChanges.clear();
  }
}
