# Lab 5: File System

## Exercise & Question Answers

> How long approximately did it take you to do this lab?

About 14 hours, including merging files, reading the document, coding and writing this document.

## Challenge

In this lab, I choose to implement a Unix-style `exec()` in user space. Since modifying the address space of a program itself will make the program crash, we fork an auxiliary child environment, load ELF image into it and set initial states of it, just like what we do in `spawn()`. To finish the work of `exec()`, we need some help from kernel. So I invented a new system call called `sys_exec_commit()`, in which the trapframe, the page fault upcall and the break pointer are copied from the auxiliary child to the current environment, and the two environments swap their page directories. Then the auxiliary child will be destroyed and the current environment will go on to execute the new program.

`execl()` is also provided, which takes variable length arguments.

Finally, I wrote a test program `user/testexec.c` to test the correctness of `exec()`.

(Another approach to implementing `exec()` is to allow user programs to alter their page directories, however we will also have to invent some new system calls and even new kernel data structures to support this.)
