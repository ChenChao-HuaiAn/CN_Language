/**
 * WikiCreator 集成测试
 */

import * as fs from 'fs-extra';
import * as path from 'path';
import { WikiCreator } from '../../src/wiki-creator';

describe('WikiCreator Integration', () => {
  let wikiCreator: WikiCreator;
  let tempWikiRoot: string;
  let tempSourceRoot: string;
  let tempConfigPath: string;

  beforeAll(async () => {
    // 创建临时目录
    tempWikiRoot = path.join(__dirname, 'temp-wiki');
    tempSourceRoot = path.join(__dirname, 'temp-source');
    tempConfigPath = path.join(__dirname, 'temp-integration-config.json');

    await fs.ensureDir(tempWikiRoot);
    await fs.ensureDir(tempSourceRoot);
    await fs.ensureDir(path.join(tempWikiRoot, 'meta'));

    // 创建测试配置文件
    const config = {
      wikiRoot: tempWikiRoot,
      sourcePatterns: ['**/*.c', '**/*.h'],
      exclusionPatterns: ['**/build/**', '**/*.o'],
      contentTemplates: {
        default: path.join(__dirname, '../../templates/default.md'),
        architecture: path.join(__dirname, '../../templates/architecture.md'),
        api: path.join(__dirname, '../../templates/api.md')
      },
      syncStrategy: 'manual' as const,
      metadataFile: 'meta/repowiki-metadata.json',
      generateCitations: true,
      generateMermaidDiagrams: true,
      maxLineLength: 100,
      indentSize: 2
    };

    await fs.writeJson(tempConfigPath, config, { spaces: 2 });
  });

  afterAll(async () => {
    // 清理临时目录
    try {
      await fs.remove(tempWikiRoot);
      await fs.remove(tempSourceRoot);
      await fs.remove(tempConfigPath);
    } catch (error) {
      // 忽略清理错误
    }
  });

  beforeEach(async () => {
    // 创建测试源文件
    const testSourceFile = path.join(tempSourceRoot, 'test_function.c');
    await fs.writeFile(testSourceFile, `
// 测试文件 - 一个简单的函数示例

#include <stdio.h>

/**
 * 计算两个整数的和
 * @param a 第一个整数
 * @param b 第二个整数
 * @return 两数之和
 */
int add(int a, int b) {
    return a + b;
}

/**
 * 主函数
 * @return 程序执行状态
 */
int main() {
    int result = add(3, 5);
    printf("Result: %d\\n", result);
    return 0;
}
`, 'utf-8');

    // 创建测试头文件
    const testHeaderFile = path.join(tempSourceRoot, 'test_types.h');
    await fs.writeFile(testHeaderFile, `
#ifndef TEST_TYPES_H
#define TEST_TYPES_H

// 测试结构体
typedef struct {
    int x;
    int y;
} Point;

// 测试枚举
typedef enum {
    RED,
    GREEN,
    BLUE
} Color;

// 测试宏
#define MAX_VALUE 100
#define MIN_VALUE 0

#endif
`, 'utf-8');
  });

  afterEach(async () => {
    // 清理测试文件
    const files = await fs.readdir(tempSourceRoot);
    for (const file of files) {
      await fs.remove(path.join(tempSourceRoot, file));
    }
  });

  describe('initialize', () => {
    it('应该成功初始化 WikiCreator', async () => {
      wikiCreator = new WikiCreator({
        wikiRoot: tempWikiRoot,
        sourceRoot: tempSourceRoot,
        configFile: tempConfigPath
      });

      await expect(wikiCreator.initialize()).resolves.not.toThrow();
      await wikiCreator.dispose();
    });
  });

  describe('generateContent', () => {
    beforeEach(async () => {
      wikiCreator = new WikiCreator({
        wikiRoot: tempWikiRoot,
        sourceRoot: tempSourceRoot,
        configFile: tempConfigPath
      });
      await wikiCreator.initialize();
    });

    afterEach(async () => {
      await wikiCreator.dispose();
    });

    it('应该为 C 文件生成内容', async () => {
      const sourceFile = path.join(tempSourceRoot, 'test_function.c');
      
      const content = await wikiCreator.generateContent({
        sourceFile,
        templateName: 'default'
      });

      expect(content.content).toBeDefined();
      expect(content.content).toContain('test_function.c');
      expect(content.content).toContain('add');
      expect(content.content).toContain('main');
      expect(content.metadata.sourceFile).toBe(sourceFile);
    });

    it('应该为头文件生成内容', async () => {
      const sourceFile = path.join(tempSourceRoot, 'test_types.h');
      
      const content = await wikiCreator.generateContent({
        sourceFile,
        templateName: 'default'
      });

      expect(content.content).toBeDefined();
      expect(content.content).toContain('test_types.h');
      expect(content.content).toContain('Point');
      expect(content.content).toContain('Color');
      expect(content.content).toContain('MAX_VALUE');
    });

    it('应该使用 architecture 模板生成内容', async () => {
      const sourceFile = path.join(tempSourceRoot, 'test_function.c');
      
      const content = await wikiCreator.generateContent({
        sourceFile,
        templateName: 'architecture'
      });

      expect(content.content).toBeDefined();
      expect(content.content).toContain('mermaid');
    });

    it('应该使用 api 模板生成内容', async () => {
      const sourceFile = path.join(tempSourceRoot, 'test_function.c');
      
      const content = await wikiCreator.generateContent({
        sourceFile,
        templateName: 'api'
      });

      expect(content.content).toBeDefined();
      expect(content.content).toContain('API 参考');
    });
  });

  describe('syncFile', () => {
    beforeEach(async () => {
      wikiCreator = new WikiCreator({
        wikiRoot: tempWikiRoot,
        sourceRoot: tempSourceRoot,
        configFile: tempConfigPath
      });
      await wikiCreator.initialize();
    });

    afterEach(async () => {
      await wikiCreator.dispose();
    });

    it('应该同步单个文件', async () => {
      const sourceFile = path.join(tempSourceRoot, 'test_function.c');
      
      const result = await wikiCreator.syncFile(sourceFile);

      expect(result.success).toBe(true);
      expect(result.processedFiles).toBeGreaterThanOrEqual(0);
    });
  });

  describe('syncAll', () => {
    beforeEach(async () => {
      wikiCreator = new WikiCreator({
        wikiRoot: tempWikiRoot,
        sourceRoot: tempSourceRoot,
        configFile: tempConfigPath
      });
      await wikiCreator.initialize();
    });

    afterEach(async () => {
      await wikiCreator.dispose();
    });

    it('应该同步所有文件', async () => {
      const result = await wikiCreator.syncAll();

      expect(result.success).toBe(true);
      expect(result.processedFiles).toBeGreaterThanOrEqual(2);
    });

    it('应该生成元数据文件', async () => {
      await wikiCreator.syncAll();

      const metadataPath = path.join(tempWikiRoot, 'meta', 'repowiki-metadata.json');
      const metadataExists = await fs.pathExists(metadataPath);
      
      expect(metadataExists).toBe(true);
    });
  });

  describe('getSyncStatus', () => {
    beforeEach(async () => {
      wikiCreator = new WikiCreator({
        wikiRoot: tempWikiRoot,
        sourceRoot: tempSourceRoot,
        configFile: tempConfigPath
      });
      await wikiCreator.initialize();
    });

    afterEach(async () => {
      await wikiCreator.dispose();
    });

    it('应该返回同步状态', async () => {
      const status = await wikiCreator.getSyncStatus();

      expect(status.lastSync).toBeDefined();
      expect(status.pendingChanges).toBeDefined();
      expect(status.syncingFiles).toBeDefined();
      expect(status.syncQueue).toBeDefined();
    });
  });

  describe('validateConfig', () => {
    beforeEach(async () => {
      wikiCreator = new WikiCreator({
        wikiRoot: tempWikiRoot,
        sourceRoot: tempSourceRoot,
        configFile: tempConfigPath
      });
      await wikiCreator.initialize();
    });

    afterEach(async () => {
      await wikiCreator.dispose();
    });

    it('应该验证配置', async () => {
      const result = await wikiCreator.validateConfig();

      expect(result.valid).toBeDefined();
      expect(result.errors).toBeDefined();
      expect(result.warnings).toBeDefined();
    });
  });
});
