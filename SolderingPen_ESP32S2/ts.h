#pragma once
// Task Scheduler lib   https://github.com/arkhipenko/TaskScheduler
#define _TASK_STD_FUNCTION   // Compile with support for std::function.
#define _TASK_SCHEDULING_OPTIONS
#define _TASK_SELF_DESTRUCT
#include "TaskSchedulerDeclarations.h"

// TaskScheduler - Let the runner object be a global, single instance shared between object files.
extern Scheduler ts;
