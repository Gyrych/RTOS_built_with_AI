/**
 * @file object_test.c
 * @brief RTOS对象模块功能测试
 * @author Assistant
 * @date 2024
 */

#include "object.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 模拟系统滴答计数 */
static uint32_t g_mock_tick_count = 0;

/* 模拟rtos_task结构体 - 简化版本用于测试 */
struct rtos_task {
    char name[16];
    uint32_t priority;
    uint32_t state;
    void *stack;
    uint32_t stack_size;
};

uint32_t rtos_system_get_tick_count(void)
{
    return g_mock_tick_count;
}

void rtos_task_add_to_ready_queue_from_wait(struct rtos_task *task)
{
    /* 模拟任务唤醒 */
    (void)task;
}

/* 测试辅助函数 */
void print_test_result(const char *test_name, bool passed)
{
    printf("[%s] %s\n", passed ? "PASS" : "FAIL", test_name);
}

/* 测试用例 */
void test_object_basic_operations(void)
{
    printf("\n=== 测试基本对象操作 ===\n");
    
    rtos_object_t obj;
    bool result = true;
    
    /* 测试对象初始化 */
    rtos_object_init(&obj, RTOS_OBJECT_TYPE_TASK, "TestTask", RTOS_OBJECT_FLAG_DYNAMIC);
    result &= (strcmp(obj.name, "TestTask") == 0);
    result &= (obj.type == RTOS_OBJECT_TYPE_TASK);
    result &= (obj.flags == RTOS_OBJECT_FLAG_DYNAMIC);
    result &= (obj.ref_count == 1);
    result &= (obj.create_time > 0);
    print_test_result("对象初始化", result);
    
    /* 测试名称设置和获取 */
    rtos_object_set_name(&obj, "NewTaskName");
    result = (strcmp(rtos_object_get_name(&obj), "NewTaskName") == 0);
    print_test_result("名称设置和获取", result);
    
    /* 测试类型获取 */
    result = (rtos_object_get_type(&obj) == RTOS_OBJECT_TYPE_TASK);
    print_test_result("类型获取", result);
    
    /* 测试标志设置和获取 */
    rtos_object_set_flags(&obj, RTOS_OBJECT_FLAG_STATIC);
    result = (rtos_object_get_flags(&obj) == RTOS_OBJECT_FLAG_STATIC);
    print_test_result("标志设置和获取", result);
    
    /* 测试对象类型检查 */
    result = rtos_object_is_dynamic(&obj);
    print_test_result("动态对象检查（应为false）", !result);
    
    rtos_object_set_flags(&obj, RTOS_OBJECT_FLAG_DYNAMIC);
    result = rtos_object_is_dynamic(&obj);
    print_test_result("动态对象检查（应为true）", result);
    
    result = rtos_object_is_static(&obj);
    print_test_result("静态对象检查（应为false）", !result);
    
    rtos_object_set_flags(&obj, RTOS_OBJECT_FLAG_SYSTEM);
    result = rtos_object_is_system(&obj);
    print_test_result("系统对象检查", result);
    
    printf("基本对象操作测试完成\n");
}

void test_object_time_operations(void)
{
    printf("\n=== 测试对象时间操作 ===\n");
    
    rtos_object_t obj;
    bool result = true;
    
    /* 设置模拟时间 */
    g_mock_tick_count = 1000;
    rtos_object_init(&obj, RTOS_OBJECT_TYPE_SEMAPHORE, "TimeTest", RTOS_OBJECT_FLAG_NONE);
    
    /* 测试创建时间获取 */
    rtos_time_ns_t create_time = rtos_object_get_create_time(&obj);
    result = (create_time > 0);
    print_test_result("创建时间获取", result);
    
    /* 测试对象年龄计算 */
    g_mock_tick_count = 2000; /* 增加1秒 */
    rtos_time_ns_t age = rtos_object_get_age(&obj);
    result = (age > 0);
    print_test_result("对象年龄计算", result);
    
    printf("对象时间操作测试完成\n");
}

void test_reference_counting(void)
{
    printf("\n=== 测试引用计数管理 ===\n");
    
    rtos_object_t obj;
    bool result = true;
    
    rtos_object_init(&obj, RTOS_OBJECT_TYPE_MUTEX, "RefTest", RTOS_OBJECT_FLAG_NONE);
    
    /* 测试初始引用计数 */
    result = (rtos_object_get_ref_count(&obj) == 1);
    print_test_result("初始引用计数", result);
    
    /* 测试引用计数增加 */
    uint32_t ref_count = rtos_object_ref_inc(&obj);
    result = (ref_count == 2);
    print_test_result("引用计数增加", result);
    
    /* 测试引用计数减少 */
    ref_count = rtos_object_ref_dec(&obj);
    result = (ref_count == 1);
    print_test_result("引用计数减少", result);
    
    /* 测试可销毁检查 */
    result = !rtos_object_can_destroy(&obj);
    print_test_result("可销毁检查（应为false）", result);
    
    rtos_object_ref_dec(&obj);
    result = rtos_object_can_destroy(&obj);
    print_test_result("可销毁检查（应为true）", result);
    
    printf("引用计数管理测试完成\n");
}

void test_object_container(void)
{
    printf("\n=== 测试对象容器管理 ===\n");
    
    rtos_object_information_t container;
    rtos_object_t obj1, obj2, obj3;
    bool result = true;
    
    /* 初始化容器 */
    rtos_object_container_init(&container, RTOS_OBJECT_TYPE_QUEUE, 10);
    result = (container.count == 0);
    result &= (container.max_count == 10);
    result &= (container.first == NULL);
    result &= (container.last == NULL);
    print_test_result("容器初始化", result);
    
    /* 测试容器状态检查 */
    result = !rtos_object_container_is_full(&container);
    print_test_result("容器满状态检查（应为false）", result);
    
    /* 添加对象到容器 */
    rtos_object_init(&obj1, RTOS_OBJECT_TYPE_QUEUE, "Queue1", RTOS_OBJECT_FLAG_NONE);
    rtos_object_init(&obj2, RTOS_OBJECT_TYPE_QUEUE, "Queue2", RTOS_OBJECT_FLAG_NONE);
    rtos_object_init(&obj3, RTOS_OBJECT_TYPE_QUEUE, "Queue3", RTOS_OBJECT_FLAG_NONE);
    
    rtos_result_t add_result = rtos_object_container_add(&container, &obj1);
    result = (add_result == RTOS_OK);
    result &= (container.count == 1);
    print_test_result("添加第一个对象", result);
    
    add_result = rtos_object_container_add(&container, &obj2);
    result = (add_result == RTOS_OK);
    result &= (container.count == 2);
    print_test_result("添加第二个对象", result);
    
    /* 测试容器统计信息 */
    uint32_t count = rtos_object_container_get_count(&container);
    result = (count == 2);
    print_test_result("容器计数获取", result);
    
    uint32_t max_count = rtos_object_container_get_max_count(&container);
    result = (max_count == 10);
    print_test_result("容器最大容量获取", result);
    
    /* 测试对象查找 */
    rtos_object_t *found = rtos_object_container_find(&container, "Queue1");
    result = (found == &obj1);
    print_test_result("对象查找（存在）", result);
    
    found = rtos_object_container_find(&container, "NonExistent");
    result = (found == NULL);
    print_test_result("对象查找（不存在）", result);
    
    /* 测试对象移除 */
    rtos_result_t remove_result = rtos_object_container_remove(&container, &obj1);
    result = (remove_result == RTOS_OK);
    result &= (container.count == 1);
    print_test_result("对象移除", result);
    
    /* 测试容器清空 */
    rtos_result_t clear_result = rtos_object_container_clear(&container);
    result = (clear_result == RTOS_OK);
    result &= (container.count == 0);
    print_test_result("容器清空", result);
    
    printf("对象容器管理测试完成\n");
}

void test_wait_queue(void)
{
    printf("\n=== 测试等待队列管理 ===\n");
    
    rtos_wait_node_t wait_queue;
    bool result = true;
    
    /* 初始化等待队列 */
    rtos_wait_queue_init(&wait_queue);
    result = rtos_wait_queue_is_empty(&wait_queue);
    print_test_result("等待队列初始化", result);
    
    /* 模拟任务结构 */
    struct rtos_task mock_task1 = {"Task1", 5, 0, NULL, 1024};
    struct rtos_task mock_task2 = {"Task2", 6, 0, NULL, 1024};
    
    /* 测试添加任务到等待队列 */
    rtos_result_t add_result = rtos_wait_queue_add(&wait_queue, &mock_task1, 1000);
    result = (add_result == RTOS_OK);
    result &= !rtos_wait_queue_is_empty(&wait_queue);
    result &= (rtos_wait_queue_get_length(&wait_queue) == 1);
    print_test_result("添加任务到等待队列", result);
    
    /* 测试添加带数据的任务 */
    add_result = rtos_wait_queue_add_with_data(&wait_queue, &mock_task2, 2000, (void*)0x12345678, RTOS_WAIT_FLAG_ALL);
    result = (add_result == RTOS_OK);
    result &= (rtos_wait_queue_get_length(&wait_queue) == 2);
    print_test_result("添加带数据的任务", result);
    
    /* 测试获取第一个任务 */
    struct rtos_task *first_task = rtos_wait_queue_get_first(&wait_queue);
    result = (first_task == &mock_task1);
    print_test_result("获取第一个任务", result);
    
    /* 测试任务移除 */
    rtos_result_t remove_result = rtos_wait_queue_remove(&wait_queue, &mock_task1);
    result = (remove_result == RTOS_OK);
    result &= (rtos_wait_queue_get_length(&wait_queue) == 1);
    print_test_result("任务移除", result);
    
    /* 测试等待队列清空 */
    rtos_result_t clear_result = rtos_wait_queue_clear(&wait_queue);
    result = (clear_result == RTOS_OK);
    result &= rtos_wait_queue_is_empty(&wait_queue);
    result &= (rtos_wait_queue_get_length(&wait_queue) == 0);
    print_test_result("等待队列清空", result);
    
    printf("等待队列管理测试完成\n");
}

void test_wait_node(void)
{
    printf("\n=== 测试等待节点操作 ===\n");
    
    rtos_wait_node_t node;
    struct rtos_task mock_task = {"TestTask", 5, 0, NULL, 1024};
    bool result = true;
    
    /* 测试等待节点初始化 */
    rtos_wait_node_init(&node, &mock_task, (void*)0x12345678, RTOS_WAIT_FLAG_ALL);
    result = (node.task == &mock_task);
    result &= (node.data == (void*)0x12345678);
    result &= (node.flags == RTOS_WAIT_FLAG_ALL);
    print_test_result("等待节点初始化", result);
    
    /* 测试等待节点数据设置和获取 */
    rtos_wait_node_set_data(&node, (void*)0x87654321);
    result = (rtos_wait_node_get_data(&node) == (void*)0x87654321);
    print_test_result("等待节点数据设置和获取", result);
    
    /* 测试等待节点标志设置和获取 */
    rtos_wait_node_set_flags(&node, RTOS_WAIT_FLAG_CLEAR_ON_EXIT);
    result = (rtos_wait_node_get_flags(&node) == RTOS_WAIT_FLAG_CLEAR_ON_EXIT);
    print_test_result("等待节点标志设置和获取", result);
    
    /* 测试等待节点任务获取 */
    result = (rtos_wait_node_get_task(&node) == &mock_task);
    print_test_result("等待节点任务获取", result);
    
    printf("等待节点操作测试完成\n");
}

void test_object_system(void)
{
    printf("\n=== 测试对象系统管理 ===\n");
    
    bool result = true;
    
    /* 测试对象系统初始化 */
    rtos_object_system_init();
    print_test_result("对象系统初始化", true);
    
    /* 测试系统时钟频率设置和获取 */
    rtos_object_set_system_clock_freq(200000000); /* 200MHz */
    uint32_t freq = rtos_object_get_system_clock_freq();
    result = (freq == 200000000);
    print_test_result("系统时钟频率设置和获取", result);
    
    /* 测试获取对象容器 */
    rtos_object_information_t *container = rtos_object_get_container(RTOS_OBJECT_TYPE_TASK);
    result = (container != NULL);
    result &= (container->type == RTOS_OBJECT_TYPE_TASK);
    print_test_result("获取对象容器", result);
    
    /* 测试获取全局对象链表头 */
    rtos_object_t *list_head = rtos_object_get_list_head();
    result = (list_head != NULL);
    print_test_result("获取全局对象链表头", result);
    
    /* 测试获取全局对象数量 */
    uint32_t total_count = rtos_object_get_total_count();
    result = (total_count == 0); /* 初始状态应该为0 */
    print_test_result("获取全局对象数量", result);
    
    printf("对象系统管理测试完成\n");
}

void test_object_statistics(void)
{
    printf("\n=== 测试对象统计信息 ===\n");
    
    bool result = true;
    
    /* 创建一些测试对象 */
    rtos_object_t obj1, obj2, obj3;
    rtos_object_init(&obj1, RTOS_OBJECT_TYPE_TASK, "StatTask1", RTOS_OBJECT_FLAG_NONE);
    rtos_object_init(&obj2, RTOS_OBJECT_TYPE_SEMAPHORE, "StatSem1", RTOS_OBJECT_FLAG_NONE);
    rtos_object_init(&obj3, RTOS_OBJECT_TYPE_MUTEX, "StatMutex1", RTOS_OBJECT_FLAG_NONE);
    
    /* 测试获取对象统计信息 */
    rtos_object_stats_t stats;
    rtos_result_t stat_result = rtos_object_get_statistics(&stats);
    result = (stat_result == RTOS_OK);
    result &= (stats.total_objects >= 3);
    print_test_result("获取对象统计信息", result);
    
    if (result) {
        printf("  总对象数量: %lu\n", (unsigned long)stats.total_objects);
        printf("  任务对象数量: %lu\n", (unsigned long)stats.type_counts[RTOS_OBJECT_TYPE_TASK]);
        printf("  信号量对象数量: %lu\n", (unsigned long)stats.type_counts[RTOS_OBJECT_TYPE_SEMAPHORE]);
        printf("  互斥量对象数量: %lu\n", (unsigned long)stats.type_counts[RTOS_OBJECT_TYPE_MUTEX]);
    }
    
    printf("对象统计信息测试完成\n");
}

void test_object_destruction(void)
{
    printf("\n=== 测试对象销毁 ===\n");
    
    bool result = true;
    
    /* 创建测试对象 */
    rtos_object_t *obj = malloc(sizeof(rtos_object_t));
    rtos_object_init(obj, RTOS_OBJECT_TYPE_QUEUE, "DestroyTest", RTOS_OBJECT_FLAG_DYNAMIC);
    
    /* 测试对象销毁（引用计数不为0时应该失败） */
    rtos_result_t destroy_result = rtos_object_destroy(obj);
    result = (destroy_result == RTOS_ERROR_RESOURCE_BUSY);
    print_test_result("对象销毁（引用计数不为0）", result);
    
    /* 减少引用计数到0 */
    while (obj->ref_count > 0) {
        rtos_object_ref_dec(obj);
    }
    
    /* 测试对象销毁（引用计数为0时应该成功） */
    destroy_result = rtos_object_destroy(obj);
    result = (destroy_result == RTOS_OK);
    print_test_result("对象销毁（引用计数为0）", result);
    
    /* 释放内存 */
    free(obj);
    
    printf("对象销毁测试完成\n");
}

/* 主测试函数 */
void run_all_object_tests(void)
{
    printf("开始RTOS对象模块功能测试...\n");
    
    test_object_basic_operations();
    test_object_time_operations();
    test_reference_counting();
    test_object_container();
    test_wait_queue();
    test_wait_node();
    test_object_system();
    test_object_statistics();
    test_object_destruction();
    
    printf("\n所有测试完成！\n");
}

/* 如果直接编译此文件，运行测试 */
#ifdef OBJECT_TEST_MAIN
int main(void)
{
    run_all_object_tests();
    return 0;
}
#endif
