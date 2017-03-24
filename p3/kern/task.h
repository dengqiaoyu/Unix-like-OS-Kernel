/** @file task.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include <elf_410.h>

#define USER_STACK_LOW 0xFFB00000
#define USER_STACK_SIZE (1 << 12)
#define KERN_STACK_SIZE (1 << 12)

struct thread;

typedef struct task {
    int task_id;
    uint32_t *page_dir;
    struct thread *main_thread;

    // list of threads

    // virtual memory mapped regions
} task_t;

typedef struct thread {
    int tid;
    task_t *task;
    uint32_t kern_sp;
    uint32_t user_sp;
    uint32_t ip;
} thread_t;

task_t *task_init(const char *fname);

thread_t *thread_init();

int load_program(simple_elf_t *header, uint32_t *page_dir);

int load_elf_section(const char *fname, unsigned long start, unsigned long len,
                     long offset, int pte_flags);

#endif
