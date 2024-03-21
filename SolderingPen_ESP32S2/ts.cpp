#define _TASK_STD_FUNCTION   // Compile with support for std::function 
#define _TASK_SCHEDULING_OPTIONS
#define _TASK_SELF_DESTRUCT
#include <TaskScheduler.h>

// TaskScheduler - Let the runner object be a global, single instance shared between object files.
Scheduler ts;
