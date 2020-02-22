#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#define MUTEX_FILE	"/tmp/chat_file.mutex"
#define CHAT_FILE	"/tmp/chat_file.base"

int		my_lock = 0;
off_t	offset = 0;

int		try_lock(void)
{
	return ((-1) != open(MUTEX_FILE, O_CREAT | O_EXCL, 0666));
}

void	lock(void)
{
	while (!try_lock());
	my_lock = 1;
}

void	unlock(void)
{
	my_lock = 0;
	unlink(MUTEX_FILE);
}

void	quit(int num)
{
	if (my_lock)
		unlock();
	exit(0);
}

void	reader(int num)
{
	struct 	stat st;
	int		fd;
	int		nread;
	char	buf[128];

	if (stat(CHAT_FILE, &st) == -1)
		goto rexit;
	if (offset >= st.st_size)
		goto rexit;
	if (!try_lock())
		goto rexit;
	my_lock = 1;
	if ((fd = open(CHAT_FILE, O_RDONLY)) >= 0)
	{
		lseek(fd, offset, SEEK_SET);
		while((nread = read(fd, buf, 128)) > 0)
		{
			write(1, buf, nread);
			offset += nread;
		}
		close(fd);
	}
	unlock();
	rexit:
		alarm(1);
}

int		main(int argc, char **argv)
{
	int		fd;
	int		nread;
	size_t	len;
	char	*line;

	signal(SIGINT, quit);
	signal(SIGALRM, reader);
	alarm(1);
	while ((nread = getline(&line, &len, stdin)) > 0)
	{
		lock();
		if ((fd = open(CHAT_FILE, O_CREAT | O_APPEND | O_WRONLY, 0666)))
		{
			offset += write(fd, line, nread);
			close(fd);
		}
		unlock();
		free(line);
		line = NULL;
		len = 0;
	}
	return (0);
}
