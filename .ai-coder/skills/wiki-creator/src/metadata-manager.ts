/**
 * 元数据管理器模块
 * 
 * 负责管理代码片段引用的元数据，支持：
 * - 添加、更新、删除代码片段元数据
 * - 持久化存储到 JSON 文件
 * - 查询和检索元数据
 */

import * as fs from 'fs-extra';
import * as path from 'path';
import * as crypto from 'crypto';

/**
 * 代码片段元数据
 */
export interface CodeSnippetMetadata {
  /** 唯一标识符 */
  id: string;
  /** 文件路径（相对路径） */
  path: string;
  /** 行号范围 */
  lineRange: string;
  /** 创建时间 */
  createdAt: string;
  /** 修改时间 */
  modifiedAt: string;
  /** 关联的 Wiki 文件 */
  wikiFiles?: string[];
  /** 代码结构哈希（用于检测变更） */
  contentHash?: string;
}

/**
 * 元数据存储结构
 */
export interface MetadataStore {
  codeSnippets: CodeSnippetMetadata[];
  lastUpdated: string;
  version: string;
}

/**
 * 元数据管理器类
 */
export class MetadataManager {
  private metadataPath: string | null = null;
  private metadata: MetadataStore | null = null;
  private readonly VERSION = '1.0.0';

  /**
   * 创建元数据管理器
   * @param metadataPath 元数据文件路径
   */
  constructor(metadataPath?: string) {
    if (metadataPath) {
      this.metadataPath = metadataPath;
    }
  }

  /**
   * 初始化元数据管理器
   * @param metadataPath 元数据文件路径
   */
  async initialize(metadataPath: string): Promise<void> {
    this.metadataPath = metadataPath;
    
    try {
      // 尝试加载现有元数据
      await this.loadMetadata();
    } catch (error) {
      // 如果文件不存在，创建新的元数据存储
      console.log('创建新的元数据存储');
      await this.createMetadata();
    }
  }

  /**
   * 加载元数据
   */
  async loadMetadata(): Promise<MetadataStore> {
    if (!this.metadataPath) {
      throw new Error('元数据文件路径未设置');
    }

    try {
      const data = await fs.readJson(this.metadataPath);
      this.metadata = data;
      return this.metadata;
    } catch (error) {
      throw new Error(`无法加载元数据文件：${this.metadataPath}`);
    }
  }

  /**
   * 创建新的元数据存储
   */
  async createMetadata(): Promise<void> {
    if (!this.metadataPath) {
      throw new Error('元数据文件路径未设置');
    }

    this.metadata = {
      codeSnippets: [],
      lastUpdated: new Date().toISOString(),
      version: this.VERSION
    };

    await this.saveMetadata();
  }

  /**
   * 保存元数据
   */
  async saveMetadata(): Promise<void> {
    if (!this.metadataPath || !this.metadata) {
      throw new Error('元数据未初始化');
    }

    // 确保目录存在
    const dir = path.dirname(this.metadataPath);
    await fs.ensureDir(dir);

    // 更新最后更新时间
    this.metadata.lastUpdated = new Date().toISOString();

    // 写入文件
    await fs.writeJson(this.metadataPath, this.metadata, { spaces: 2 });
  }

  /**
   * 添加代码片段元数据
   * @param snippet 代码片段元数据
   */
  async addSnippet(snippet: Omit<CodeSnippetMetadata, 'id' | 'createdAt' | 'modifiedAt'>): Promise<CodeSnippetMetadata> {
    if (!this.metadata) {
      throw new Error('元数据未初始化');
    }

    const now = new Date().toISOString();
    const newSnippet: CodeSnippetMetadata = {
      ...snippet,
      id: this.generateId(snippet.path, snippet.lineRange),
      createdAt: now,
      modifiedAt: now
    };

    // 检查是否已存在
    const existingIndex = this.metadata.codeSnippets.findIndex(
      s => s.path === snippet.path && s.lineRange === snippet.lineRange
    );

    if (existingIndex !== -1) {
      // 更新现有元数据
      this.metadata.codeSnippets[existingIndex] = {
        ...this.metadata.codeSnippets[existingIndex],
        ...newSnippet,
        createdAt: this.metadata.codeSnippets[existingIndex].createdAt,
        modifiedAt: now
      };
    } else {
      // 添加新元数据
      this.metadata.codeSnippets.push(newSnippet);
    }

    await this.saveMetadata();
    return newSnippet;
  }

  /**
   * 更新代码片段元数据
   * @param id 元数据 ID
   * @param updates 更新内容
   */
  async updateSnippet(id: string, updates: Partial<CodeSnippetMetadata>): Promise<CodeSnippetMetadata | null> {
    if (!this.metadata) {
      throw new Error('元数据未初始化');
    }

    const index = this.metadata.codeSnippets.findIndex(s => s.id === id);
    if (index === -1) {
      return null;
    }

    this.metadata.codeSnippets[index] = {
      ...this.metadata.codeSnippets[index],
      ...updates,
      id, // 不允许修改 ID
      createdAt: this.metadata.codeSnippets[index].createdAt,
      modifiedAt: new Date().toISOString()
    };

    await this.saveMetadata();
    return this.metadata.codeSnippets[index];
  }

  /**
   * 删除代码片段元数据
   * @param id 元数据 ID
   */
  async deleteSnippet(id: string): Promise<boolean> {
    if (!this.metadata) {
      throw new Error('元数据未初始化');
    }

    const index = this.metadata.codeSnippets.findIndex(s => s.id === id);
    if (index === -1) {
      return false;
    }

    this.metadata.codeSnippets.splice(index, 1);
    await this.saveMetadata();
    return true;
  }

  /**
   * 根据文件路径查找元数据
   * @param filePath 文件路径
   */
  async findSnippetsByPath(filePath: string): Promise<CodeSnippetMetadata[]> {
    if (!this.metadata) {
      throw new Error('元数据未初始化');
    }

    return this.metadata.codeSnippets.filter(s => s.path === filePath);
  }

  /**
   * 根据 ID 查找元数据
   * @param id 元数据 ID
   */
  async findSnippetById(id: string): Promise<CodeSnippetMetadata | null> {
    if (!this.metadata) {
      throw new Error('元数据未初始化');
    }

    return this.metadata.codeSnippets.find(s => s.id === id) || null;
  }

  /**
   * 获取所有元数据
   */
  async getAllSnippets(): Promise<CodeSnippetMetadata[]> {
    if (!this.metadata) {
      throw new Error('元数据未初始化');
    }

    return [...this.metadata.codeSnippets];
  }

  /**
   * 获取元数据统计信息
   */
  async getStats(): Promise<{
    totalSnippets: number;
    uniqueFiles: number;
    lastUpdated: string;
  }> {
    if (!this.metadata) {
      throw new Error('元数据未初始化');
    }

    const uniqueFiles = new Set(this.metadata.codeSnippets.map(s => s.path));
    
    return {
      totalSnippets: this.metadata.codeSnippets.length,
      uniqueFiles: uniqueFiles.size,
      lastUpdated: this.metadata.lastUpdated
    };
  }

  /**
   * 清理过期的元数据
   * @param maxAge 最大年龄（毫秒）
   */
  async cleanupExpired(maxAge: number): Promise<number> {
    if (!this.metadata) {
      throw new Error('元数据未初始化');
    }

    const now = Date.now();
    const initialCount = this.metadata.codeSnippets.length;

    this.metadata.codeSnippets = this.metadata.codeSnippets.filter(snippet => {
      const modifiedTime = new Date(snippet.modifiedAt).getTime();
      return (now - modifiedTime) < maxAge;
    });

    const removedCount = initialCount - this.metadata.codeSnippets.length;
    
    if (removedCount > 0) {
      await this.saveMetadata();
    }

    return removedCount;
  }

  /**
   * 生成唯一 ID
   */
  private generateId(filePath: string, lineRange: string): string {
    const content = `${filePath}:${lineRange}`;
    return crypto.createHash('md5').update(content).digest('hex');
  }

  /**
   * 计算内容哈希
   * @param content 文件内容
   */
  calculateContentHash(content: string): string {
    return crypto.createHash('md5').update(content).digest('hex');
  }
}
