/**
 * @file class_cgen.h
 * @brief CN语言面向对象编程 - 类代码生成接口
 * 
 * 本文件定义了CN语言类和接口的C代码生成接口，包括：
 * - 类结构体定义生成
 * - 类成员函数生成
 * - 接口虚函数表生成
 * - 继承关系处理
 */

#ifndef CN_BACKEND_CGEN_CLASS_CGEN_H
#define CN_BACKEND_CGEN_CLASS_CGEN_H

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明 */
struct CnAstClassDecl;
struct CnAstInterfaceDecl;
struct CnCCodeGenContext;
struct CnClassMember;

/* ============================================================================
 * 类代码生成函数
 * ============================================================================ */

/**
 * @brief 生成类定义的C代码
 * 
 * 将CN语言的类定义转换为C代码，包括：
 * - 结构体定义（成员变量）
 * - 成员函数声明和定义
 * - 虚函数表（如果有虚函数）
 * - 构造函数和析构函数
 * 
 * @param ctx 代码生成上下文
 * @param class_decl 类声明AST节点
 * @return true 成功，false 失败
 */
bool cn_cgen_class_decl(struct CnCCodeGenContext *ctx, struct CnAstClassDecl *class_decl);

/**
 * @brief 生成接口定义的C代码
 * 
 * 将CN语言的接口定义转换为C代码，包括：
 * - 虚函数表结构体定义
 * - 接口方法声明
 * 
 * @param ctx 代码生成上下文
 * @param interface_decl 接口声明AST节点
 * @return true 成功，false 失败
 */
bool cn_cgen_interface_decl(struct CnCCodeGenContext *ctx, struct CnAstInterfaceDecl *interface_decl);

/**
 * @brief 生成类结构体定义
 * 
 * 生成类的C结构体定义，包含所有成员变量
 * 
 * @param ctx 代码生成上下文
 * @param class_decl 类声明AST节点
 * @return true 成功，false 失败
 */
bool cn_cgen_class_struct(struct CnCCodeGenContext *ctx, struct CnAstClassDecl *class_decl);

/**
 * @brief 生成类成员函数
 * 
 * 生成类的所有成员函数的C代码
 * 
 * @param ctx 代码生成上下文
 * @param class_decl 类声明AST节点
 * @return true 成功，false 失败
 */
bool cn_cgen_class_methods(struct CnCCodeGenContext *ctx, struct CnAstClassDecl *class_decl);

/**
 * @brief 生成虚函数表
 *
 * 为包含虚函数的类生成虚函数表结构体和实例
 *
 * @param ctx 代码生成上下文
 * @param class_decl 类声明AST节点
 * @return true 成功，false 失败
 */
bool cn_cgen_vtable(struct CnCCodeGenContext *ctx, struct CnAstClassDecl *class_decl);

/**
 * @brief 生成初始化列表代码
 *
 * 将构造函数的初始化列表转换为成员赋值语句
 * 例如：: x(0), y(0) 生成 self->x = 0; self->y = 0;
 *
 * @param ctx 代码生成上下文
 * @param class_decl 类声明AST节点
 * @param constructor 构造函数成员节点
 * @return true 成功，false 失败
 */
bool cgen_initializer_list(struct CnCCodeGenContext *ctx, struct CnAstClassDecl *class_decl,
                           struct CnClassMember *constructor);

/* ============================================================================
 * 虚函数调用代码生成函数
 * ============================================================================ */

/**
 * @brief 生成虚函数调用的C代码
 *
 * 将虚函数调用转换为通过vtable的间接调用：
 * obj->vtable->method(obj, args...)
 *
 * @param ctx 代码生成上下文
 * @param class_name 类名
 * @param class_name_len 类名长度
 * @param method_name 方法名
 * @param method_name_len 方法名长度
 * @param object_expr 对象表达式（C代码字符串）
 * @param args 参数表达式数组
 * @param arg_count 参数数量
 * @return true 成功，false 失败
 */
bool cn_cgen_virtual_call(struct CnCCodeGenContext *ctx,
                          const char *class_name, size_t class_name_len,
                          const char *method_name, size_t method_name_len,
                          const char *object_expr,
                          struct CnAstExpr **args, size_t arg_count);

/**
 * @brief 检查方法是否为虚函数
 *
 * 在类层次结构中查找方法，判断是否为虚函数
 *
 * @param class_decl 类声明AST节点
 * @param method_name 方法名
 * @param method_name_len 方法名长度
 * @param out_method 输出找到的方法成员指针（可为NULL）
 * @return true 是虚函数，false 不是虚函数或未找到
 */
bool cn_cgen_is_virtual_method(struct CnAstClassDecl *class_decl,
                                const char *method_name, size_t method_name_len,
                                struct CnClassMember **out_method);

/**
 * @brief 生成成员访问表达式的C代码（支持虚函数调用）
 *
 * 处理 obj.method(...) 和 obj->method(...) 形式的调用
 * 如果方法是虚函数，生成vtable调用；否则生成直接调用
 *
 * @param ctx 代码生成上下文
 * @param object_expr 对象表达式
 * @param member_name 成员名
 * @param member_name_len 成员名长度
 * @param is_arrow 是否为箭头访问（->）
 * @param args 参数表达式数组（可为NULL，表示非调用）
 * @param arg_count 参数数量
 * @param class_decl 对象所属类的声明（可为NULL）
 */
void cn_cgen_member_access_call(struct CnCCodeGenContext *ctx,
                                 struct CnAstExpr *object_expr,
                                 const char *member_name, size_t member_name_len,
                                 bool is_arrow,
                                 struct CnAstExpr **args, size_t arg_count,
                                 struct CnAstClassDecl *class_decl);

/* ============================================================================
 * 多继承this指针调整函数
 * ============================================================================ */

/**
 * @brief 生成基类方法调用时的this指针调整代码
 *
 * 当调用基类的方法时，需要将派生类的this指针调整为基类子对象的指针
 * 例如：C类继承A和B，调用B的方法时需要：
 *   B* b_ptr = (B*)((char*)c_ptr + B_OFFSET);
 *   b_ptr->method(b_ptr);
 *
 * @param ctx 代码生成上下文
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param method_name 方法名称
 * @param method_name_len 方法名称长度
 * @param args 参数表达式数组
 * @param arg_count 参数数量
 */
void cn_cgen_base_method_call(struct CnCCodeGenContext *ctx,
                               struct CnAstClassDecl *class_decl,
                               const char *base_name, size_t base_name_len,
                               const char *method_name, size_t method_name_len,
                               struct CnAstExpr **args, size_t arg_count);

/**
 * @brief 生成向上转型代码（派生类指针转基类指针）
 *
 * @param out 输出文件
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param expr 派生类对象表达式
 */
void cn_cgen_upcast(FILE *out, struct CnAstClassDecl *class_decl,
                    const char *base_name, size_t base_name_len,
                    const char *expr);

/**
 * @brief 生成向下转型代码（基类指针转派生类指针）
 *
 * 注意：向下转型需要运行时类型检查（RTTI），这里生成简化版本
 *
 * @param out 输出文件
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param expr 基类对象表达式
 */
void cn_cgen_downcast(FILE *out, struct CnAstClassDecl *class_decl,
                      const char *base_name, size_t base_name_len,
                      const char *expr);

/**
 * @brief 生成基类成员访问代码
 *
 * 访问基类的成员变量时，需要正确计算偏移量
 *
 * @param out 输出文件
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param member_name 成员名称
 * @param member_name_len 成员名称长度
 * @param is_arrow 是否使用箭头操作符
 */
void cn_cgen_base_member_access(FILE *out, struct CnAstClassDecl *class_decl,
                                 const char *base_name, size_t base_name_len,
                                 const char *member_name, size_t member_name_len,
                                 bool is_arrow);

#ifdef __cplusplus
}
#endif

#endif /* CN_BACKEND_CGEN_CLASS_CGEN_H */
