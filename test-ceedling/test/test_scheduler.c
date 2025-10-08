#include "unity.h"
#include "scheduler.h"

void dummy_task(void) {
}

void another_dummy_task(void) {
}

void test_scheduler_initialize_all_task_to_null(void) {
    SCH_Init();

    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        TEST_ASSERT_NULL(SCH_tasks_G[i].pTask);
        TEST_ASSERT_EQUAL_UINT16(0, SCH_tasks_G[i].Delay);
        TEST_ASSERT_EQUAL_UINT16(0, SCH_tasks_G[i].Period);
        TEST_ASSERT_EQUAL_UINT8(0, SCH_tasks_G[i].RunMe);
    }
}

void test_scheduler_initialize_only_first_task_to_null(void) {
    // Arrange
    SCH_Init();

    // Act
    int8_t index = SCH_Add_Task(dummy_task, 100, 500);

    // Assert
    TEST_ASSERT_EQUAL_INT8(0, index);
    TEST_ASSERT_EQUAL_PTR(dummy_task, SCH_tasks_G[0].pTask);
    TEST_ASSERT_EQUAL_UINT16(100, SCH_tasks_G[0].Delay);
    TEST_ASSERT_EQUAL_UINT16(500, SCH_tasks_G[0].Period);
    TEST_ASSERT_EQUAL_UINT8(0, SCH_tasks_G[0].RunMe);

    // Y los demás slots deben permanecer vacíos
    for (uint8_t i = 1; i < SCH_MAX_TASKS; i++) {
        TEST_ASSERT_NULL(SCH_tasks_G[i].pTask);
    }
}

void test_scheduler_does_not_allow_more_than_max_tasks(void) {
    SCH_Init();

    // Agregamos SCH_MAX_TASKS tareas válidas
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        int8_t index = SCH_Add_Task(dummy_task, i * 10, i * 100);
        TEST_ASSERT_EQUAL_INT8(i, index);
    }

    // Intentamos agregar una más (debería fallar)
    int8_t result = SCH_Add_Task(another_dummy_task, 1000, 2000);
    TEST_ASSERT_EQUAL_INT8(0, result);

    // Verificamos que no se haya sobrescrito ninguna tarea válida
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        TEST_ASSERT_EQUAL_PTR(dummy_task, SCH_tasks_G[i].pTask);
    }
}

void test_scheduler_update_decrements_delay_and_sets_runme(void) {
    SCH_Init();

    // Agregar una tarea con delay de 2 y periodo de 0 (una sola ejecución)
    SCH_Add_Task(dummy_task, 2, 0);

    // Primer tick: delay pasa de 2 -> 1
    SCH_Update();
    TEST_ASSERT_EQUAL_UINT16(1, SCH_tasks_G[0].Delay);
    TEST_ASSERT_EQUAL_UINT8(0, SCH_tasks_G[0].RunMe);

    // Segundo tick: delay pasa de 1 -> 0 y RunMe aumenta a 1
    SCH_Update();
    TEST_ASSERT_EQUAL_UINT16(0, SCH_tasks_G[0].Delay);
    TEST_ASSERT_EQUAL_UINT8(1, SCH_tasks_G[0].RunMe);

    // Tercer tick: al ser de periodo 0, no debe reprogramarse
    SCH_Update();
    TEST_ASSERT_EQUAL_UINT16(0, SCH_tasks_G[0].Delay);
    TEST_ASSERT_EQUAL_UINT8(1, SCH_tasks_G[0].RunMe);
}