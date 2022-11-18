# Overview

The Pintos userspace that you implemented in Project 1 is subject to two major limitations. First, the stack which you set up properly to load a new process is confined to a single page. If a user program’s stack
grows beyond 4,096 bytes, due to a long chain of function calls (e.g. due to a recursive function) and/or
stack-allocated data, then the process will be terminated on its first memory access beyond the first stack
page. Second, there is no way to perform any dynamic memory allocation in a user program. Presently, all
memory in a user process must either have been loaded from the executable (e.g. code, globals) or must be
allocated on the stack.


In this homework, you will remove both of these limitations. You will modify Pintos to dynamically extend
the stack space of a process in response to memory accesses beyond the currently allocated stack. In addition,
you will provide a way for a user process to explicitly request more memory from the Pintos kernel.


# Assigment

https://inst.eecs.berkeley.edu/~cs162/sp22/static/hw/memory.pdf
