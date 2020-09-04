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
******************************************************************************/

/*
 * Author   : Yahia Farghaly Ashour
 *
 * Purpose  : PrettyOS Configuration File.
 *
 * Language:  C
 * 
 * Set 1 tab = 4 spaces for better comments readability.
 */

#ifndef __PRETTY_CONFIG_H_
#define __PRETTY_CONFIG_H_

/*
*******************************************************************************
*																			  *
*																			  *
*                       OS Miscellaneous Configurations                       *
*                       													  *
*                                     										  *
*******************************************************************************
*/

/******************************************************************************/
/**********************	  Enabling/Disabling Configs    ***********************/
/******************************************************************************/


#define 	OS_CONFIG_ENABLE				(1U)		/* TRUE  value for Enabling  a macro.		*/
#define 	OS_CONFIG_DISABLE				(0U)		/* FALSE value for Disabling a macro. 		*/

/*===============  Enable/Disable   Mutex 	 service in the code.   ===========*/

#define 	OS_CONFIG_MUTEX_EN				(OS_CONFIG_ENABLE)

/*===============  Enable/Disable Semaphores service in the code. 	===========*/

#define 	OS_CONFIG_SEMAPHORE_EN			(OS_CONFIG_ENABLE)

/*===============  Enable/Disable Mailboxes service in the code. 	===========*/

#define 	OS_CONFIG_MAILBOX_EN			(OS_CONFIG_ENABLE)

/*===============  Enable/Disable Event Flag service in the code. 	===========*/

#define		OS_CONFIG_FLAG_EN				(OS_CONFIG_ENABLE)

/*===============  Enable/Disable Memory Management service in the code. ======*/

#define		OS_CONFIG_MEMORY_EN				(OS_CONFIG_ENABLE)

/*===============  Enable/Disable  OS_ERRNO  service in the code.   ===========*/

#define 	OS_CONFIG_ERRNO_EN   			(OS_CONFIG_ENABLE)

/*=========  Enable/Disable Storing TaskEntry/Args in TCB Structure. ==========*/

#define OS_CONFIG_TCB_TASK_ENTRY_STORE_EN	(OS_CONFIG_ENABLE)

/*=========  Enable/Disable Storing OSTCBExtension in TCB Structure. ==========*/

#define OS_CONFIG_TCB_EXTENSION_EN			(OS_CONFIG_ENABLE)

/*=========  Enable/Disable Set/Get functions of current system time. =========*/

#define OS_CONFIG_SYSTEM_TIME_SET_GET_EN	(OS_CONFIG_ENABLE)


/******************************************************************************/
/**********************	  Application Hooks Configs     ***********************/
/* 					   [Required in Application Level] 						  */
/******************************************************************************/


/*=======  Enable/Disable Application specific code on OS Idle State. =========*/

#define OS_CONFIG_APP_TASK_IDLE				(OS_CONFIG_ENABLE)

/*=======  Enable/Disable Application specific code on OS tasks switches. =====*/

#define OS_CONFIG_APP_TASK_SWITCH			(OS_CONFIG_DISABLE)

/*=======  Enable/Disable Application specific code on OS task creation. ======*/

#define OS_CONFIG_APP_TASK_CREATED			(OS_CONFIG_DISABLE)

/*=======  Enable/Disable Application specific code on OS task deletion. ======*/

#define OS_CONFIG_APP_TASK_DELETED			(OS_CONFIG_DISABLE)

/*=======  Enable/Disable Application specific code on OS task return.   ======*/

#define OS_CONFIG_APP_TASK_RETURNED			(OS_CONFIG_DISABLE)

/*=======  Enable/Disable Application specific code on OS system tick.   ======*/

#define OS_CONFIG_APP_TIME_TICK				(OS_CONFIG_DISABLE)

/*= Enable/Disable Application specific code on OS POST operation of stack overflow detection. =*/

#define OS_CONFIG_APP_STACK_OVERFLOW        (OS_CONFIG_ENABLE)

/********************************************************************************/
/**********************	      Parameterized Configs	      ***********************/
/********************************************************************************/


/*====================== Number of System Ticks/Second. =======================*/

#define OS_CONFIG_TICKS_PER_SEC     								(100U)		/* 100 or 1000 is acceptable.  			*/

/*=================== Max Number of Possible Created Tasks. ===================*/

#define OS_CONFIG_TASK_COUNT  										(128U)   	/* Required to be multiple of 8.   		*/

/*=================== Max Number of Possible Created Events. ===================*/

#define OS_CONFIG_MAX_EVENTS         								(10U)     	/* Max. of Event Objects				*/

/*=================== Max Number of Possible Created Events. ===================*/

#define OS_CONFIG_MAX_EVENT_FLAGS         							(10U)     	/* Max. of Event Flag Objects			*/

/*============= Number of bits of OS_FLAGS data type (8, 16 or 32). ============*/

#define OS_FLAGS_NBITS           									(8U)   		/* 8, 16, 32 or 64 bits.            	*/

/*=================== Max Number of Possible Memory Partition. =================*/

#define OS_CONFIG_MEMORY_PARTITION_COUNT							(10U)		/* Max. of Memory Partition Objects.	*/


/******************************************************************************/
/************************* A U T O GENERATED MACROS ***************************/
/******************************************************************************/

#define OS_AUTO_CONFIG_INCLUDE_EVENTS	(OS_CONFIG_SEMAPHORE_EN || OS_CONFIG_MUTEX_EN || OS_CONFIG_MAILBOX_EN)


/******************************************************************************/
/************************* Configurable DataTypes  ****************************/
/******************************************************************************/


typedef CPU_t16U        OS_SEM_COUNT;       	/* Max. Semaphore Count Limit. 		*/
typedef CPU_t32U		OS_MEMORY_BLOCK;		/* Max. Size of memory block.		*/


#endif /* __PRETTY_CONFIG_H_ */
