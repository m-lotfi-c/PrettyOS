<p align="center">
  <img src="logo.png">
</p>
<p align="center">
  <b>A Preemptive Hard Real time kernel for embedded devices</b>.
</p>

#### ☑ List of Supported Features

- **Preemptive Multitasking** Scheduling.
    - Using fixed priority algorithms as RM ([Rate Monotonic](https://en.wikipedia.org/wiki/Rate-monotonic_scheduling)) or DM ([Deadline Monotonic](https://en.wikipedia.org/wiki/Deadline-monotonic_scheduling)).
    - Number of tasks at each priority level is 1. 

- **Runtime Priority** Change.

- **Configurable** Number of Tasks.

- **Lock/Unlock** Scheduler.

- **Suspend/Resume** Tasks.

- **Mutex** Support. 
    - Including **OCPP** ( [Original Ceiling Priority Protocol](https://en.wikipedia.org/wiki/Priority_ceiling_protocol) ) to overcome priority inversion scenarios.

- Support **Semaphores**, **Message Mailboxes** and **EventFlags** .

- Support **Memory Management** .
    - Using a basic memory manager for fixed-sized allocatable objects in a memory partition (i.e region).    

- **Hooks APIs** at Application and CPU port level.

- Software based Tasks' **stack overflow detection**.

#### 💻 Porting availability
| System      			    | BSP / CPU Port 		| Notes                                 |
| ----------------------|:-----------------:|:-------------------------------------:|
| TI Stellaris LM4F120 	|✔️ 			           |                                       |
| Linux machine         | ✔️                 |Requires POSIX.1b standards as minimal |

To add another port, Please read this [porting guide](port/porting_guide.md) first.

#### 🗃️ Include the RTOS
You include only a single header file [pretty_os.h](kernel/pretty_os.h) which contains the list
of the public APIs with a proper description for each one.


#### 📝 License
Copyright © 2020 - present, Yahia Farghaly Ashour.<br>
This project is [MIT](https://github.com/yahiafarghaly/PrettyOS/blob/master/LICENSE) Licensed.
