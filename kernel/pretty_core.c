/*****************************************************************************
MIT License

Copyright (c) 2020 Yahia Farghaly Ashour

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

 _______                        __      __                 ______    ______
|       \                      |  \    |  \               /      \  /      \
| $$$$$$$\  ______    ______  _| $$_  _| $$_    __    __ |  $$$$$$\|  $$$$$$\
| $$__/ $$ /      \  /      \|   $$ \|   $$ \  |  \  |  \| $$  | $$| $$___\$$
| $$    $$|  $$$$$$\|  $$$$$$\\$$$$$$ \$$$$$$  | $$  | $$| $$  | $$ \$$    \
| $$$$$$$ | $$   \$$| $$    $$ | $$ __ | $$ __ | $$  | $$| $$  | $$ _\$$$$$$\
| $$      | $$      | $$$$$$$$ | $$|  \| $$|  \| $$__/ $$| $$__/ $$|  \__| $$
| $$      | $$       \$$     \  \$$  $$ \$$  $$ \$$    $$ \$$    $$ \$$    $$
 \$$       \$$        \$$$$$$$   \$$$$   \$$$$  _\$$$$$$$  \$$$$$$   \$$$$$$
                                               |  \__| $$
                                                \$$    $$
                                                 \$$$$$$


******************************************************************************/

/*
 * Author   :   Yahia Farghaly Ashour
 *
 * Purpose  :   The heart of prettyOS core functions.
 *                  Here is where the tasks schedulability live, the ready and blocked tasks status and others.
 * 
 *              The scheduler of the prettyOS is a preemptive scheduler based on tasks priorities.
 *              It execute the higher priority task when it's available to be executed ( i.e not blocked due to delay or an event ).
 *              
 *              The scheduler is designed as a higher priority task corresponds to a higher assigned priority number to a task when was created.
 *                
 *
 * 				List of Available APIs		:	Short Description
 * 				=================================================
 * 					- OS_Init ()	        :	Initialize prettyOS internal structures and variables.
 * 					- OS_IntEnter ()  	    :	Notify prettyOS that we're entering an ISR.
 * 					- OS_IntExit ()  	    :	Notify prettyOS that we're exiting  an ISR. So, it can schedule a high priority task.
 * 					- OS_SchedLock ()       :	Lock   the scheduler. i.e disable the preemption. 
 *                  - OS_SchedUnlock ()     :   Unlock the scheduler.
 *                  - OS_Sched ()           :   Schedule the high priority task which is in a ready state.
 *                  - OS_Run()              :   Start Running prettyOS and give it the control over the running application.
 * 
 * Language :   C
 * 
 * Set 1 tab = 4 spaces for better comments readability.
 */

/*
*******************************************************************************
*                               Includes Files                                *
*******************************************************************************
*/
#include "pretty_os.h"
#include "pretty_shared.h"

/*
*******************************************************************************
*                              OS Internal Macros                             *
*******************************************************************************
*/

#define OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD     (CPU_CONFIG_DATA_SIZE_BITS)

#if ((OS_CONFIG_TASK_COUNT) & (OS_CONFIG_TASK_COUNT - 1U))
    #error "OS_CONFIG_TASK_COUNT Must be multiple of power of 2. "
#endif

#if ((OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD) & (OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD - 1U))
    #error "OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD Must be multiple of power of 2. The minimum value is 8-bit for a supported CPU."
#endif

#define OS_AUTO_CONFIG_MAX_PRIO_ENTRIES      (OS_CONFIG_TASK_COUNT / OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD) /* Number of priority entries (levels), minimum value = 1 */

#if (OS_AUTO_CONFIG_MAX_PRIO_ENTRIES == 0U)
    #warning "OS_CONFIG_TASK_COUNT is less than #bits of one entry of priority map."
    #warning "OS_CONFIG_TASK_COUNT is re-defined to be equal to #bits of OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD."
    #undef  OS_AUTO_CONFIG_MAX_PRIO_ENTRIES
    #undef  OS_CONFIG_TASK_COUNT
    #define OS_CONFIG_TASK_COUNT     OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD
    #define OS_AUTO_CONFIG_MAX_PRIO_ENTRIES     (1U)
#endif

/*
*******************************************************************************
*                               static variables                              *
*******************************************************************************
*/

/* Array of bit-mask of tasks that are ready to run.
 * (Accessible by task priority)                                              */
static CPU_tWORD OS_TblReady		[OS_AUTO_CONFIG_MAX_PRIO_ENTRIES] = { 0U };

/* Array of bit-mask of tasks that blocked due to time delay.
 * (Accessible by task priority)                                              */
static CPU_tWORD OS_TblTimeBlocked	[OS_AUTO_CONFIG_MAX_PRIO_ENTRIES] = { 0U };


/*
*******************************************************************************
*                               static functions                              *
*******************************************************************************
*/

static void         OS_ScheduleNext(void);
static OS_PRIO      OS_PriorityHighestGet(void);
static CPU_tWORD    OS_Log2(const CPU_tWORD x);

/*
*******************************************************************************
*                               Global variables                              *
*******************************************************************************
*/

/* Status of the OS. Values are OS_TRUE/OS_FALSE                              */
CPU_tWORD      volatile OS_Running;

/* Pointer to the current running TCB.                                        */
OS_TASK_TCB*   volatile OS_currentTask;

/* Pointer to the next TCB to resume.                                         */
OS_TASK_TCB*   volatile OS_nextTask;

/* Interrupt nesting level. Max value is 255 levels.                          */
CPU_t08U       volatile OS_IntNestingLvl;

/* Scheduler nesting lock level. Max lock levels is 255.                      */
CPU_t08U       volatile OS_LockSchedNesting;

#if (OS_CONFIG_SYSTEM_TIME_SET_GET_EN == OS_CONFIG_ENABLE)
/* Contain the system time in clock ticks since the first OS_TimerTick call.  */
OS_TICK		   volatile OS_TickTime;
#endif

/*
 * Array of TCBs pointers, where each pointer refers to a reserved TCB entry.
 * For a task, Mutex, ... etc or ((OS_TASK_TCB*)0U) if not pointing to a TCB
 * Entry.
 *  */
OS_TASK_TCB*            OS_tblTCBPrio [OS_CONFIG_TASK_COUNT];

#if(OS_CONFIG_EDF_EN == OS_CONFIG_ENABLE)
	List OS_ReadyList;
	List OS_PendingList;
	List_Item OS_TCBList [OS_CONFIG_TASK_COUNT];
#endif

/*
*******************************************************************************
*                                                                             *
*                         PrettyOS Hook Functions                             *
*                                                                             *
*******************************************************************************
*/

/* The Idle Task of prettyOS                                                */
void
OS_IdleTask (void* args)
{
    (void)args;                 /* Prevent compiler warning.                */
    while(1)
    {

#if(OS_CONFIG_CPU_IDLE == OS_CONFIG_ENABLE)
    	OS_CPU_Hook_Idle();		/* Call low level CPU idle routine.			*/
#endif

#if (OS_CONFIG_APP_TASK_IDLE == OS_CONFIG_ENABLE)
    	App_Hook_TaskIdle();    /* Call user's idle function.               */
#endif
    	OS_TaskYield();
    }
}

/*
*******************************************************************************
*                                                                             *
*                         PrettyOS Core Functions                             *
*                                                                             *
*******************************************************************************
*/

/*
 * Function:  OS_Init
 * --------------------
 * Initialize the prettyOS services.
 *
 * Arguments    :  pStackBaseIdleTask    is a pointer to the bottom of the Idle task stack.(i.e stack[0] of the task).
 *                 priority              is the task stack size.
 *
 * Returns      :  OS_RET_OK, OS_ERR_PARAM
 */
OS_tRet
OS_Init (CPU_tSTK* pStackBaseIdleTask, CPU_tSTK  stackSizeIdleTask)
{

    OS_tRet ret;
    CPU_t32U idx;

#if(OS_CONFIG_CPU_INIT == OS_CONFIG_ENABLE)
    OS_CPU_Hook_Init();								/* Call port specific initialization code.				  		*/
#endif
    												/* Initialize Common PrettyOS Global/static to default values.  */
    OS_currentTask      = OS_NULL(OS_TASK_TCB);
    OS_nextTask         = OS_NULL(OS_TASK_TCB);
    OS_IntNestingLvl    = 0U;
    OS_LockSchedNesting = 0U;
#if (OS_CONFIG_SYSTEM_TIME_SET_GET_EN == OS_CONFIG_ENABLE)
    OS_TickTime			= 0U;
#endif
    OS_Running          = OS_FAlSE;

#if (OS_AUTO_CONFIG_INCLUDE_LIST == OS_CONFIG_ENABLE)

    list_Init(&OS_ReadyList);
    list_Init(&OS_PendingList);

    for(idx = 0; idx < OS_CONFIG_TASK_COUNT; idx++)
    {
    	listItem_Init(&OS_TCBList[idx]);
    }

#endif

    OS_TCB_ListInit();

    OS_Memory_Init();

    for(idx = 0; idx < OS_AUTO_CONFIG_MAX_PRIO_ENTRIES; ++idx)
    {
        OS_TblReady[idx]        = 0U;
        OS_TblTimeBlocked[idx]  = 0U;
    }

#if (OS_AUTO_CONFIG_INCLUDE_EVENTS == OS_CONFIG_ENABLE)

    OS_Event_FreeListInit();

#endif

#if(OS_CONFIG_FLAG_EN == OS_CONFIG_ENABLE)
    OS_Event_Flag_FreeListInit();
#endif

#if (OS_CONFIG_EDF_EN == OS_CONFIG_DISABLE)

    ret = OS_TaskCreate(OS_IdleTask,
                        OS_NULL(void),
                        pStackBaseIdleTask,
                        stackSizeIdleTask,
                        OS_IDLE_TASK_PRIO_LEVEL);

#else

    OS_TaskCreate(OS_IdleTask,
            OS_NULL(void),
            pStackBaseIdleTask,
            stackSizeIdleTask,
			OS_TASK_PERIODIC,
			(CPU_tWORD)0xFFFFFFFF,
			OS_CONFIG_TICKS_PER_SEC);

#if(OS_CONFIG_ERRNO_EN == OS_CONFIG_ENABLE)
    ret = OS_ERRNO;
#else
    ret = OS_ERR_NONE;
#endif

#endif

    return (ret);
}

/*
 * Function:  OS_IntEnter
 * --------------------
 * Notify PrettyOS that you are about to service an interrupt service routine (ISR).
 * This allows PrettyOS to keep track of the nested interrupts and thus performing the
 * rescheduling at the last nested ISR.
 *
 * Arguments    : None.
 *
 * Returns      : None.
 *
 * Notes        :   1) This function must be called with interrupts disabled.
 *                  2) You MUST invoke OS_IntEnter() and OS_IntExit() in pair.
 *                      For every call of OS_IntEnter() at the ISR beginning, you have to call OS_IntExit()
 *                      at the end of the ISR.
 *                  3) Nested interrupts are allowed up to 255 interrupts.
 */
void
OS_IntEnter (void)
{
    if(OS_TRUE == OS_Running)
    {
        if(OS_IntNestingLvl < 255U)
        {
            ++OS_IntNestingLvl;
        }
    }
}

/*
 * Function:  OS_IntExit
 * --------------------
 * Notify PrettyOS that you have completed servicing an ISR.
 * When the last nested ISR has completed, the PrettyOS Scheduler is called to determine
 * the new, highest-priority task is ready to run.
 *
 * Arguments    : None.
 *
 * Returns      : None.
 *
 * Notes        :   1) You MUST invoke OS_IntEnter() and OS_IntExit() in pair.
 *                      For every call of OS_IntEnter() at the ISR beginning, you have to call OS_IntExit()
 *                      at the end of the ISR.
 */
void
OS_IntExit (void)
{
    CPU_SR_ALLOC();

    if(OS_TRUE == OS_Running)                           /* The kernel has already started.                            */
    {
        OS_CRTICAL_BEGIN();
        if(OS_IntNestingLvl > 0U)                       /* Prevent OS_IntNestingLvl from wrapping                     */
        {
            --OS_IntNestingLvl;
        }

        if(0U == OS_IntNestingLvl)                      /* Re-schedule if all ISRs are completed...                    */
        {
            if(0U == OS_LockSchedNesting)               /* ... and not locked                                          */
            {
                OS_ScheduleNext();                   	/* Determine the next high task to run.                        */
                if(OS_nextTask != OS_currentTask)       /* No context switch if the current task is the highest.       */
                {
                    OS_CPU_InterruptContexSwitch();     /* Perform a CPU specific code for interrupt context switch.   */
                }
            }
        }

        OS_CRTICAL_END();
    }
}

/*
 * Function:  OS_SchedLock
 * --------------------
 * Prevent re-scheduling to take place.
 * The task that calls OSSchedLock() keeps control of the CPU
 * even though other higher priority tasks are ready to run.
 *
 * Arguments    : None.
 *
 * Returns      : None.
 *
 * Notes        :   1) You MUST invoke OS_SchedLock() and OS_SchedUnlock() in pair.
 *                  2) The system ISRs are still serviced.
 *                  3) Must be used with caution because it affects the normal management of tasks.
 *                     And your application must not make any system calls that suspend execution
 *                     of the current task since this may lead to system lock-up.
 *                  4) Nested lock are up to 255 locks.
 */
void
OS_SchedLock (void)
{
    CPU_SR_ALLOC();

    if(OS_TRUE == OS_Running)
    {
        OS_CRTICAL_BEGIN();
        if(0U == OS_IntNestingLvl)                     /* Don't call from an ISR                             */
        {
            if (OS_LockSchedNesting < 255U) {          /* Prevent wrapping back to 0                         */
                ++OS_LockSchedNesting;                 /* Increment lock nesting level                       */
            }
        }
        OS_CRTICAL_END();
    }
}

/*
 * Function:  OS_SchedUnlock
 * --------------------
 * Re-allow re-scheduling.
 *
 * Arguments    : None.
 *
 * Returns      : None.
 *
 * Notes        :   1) You MUST invoke OS_SchedLock() and OS_SchedUnlock() in pair.
 *                  2) It calls the OS scheduler when all nesting locks are unlocked
 *                     because the current task could have made higher priority tasks ready to run
 *                     while scheduling was locked.
 */
void
OS_SchedUnlock (void)
{
    CPU_SR_ALLOC();

    if(OS_TRUE == OS_Running)
    {
        OS_CRTICAL_BEGIN();
        if(0U == OS_IntNestingLvl)                     /* Don't call from an ISR                             */
        {
            if(OS_LockSchedNesting > 0U)               /* Don't decrement if it's 0                          */
            {
                --OS_LockSchedNesting;                 /* Decrement lock nesting level                       */
                if(0U == OS_LockSchedNesting)          /* Call the scheduler if lock reached to 0            */
                {
                    OS_CRTICAL_END();
                    OS_Sched();
                }
            }
        }
        OS_CRTICAL_END();
    }
}

/*
 * Function:  OS_Sched
 * --------------------
 * Determine the next highest priority task that is ready to run,
 * And perform a task context switch if needed.
 * This function is invoked by a TASK level code and is not used to re-schedule tasks from ISRs.
 * (see OS_IntExit() for ISR rescheduling).
 *
 * Arguments    : None.
 *
 * Returns      : None.
 *
 * Notes        : 1) This function is internal to PrettyOS functions.
 */
void
OS_Sched (void)
{
    CPU_SR_ALLOC();

    OS_CRTICAL_BEGIN();

    if(0U == OS_IntNestingLvl)                      /* Re-schedule if all ISRs are completed.                      */
    {
        if(0U == OS_LockSchedNesting)               /* Re-schedule if it's not locked.                             */
        {
            OS_ScheduleNext();                   	/* Determine the next high task to run.                        */
            if(OS_nextTask != OS_currentTask)       /* No context switch if the current task is the highest.       */
            {
                OS_CPU_ContexSwitch();              /* Perform a CPU specific code for task context switch.        */
            }
        }
    }

    OS_CRTICAL_END();
}

/*
 * Function:  OS_ScheduleNext
 * --------------------
 * Determine the next highest priority task that is ready to run.
 * The global variable `OS_nextTask` is changed accordingly.
 *
 * Arguments    : None.
 *
 * Returns      : None.
 *
 * Notes        : 1) Interrupts are assumed to be disabled.
 *                2) This function is internal to PrettyOS functions.
 */
void
OS_ScheduleNext (void)
{
#if(OS_CONFIG_EDF_EN == OS_CONFIG_DISABLE)

	/* 							Static Priority Scheduling.						*/

	OS_PRIO OS_HighPrio =  OS_PriorityHighestGet();

    if(OS_IDLE_TASK_PRIO_LEVEL == OS_HighPrio)
    {
        OS_nextTask = OS_tblTCBPrio[OS_IDLE_TASK_PRIO_LEVEL];
    }
    else
    {
        OS_nextTask = OS_tblTCBPrio[OS_HighPrio];
    }
#else

    /* 							Earliest Deadline Scheduling.					*/

    if(OS_ReadyList.itemsCnt != 0U)
    {
        /* If a ready task with right time arrival, Schedule it first.          */
    	if(OS_TickTime == 0U || OS_ReadyList.head->itemVal <= OS_TickTime)
		{
			OS_nextTask = (OS_TASK_TCB*)OS_ReadyList.head->pOwner;
			(void)ListItemRemove(OS_ReadyList.head);
		}
    }
    else
    {
    	// if(OS_currentTask == (OS_TASK_TCB*)OS_TCBList[0].pOwner)
    	// {
        // 	OS_nextTask = OS_currentTask;
    	// }
        OS_nextTask = OS_currentTask;
    }

#endif
}

/*
 * Function:  OS_Run
 * --------------------
 * Start running and transfer the control to the PrettyOS to run the tasks.
 *
 * Arguments    : cpuClockFreq          is the running CPU frequency in Hertz .
 *
 * Returns      : None.
 */
void
OS_Run (CPU_t32U cpuClockFreq)
{
    CPU_SR_ALLOC();

    if(OS_TRUE == OS_Running)
    {
        return;
    }
    else
    {
        OS_CPU_SystemTimerSetup(cpuClockFreq / OS_CONFIG_TICKS_PER_SEC);

        OS_CRTICAL_BEGIN();

        OS_ScheduleNext();                         /* Find the highest priority task to be scheduled.                        */
        OS_CPU_FirstStart();                       /* Start the highest task.                                                */

        OS_CRTICAL_END();                          /* Enable the processor interrupt in case accidentally it is not enabled. */
    }

    for(;;);                                       /* This should never be executed.                                         */
}

/*
 * Function:  OS_PriorityHighestGet
 * --------------------
 * Get the highest priority of a task which is in a ready state.
 *
 * Arguments    : None.
 *
 * Returns      : The highest priority number.
 */
OS_PRIO inline
OS_PriorityHighestGet (void)
{
    CPU_tWORD   *r_tbl;
    OS_PRIO      prio;

    prio  = (OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD*OS_AUTO_CONFIG_MAX_PRIO_ENTRIES);
    r_tbl = &OS_TblReady[OS_AUTO_CONFIG_MAX_PRIO_ENTRIES - 1U];

    while (*r_tbl == (CPU_tWORD)0) {                    /* Loop Through Entries until find a non empty entry                */
        prio -= OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD;  /* Go Back by a Complete Entry                                      */
        r_tbl = r_tbl - 1;
    }
    prio -= OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD;
    prio += ((OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD - (CPU_tWORD)CPU_CountLeadZeros(*r_tbl)) - 1U);
    return (prio);
}

/*
 * Function:  OS_PrioritySet
 * --------------------
 * Insert a task with a certain priority to the ready state.
 *
 * Arguments    : prio    is the task's priority.
 *
 * Returns      : None.
 */
void inline
OS_SetReady (OS_PRIO prio)
{
    CPU_tWORD bit_pos       = prio & (OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD - 1);
    CPU_tWORD entry_pos     = prio >> OS_Log2(OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD);
    OS_TblReady[entry_pos] |= (1U << bit_pos);
}

/*
 * Function:  OS_PriorityClear
 * --------------------
 * Remove a task with a certain priority from the ready state.
 *
 * Arguments    : prio    is the task's priority.
 *
 * Returns      : None.
 */
void inline
OS_RemoveReady (OS_PRIO prio)
{
    CPU_tWORD bit_pos       = prio & (OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD - 1);
    CPU_tWORD entry_pos     = prio >> OS_Log2(OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD);
    OS_TblReady[entry_pos] &= ~(1U << bit_pos);
}

/*
 * Function:  OS_BlockTime
 * --------------------
 * Put a task with a certain priority in the block time state.
 *
 * Arguments    : prio    is the task's priority.
 *
 * Returns      : None.
 */
void inline
OS_BlockTime (OS_PRIO prio)
{
    CPU_tWORD bit_pos             = prio & (OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD - 1);
    CPU_tWORD entry_pos           = prio >> OS_Log2(OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD);
    OS_TblTimeBlocked[entry_pos] |= (1U << bit_pos);
}

/*
 * Function:  OS_UnBlockTime
 * --------------------
 * Remove a task with a certain priority from the block time state.
 *
 * Arguments    : prio    is the task's priority.
 *
 * Returns      : None.
 */
void inline
OS_UnBlockTime (OS_PRIO prio)
{
    CPU_tWORD bit_pos             = prio & (OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD - 1);
    CPU_tWORD entry_pos           = prio >> OS_Log2(OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD);
    OS_TblTimeBlocked[entry_pos] &= ~(1U << bit_pos);
}

/*
 * Function:  OS_Log2
 * --------------------
 * Compute the logarithmic number of base 2.
 * Log(x) = 2^k, This function returns `k` for few numbers of x
 * Implemented for 128, 64, 32, 16, 8, 4, 2 and 1 .
 *
 * Arguments    :   x   is the number raised by the number the function returns.
 *
 * Returns      :   The raised number. (i.e: k)
 */
static inline CPU_tWORD
OS_Log2(const CPU_tWORD x)
{
    switch(x)
    {
    case 128U:
        return 7;
    case 64U:
        return 6;
    case 32U:
        return 5;
    case 16U:
        return 4;
    case 8U:
        return 3;
    case 4U:
        return 2;
    case 2U:
        return 1;
    default:
        break;
    }
    return 0;
}

/*
 * Function:  OS_MemoryByteClear
 * --------------------
 * This function is called to clear a contiguous block of RAM.
 *
 * Arguments    :   pdest   is pointer to the first RAM byte to start to clear.
 *
 * 					size	is the number of bytes to clear.
 *
 * Returns      :   None.
 *
 * Note(s)		:	1) This function is for internal use.
 * 					2) It's selected to be byte clear at a time since it will work correctly regardless
 * 						the processor alignment.
 */
void
OS_MemoryByteClear (CPU_t08U* pdest, CPU_t32U size)
{
    while (size > 0U) {
        *pdest = (CPU_t08U)0;
         ++pdest;
		 --size;
    }
}

#if(OS_CONFIG_EDF_EN == OS_CONFIG_ENABLE)
/*
 * Function:  OS_TaskYield
 * --------------------
 * Give up the current task execution from the CPU & schedule another task.
 *
 * Arguments    :   None.
 *
 * Returns      :   None.
 *
 * Note(s)		:	1) This Function should be used @ the end of task execution.
 */
void
OS_TaskYield (void)
{
	CPU_SR_ALLOC();

	OS_CRTICAL_BEGIN();

	if(OS_currentTask != (OS_TASK_TCB*)OS_TCBList[0].pOwner)
	{
		if(OS_currentTask->EDF_params.task_type == OS_TASK_PERIODIC)
		{
			if(OS_currentTask->EDF_params.tick_arrive < OS_TickTime)
			{
				OS_currentTask->EDF_params.tick_arrive += OS_currentTask->EDF_params.task_period;
				OS_currentTask->EDF_params.tick_absolate_deadline = OS_currentTask->EDF_params.tick_arrive + OS_currentTask->EDF_params.tick_relative_deadline;
				OS_currentTask->pListItemOwner->itemVal = OS_currentTask->EDF_params.tick_arrive;
				listItemInsert(&OS_PendingList,OS_currentTask->pListItemOwner);						/* Insert by arrival time.	*/
			}
		}

		OS_Sched();
	}
	else
	{
        OS_TASK_COUNT tskcnt = 1;
        OS_TASK_TCB* ptcb = OS_tblTCBPrio[1];
        /* Here I am not sure what i am doing :/ */
        while(ptcb->pListItemOwner)
        {
            if(ptcb->EDF_params.tick_arrive <= OS_TickTime)
            {
            	listItemInsert(&OS_ReadyList,ptcb->pListItemOwner);
            }

            ++tskcnt;
            ptcb = OS_tblTCBPrio[tskcnt];
        }

		OS_ScheduleNext();
		if(OS_currentTask != OS_nextTask)
		{
			OS_CPU_ContexSwitch();
		}
	}
    OS_CRTICAL_END();
}
#endif

/*
 * Function:  OS_TimerTick
 * --------------------
 * Signal the occurrence of a "system tick" to the prettyOS which reflects
 * to the services depending on this "system tick".
 *
 * Arguments    : None.
 *
 * Returns      : None.
 *
 * Notes        : 1) This function must be called from a ticker ISR.
 */
void
OS_TimerTick (void)
{
    CPU_tWORD i = 0;
    CPU_tWORD workingSet;
    CPU_SR_ALLOC();

#if (OS_CONFIG_SYSTEM_TIME_SET_GET_EN == OS_CONFIG_ENABLE)
    OS_CRTICAL_BEGIN();
    OS_TickTime += 1U;												/* Update System time of ticks.														*/
    OS_CRTICAL_END();
#endif

#if(OS_CONFIG_CPU_TIME_TICK == OS_CONFIG_ENABLE)
    OS_CPU_Hook_TimeTick();											/* Call Port Specific Tick Hook.						  							*/
#endif

#if (OS_CONFIG_APP_TIME_TICK == OS_CONFIG_ENABLE)
	App_Hook_TimeTick();  											/* Calls Application specific code when an OS system tick occurs.					*/
#endif

    if(OS_Running == OS_FAlSE)
    {
        return;
    }

    OS_CRTICAL_BEGIN();

#if(OS_CONFIG_EDF_EN == OS_CONFIG_DISABLE)
    for(i = 0; i < OS_AUTO_CONFIG_MAX_PRIO_ENTRIES; i++)
    {
        if(OS_TblTimeBlocked[i] != 0U)
        {
            workingSet = OS_TblTimeBlocked[i];
            while(workingSet != 0U)
            {
                CPU_tWORD task_pos = ((OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD - (CPU_tWORD)CPU_CountLeadZeros(workingSet)) - 1U);
                OS_TASK_TCB* t = OS_tblTCBPrio[ task_pos + (i * OS_AUTO_CONFIG_CPU_BITS_PER_DATA_WORD) ];
                if(t != OS_NULL(OS_TASK_TCB))
                {
                    --(t->TASK_Ticks);
                    if(0U == t->TASK_Ticks)                         /* No more ticks to tick                                                             */
                    {
                        t->TASK_Stat &= ~(OS_TASK_STAT_DELAY);      /* Clear the delay bit                                                               */
                        OS_UnBlockTime(t->TASK_priority);

#if (OS_AUTO_CONFIG_INCLUDE_EVENTS == OS_CONFIG_ENABLE)

                        if(t->TASK_Stat & OS_TASK_STATE_PEND_ANY)
                        {
                            t->TASK_PendStat = OS_STAT_PEND_TIMEOUT;
                        }

#endif
                        /* If it's not waiting on any events or suspension,
                           Add the current task to the ready table to be scheduled. */
                        if((t->TASK_Stat & OS_TASK_STAT_SUSPENDED) == OS_TASK_STAT_READY)
                        {
                            OS_SetReady(t->TASK_priority);
                        }
                    }
                }
                workingSet &= ~(1U << task_pos);                /* Remove this processed bit and go to the next priority task in the same entry level. */
            }
        }
    }
#else

    if(OS_PendingList.itemsCnt != 0U)
    {
        List_Item* pIterator = OS_PendingList.head;
        while(pIterator != OS_NULL(List_Item))
        {
        	OS_TASK_TCB* tsk = (OS_TASK_TCB*)pIterator->pOwner;
				if(pIterator->itemVal <= OS_TickTime)				/* if(Task'arrival time == current system tick)	*/
				{
		        	tsk->TASK_Stat &= ~(OS_TASK_STAT_DELAY);
					ListItemRemove(tsk->pListItemOwner);
					pIterator->itemVal = tsk->EDF_params.tick_absolate_deadline;
					listItemInsert(&OS_ReadyList,pIterator);		/* Insert by absolute deadline time.			*/
				}
        	pIterator = pIterator->next;
        }
    }

#endif

    OS_CRTICAL_END();
}


#if (OS_CONFIG_CPU_SOFT_STK_OVERFLOW_DETECTION == OS_CONFIG_ENABLE)

/*
 * Function:  OS_StackOverflow_Detected
 * --------------------
 * This function should be called when an stack overflow is detected whether the detection is done
 * in software-based using water-marks or stack-limit variable (Currently Used in prettyOS code).
 * or in hardware-based as MPU or stack-limit hardware register such the one exist in CPUs based on ARMv8-M
 *
 * Arguments    : ptcb          is a pointer to the task TCB (OS_TASK_TCB) which caused or will cause a task stack overflow.
 *
 * Returns      : None.
 *
 * Notes        : 1) This function must be called from the port level code.
 *                2) This function must not be called by the Application level code.
 */
void
OS_StackOverflow_Detected ( void* ptcb )
{
    for(;;)
    {

#if (OS_CONFIG_CPU_STACK_OVERFLOW == OS_CONFIG_ENABLE)
        OS_CPU_Hook_StackOverflow_Detected();
#endif

#if (OS_CONFIG_APP_STACK_OVERFLOW == OS_CONFIG_ENABLE)
        App_Hook_StackOverflow_Detected ((OS_TASK_TCB*)ptcb);       /* Calls Application specific code for a possible event of a task's stack overflow is detected. */
#endif

    }
}

#endif
