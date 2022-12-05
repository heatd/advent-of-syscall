/* SPDX-License-Identifier: GPL-2.0 */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <err.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/futex.h>
#include <stdatomic.h>
#include <assert.h>
#include <sys/syscall.h>

struct semaphore
{
    _Atomic unsigned int counter;
};

void sem_init(struct semaphore *sem, unsigned int units)
{
    sem->counter = units;
}

int futex_wait(unsigned int *addr, unsigned int expected)
{
    return syscall(SYS_futex, addr, FUTEX_WAIT, expected, 0, 0, 0);
}

int futex_wake(unsigned int *addr, unsigned int wake)
{
    return syscall(SYS_futex, addr, FUTEX_WAKE, wake, 0, 0, 0);
}

void sem_down(struct semaphore *sem)
{
    for (;;)
    {
        unsigned int counter = sem->counter;

        if (counter > 0)
        {
            if (atomic_compare_exchange_strong(&sem->counter, &counter, counter - 1))
                return;
        }
        else
        {
            futex_wait((unsigned int *) &sem->counter, 0);
        }
    }
}

void sem_up(struct semaphore *sem)
{
    unsigned int old = atomic_fetch_add(&sem->counter, 1);

    if (old == 0)
    {
        futex_wake((unsigned int *) &sem->counter, 1);
    }
}

struct bbuf
{
    unsigned int pos;
    unsigned int data[100];
    struct semaphore empty;
    struct semaphore full;
    struct semaphore lock;
};

void bb_init(struct bbuf *bbuf)
{
    bbuf->pos = 0;
    sem_init(&bbuf->empty, 100);
    sem_init(&bbuf->full, 0);
    sem_init(&bbuf->lock, 1);
}

void bb_put(struct bbuf *bb, unsigned int data)
{
    sem_down(&bb->empty);

    sem_down(&bb->lock);

    bb->data[bb->pos++] = data;

    sem_up(&bb->lock);

    sem_up(&bb->full);
}

unsigned int bb_get(struct bbuf *bb)
{
    unsigned int u;
    sem_down(&bb->full);

    sem_down(&bb->lock);

    u = bb->data[0];

    memmove(&bb->data, &bb->data[1], (bb->pos - 1) * sizeof(unsigned int));
    bb->pos--;

    sem_up(&bb->lock);

    sem_up(&bb->empty);

    return u;
}

int main()
{
    void *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

    if (ptr == MAP_FAILED)
    {
        err(1, "mmap");
    }

    struct bbuf *bb = ptr;

    bb_init(bb);

    pid_t pid = fork();

    if (pid < 0)
        err(1, "fork");
    else if (pid == 0)
    {
        sleep(1);
        for (unsigned int i = 0; i < 100; i++)
            bb_put(bb, i);
    }
    else
    {
        for (unsigned int i = 0; i < 100; i++)
        {
            unsigned int data = bb_get(bb);
            //printf("child got %u\n", data);
            assert(data == i);
        }
    }
}
