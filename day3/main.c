/* SPDX-License-Identifier: GPL-2.0 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define persistent __attribute__((section("persistent"), aligned(4096)))

persistent int count = 10;

void setup_persistent(const char *filename)
{
    int fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fd < 0)
        return (void) perror("setup_persistent");
    if (ftruncate(fd, 4096) < 0)
        return (void) perror("ftruncate");
    void *ret = mmap(&count, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);

    if (ret == MAP_FAILED)
    {
        perror("mmap");
        return;
    }

    close(fd);
}

int main(int argc, char **argv)
{
    setup_persistent("mmap.persistent");
    printf("count = %d\n", count++);
}
