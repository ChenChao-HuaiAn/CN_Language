/**
 * 内容生成器模块
 * 
 * 负责根据源代码生成 Wiki 内容，支持：
 * - 结构提取：从源代码中提取函数、类、模块等结构信息
 * - 注释解析：解析源代码中的文档注释
 * - 引用生成：自动生成文件引用和行号范围
 * - 模板渲染：使用预定义模板生成 Markdown 内容
 */

import * as fs from 'fs-extra';
import * as path from 'path';
import * as Handlebars from 'handlebars';
import { WikiCreatorConfig } from './config-manager';

/**
 * 代码结构信息
 */
export interface CodeStructure {
  /** 文件路径 */
  filePath: string;
  /** 文件类型 */
  fileType: 'c' | 'h' | 'cn' | 'other';
  /** 函数列表 */
  functions: FunctionInfo[];
  /** 结构体列表 */
  structs?: StructInfo[];
  /** 枚举列表 */
  enums?: EnumInfo[];
  /** 宏定义列表 */
  macros?: MacroInfo[];
  /** 文件注释 */
  fileComment?: string;
}

/**
 * 函数信息
 */
export interface FunctionInfo {
  /** 函数名称 */
  name: string;
  /** 返回类型 */
  returnType: string;
  /** 参数列表 */
  parameters: ParameterInfo[];
  /** 起始行号 */
  startLine: number;
  /** 结束行号 */
  endLine: number;
  /** 函数注释 */
  comment?: string;
  /** 是否为静态函数 */
  isStatic: boolean;
  /** 是否为内联函数 */
  isInline: boolean;
}

/**
 * 参数信息
 */
export interface ParameterInfo {
  /** 参数名称 */
  name: string;
  /** 参数类型 */
  type: string;
}

/**
 * 结构体信息
 */
export interface StructInfo {
  /** 结构体名称 */
  name: string;
  /** 成员列表 */
  members: MemberInfo[];
  /** 起始行号 */
  startLine: number;
  /** 结束行号 */
  endLine: number;
  /** 结构体注释 */
  comment?: string;
}

/**
 * 成员信息
 */
export interface MemberInfo {
  /** 成员名称 */
  name: string;
  /** 成员类型 */
  type: string;
  /** 成员注释 */
  comment?: string;
}

/**
 * 枚举信息
 */
export interface EnumInfo {
  /** 枚举名称 */
  name: string;
  /** 枚举值列表 */
  values: EnumValueInfo[];
  /** 起始行号 */
  startLine: number;
  /** 结束行号 */
  endLine: number;
  /** 枚举注释 */
  comment?: string;
}

/**
 * 枚举值信息
 */
export interface EnumValueInfo {
  /** 枚举值名称 */
  name: string;
  /** 枚举值 */
  value?: string;
  /** 注释 */
  comment?: string;
}

/**
 * 宏定义信息
 */
export interface MacroInfo {
  /** 宏名称 */
  name: string;
  /** 宏定义内容 */
  definition: string;
  /** 起始行号 */
  startLine: number;
  /** 宏注释 */
  comment?: string;
}

/**
 * 内容生成请求
 */
export interface ContentGenerationRequest {
  /** 源文件路径 */
  sourceFile: string;
  /** 起始行号 */
  startLine?: number;
  /** 结束行号 */
  endLine?: number;
  /** 模板名称 */
  templateName?: string;
  /** 上下文数据 */
  context?: Record<string, any>;
}

/**
 * 生成的内容
 */
export interface GeneratedContent {
  /** 生成的 Markdown 内容 */
  content: string;
  /** 元数据 */
  metadata: {
    sourceFile: string;
    lineRange: string;
    generatedAt: Date;
    templateUsed: string;
    codeStructure?: CodeStructure;
  };
}

/**
 * 内容生成器类
 */
export class ContentGenerator {
  private config: WikiCreatorConfig;
  private templates: Map<string, Handlebars.TemplateDelegate> = new Map();
  private templatePaths: Map<string, string> = new Map();

  constructor(config: WikiCreatorConfig) {
    this.config = config;
  }

  /**
   * 加载模板
   * @param templateName 模板名称
   * @param templatePath 模板文件路径
   */
  async loadTemplate(templateName: string, templatePath: string): Promise<void> {
    try {
      const templateContent = await fs.readFile(templatePath, 'utf-8');
      const template = Handlebars.compile(templateContent);
      this.templates.set(templateName, template);
      this.templatePaths.set(templateName, templatePath);
      console.log(`模板已加载：${templateName}`);
    } catch (error) {
      console.error(`加载模板失败：${templateName}`, error);
      throw error;
    }
  }

  /**
   * 生成内容
   * @param request 内容生成请求
   */
  async generateContent(request: ContentGenerationRequest): Promise<GeneratedContent> {
    const templateName = request.templateName || 'default';
    const template = this.templates.get(templateName);

    if (!template) {
      throw new Error(`模板未加载：${templateName}`);
    }

    // 解析代码结构
    const codeStructure = await this.parseCodeStructure(request.sourceFile);
    
    // 构建上下文
    const context = {
      ...request.context,
      codeStructure,
      sourceFile: request.sourceFile,
      startLine: request.startLine || 1,
      endLine: request.endLine || codeStructure.functions.reduce((max, fn) => Math.max(max, fn.endLine), 0),
      generatedAt: new Date(),
      config: this.config
    };

    // 渲染模板
    const content = template(context);

    // 构建元数据
    const metadata: GeneratedContent['metadata'] = {
      sourceFile: request.sourceFile,
      lineRange: `${request.startLine || 1}-${request.endLine || 'end'}`,
      generatedAt: new Date(),
      templateUsed: templateName,
      codeStructure
    };

    return {
      content,
      metadata
    };
  }

  /**
   * 为文件生成内容
   * @param sourceFile 源文件路径
   */
  async generateContentForFile(sourceFile: string): Promise<GeneratedContent[]> {
    const results: GeneratedContent[] = [];
    
    // 为每个模板生成内容
    for (const [templateName] of this.templates) {
      try {
        const content = await this.generateContent({
          sourceFile,
          templateName
        });
        results.push(content);
      } catch (error) {
        console.error(`为文件生成内容失败 (${templateName}): ${sourceFile}`, error);
      }
    }

    return results;
  }

  /**
   * 解析代码结构
   * @param filePath 文件路径
   */
  private async parseCodeStructure(filePath: string): Promise<CodeStructure> {
    const content = await fs.readFile(filePath, 'utf-8');
    const lines = content.split('\n');
    const ext = path.extname(filePath).toLowerCase();

    const structure: CodeStructure = {
      filePath,
      fileType: ext === '.c' ? 'c' : ext === '.h' ? 'h' : ext === '.cn' ? 'cn' : 'other',
      functions: [],
      structs: [],
      enums: [],
      macros: []
    };

    // 解析函数
    structure.functions = this.parseFunctions(lines, ext);
    
    // 解析结构体
    if (ext === '.h' || ext === '.c') {
      structure.structs = this.parseStructs(lines);
      structure.enums = this.parseEnums(lines);
      structure.macros = this.parseMacros(lines);
    }

    // 解析文件注释
    structure.fileComment = this.parseFileComment(lines);

    return structure;
  }

  /**
   * 解析函数
   */
  private parseFunctions(lines: string[], ext: string): FunctionInfo[] {
    const functions: FunctionInfo[] = [];
    const functionRegex = ext === '.cn' 
      ? /^\s*函数\s+(\w+)\s*\(([^)]*)\)\s*\{?/
      : /^\s*(?:static\s+)?(?:inline\s+)?(\w+)\s+(\w+)\s*\(([^)]*)\)\s*\{?/;

    let i = 0;
    while (i < lines.length) {
      const line = lines[i];
      const match = line.match(functionRegex);
      
      if (match) {
        const func: FunctionInfo = {
          name: ext === '.cn' ? match[1] : match[2],
          returnType: ext === '.cn' ? 'void' : match[1],
          parameters: this.parseParameters(match[3]),
          startLine: i + 1,
          endLine: i + 1,
          isStatic: line.includes('static'),
          isInline: line.includes('inline')
        };

        // 查找函数结束位置
        let braceCount = 0;
        let foundOpen = false;
        for (let j = i; j < lines.length; j++) {
          if (lines[j].includes('{')) {
            braceCount++;
            foundOpen = true;
          }
          if (lines[j].includes('}')) {
            braceCount--;
            if (foundOpen && braceCount === 0) {
              func.endLine = j + 1;
              break;
            }
          }
        }

        // 查找函数注释
        func.comment = this.findCommentAbove(lines, i);

        functions.push(func);
      }
      i++;
    }

    return functions;
  }

  /**
   * 解析参数
   */
  private parseParameters(paramString: string): ParameterInfo[] {
    const params: ParameterInfo[] = [];
    
    if (!paramString || paramString.trim() === 'void' || paramString.trim() === '') {
      return params;
    }

    const paramParts = paramString.split(',');
    for (const part of paramParts) {
      const trimmed = part.trim();
      const paramMatch = trimmed.match(/(\w+)\s+(\w+)/);
      if (paramMatch) {
        params.push({
          type: paramMatch[1],
          name: paramMatch[2]
        });
      }
    }

    return params;
  }

  /**
   * 解析结构体
   */
  private parseStructs(lines: string[]): StructInfo[] {
    const structs: StructInfo[] = [];
    const structStartRegex = /^\s*struct\s+(\w+)\s*\{/;
    
    let i = 0;
    while (i < lines.length) {
      const match = lines[i].match(structStartRegex);
      if (match) {
        const struct: StructInfo = {
          name: match[1],
          members: [],
          startLine: i + 1,
          endLine: i + 1
        };

        // 查找结构体结束位置并解析成员
        let braceCount = 0;
        for (let j = i; j < lines.length; j++) {
          if (lines[j].includes('{')) {
            braceCount++;
          }
          if (lines[j].includes('}')) {
            braceCount--;
            if (braceCount === 0) {
              struct.endLine = j + 1;
              break;
            }
          }
          
          // 解析成员
          if (j > i) {
            const memberMatch = lines[j].match(/^\s*(\w+)\s+(\w+)\s*;/);
            if (memberMatch) {
              struct.members.push({
                name: memberMatch[2],
                type: memberMatch[1],
                comment: this.findCommentAbove(lines, j)
              });
            }
          }
        }

        struct.comment = this.findCommentAbove(lines, i);
        structs.push(struct);
      }
      i++;
    }

    return structs;
  }

  /**
   * 解析枚举
   */
  private parseEnums(lines: string[]): EnumInfo[] {
    const enums: EnumInfo[] = [];
    const enumStartRegex = /^\s*enum\s+(\w+)\s*\{/;
    
    let i = 0;
    while (i < lines.length) {
      const match = lines[i].match(enumStartRegex);
      if (match) {
        const enumInfo: EnumInfo = {
          name: match[1],
          values: [],
          startLine: i + 1,
          endLine: i + 1
        };

        let braceCount = 0;
        for (let j = i; j < lines.length; j++) {
          if (lines[j].includes('{')) {
            braceCount++;
          }
          if (lines[j].includes('}')) {
            braceCount--;
            if (braceCount === 0) {
              enumInfo.endLine = j + 1;
              break;
            }
          }
          
          if (j > i) {
            const valueMatch = lines[j].match(/^\s*(\w+)(?:\s*=\s*(\d+))?/);
            if (valueMatch && !lines[j].trim().startsWith('//')) {
              enumInfo.values.push({
                name: valueMatch[1],
                value: valueMatch[2],
                comment: this.findCommentAbove(lines, j)
              });
            }
          }
        }

        enumInfo.comment = this.findCommentAbove(lines, i);
        enums.push(enumInfo);
      }
      i++;
    }

    return enums;
  }

  /**
   * 解析宏定义
   */
  private parseMacros(lines: string[]): MacroInfo[] {
    const macros: MacroInfo[] = [];
    const macroRegex = /^\s*#define\s+(\w+)\s*(.*)/;
    
    for (let i = 0; i < lines.length; i++) {
      const match = lines[i].match(macroRegex);
      if (match) {
        macros.push({
          name: match[1],
          definition: match[2].trim(),
          startLine: i + 1,
          comment: this.findCommentAbove(lines, i)
        });
      }
    }

    return macros;
  }

  /**
   * 解析文件注释
   */
  private parseFileComment(lines: string[]): string {
    const commentLines: string[] = [];
    
    for (const line of lines) {
      if (line.trim().startsWith('//') || line.trim().startsWith('/*')) {
        commentLines.push(line.replace(/^\s*\/\/\s?/, '').replace(/^\s*\/\*\s?/, '').replace(/\s?\*\/$/, ''));
      } else if (commentLines.length > 0) {
        break;
      }
    }

    return commentLines.join('\n');
  }

  /**
   * 查找行上方的注释
   */
  private findCommentAbove(lines: string[], lineIndex: number): string | undefined {
    const commentLines: string[] = [];
    
    for (let i = lineIndex - 1; i >= 0; i--) {
      const line = lines[i].trim();
      if (line.startsWith('//')) {
        commentLines.unshift(line.replace(/^\/\/\s?/, ''));
      } else if (line === '') {
        continue;
      } else {
        break;
      }
    }

    return commentLines.length > 0 ? commentLines.join('\n') : undefined;
  }

  /**
   * 生成引用文本
   * @param filePath 文件路径
   * @param startLine 起始行号
   * @param endLine 结束行号
   */
  generateCitation(filePath: string, startLine: number, endLine: number): string {
    const relativePath = path.relative(process.cwd(), filePath);
    return `- [${path.basename(filePath)}](file://${relativePath}#${startLine}-${endLine})`;
  }
}
