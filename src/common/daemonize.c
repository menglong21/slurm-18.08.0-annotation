/*****************************************************************************\
 *  daemonize.c - daemonization routine
 *****************************************************************************
 *  Copyright (C) 2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Mark A. Grondona <mgrondona@llnl.gov>.
 *  CODE-OCEC-09-009. All rights reserved.
 *
 *  This file is part of Slurm, a resource management program.
 *  For details, see <https://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  Slurm is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  Slurm is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Slurm; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#include <config.h>

#define _GNU_SOURCE

#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "src/common/daemonize.h"
#include "src/common/fd.h"
#include "src/common/log.h"
#include "src/common/macros.h"
#include "src/common/xassert.h"

/*
 * Double-fork and go into background.
 * Caller is responsible for umasks  1.调用该函数之前处理了umask
 */
int xdaemon(void)
{
	int devnull;
	//2.第一次fork的作用就是让shell认为这条命令已经终止，不用挂在终端输入上
	switch (fork()) {
		case  0 : break;        /* child *///子进程继续执行
		case -1 : return -1;
		default : _exit(0);     /* exit parent *///父进程退出
	}
	//3.创建一个新的Session,并成为Session Leader
	if (setsid() < 0)
		return -1;
	//4. 第二次fork不是必须的。目的是防止进程再次打开一个控制终端。
	//因为打开一个控制终端的前提条件是该进程必须是会话首进程。
	//再fork一次，子进程ID != sid（sid是进程父进程的sid）。
	//所以也无法打开新的控制终端，也不会成为前台进程。
	//APUE：如果一个进程fork一个子进程，但不要它等待子进程终止，
	//也不希望子进程处于僵死状态直到父进程终止，实现这一要求的诀窍是调用fork两次。
	switch (fork()) {
		case 0 : break;         /* child */
		case -1: return -1;
		default: _exit(0);      /* exit parent */
	}

	/*
	 * dup stdin, stdout, and stderr onto /dev/null
	 */
	 //5.将stdin, stdout, and stderr重定向到/dev/null
	devnull = open("/dev/null", O_RDWR);
	if (devnull < 0)
		error("Unable to open /dev/null: %m");
	if (dup2(devnull, STDIN_FILENO) < 0)
		error("Unable to dup /dev/null onto stdin: %m");
	if (dup2(devnull, STDOUT_FILENO) < 0)
		error("Unable to dup /dev/null onto stdout: %m");
	if (dup2(devnull, STDERR_FILENO) < 0)
		error("Unable to dup /dev/null onto stderr: %m");
	if (close(devnull) < 0)
		error("Unable to close /dev/null: %m");

	return 0;
}

/*
 * Read and return pid stored in pidfile.
 * Returns 0 if file doesn't exist or pid cannot be read.
 * If pidfd != NULL, the file will be kept open and the fd
 * returned.
 */
pid_t
read_pidfile(const char *pidfile, int *pidfd)
{
	int fd;
	FILE *fp = NULL;
	unsigned long pid;
	pid_t         lpid;

	if ((fd = open(pidfile, O_RDONLY)) < 0)
		return ((pid_t) 0);

	if (!(fp = fdopen(fd, "r"))) {
		error ("Unable to access old pidfile at `%s': %m", pidfile);
		(void) close(fd);
		return ((pid_t) 0);
	}

	if (fscanf(fp, "%lu", &pid) < 1) {
		error ("Possible corrupt pidfile `%s'", pidfile);
		(void) close(fd);
		return ((pid_t) 0);
	}

	if ((lpid = fd_is_read_lock_blocked(fd)) == (pid_t) 0) {
		verbose ("pidfile not locked, assuming no running daemon");
		(void) close(fd);
		return ((pid_t) 0);
	}

	if (lpid != (pid_t) pid)
		fatal ("pidfile locked by %lu but contains pid=%lu",
		       (unsigned long) lpid, (unsigned long) pid);

	if (pidfd != NULL)
		*pidfd = fd;
	else
		(void) close(fd);

/*	fclose(fp);	NOTE: DO NOT CLOSE, "fd" CONTAINS FILE DESCRIPTOR */
	return (lpid);
}



int
create_pidfile(const char *pidfile, uid_t uid)
{
	FILE *fp;
	int fd;

	xassert(pidfile != NULL);
	xassert(pidfile[0] == '/');

	fd = open(pidfile, O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		error("Unable to open pidfile `%s': %m", pidfile);
		return -1;
	}

	if (!(fp = fdopen(fd, "w"))) {
		error("Unable to access pidfile at `%s': %m", pidfile);
		(void) close(fd);
		return -1;
	}

	if (fd_get_write_lock(fd) < 0) {
		error ("Unable to lock pidfile `%s': %m", pidfile);
		goto error;
	}
	//将pid写入文件
	if (fprintf(fp, "%lu\n", (unsigned long) getpid()) == EOF) {
		error("Unable to write to pidfile `%s': %m", pidfile);
		goto error;
	}

	fflush(fp);

	if (uid && (fchown(fd, uid, -1) < 0))
		error ("Unable to reset owner of pidfile: %m");

/*	fclose(fp);	NOTE: DO NOT CLOSE, "fd" CONTAINS FILE DESCRIPTOR */
	return fd;

  error:
	(void)fclose(fp); /* Ignore errors */

	if (unlink(pidfile) < 0)
		error("Unable to remove pidfile `%s': %m", pidfile);
	return -1;
}

void
test_core_limit(void)
{
#ifdef RLIMIT_CORE
	struct rlimit rlim[1];
	if (getrlimit(RLIMIT_CORE, rlim) < 0)//RLIMIT_CORE 内核转存文件的最大长度
		error("Unable to get core limit");
	else if (rlim->rlim_cur != RLIM_INFINITY) {
		rlim->rlim_cur /= 1024;	/* bytes to KB */
		if (rlim->rlim_cur < 2048) {
			verbose("Warning: Core limit is only %ld KB",
				(long int) rlim->rlim_cur);
		}
	}
#endif
	return;
}
