/**
 * 元数据管理器单元测试
 */

import * as fs from 'fs-extra';
import * as path from 'path';
import { MetadataManager, CodeSnippetMetadata } from '../../src/metadata-manager';

describe('MetadataManager', () => {
  let metadataManager: MetadataManager;
  let tempMetadataPath: string;

  beforeEach(async () => {
    metadataManager = new MetadataManager();
    tempMetadataPath = path.join(__dirname, 'temp-metadata.json');
  });

  afterEach(async () => {
    // 清理临时文件
    try {
      await fs.remove(tempMetadataPath);
    } catch (error) {
      // 忽略清理错误
    }
  });

  describe('initialize', () => {
    it('应该创建新的元数据存储', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      const exists = await fs.pathExists(tempMetadataPath);
      expect(exists).toBe(true);
      
      const metadata = await fs.readJson(tempMetadataPath);
      expect(metadata.codeSnippets).toEqual([]);
      expect(metadata.version).toBe('1.0.0');
    });

    it('应该加载现有的元数据', async () => {
      // 创建初始元数据
      const initialData = {
        codeSnippets: [
          {
            id: 'test-id',
            path: 'test/file.c',
            lineRange: '1-10',
            createdAt: new Date().toISOString(),
            modifiedAt: new Date().toISOString()
          }
        ],
        lastUpdated: new Date().toISOString(),
        version: '1.0.0'
      };
      await fs.writeJson(tempMetadataPath, initialData);
      
      await metadataManager.initialize(tempMetadataPath);
      const snippets = await metadataManager.getAllSnippets();
      
      expect(snippets).toHaveLength(1);
      expect(snippets[0].path).toBe('test/file.c');
    });
  });

  describe('addSnippet', () => {
    it('应该添加新的代码片段元数据', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      const snippet = await metadataManager.addSnippet({
        path: 'src/test/file.c',
        lineRange: '1-50'
      });
      
      expect(snippet.id).toBeDefined();
      expect(snippet.path).toBe('src/test/file.c');
      expect(snippet.lineRange).toBe('1-50');
      expect(snippet.createdAt).toBeDefined();
      expect(snippet.modifiedAt).toBeDefined();
    });

    it('应该更新已存在的代码片段', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      // 添加第一次
      const snippet1 = await metadataManager.addSnippet({
        path: 'src/test/file.c',
        lineRange: '1-50'
      });
      
      // 添加第二次（相同路径和行范围）
      const snippet2 = await metadataManager.addSnippet({
        path: 'src/test/file.c',
        lineRange: '1-50',
        contentHash: 'new-hash'
      });
      
      expect(snippet2.id).toBe(snippet1.id);
      expect(snippet2.contentHash).toBe('new-hash');
      expect(snippet2.modifiedAt).not.toBe(snippet1.modifiedAt);
    });

    it('应该在未初始化时抛出错误', async () => {
      await expect(metadataManager.addSnippet({ path: 'test.c', lineRange: '1-10' }))
        .rejects.toThrow('元数据未初始化');
    });
  });

  describe('updateSnippet', () => {
    it('应该更新代码片段元数据', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      const snippet = await metadataManager.addSnippet({
        path: 'src/test/file.c',
        lineRange: '1-50'
      });
      
      const updated = await metadataManager.updateSnippet(snippet.id, {
        wikiFiles: ['wiki/test.md']
      });
      
      expect(updated).not.toBeNull();
      expect(updated?.wikiFiles).toEqual(['wiki/test.md']);
    });

    it('应该在 ID 不存在时返回 null', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      const updated = await metadataManager.updateSnippet('non-existent-id', {
        path: 'new/path.c'
      });
      
      expect(updated).toBeNull();
    });
  });

  describe('deleteSnippet', () => {
    it('应该删除代码片段元数据', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      const snippet = await metadataManager.addSnippet({
        path: 'src/test/file.c',
        lineRange: '1-50'
      });
      
      const deleted = await metadataManager.deleteSnippet(snippet.id);
      expect(deleted).toBe(true);
      
      const snippets = await metadataManager.getAllSnippets();
      expect(snippets).toHaveLength(0);
    });

    it('应该在 ID 不存在时返回 false', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      const deleted = await metadataManager.deleteSnippet('non-existent-id');
      expect(deleted).toBe(false);
    });
  });

  describe('findSnippetsByPath', () => {
    it('应该根据文件路径查找元数据', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      await metadataManager.addSnippet({ path: 'src/test/file1.c', lineRange: '1-50' });
      await metadataManager.addSnippet({ path: 'src/test/file2.c', lineRange: '1-30' });
      await metadataManager.addSnippet({ path: 'src/test/file1.c', lineRange: '51-100' });
      
      const snippets = await metadataManager.findSnippetsByPath('src/test/file1.c');
      
      expect(snippets).toHaveLength(2);
    });
  });

  describe('findSnippetById', () => {
    it('应该根据 ID 查找元数据', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      const snippet = await metadataManager.addSnippet({
        path: 'src/test/file.c',
        lineRange: '1-50'
      });
      
      const found = await metadataManager.findSnippetById(snippet.id);
      
      expect(found).not.toBeNull();
      expect(found?.id).toBe(snippet.id);
    });

    it('应该在 ID 不存在时返回 null', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      const found = await metadataManager.findSnippetById('non-existent-id');
      
      expect(found).toBeNull();
    });
  });

  describe('getStats', () => {
    it('应该返回元数据统计信息', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      await metadataManager.addSnippet({ path: 'src/file1.c', lineRange: '1-50' });
      await metadataManager.addSnippet({ path: 'src/file2.c', lineRange: '1-30' });
      await metadataManager.addSnippet({ path: 'src/file1.c', lineRange: '51-100' });
      
      const stats = await metadataManager.getStats();
      
      expect(stats.totalSnippets).toBe(3);
      expect(stats.uniqueFiles).toBe(2);
      expect(stats.lastUpdated).toBeDefined();
    });
  });

  describe('cleanupExpired', () => {
    it('应该清理过期的元数据', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      // 添加一个 snippet
      await metadataManager.addSnippet({ path: 'src/file.c', lineRange: '1-50' });
      
      // 清理所有（maxAge=0）
      const removed = await metadataManager.cleanupExpired(0);
      
      expect(removed).toBe(1);
      
      const stats = await metadataManager.getStats();
      expect(stats.totalSnippets).toBe(0);
    });

    it('应该保留未过期的元数据', async () => {
      await metadataManager.initialize(tempMetadataPath);
      
      await metadataManager.addSnippet({ path: 'src/file.c', lineRange: '1-50' });
      
      // 清理时设置很大的 maxAge
      const removed = await metadataManager.cleanupExpired(999999999999);
      
      expect(removed).toBe(0);
      
      const stats = await metadataManager.getStats();
      expect(stats.totalSnippets).toBe(1);
    });
  });

  describe('calculateContentHash', () => {
    it('应该计算内容哈希', () => {
      const content = 'test content';
      const hash1 = metadataManager.calculateContentHash(content);
      const hash2 = metadataManager.calculateContentHash(content);
      const hash3 = metadataManager.calculateContentHash('different content');
      
      expect(hash1).toBe(hash2);
      expect(hash1).not.toBe(hash3);
    });
  });
});
