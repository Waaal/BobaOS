# Talk with the kernal as a Process
Userland and kernel communication is done via a interrupt. The Userprocess is calling a interrupt, a kernel interrupt routine performs the interrupt and returns the result back to the process (the result is normaly in the eax register).


We can have 256 entries in a IDT. And we know that some are reserved for CPU Exceptions and some are mapped with IRQ numbers. So we could have the rest for Kernel call interrupts. But normally there is only one interrupt mapped for kernel interrupts. Linux and windows mappes this vector to 0x80. To perform different actions with this one interrupt, we provide a extra argument in the EAX register. With this argument we can perform different actions. For example if we call the kernel wit EAX set to 1 this could mean print. If EAX is 2 this could mean sleep.


*Note: When we call the kernel with the int instruction the processor pushes the same information to return to userland as we did in the [Userland](Userland.md) doc. So it pushes the Data segment, stack address etc in the right order so when we are finished with the kernel routine we can simply leave kernel land and return back to userland with a iret instruction*

## Calling kernel overview
Here is fast overview what needs to happen if we call the kernel:
- We call int instruction
- ESP, SS is changed with the TSS register to kernel stack and the info to return back to userland is pushed by the CPU
- Data Segments need to be changed to kernel Data segments
- Paging needs to be changed to kernel space
- All registers from Task that executed the interrupt needs to be saved
- Kernel routins begins execution command number is passed by the EAX register
- Interrupt handler in C is called in kernel space
- Kernel does the action that is suppost to do
- kernel command handler returns a value
- The EAX register is populated with the return value
- All registers from the task are restored
- Execution in the user process resumes after the int instruction

## How to hand a interrupt
To handle a interrupt we need to make a entry in the IDT with the vector we want the userland to use for kernel interrupts.


Example handeler in C
``` c
/*
 command: The command that was in the eax register
 frame: All of the Task registers
*/
void* isr80h_handler(int command, struct interrupt_frame* frame)
{
    void* res = 0;
    kernel_page();	//Switch paging to kernel page and kernel Data segments (ES,GS,FS and DS NOT SS and CS is done by int, TSS and IDT table)
    task_current_save_state(frame);     //Save all of tasks registers for multi-tasking purposes
    res = isr80h_handle_command(command, frame); //Call command handeler

    task_page(); //Switches back to task paging and back to task Data segments (ES,GS,FS and DS NOT SS, it is done by iret instruction)
    return res;
}

```


## Example system commands
Some example system commands are:
- exit
- print
- get_key
- malloc
- sleep
- video_draw
- get_kernel_info