/* SPDX-License-Identifier: GPL-2.0 */
#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>

#include <sys/wait.h>

pid_t mainpid = 0;

int other(void *)
{
    return getpid() != mainpid && getppid() == mainpid;
}

int other_ns(void *uid_)
{
    uid_t uid = (uid_t) (unsigned long) uid_;
    int fd = open("/proc/self/uid_map", O_WRONLY);
    printf("curr uid %u\n", uid);
    dprintf(fd, "0 %u 1\n", uid);
    close(fd);
    printf("current uid %u\n", getuid());
    return 0;
}

int main(int argc, char **argv, char **envp)
{
    mainpid = getpid();
    char stack[4096];
    clone(other, stack + 4096, SIGCHLD, NULL);
    wait(NULL);

    clone(other, stack + 4096, CLONE_VM | SIGCHLD, NULL);
    wait(NULL);

    clone(other_ns, stack + 4096, CLONE_NEWUSER | SIGCHLD, (void *) (unsigned long) getuid());
    wait(NULL);
}
