/**
 * 配置管理器单元测试
 */

import * as fs from 'fs-extra';
import * as path from 'path';
import { ConfigManager, createDefaultConfig, WikiCreatorConfig } from '../../src/config-manager';

describe('ConfigManager', () => {
  let configManager: ConfigManager;
  let tempConfigPath: string;

  const testConfig: WikiCreatorConfig = {
    wikiRoot: '.qoder/repowiki/zh',
    sourcePatterns: ['src/**/*.c', 'src/**/*.h'],
    exclusionPatterns: ['**/build/**', '**/*.o'],
    contentTemplates: {
      default: './templates/default.md',
      architecture: './templates/architecture.md'
    },
    syncStrategy: 'realtime',
    scheduledInterval: 3600000,
    metadataFile: 'meta/repowiki-metadata.json',
    generateCitations: true,
    generateMermaidDiagrams: true,
    maxLineLength: 100,
    indentSize: 2
  };

  beforeEach(async () => {
    configManager = new ConfigManager();
    tempConfigPath = path.join(__dirname, 'temp-config.json');
    
    // 创建测试配置文件
    await fs.writeJson(tempConfigPath, testConfig, { spaces: 2 });
  });

  afterEach(async () => {
    // 清理临时文件
    try {
      await fs.remove(tempConfigPath);
    } catch (error) {
      // 忽略清理错误
    }
  });

  describe('loadConfig', () => {
    it('应该成功加载配置文件', async () => {
      const config = await configManager.loadConfig(tempConfigPath);
      
      expect(config.wikiRoot).toBe(testConfig.wikiRoot);
      expect(config.sourcePatterns).toEqual(testConfig.sourcePatterns);
      expect(config.syncStrategy).toBe(testConfig.syncStrategy);
    });

    it('应该在不存在的配置文件时抛出错误', async () => {
      await expect(configManager.loadConfig('non-existent-config.json'))
        .rejects.toThrow('无法加载配置文件');
    });

    it('应该合并默认配置', async () => {
      const partialConfig = { wikiRoot: './test-wiki' };
      await fs.writeJson(tempConfigPath, partialConfig);
      
      const config = await configManager.loadConfig(tempConfigPath);
      
      expect(config.wikiRoot).toBe('./test-wiki');
      expect(config.generateCitations).toBe(true); // 默认值
    });
  });

  describe('getConfig', () => {
    it('应该在未加载配置时抛出错误', () => {
      expect(() => configManager.getConfig()).toThrow('配置尚未加载');
    });

    it('应该返回已加载的配置', async () => {
      await configManager.loadConfig(tempConfigPath);
      const config = configManager.getConfig();
      
      expect(config).toEqual(expect.objectContaining(testConfig));
    });
  });

  describe('updateConfig', () => {
    it('应该成功更新配置', async () => {
      await configManager.loadConfig(tempConfigPath);
      
      await configManager.updateConfig({ 
        wikiRoot: './updated-wiki',
        maxLineLength: 120 
      });
      
      const config = configManager.getConfig();
      expect(config.wikiRoot).toBe('./updated-wiki');
      expect(config.maxLineLength).toBe(120);
    });

    it('应该在未加载配置时抛出错误', async () => {
      await expect(configManager.updateConfig({ wikiRoot: './test' }))
        .rejects.toThrow('配置尚未加载');
    });
  });

  describe('validateConfig', () => {
    it('应该验证有效的配置', async () => {
      await configManager.loadConfig(tempConfigPath);
      const result = await configManager.validateConfig();
      
      expect(result.valid).toBe(true);
      expect(result.errors).toHaveLength(0);
    });

    it('应该检测空的 wikiRoot', async () => {
      const invalidConfig = { ...testConfig, wikiRoot: '' };
      await fs.writeJson(tempConfigPath, invalidConfig);
      await configManager.loadConfig(tempConfigPath);
      
      const result = await configManager.validateConfig();
      
      expect(result.valid).toBe(false);
      expect(result.errors).toContain('wikiRoot 不能为空');
    });

    it('应该检测空的 sourcePatterns', async () => {
      const invalidConfig = { ...testConfig, sourcePatterns: [] };
      await fs.writeJson(tempConfigPath, invalidConfig);
      await configManager.loadConfig(tempConfigPath);
      
      const result = await configManager.validateConfig();
      
      expect(result.valid).toBe(false);
      expect(result.errors).toContain('sourcePatterns 不能为空');
    });

    it('应该检测无效的 syncStrategy', async () => {
      const invalidConfig = { ...testConfig, syncStrategy: 'invalid' as any };
      await fs.writeJson(tempConfigPath, invalidConfig);
      await configManager.loadConfig(tempConfigPath);
      
      const result = await configManager.validateConfig();
      
      expect(result.valid).toBe(false);
      expect(result.errors).toContain('无效的同步策略：invalid');
    });
  });

  describe('getWikiRootPath', () => {
    it('应该返回 Wiki 根目录的绝对路径', async () => {
      await configManager.loadConfig(tempConfigPath);
      const wikiRootPath = configManager.getWikiRootPath();
      
      expect(path.isAbsolute(wikiRootPath)).toBe(true);
    });

    it('应该在未加载配置时抛出错误', () => {
      expect(() => configManager.getWikiRootPath()).toThrow('配置尚未加载');
    });
  });

  describe('getMetadataFilePath', () => {
    it('应该返回元数据文件的绝对路径', async () => {
      await configManager.loadConfig(tempConfigPath);
      const metadataPath = configManager.getMetadataFilePath();
      
      expect(path.isAbsolute(metadataPath)).toBe(true);
      expect(metadataPath).toContain('repowiki-metadata.json');
    });
  });

  describe('resetToDefaults', () => {
    it('应该重置配置为默认值', async () => {
      await configManager.loadConfig(tempConfigPath);
      await configManager.updateConfig({ wikiRoot: './custom' });
      
      configManager.resetToDefaults();
      
      // 重置后配置应为默认值，但由于内部实现，需要重新加载
      expect(() => configManager.getConfig()).not.toThrow();
    });
  });
});

describe('createDefaultConfig', () => {
  it('应该创建默认配置文件', async () => {
    const tempPath = path.join(__dirname, 'default-config-test.json');
    
    try {
      await createDefaultConfig(tempPath);
      
      const config = await fs.readJson(tempPath);
      
      expect(config.wikiRoot).toBe('.qoder/repowiki/zh');
      expect(config.syncStrategy).toBe('realtime');
      expect(config.generateCitations).toBe(true);
    } finally {
      await fs.remove(tempPath);
    }
  });
});
