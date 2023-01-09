#include "user/threads.h"

#include "kernel/types.h"
#include "user/setjmp.h"
#include "user/user.h"
#define NULL 0

static struct thread *current_thread = NULL;
static struct thread *root_thread = NULL;
static int id = 1;
static jmp_buf env_st;   // env for main
static jmp_buf env_tmp;  // env for threads
// TODO: necessary declares, if any

struct thread *thread_create(void (*f)(void *), void *arg) {
    // creates a new thread and allocates the space in stack to the thread
    struct thread *t = (struct thread *)malloc(sizeof(struct thread));
    // unsigned long stack_p = 0;
    unsigned long new_stack_p;
    unsigned long new_stack;
    new_stack = (unsigned long)malloc(sizeof(unsigned long) * 0x100);
    new_stack_p = new_stack + 0x100 * 8 - 0x2 * 8;
    t->fp = f;
    t->arg = arg;
    t->ID = id;
    t->buf_set = 0;
    t->stack = (void *)new_stack;      // frame pointer
    t->stack_p = (void *)new_stack_p;  // stack pointer
    t->left = t->right = t->parent = NULL;
    id++;
    return t;
}
void thread_add_runqueue(struct thread *t) {
    // adds an initialized thread into the runqueue
    // binary tree made of thread*
    if (current_thread == NULL) {
        // TODO
        root_thread = current_thread = t;
        return;
    } else {
        // TODO
        t->parent = current_thread;
        if (current_thread->left == NULL)
            current_thread->left = t;
        else if (current_thread->right == NULL)
            current_thread->right = t;
        else {
            // TODO discard t
            free(t->stack);
            free(t);
        }
    }
}
void thread_yield(void) {
    // suspends the current thread by saving its context to the jmp_buf
    // TODO
    if (setjmp(current_thread->env) == 0) {
        schedule();
        dispatch();
    }
}
void dispatch(void) {
    // executes a thread decided by schedule()
    // TODO
    // debug
    /*
    if (current_thread != NULL) {
        printf("\n");
        printf("tid: %d\n", current_thread->ID);
        if (current_thread->left != NULL)
            printf(" left: %d\n", current_thread->left->ID);
        if (current_thread->right != NULL)
            printf(" right: %d\n", current_thread->right->ID);
        if (current_thread->parent != NULL)
            printf(" parent: %d\n", current_thread->parent->ID);
    }
    */

    if (current_thread->buf_set == 0) {
        // have never run before
        // move the stack pointer sp to the allocated stack of the threads
        // modify/access with setjmp/longjmp
        // setjmp(current_thread->env);
        if (setjmp(env_tmp) == 0) {
            env_tmp->sp = (unsigned long)current_thread->stack_p;
            longjmp(env_tmp, 1);
        }
        current_thread->buf_set = 1;
        current_thread->fp(current_thread->arg);
        thread_exit();
    } else {
        // restore the context
        longjmp(current_thread->env, 1);
    }
}
void schedule(void) {
    // decide which thread to run next
    // updates current_thread
    // TODO
    if (current_thread == NULL || root_thread == NULL) {
        fprintf(2, "Error on schedule!!\n");
        return;
    }
    struct thread *tmp_thread = NULL;
    while (1) {
        if (current_thread == NULL) {
            current_thread = root_thread;
            return;
        }
        int avail[2];
        avail[0] = avail[1] = 1;

        if (current_thread->left == NULL || current_thread->left == tmp_thread)
            avail[0] = 0;
        if (current_thread->right == NULL)
            avail[1] = 0;
        if (current_thread->right == tmp_thread && tmp_thread != NULL)
            avail[0] = avail[1] = 0;

        if (avail[0]) {
            current_thread = current_thread->left;
            return;
        } else if (avail[1]) {
            current_thread = current_thread->right;
            return;
        }
        tmp_thread = current_thread;
        current_thread = current_thread->parent;
    }
}
void thread_exit(void) {
    // removes the calling thread from the runqueue
    if (current_thread == root_thread && current_thread->left == NULL && current_thread->right == NULL) {
        // TODO
        // Hint: No more thread to execute
        free(current_thread->stack);
        free(current_thread);
        current_thread = root_thread = NULL;
        longjmp(env_st, 1);
    } else {
        // TODO
        struct thread *t = current_thread, *t2 = current_thread;
        if (t->left == NULL && t->right == NULL) {
            // leaf
            schedule();
            if (t->parent->left == t)
                t->parent->left = NULL;
            else
                t->parent->right = NULL;
        } else {
            t2 = t;
            while (t2->left != NULL || t2->right != NULL) {
                if (t2->right != NULL)
                    t2 = t2->right;
                else
                    t2 = t2->left;
            }
            if (t2->parent != NULL) {
                if (t2->parent->left == t2)
                    t2->parent->left = NULL;
                else
                    t2->parent->right = NULL;
            }
            if (t->parent != NULL) {
                if (t->parent->left == t)
                    t->parent->left = t2;
                else
                    t->parent->right = t2;
            }
            if (t->left != NULL)
                t->left->parent = t2;
            if (t->right != NULL)
                t->right->parent = t2;
            t2->left = t->left, t2->right = t->right, t2->parent = t->parent;
            current_thread = t2;
            if (t2->parent == NULL) root_thread = t2;
            schedule();
        }
        free(t->stack);
        free(t);
        dispatch();
    }
}

void thread_start_threading(void) {
    // return when all threads have exited
    // TODO
    if (setjmp(env_st) == 0) {
        dispatch();
    }
    // printf("end\n");
}