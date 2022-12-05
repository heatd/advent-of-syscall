/* SPDX-License-Identifier: GPL-2.0 */
#include <unistd.h>
#include <fcntl.h>

/* catty(1) - bad cat(1) implementation */
int main(int argc, char **argv, char **envp)
{
	for (int i = 1; i < argc; i++)
	{
		int fd = open(argv[i], O_RDONLY);
		if (fd < 0)
			return 1;

		char buf[4096];
		ssize_t len;
		while ((len = read(fd, buf, 4096)) > 0)
		{
			write(STDOUT_FILENO, buf, len);
		}

		close(fd);
	}
}

