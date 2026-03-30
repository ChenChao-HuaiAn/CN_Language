/**
 * Wiki Creator - AI Coder Wiki 创建技能模块
 * 
 * 基于代码变更自动感知并动态生成或更新对应 Wiki 内容的标准化技能模块
 * 
 * @packageDocumentation
 */

export { ConfigManager, createDefaultConfig } from './config-manager';
export type {
  WikiCreatorConfig,
  SyncStrategy,
  ContentTemplates,
  ConfigValidationResult
} from './config-manager';

export { FileWatcher, matchesFilePattern } from './file-watcher';
export type {
  FileChangeEvent,
  FileChangeType,
  FileChangeCallback
} from './file-watcher';

export { ContentGenerator } from './content-generator';
export type {
  ContentGenerationRequest,
  GeneratedContent,
  CodeStructure,
  FunctionInfo,
  StructInfo,
  EnumInfo
} from './content-generator';

export { MetadataManager } from './metadata-manager';
export type {
  CodeSnippetMetadata,
  MetadataStore
} from './metadata-manager';

export { SyncCoordinator } from './sync-coordinator';
export type {
  SyncResult,
  SyncStatus,
  ConflictInfo
} from './sync-coordinator';

export { WikiCreator } from './wiki-creator';
