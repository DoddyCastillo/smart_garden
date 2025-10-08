#include <stdint.h>

#define SCH_MAX_TASKS         5
#define SCH_NO_TASK_AVAILABLE 0

typedef void (*TaskFunction)(void);

typedef struct {
    TaskFunction pTask;
    uint16_t Delay;
    uint16_t Period;
    uint8_t RunMe;
} sTask;

extern sTask SCH_tasks_G[SCH_MAX_TASKS];

void SCH_Init(void);
uint8_t SCH_Add_Task(TaskFunction pFunction, uint16_t DELAY, uint16_t PERIOD);
void SCH_Update(void);