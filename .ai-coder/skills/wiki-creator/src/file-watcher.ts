/**
 * 文件监听器模块
 * 
 * 负责监控源代码文件变更，支持：
 * - 递归监听指定目录
 * - 过滤无关文件变更
 * - 跨平台文件事件处理
 */

import * as chokidar from 'chokidar';
import * as path from 'path';
import { WikiCreatorConfig } from './config-manager';

/**
 * 文件变更类型
 */
export type FileChangeType = 'created' | 'modified' | 'deleted' | 'renamed';

/**
 * 文件变更事件
 */
export interface FileChangeEvent {
  /** 文件路径 */
  filePath: string;
  /** 变更类型 */
  changeType: FileChangeType;
  /** 时间戳 */
  timestamp: Date;
  /** 相对路径 */
  relativePath?: string;
  /** 文件扩展名 */
  extension?: string;
}

/**
 * 文件变更回调函数类型
 */
export type FileChangeCallback = (event: FileChangeEvent) => void | Promise<void>;

/**
 * 文件监听器类
 */
export class FileWatcher {
  private watcher: chokidar.FSWatcher | null = null;
  private config: WikiCreatorConfig | null = null;
  private callbacks: FileChangeCallback[] = [];
  private isWatching: boolean = false;
  private sourceRoot: string = '';

  /**
   * 创建文件监听器
   * @param config Wiki 创建器配置
   * @param sourceRoot 源代码根目录
   */
  constructor(config: WikiCreatorConfig, sourceRoot: string) {
    this.config = config;
    this.sourceRoot = path.resolve(sourceRoot);
  }

  /**
   * 开始监听文件变更
   * @param callback 文件变更回调函数
   */
  async watchFiles(callback: FileChangeCallback): Promise<void> {
    if (!this.config) {
      throw new Error('配置未设置');
    }

    if (this.isWatching) {
      console.warn('文件监听器已在运行');
      return;
    }

    this.callbacks.push(callback);

    // 构建监听路径
    const watchPaths = this.config.sourcePatterns.map(pattern => 
      path.join(this.sourceRoot, pattern)
    );

    // 创建监听器
    this.watcher = chokidar.watch(watchPaths, {
      ignored: this.config.exclusionPatterns,
      persistent: true,
      ignoreInitial: true,
      awaitWriteFinish: {
        stabilityThreshold: 100,
        pollInterval: 10
      },
      depth: 10,
      ignorePermissionErrors: true
    });

    // 绑定事件处理
    this.watcher
      .on('add', (filePath) => this.handleFileChange(filePath, 'created'))
      .on('change', (filePath) => this.handleFileChange(filePath, 'modified'))
      .on('unlink', (filePath) => this.handleFileChange(filePath, 'deleted'))
      .on('unlinkDir', (dirPath) => this.handleFileChange(dirPath, 'deleted'))
      .on('addDir', (dirPath) => this.handleFileChange(dirPath, 'created'))
      .on('error', (error) => this.handleError(error));

    this.isWatching = true;
    console.log(`文件监听器已启动，监听路径：${watchPaths.join(', ')}`);
  }

  /**
   * 停止监听文件变更
   */
  async stopWatching(): Promise<void> {
    if (!this.watcher) {
      return;
    }

    try {
      await this.watcher.close();
      this.watcher = null;
      this.isWatching = false;
      console.log('文件监听器已停止');
    } catch (error) {
      console.error('停止文件监听器失败:', error);
    }
  }

  /**
   * 检查是否正在监听
   */
  isCurrentlyWatching(): boolean {
    return this.isWatching;
  }

  /**
   * 处理文件变更事件
   */
  private async handleFileChange(filePath: string, changeType: FileChangeType): Promise<void> {
    const event: FileChangeEvent = {
      filePath: path.resolve(filePath),
      changeType,
      timestamp: new Date(),
      relativePath: path.relative(this.sourceRoot, filePath),
      extension: path.extname(filePath)
    };

    console.log(`文件变更：${changeType} - ${event.relativePath}`);

    // 调用所有回调函数
    for (const callback of this.callbacks) {
      try {
        await callback(event);
      } catch (error) {
        console.error('文件变更回调处理失败:', error);
      }
    }
  }

  /**
   * 处理错误事件
   */
  private handleError(error: Error): void {
    console.error('文件监听器错误:', error);
  }

  /**
   * 添加文件变更回调
   * @param callback 回调函数
   */
  addCallback(callback: FileChangeCallback): void {
    this.callbacks.push(callback);
  }

  /**
   * 移除文件变更回调
   * @param callback 回调函数
   */
  removeCallback(callback: FileChangeCallback): void {
    const index = this.callbacks.indexOf(callback);
    if (index !== -1) {
      this.callbacks.splice(index, 1);
    }
  }

  /**
   * 获取监听状态
   */
  getStatus(): {
    isWatching: boolean;
    watchedPaths: string[];
    callbackCount: number;
  } {
    return {
      isWatching: this.isWatching,
      watchedPaths: this.config?.sourcePatterns.map(p => path.join(this.sourceRoot, p)) || [],
      callbackCount: this.callbacks.length
    };
  }
}

/**
 * 检查文件是否匹配配置的模式
 * @param filePath 文件路径
 * @param config 配置
 */
export function matchesFilePattern(filePath: string, config: WikiCreatorConfig): boolean {
  const minimatch = require('minimatch');
  
  // 检查是否匹配源文件模式
  const matchesSource = config.sourcePatterns.some(pattern => 
    minimatch(filePath, pattern)
  );

  if (!matchesSource) {
    return false;
  }

  // 检查是否匹配排除模式
  const matchesExclusion = config.exclusionPatterns.some(pattern => 
    minimatch(filePath, pattern)
  );

  return !matchesExclusion;
}
