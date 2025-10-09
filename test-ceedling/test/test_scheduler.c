#include "unity.h"
#include "scheduler.h"
#include <stdbool.h>

/* --- Variables de apoyo --- */
static bool dummy_task_executed = false;
static bool periodic_task_executed = false;
static bool one_shot_executed = false;
static int periodic_executions = 0;

/* --- Tareas simuladas --- */
void one_shot_task(void) {
    one_shot_executed = true;
}
void periodic_task_counter(void) {
    periodic_executions++;
}
void periodic_task(void) {
    periodic_task_executed = true;
}
void dummy_task_flag(void) {
    dummy_task_executed = true;
}
void dummy_task(void) {
}
void another_dummy_task(void) {
}

/* --- Fixtures Unity --- */
void setUp(void) {
    SCH_Init();
}
void tearDown(void) {
}

/* ===========================================================
 * TEST 0-1-N: inicialización y registro de tareas
 * =========================================================== */

/**
 * @test Verifica que SCH_Init() limpia completamente la tabla de tareas.
 *       (Caso “0”: sin tareas registradas)
 */
void test_scheduler_initialize_all_task_to_null(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        TEST_ASSERT_NULL(SCH_tasks_G[i].pTask);
        TEST_ASSERT_EQUAL_UINT16(0, SCH_tasks_G[i].Delay);
        TEST_ASSERT_EQUAL_UINT16(0, SCH_tasks_G[i].Period);
        TEST_ASSERT_EQUAL_UINT8(0, SCH_tasks_G[i].RunMe);
    }
}

/**
 * @test SCH_Add_Task() inserta una tarea en el primer slot libre.
 */
void test_scheduler_adds_task_in_first_empty_slot(void) {
    int8_t index = SCH_Add_Task(dummy_task, 100, 500);

    TEST_ASSERT_EQUAL_INT8(0, index);
    TEST_ASSERT_EQUAL_PTR(dummy_task, SCH_tasks_G[0].pTask);
    TEST_ASSERT_EQUAL_UINT16(100, SCH_tasks_G[0].Delay);
    TEST_ASSERT_EQUAL_UINT16(500, SCH_tasks_G[0].Period);
    TEST_ASSERT_EQUAL_UINT8(0, SCH_tasks_G[0].RunMe);
}

/**
 * @test Al agregar una tarea, solo el primer slot se ocupa;
 *       los demás deben permanecer vacíos.
 */
void test_scheduler_only_first_task_initialized(void) {
    int8_t index = SCH_Add_Task(dummy_task, 100, 500);
    TEST_ASSERT_EQUAL_INT8(0, index);

    for (uint8_t i = 1; i < SCH_MAX_TASKS; i++) {
        TEST_ASSERT_NULL(SCH_tasks_G[i].pTask);
    }
}

/**
 * @test No se deben agregar más de SCH_MAX_TASKS tareas.
 *       (Caso “N”: saturación)
 */
void test_scheduler_does_not_allow_more_than_max_tasks(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        int8_t index = SCH_Add_Task(dummy_task, i * 10, i * 100);
        TEST_ASSERT_EQUAL_INT8(i, index);
    }

    int8_t result = SCH_Add_Task(another_dummy_task, 1000, 2000);
    TEST_ASSERT_EQUAL_INT8(SCH_NO_TASK_AVAILABLE, result);

    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        TEST_ASSERT_EQUAL_PTR(dummy_task, SCH_tasks_G[i].pTask);
    }
}

/* ===========================================================
 * TESTS: comportamiento de SCH_Update()
 * =========================================================== */

/**
 * @test SCH_Update() debe decrementar el delay y activar RunMe al llegar a cero.
 */
void test_scheduler_update_decrements_delay_and_sets_runme(void) {
    SCH_Add_Task(dummy_task, 2, 0);

    // Tick 1: delay 2→1
    SCH_Update();
    TEST_ASSERT_EQUAL_UINT16(1, SCH_tasks_G[0].Delay);
    TEST_ASSERT_EQUAL_UINT8(0, SCH_tasks_G[0].RunMe);

    // Tick 2: delay 1→0, RunMe++
    SCH_Update();
    TEST_ASSERT_EQUAL_UINT16(0, SCH_tasks_G[0].Delay);
    TEST_ASSERT_EQUAL_UINT8(1, SCH_tasks_G[0].RunMe);

    // Tick 3: no debe reprogramarse (periodo 0)
    SCH_Update();
    TEST_ASSERT_EQUAL_UINT16(0, SCH_tasks_G[0].Delay);
    TEST_ASSERT_EQUAL_UINT8(1, SCH_tasks_G[0].RunMe);
}

/* ===========================================================
 * TESTS: ejecución y despacho de tareas
 * =========================================================== */

/**
 * @test Una tarea one-shot (periodo=0) debe ejecutarse una vez y eliminarse.
 */
void test_scheduler_dispatch_executes_and_deletes_one_shot_tasks(void) {
    dummy_task_executed = false;
    SCH_Add_Task(dummy_task_flag, 0, 0);
    SCH_tasks_G[0].RunMe = 1;

    SCH_Dispatch_Tasks();

    TEST_ASSERT_TRUE(dummy_task_executed);
    TEST_ASSERT_EQUAL_UINT8(0, SCH_tasks_G[0].RunMe);
    TEST_ASSERT_NULL(SCH_tasks_G[0].pTask);
}

/**
 * @test Una tarea periódica debe reprogramarse automáticamente.
 */
void test_scheduler_dispatch_reprograms_periodic_tasks(void) {
    periodic_task_executed = false;
    SCH_Add_Task(periodic_task, 0, 3);
    SCH_tasks_G[0].RunMe = 1;

    SCH_Dispatch_Tasks();
    TEST_ASSERT_TRUE(periodic_task_executed);
    TEST_ASSERT_NOT_NULL(SCH_tasks_G[0].pTask);
    TEST_ASSERT_EQUAL_UINT8(0, SCH_tasks_G[0].RunMe);

    // Avanzar 3 ticks → debe activarse otra vez
    periodic_task_executed = false;
    for (int i = 0; i < 3; i++)
        SCH_Update();

    TEST_ASSERT_EQUAL_UINT8(1, SCH_tasks_G[0].RunMe);
    SCH_Dispatch_Tasks();
    TEST_ASSERT_TRUE(periodic_task_executed);
}

/**
 * @test Verifica un ciclo completo: tarea one-shot y tarea periódica coexistiendo.
 */
void test_scheduler_full_cycle_one_shot_and_periodic(void) {
    one_shot_executed = false;
    periodic_executions = 0;

    SCH_Add_Task(one_shot_task, 2, 0);
    SCH_Add_Task(periodic_task_counter, 1, 3);

    for (int tick = 0; tick < 10; tick++) {
        SCH_Update();
        SCH_Dispatch_Tasks();
    }

    TEST_ASSERT_TRUE(one_shot_executed);
    TEST_ASSERT_NULL(SCH_tasks_G[0].pTask);
    TEST_ASSERT_TRUE(periodic_executions >= 3);
    TEST_ASSERT_NOT_NULL(SCH_tasks_G[1].pTask);
}

/* ===========================================================
 * TESTS: condiciones límite y casos “N”
 * =========================================================== */

/**
 * @test Ejecuta múltiples tareas listas en el mismo tick.
 */
void test_scheduler_dispatch_executes_multiple_ready_tasks(void) {
    bool taskA_executed = false, taskB_executed = false;

    void taskA(void) {
        taskA_executed = true;
    }
    void taskB(void) {
        taskB_executed = true;
    }

    SCH_Add_Task(taskA, 0, 0);
    SCH_Add_Task(taskB, 0, 0);
    SCH_tasks_G[0].RunMe = 1;
    SCH_tasks_G[1].RunMe = 1;

    SCH_Dispatch_Tasks();
    TEST_ASSERT_TRUE(taskA_executed);
    TEST_ASSERT_TRUE(taskB_executed);
}

/**
 * @test SCH_Dispatch_Tasks() debe ignorar slots sin función válida.
 */
void test_scheduler_dispatch_skips_null_task(void) {
    SCH_tasks_G[0].pTask = 0;
    SCH_tasks_G[0].RunMe = 1;

    SCH_Dispatch_Tasks();

    TEST_ASSERT_EQUAL_UINT8(1, SCH_tasks_G[0].RunMe);
    TEST_ASSERT_NULL(SCH_tasks_G[0].pTask);
}

/**
 * @test No se debe ejecutar ninguna tarea si RunMe=0.
 */
void test_scheduler_dispatch_does_not_execute_if_runme_zero(void) {
    bool called = false;
    void task(void) {
        called = true;
    }

    SCH_Add_Task(task, 0, 0);
    SCH_tasks_G[0].RunMe = 0;

    SCH_Dispatch_Tasks();
    TEST_ASSERT_FALSE(called);
}

/**
 * @test Si RunMe>1, la tarea debe ejecutarse tantas veces como se haya acumulado.
 */
void test_scheduler_dispatch_handles_multiple_runme_counts(void) {
    uint8_t executions = 0;
    void task(void) {
        executions++;
    }

    SCH_Add_Task(task, 0, 3);
    SCH_tasks_G[0].RunMe = 3;

    SCH_Dispatch_Tasks();

    TEST_ASSERT_EQUAL_UINT8(3, executions);
    TEST_ASSERT_EQUAL_UINT8(0, SCH_tasks_G[0].RunMe);
}

/**
 * @test Eliminar una tarea one-shot no debe afectar a una tarea periódica.
 */
void test_scheduler_dispatch_does_not_affect_periodic_tasks_on_delete(void) {
    bool taskA_executed = false, taskB_executed = false;
    void taskA(void) {
        taskA_executed = true;
    }
    void taskB(void) {
        taskB_executed = true;
    }

    SCH_Add_Task(taskA, 0, 0);
    SCH_Add_Task(taskB, 0, 3);
    SCH_tasks_G[0].RunMe = 1;
    SCH_tasks_G[1].RunMe = 1;

    SCH_Dispatch_Tasks();

    TEST_ASSERT_TRUE(taskA_executed);
    TEST_ASSERT_TRUE(taskB_executed);
    TEST_ASSERT_NULL(SCH_tasks_G[0].pTask);
    TEST_ASSERT_NOT_NULL(SCH_tasks_G[1].pTask);
}
