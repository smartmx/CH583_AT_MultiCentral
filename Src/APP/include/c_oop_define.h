/*
 * Copyright (c) 2023, smartmx <smartmx@qq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-05-03     smartmx      the first version
 *
 */
#ifndef _C_OOP_DEFINE_H_
#define _C_OOP_DEFINE_H_

typedef struct __oop_variable_struct
{
    void (*set)(void *value);
    void (*get)(void *value);
} oop_variable_t;

#define OOP_VARIABLE_DEFINE(TYPE, NAME, ...)                    \
    static void oop_##NAME(void *set, void *get)                \
    {                                                           \
        static TYPE oop_##NAME = __VA_ARGS__;                   \
        if(set != NULL)                                         \
        {                                                       \
            oop_##NAME = *((TYPE *)(set));                      \
        }                                                       \
        else if(get != NULL)                                    \
        {                                                       \
            *((TYPE *)(get)) = oop_##NAME;                      \
        }                                                       \
    }                                                           \
    static void set_##NAME(void *set)                           \
    {                                                           \
        oop_##NAME(set, NULL);                                  \
    }                                                           \
    static void get_##NAME(void *get)                           \
    {                                                           \
        oop_##NAME(NULL, get);                                  \
    }                                                           \
    const oop_variable_t NAME =                                 \
    {                                                           \
        set_##NAME,                                             \
        get_##NAME,                                             \
    };

#define OOP_VARIABLE_EXTERN(NAME)       extern const oop_variable_t NAME;

#endif /* _C_OOP_DEFINE_H_ */
