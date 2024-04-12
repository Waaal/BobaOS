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

Example handeler in asm (in protected mode):
``` assembly
isr80h_wrapper:
                                ; Already pushed by the processor
                                ; SS (User Data)
                                ; SP
                                ; flags
                                ; CS
                                ; IP <-- Stack pointer points here

    pushad                      ; Pushes all general purpose registers

    ; FRAME END

    push esp                    ; Push stack pointer so it is second argument of isr80h_handler
    push eax                    ; Push command that are in eax on stack so isr80h_handler can have it as first argument
    call isr80h_handler
    mov dword[tmp_res], eax     ; Move response from isr80h_handler in temp save spot

    add esp, 8                  ; Move stack back to FRAME END
    popad                       ; Restore general purpose registers for user land
    mov eax, [tmp_res]          ; Move result back in eax registers

    iretd
```

Example isr 80h handeler in C
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
## Accessing a Task variables
We want to access a tasks variable that the process provided. For example the task provided a argument which is a pointer to a string that we should print. Sound easy we copy the pointers value and work with it. Sadly it is not that easy. Because each process is mapped to virtual address 0x400000. So the pointer the process gives us is a virtual address of this processes virtual memory space. For example the pointer points to address 0x400100. But the kernel is mapped 1:1 and works with physical addresses. So virtual address 0x400100 is not physical address 0x400100.


So we first need to translate the virtual address 0x400100 of the process virtual memory to a physical address so that the kernel can access it. There are of course multiple ways to do this. One way is to allocate some temporary memory in the kernel space and then map thet memory 1:1 in the process space (If we allocate physical memory 0x500000 we map it to virtual address 0x500000). Then switch paging to the process page table and copy the value of pointer 0x400100 to the newly mapped temporary memory. 
Switch back to kernel page and we can access the value of the pointer. Then remove the temporary mapped pointer from the process space and restore the old value.


Simple implementation:
``` c
//task: task structure
//virtual: Pointer to string in virtual memory space
//physical: Kernel memory where the content of the string in virtual should end up in
void copy_string_from_task(struct task* task, void* virtual, void* physical, int max)
{
    //Allocate some memory in the kernel land
    char* tmp = kzalloc(max);
    
    //Save old page table entry so we can restore it later
    uint32_t old_entrie = paging_get(task->page_directory->directory_entry, tmp);
    
    //Map physical address of tmp to virtual address of tmp (1:1 mapping)
    paging_map_range(task->page_directory, tmp, tmp, 1, PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

    //Switch to process page table
    paging_switch(task->page_directory);
    
    //Copy string from virtual address to tmp, which is now accessible by the process
    strncpy(tmp, virtual, max);

    //Switch back to kernel page
    kernel_page();

    //restore old entry we overwrote with tmp
    paging_set(task_page_directory->directory_entry, tmp, old_entrie);

    //Finally copy value of tmp to physical address for kernel
    strncpy(physical, tmp, max);

    //Free temporary variable
    kfree(tmp);

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