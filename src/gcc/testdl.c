/*=============================================================================
# Filename: testdl.c
# Author: bookug
# Mail: bookug@qq.com
# Last Modified: 2018-10-09 08:13
# Description: use dlopen and dlclose to call dynamic libraries
# http://www.yolinux.com/TUTORIALS/LibraryArchives-StaticAndDynamic.html
# gcc -std=c99 -rdynamic -o run.exe -I./inc testdl.c -ldl -lm
# Discuss: it seems the -rdynamic option can be removed? -rdynamic is used to print more debug information
# Experiment: the advantages of using dlopen/dlclose, a library opened by system will occupy memory and a file descriptor
# How to see the libraries that has been loaded into memory? (ldd only sees the dependency)
# 1. find the pid of your program by ps command, and go to /proc and cd the pid directory, cat maps to see the memory region mappings
# 2. lsof -p PID
# 3. pmap
ipcs shows shared memory segments.
fuser or lsof  shows open files by process.
ldd shows what libraries a module will open.
=============================================================================*/

#include <stdio.h>
#include <math.h> 
#include <stdlib.h> 
#include <limits.h> 
//NOTICE: this header file must be included to use dlopen...
#include <dlfcn.h> 

//Another choice is to use 'inc/list.h' here
#include "list.h"

//A safe way to release memory
#define xfree(p) free(p);p=NULL;

#define list1_num 1000
#define list2_num 1000

//this is placed in .data segment, and it can be assigned larger space than function stack(1M by default). For larger space, use heap memory instead
int list1[list1_num];
int list2[list2_num];

/**
 * @author bookug
 * @email bookug@qq.com
 * @function prepare the data for two lists
 * @param no parameters needed
 * @return no value returned here
 */
void produceData()
{
    for(int i = 0; i < list1_num; ++i)
    {
        list1[i] = 2 * i + 1;
    }

    for(int i = 0; i < list2_num; ++i)
    {
        list2[i] = 2 * (i+1);
    }
}

/**
 * @author bookug
 * @email bookug@qq.com
 * @function the main function of C program
 * @param number of arguments in the command, including the program itself
 * @param all argument strings
 * @return no value returned here
 */
int main(int argc, const char* argv[])
{
    produceData();
    //NOTICE: in this case the code below will be run and output even the liblist.so is not found
    //The reason is that when compiling the -llist is not added, and it will not occur in the header part of executable if using readelf -d to check
    //In the case of using dynamic library and using -llist when compiling, the liblist.so occurs in the header, so the system will find it first when this executable starts running(you can use strace command to see this process)
    printf("check before dynamic\n");

    void *lib_handle;
    double (*fn)(Node**, Node**, int);
    int x;
    char *error;
    //NOTICE: this is not an absolute path, so when working directory changes, it may report error
    lib_handle = dlopen("./inc/liblist.so", RTLD_LAZY);
    //WARN: RTLD_GLOBAL not works in this case(compiling is ok, report error when running)
    /*lib_handle = dlopen("./inc/liblist.so", RTLD_GLOBAL);*/
    if(!lib_handle)
    {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
    fn = dlsym(lib_handle, "addResult");
    if((error = dlerror()) != NULL)
    {
        fprintf(stderr, "%s\n", error);
        exit(1);
    }

    Node *result_head = NULL, *result_tail = NULL;
    int pos1 = 0, pos2 = 0;
    //NOTICE: this strategy not always fails, for example, the final elements of the two lists are the same
    while(pos1 < list1_num && pos2 < list2_num)
    //while(pos1 < list1_num && pos2 < list2_num)
    {
        /*printf("check %d %d\n", pos1, pos2);*/
        if(list1[pos1] < list2[pos2])
        {
            (*fn)(&result_head, &result_tail, list1[pos1]);
            pos1++;
        }
        else if(list1[pos1] > list2[pos2])
        {
            (*fn)(&result_head, &result_tail, list2[pos2]);
            pos2++;
        }
        else   //equal case, only add once
        {
            (*fn)(&result_head, &result_tail, list1[pos1]);
            pos1++;
            pos2++;
        }
    }

    while(pos1 < list1_num)
    {
        (*fn)(&result_head, &result_tail, list1[pos1]);
        pos1++;
    }
    while(pos2 < list2_num)
    {
        (*fn)(&result_head, &result_tail, list2[pos2]);
        pos2++;
    }

    printf("check library in memory before dlclose\n");
    getchar();

    dlclose(lib_handle);

    printf("check library in memory after dlclose\n");
    getchar();

    //NOTICE: the declaration Node* p=... in for-loop is only allowed in C99 standard
    //output each item in result after sqrt operation
    for(Node* p = result_head; p != NULL; p = p->next)
    {
        //NOTICE: sqrt(8) works but sqrt(n) must be compiled by -lm
        double r = sqrt((double)(p->val));
        printf("%lf\n", r);
    }

    //NOTICE: another bug exists here, but error is not reported when running
    //printf("%p %p\n", result_head, result_tail);
    //release the result list
    for(Node* p = result_head; p != NULL; )
    {
        Node* tmp = p->next;
        free(p);
        p = tmp;
        //NOTICE: this set will cause segment fault immediately
        //p = NULL;
        //printf("%p\n", p);
    }
    //clear the list pointers, head and tail
    result_head = result_tail = NULL;

    return 0;
}

