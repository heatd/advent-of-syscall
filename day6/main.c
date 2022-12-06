/* SPDX-License-Identifier: GPL-2.0 */
#define _GNU_SOURCE
#include <signal.h>
#include <err.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ucontext.h>

sig_atomic_t do_exit = 0;

void segv_handler(int sig, siginfo_t *info, void *mctx)
{
    unsigned long addr = (unsigned long) info->si_addr & -4096;
    if (mmap((void *) addr, 4096, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0) == MAP_FAILED)
        abort();
}

void ill_handler(int sig, siginfo_t *info, void *mctx)
{
    ucontext_t *ctx = mctx;
    ctx->uc_mcontext.gregs[REG_RIP] += 2; // size of ud2 in x86_64
}

void int_handler(int sig, siginfo_t *info, void *mctx)
{
    do_exit = 1;
}

int main(int argc, char **argv)
{
    struct sigaction sa;
    sa.sa_sigaction = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;

    if (sigaction(SIGSEGV, &sa, NULL) < 0)
        err(1, "sigaction");
    
    sa.sa_sigaction = ill_handler;
    if (sigaction(SIGILL, &sa, NULL) < 0)
        err(1, "sigaction");
    
    sa.sa_sigaction = int_handler;
    if (sigaction(SIGINT, &sa, NULL) < 0)
        err(1, "sigaction");
    
    volatile char *madptr = (volatile char *) 0xdeadbeef;

    while (!do_exit)
    {
        *madptr = 10;
        __asm__("ud2");
    }

    return 0;
}
