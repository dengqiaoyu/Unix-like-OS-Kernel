/** @file task.c
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <simics.h>
#include <page.h>
#include <asm.h>            /* disable_interrupts enable_interrupts */
#include <x86/cr.h>

#include "vm.h"
#include "utils/maps.h"
#include "task.h"
#include "scheduler.h"
#include "utils/tcb_hashtab.h"

thread_t *idle_thread;
thread_t *init_thread;

static int thread_id_counter;
static mutex_t thread_id_counter_mutex;

int id_counter_init() {
    thread_id_counter = 0;
    int ret = mutex_init(&thread_id_counter_mutex);
    return ret;
}

int gen_thread_id() {
    mutex_lock(&thread_id_counter_mutex);
    int tid = thread_id_counter++;
    mutex_unlock(&thread_id_counter_mutex);
    return tid;
}

task_t *task_init() {
    task_node_t *task_node = malloc(sizeof(task_node_t) + sizeof(task_t));

    if (task_node == NULL) {
        lprintf("malloc() failed in task_init at line %d", __LINE__);
        return NULL;
    }
    task_t *task = LIST_NODE_TO_TASK(task_node);

    task->page_dir = page_dir_init();
    if (task->page_dir == NULL) {
        lprintf("page_dir_init() failed in task_init at line %d", __LINE__);
        free(task_node);
        return NULL;
    }

    if (task_lists_init(task) < 0) {
        lprintf("task_lists_init() failed in task_init at line %d", __LINE__);
        sfree(task->page_dir, PAGE_SIZE);
        free(task_node);
        return NULL;
    }

    if (task_mutexes_init(task) < 0) {
        lprintf("task_mutexes_init() failed in task_init at line %d", __LINE__);
        task_lists_destroy(task);
        sfree(task->page_dir, PAGE_SIZE);
        free(task_node);
        return NULL;
    }

    task->maps = maps_init();
    if (task->maps == NULL) {
        lprintf("maps_init() failed in task_init at line %d", __LINE__);
        task_mutexes_destroy(task);
        task_lists_destroy(task);
        sfree(task->page_dir, PAGE_SIZE);
        free(task_node);
        return NULL;
    }

    return task;
}

void undo_task_init(task_t *task) {
    free(task->maps);
    task_mutexes_destroy(task);
    task_lists_destroy(task);
    sfree(task->page_dir, PAGE_SIZE);
    task_node_t *task_node = TASK_TO_LIST_NODE(task);
    free(task_node);
}

void task_clear(task_t *task) {
    /*
     * a task's resources are always freed together, so we just use the page
     * directory pointer as a flag for whether the task has been cleared
     */
    if (task->page_dir != NULL) {
        page_dir_clear(task->page_dir);
        sfree(task->page_dir, PAGE_SIZE);
        task->page_dir = NULL;

        maps_destroy(task->maps);
        reap_threads(task);

        task_lists_destroy(task);
        task_mutexes_destroy(task);
    }
}

// assumes no active children
void task_destroy(task_t *task) {
    task_clear(task);
    free(TASK_TO_LIST_NODE(task));
}

void reap_threads(task_t *task) {
    node_t *thread_node = pop_first_node(task->zombie_thread_list);
    while (thread_node != NULL) {
        thread_destroy(LIST_NODE_TO_TCB(thread_node));
        thread_node = pop_first_node(task->zombie_thread_list);
    }
}

int task_lists_init(task_t *task) {
    task->live_thread_list = list_init();
    if (task->live_thread_list == NULL) return -1;

    task->zombie_thread_list = list_init();
    if (task->zombie_thread_list == NULL) {
        list_destroy(task->live_thread_list);
        return -1;
    }

    task->child_task_list = list_init();
    if (task->child_task_list == NULL) {
        list_destroy(task->live_thread_list);
        list_destroy(task->zombie_thread_list);
        return -1;
    }

    task->zombie_task_list = list_init();
    if (task->zombie_task_list == NULL) {
        list_destroy(task->live_thread_list);
        list_destroy(task->zombie_thread_list);
        list_destroy(task->child_task_list);
        return -1;
    }

    task->waiting_thread_list = list_init();
    if (task->waiting_thread_list == NULL) {
        list_destroy(task->live_thread_list);
        list_destroy(task->zombie_thread_list);
        list_destroy(task->child_task_list);
        list_destroy(task->zombie_task_list);
        return -1;
    }

    return 0;
}

void task_lists_destroy(task_t *task) {
    list_destroy(task->live_thread_list);
    list_destroy(task->zombie_thread_list);
    list_destroy(task->child_task_list);
    list_destroy(task->zombie_task_list);
    list_destroy(task->waiting_thread_list);
}

int task_mutexes_init(task_t *task) {
    int ret;

    ret = mutex_init(&(task->thread_list_mutex));
    if (ret < 0) {
        return -1;
    }

    ret = mutex_init(&(task->child_task_list_mutex));
    if (ret < 0) {
        mutex_destroy(&(task->thread_list_mutex));
        return -1;
    }

    ret = mutex_init(&(task->wait_mutex));
    if (ret < 0) {
        mutex_destroy(&(task->thread_list_mutex));
        mutex_destroy(&(task->child_task_list_mutex));
        return -1;
    }

    ret = mutex_init(&(task->vanish_mutex));
    if (ret < 0) {
        mutex_destroy(&(task->thread_list_mutex));
        mutex_destroy(&(task->child_task_list_mutex));
        mutex_destroy(&(task->wait_mutex));
        return -1;
    }

    return 0;
}

void task_mutexes_destroy(task_t *task) {
    mutex_destroy(&(task->thread_list_mutex));
    mutex_destroy(&(task->child_task_list_mutex));
    mutex_destroy(&(task->wait_mutex));
    mutex_destroy(&(task->vanish_mutex));
}

// leaves task pointer and status to be set outside
thread_t *thread_init() {
    int size = sizeof(sche_node_t) + sizeof(tcb_tb_node_t) +
               sizeof(thread_node_t) + sizeof(thread_t);
    sche_node_t *sche_node = malloc(size);

    if (sche_node == NULL) {
        lprintf("malloc() failed in thread_init at line %d", __LINE__);
        return NULL;
    }

    thread_t *thread = SCHE_NODE_TO_TCB(sche_node);
    thread->tid = gen_thread_id();

    // TODO CURRENT
    // void *kern_stack = malloc(KERN_STACK_SIZE);
    void *kern_stack = smemalign(KERN_STACK_SIZE, KERN_STACK_SIZE);
    if (kern_stack == NULL) {
        lprintf("smemalign() failed in thread_init at line %d", __LINE__);
        free(sche_node);
        return NULL;
    }
    thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    thread->cur_sp = USER_STACK_START;

    tcb_hashtab_put(thread);
    return thread;
}

void thread_destroy(thread_t *thread) {
    void *kern_stack = (void *)(thread->kern_sp - KERN_STACK_SIZE);

    // TODO CURRENT
    // free(kern_stack);
    sfree(kern_stack, KERN_STACK_SIZE);

    tcb_hashtab_rmv(thread);

    // allocator_free(TCB_TO_SCHE_NODE(thread));
    free(TCB_TO_SCHE_NODE(thread));
}

int validate_user_mem(uint32_t addr, uint32_t len, int perms) {
    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    if (len == 0) return -1;
    uint32_t low = addr;
    uint32_t high = addr + (len - 1);
    // check overflow
    if (high < low) return -1;

    map_t *map = maps_find(task->maps, low, high);
    if (map == NULL) return -1;
    if ((perms & map->perms) != perms) return -1;

    int ret = 0;
    if (low < map->low) {
        ret = validate_user_mem(low, map->low - low, perms);
        if (ret < 0) return -1;
    }
    if (map->high < high) {
        ret = validate_user_mem(map->high + 1, high - map->high, perms);
        if (ret < 0) return -1;
    }

    return 0;
}

// TODO
// returns negative if string goes outside user memory
// returns 0 if string is in valid memory but does not terminate in max_len
// returns length including null terminator otherwise
int validate_user_string(uint32_t addr, int max_len) {
    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    uint32_t low = addr;
    if (max_len <= 0) return -1;
    int len = 0;

    while (len < max_len) {
        map_t *map = maps_find(task->maps, low, low);
        if (map == NULL) return -1;
        if (!(MAP_USER & map->perms)) return -1;

        uint32_t coverage = map->high - low + 1;
        char *check = (char *)low;
        int i;
        for (i = 0; i < coverage; i++) {
            len++;
            if (len > max_len)
                return 0;
            else if (*check == '\0')
                return len;
            check++;
        }

        low += coverage;
    }

    return 0;
}

// assumes cr3 is already set
int load_program(simple_elf_t *header, map_list_t *maps) {
    int ret;
    uint32_t high;

    lprintf("text");
    ret = load_elf_section(header->e_fname,
                           header->e_txtstart,
                           header->e_txtlen,
                           header->e_txtoff,
                           PTE_USER | PTE_PRESENT);
    if (ret < 0) return -1;
    if (header->e_txtlen > 0) {
        high = header->e_txtstart + (header->e_txtlen - 1);
        maps_insert(maps, header->e_txtstart, high, MAP_USER);
    }

    lprintf("dat");
    ret = load_elf_section(header->e_fname,
                           header->e_datstart,
                           header->e_datlen,
                           header->e_datoff,
                           PTE_USER | PTE_WRITE | PTE_PRESENT);
    if (ret < 0) return -1;
    if (header->e_datlen > 0) {
        high = header->e_datstart + (header->e_datlen - 1);
        maps_insert(maps, header->e_datstart, high, MAP_USER | MAP_WRITE);
    }

    lprintf("rodat");
    ret = load_elf_section(header->e_fname,
                           header->e_rodatstart,
                           header->e_rodatlen,
                           header->e_rodatoff,
                           PTE_USER | PTE_PRESENT);
    if (ret < 0) return -1;
    if (header->e_rodatlen > 0) {
        high = header->e_rodatstart + (header->e_rodatlen - 1);
        maps_insert(maps, header->e_rodatstart, high, MAP_USER);
    }

    lprintf("bss");
    ret = load_elf_section(header->e_fname,
                           header->e_bssstart,
                           header->e_bsslen,
                           -1,
                           PTE_USER | PTE_WRITE | PTE_PRESENT);
    if (ret < 0) return -1;
    if (header->e_bsslen > 0) {
        high = header->e_bssstart + (header->e_bsslen - 1);
        maps_insert(maps, header->e_bssstart, high, MAP_USER | MAP_WRITE);
    }

    lprintf("stack");
    ret = load_elf_section(header->e_fname,
                           USER_STACK_LOW,
                           USER_STACK_SIZE,
                           -1,
                           PTE_USER | PTE_WRITE | PTE_PRESENT);
    if (ret < 0) return -1;
    high = USER_STACK_LOW + (USER_STACK_SIZE - 1);
    maps_insert(maps, USER_STACK_LOW, high, MAP_USER | MAP_WRITE);

    // test maps
    // maps_print(maps);

    return 0;
}

int load_elf_section(const char *fname, unsigned long start, unsigned long len,
                     long offset, int pte_flags) {
    lprintf("%p", (void *)start);
    lprintf("%p", (void *)len);
    lprintf("%p", (void *)offset);

    uint32_t low = (uint32_t)start & PAGE_ALIGN_MASK;
    uint32_t high = (uint32_t)(start + len);

    uint32_t addr = low;
    while (addr < high) {
        if (!(get_pte(addr) & PTE_PRESENT)) {
            // TODO fix error handling here
            if (dec_num_free_frames(1) < 0) {
                return -1;
            }
            uint32_t frame_addr = get_frame();
            set_pte(addr, frame_addr, pte_flags);
        }
        addr += PAGE_SIZE;
    }

    if (offset != -1) {
        getbytes(fname, offset, len, (char *)start);
    }

    return 0;
}

// sends orphaned children to the init task
void orphan_children(task_t *task) {
    task_t *init_task = init_thread->task;

    mutex_lock(&(task->child_task_list_mutex));
    node_t *child_node = (node_t *)get_first_node(task->child_task_list);
    mutex_unlock(&(task->child_task_list_mutex));

    while (child_node != NULL) {
        task_t *child = LIST_NODE_TO_TASK(child_node);
        mutex_lock(&(child->vanish_mutex));

        mutex_lock(&(child->thread_list_mutex));
        int live_threads = get_list_size(child->live_thread_list);
        mutex_unlock(&(child->thread_list_mutex));
        /*
         * live_threads can only be 0 if the task vanished on its own in the
         * time between when we (a) found it in the child task list and (b)
         * locked its vanish mutex. it only unlocked the vanish mutex after
         * moving itself to the zombie task list, so we can orphan it later.
         */
        mutex_lock(&(task->child_task_list_mutex));
        if (live_threads > 0) {
            remove_node(task->child_task_list, child_node);

            child->parent_task = init_task;
            mutex_lock(&(init_task->child_task_list_mutex));
            add_node_to_tail(init_task->child_task_list, child_node);
            mutex_unlock(&(init_task->child_task_list_mutex));
        }
        mutex_unlock(&(child->vanish_mutex));

        child_node = get_first_node(task->child_task_list);
        mutex_unlock(&(task->child_task_list_mutex));
    }
}

// sends orphaned zombies to the init task
void orphan_zombies(task_t *task) {
    task_t *init_task = init_thread->task;
    node_t *zombie_node;

    /*
     * we don't need to lock here as there will be no waiters (we are the last
     * alive) and no vanishing children (orphan_children was called first).
     */
    while (get_list_size(task->zombie_task_list) > 0) {
        zombie_node = pop_first_node(task->zombie_task_list);
        mutex_lock(&(init_task->wait_mutex));

        if (get_list_size(init_task->waiting_thread_list) > 0) {
            node_t *node = pop_first_node(init_task->waiting_thread_list);
            mutex_unlock(&(init_task->wait_mutex));

            wait_node_t *waiter = (wait_node_t *)node;
            waiter->zombie = LIST_NODE_TO_TASK(zombie_node);

            // disable interrupts to protect the scheduler
            disable_interrupts();
            waiter->thread->status = RUNNABLE;
            sche_push_back(waiter->thread);
            enable_interrupts();
        } else {
            node_t *last_node = get_last_node(init_task->zombie_task_list);
            if (last_node != NULL) {
                // we can free the previous zombie's vm and thread resources
                task_t *prev_zombie = LIST_NODE_TO_TASK(last_node);
                task_clear(prev_zombie);
            }

            add_node_to_tail(init_task->zombie_task_list, zombie_node);
            mutex_unlock(&(init_task->wait_mutex));
        }
    }
}
