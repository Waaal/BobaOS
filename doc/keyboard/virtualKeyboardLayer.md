# Virtual keyboard layer
Virtual keyboard layer is a layer that allows the kernel to support a lot of differnt keyboard driver. Because the VKL abstracts out low level keyboad driver and provides funtions for interacting with one.


So with VKL the interface to interact with a keyboad remains the same for different keyboards.


The VKL has functions for inserting a driver to the VKL. And functions to push and pop to the keyboard buffer. 

#### Insert
The insert function is for registering different keyboard drivers. The struct for a keyboard driver is extreme simple with just a name and a init function. This init function can be called by the VKL as soon as the keyboard gest insertet into the VKL.

#### Push
Each process needs to have its own keyboard buffer. The Push function, pushes a char to the current in focus programm. So for example in windows if we have notepad in focus and we press a key, the key gets send to the notepad because that is the function in our focus. Thats why each process needs to have its own keyboard buffer. So the push function pushes a chat to the current process in focus.

**Implementation:**
``` c
void keyboard_push(char c)
{
	//Get the current process in user foceus
	struct process* process = process_current();
	if(!process)
	{
		return;
	}

	//Write in keyboard buffer the new character
	int real_index = keyboard_get_tail_index(process);
	process->keyboard.buffer[real_index] = c;
	process->keyboard.tail++;
}
```


#### Pop
The pop function pops a char from the **currently running tasks process** buffer, not from the process in focus. Because a task can pop from its own buffer even when the task is not in focus.

**Implementation:**
``` c
char keyboard_pop()
{
	if(!task_current())
	{
		return 0;
	}

	//Get process that is currently in cpu time
	struct process* process = task_current()->process;

	//Read char from keyboard buffer
	int real_index = process->keyboard.head % BOBAOS_KEYBOARD_BUFFER_SIZE;
	char c = process->keyboard.buffer[real_index];

	//If char is 0x00 then we have nothing to read
	if(c == 0x00)
	{
		return 0x00;
	}

	//Remove read character and push read head forward
	process->keyboard.buffer[real_index] = 0x00;
	process->keyboard.head++;

	return c;
}
```

<hr>
With this push and pop functions each keyboad (or even a virtual keyboard) can just call the push function of the VKL if the receive a key. The VKL handles the rest.


>[!NOTE]
>Abstraction works with function pointer