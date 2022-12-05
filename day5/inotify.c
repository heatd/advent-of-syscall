/* SPDX-License-Identifier: GPL-2.0 */

#include <err.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/inotify.h>

int main(int argc, char **argv)
{
    int infd = inotify_init1(IN_CLOEXEC);

    if (infd < 0)
        err(1, "inotify_init");

    int desc = inotify_add_watch(infd, ".", IN_OPEN | IN_ACCESS | IN_CLOSE);

    if (desc < 0)
        err(1, "inotify_add_watch");
    
    char buf[sizeof(struct inotify_event) + NAME_MAX + 1];

    ssize_t st;

    while ((st = read(infd, buf, sizeof(buf))) > 0)
    {
        struct inotify_event *ev = (struct inotify_event *) buf;

        for (char *ptr = buf; ptr < buf + st; ptr += sizeof(struct inotify_event) + ev->len, ev = (struct inotify_event *) ptr)
        {
            if (ev->wd == desc)
            {
                const char *msg;

                while (ev->mask)
                {
                    if (ev->mask == IN_OPEN)
                        msg = "open", ev->mask &= ~IN_OPEN;
                    else if (ev->mask == IN_ACCESS)
                        msg = "access", ev->mask &= ~IN_ACCESS;
                    else if (ev->mask == IN_CLOSE)
                        msg = "close", ev->mask &= ~IN_CLOSE;
                    else if (ev->mask == IN_CLOSE_NOWRITE)
                        msg = "close_nowrite", ev->mask &= ~IN_CLOSE_NOWRITE;
                    else if (ev->mask == IN_CLOSE_WRITE)
                        msg = "close_write", ev->mask &= ~IN_CLOSE_WRITE;
                    else
                        msg = "unknown", ev->mask = 0;
                    printf("./%s [%s]\n", ev->name, msg);
                }
            }
        }
    }
}
