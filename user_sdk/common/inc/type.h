/****************************************************************************
 * Copyright(c), 2001-2060, ***************************** 版权所有
 ****************************************************************************
 * 文件名称             : type.h
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰(1417)
 * 创建日期             : 2014年04月08日
 * 描述                 : ****数据类型定义
 ****************************************************************************/

#ifndef YAAN_TYPE_H_
#define YAAN_TYPE_H_
#include <stdint.h>

typedef char JA_C8;                  /* 未指定符号8位整型数据 */
typedef int8_t JA_SI8;               /* 有符号8位整型数据 */
typedef int16_t JA_SI16;             /* 有符号16位整型数据 */
typedef int32_t JA_SI32;             /* 有符号32位整型数据 */
typedef int64_t JA_SI64;             /* 有符号64位整型数据 */
typedef uint8_t JA_UI8;              /* 无符号8位整型数据 */
typedef uint16_t JA_UI16;            /* 无符号16位整型数据 */
typedef uint32_t JA_UI32;            /* 无符号32位整型数据 */
typedef uint64_t JA_UI64;            /* 无符号64位整型数据 */

typedef float JA_F32;                /* 单精度浮点数 */
typedef double JA_D64;               /* 双精度浮点数 */

typedef void JA_VOID;                /* 未定义类型 */

#ifndef __cplusplus
#define JA_BOOL     _Bool            /* 布尔类型 */
#else
typedef bool JA_BOOL;
#endif
#define JA_TRUE     true             /* 布尔类型真 */
#define JA_FALSE    false            /* 布尔类型假 */


#endif /* YAAN_TYPE_H_ */
