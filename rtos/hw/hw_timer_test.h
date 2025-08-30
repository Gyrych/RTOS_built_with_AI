/**
 * @file hw_timer_test.h
 * @brief 硬件定时器测试程序头文件
 * @author Assistant
 * @date 2024
 */

#ifndef __RTOS_HW_TIMER_TEST_H__
#define __RTOS_HW_TIMER_TEST_H__

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 运行所有硬件定时器测试
 * @return 测试结果
 */
rtos_result_t rtos_hw_run_timer_tests(void);

/**
 * @brief 获取测试统计信息
 * @return 测试统计信息结构体指针
 */
const void* rtos_hw_get_test_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_HW_TIMER_TEST_H__ */
