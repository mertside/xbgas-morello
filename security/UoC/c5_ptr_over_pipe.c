/*
 * SPDX-License-Identifier: BSD-2-Clause-DARPA-SSITH-ECATS-HR0011-18-C-0016
 * Copyright (c) 2020 SRI International
 */
#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include "xbrtime_morello.h"

const char hello[] = "Hello World!";

int
main(void)
{

  xbrtime_init();

  int num_pes = xbrtime_num_pes();

	int fds[2];
	pid_t pid;
	const char *ptr;

	if (pipe(fds) == -1)
    printf("pipe...\n");
		err(1, "pipe");
	if ((pid = fork()) == -1)
    printf("fork...\n");
		err(1, "fork");
	if (pid == 0) {
		ptr = hello;
		if (write(fds[0], &ptr, sizeof(ptr)) != sizeof(ptr))
			printf("write...\n");
      err(1, "write");
	} else {
		if (read(fds[1], &ptr, sizeof(ptr)) != sizeof(ptr))
			printf("read...\n");
      err(1, "read");
		printf("received %s\n", ptr);
	}

	return 0;
}
