# Instructions Explained
## call

## Iret
iret returns from a interrupt. Special about this versus the normal return address is that the iret instruction pops at least 3 elements from the stack. It pops the return address (IP register), CS and the EFLAGS. If the  RPL bits in the popped CS register are different from the current CS then it also pops the data segment and the esp. 

#### In short:
It pops the IP, CS, EFLAGS if the interrupt routine was in the same priviledge level as it returns to.


It pops the IP, CS, EFLAGS, DS, ESP if the interrrupt routine is in kernel space and it needs to return back to user land.


Because interrupts occure while other code was executet the iret needs to pop all this information off the stack so it can continue executing where it left off before the interrupt. It also changes all the Segment selectors and stack pointer back to ring3 stack and segments if it needs to return back to ring3. 