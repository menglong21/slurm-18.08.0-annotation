/*****************************************************************************\
 * src/slurmd/slurmd/slurmd.h - header for slurmd
 *****************************************************************************
 *  Copyright (C) 2002-2007 The Regents of the University of California.
 *  Copyright (C) 2008-2010 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Mark Grondona <mgrondona@llnl.gov>.
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
#ifndef _SLURMD_H
#define _SLURMD_H

#include <inttypes.h>
#include <pthread.h>
#include <sys/types.h>

#include "src/common/log.h"
#include "src/common/list.h"
#include "src/common/slurm_protocol_api.h"
#include "src/common/slurm_cred.h"

#ifndef __USE_XOPEN_EXTENDED
extern pid_t getsid(pid_t pid);		/* missing from <unistd.h> */
extern pid_t getpgid(pid_t pid);
#endif

extern int devnull;
extern bool get_reg_resp;

/*
 * Message aggregation types
 */
typedef enum {
	WINDOW_TIME,
	WINDOW_MSGS
} msg_aggr_param_type_t;

/*
 * Global config type
 */
typedef struct slurmd_config {
	char         *prog;		/* 程序basename		   */
	char         ***argv;           /* 指向参数向量的指针,三级指针     */
	int          *argc;             /* 指向参数计数的指针       */
	char         *auth_info;	/* 用于msg身份验证的auth_info */ 
	char         *cluster_name; 	/* conf集群名		   */
	char         *hostname;	 	/* 本地主机名		   */
	uint16_t     cpus;              /* lowest-level 逻辑处理器数 */
	uint16_t     boards;            /* 总板数 */
	uint16_t     sockets;           /* 总插槽数 */
	uint16_t     threads;           /* 每个核心线程数	*/
	char         *cpu_spec_list;    /* cpu specialization list         */
	uint16_t     core_spec_cnt;     /* core specialization count       */
	uint64_t     mem_spec_limit;    /* memory specialization limit     */
	uint16_t     cores;             /* 每个插槽核心数          */
	uint16_t     conf_cpus;         /* 配置文件中 logical processors    */
	uint16_t     conf_boards;       /* conf file boards count          */
	uint16_t     conf_sockets;      /* conf file sockets count         */
	uint16_t     conf_cores;        /* conf file core count            */
	uint16_t     conf_threads;      /* conf file thread per core count */
	uint16_t     actual_cpus;       /* 实际 logical processors       */
	uint16_t     actual_boards;     /* actual boards count             */
	uint16_t     actual_sockets;    /* actual sockets count            */
	uint16_t     actual_cores;      /* actual core count               */
	uint16_t     actual_threads;    /* actual thread per core count    */
	uint64_t     real_memory_size;  /* 实际存储器容量	   */
	uint32_t     tmp_disk_space;    /* 临时磁盘大小	*/
	uint32_t     up_time;		/* 自上次启动后的秒数    */
	uint16_t     block_map_size;	/* 块映射大小               */
	uint16_t     *block_map;	/* abstract->machine 块映射     */
	uint16_t     *block_map_inv;	/* machine->abstract (inverse) map */
	uint16_t      cr_type;		/* 消耗性资源类型       *
					 * CR_SOCKET, CR_CORE, CR_MEMORY,  *
					 * CR_DEFAULT, etc.                */
	char         *hwloc_xml;	/* path of hwloc xml file if using */
	time_t        last_update;	/* 构建参数的上次更新时间 */
	uint16_t      mem_limit_enforce; /* enforce mem limit on running job */
	int           nice;		/* 命令行的nice值规格    */
	char         *node_name;	/* 节点名称                       */
	char         *node_addr;	/* 节点地址                 */
	char         *node_topo_addr;   /* 节点拓扑地址         */
	char         *node_topo_pattern;/* 节点拓扑地址模式 */
	char         *conffile;		/* 配置文件名                 */
	char         *logfile;		/* 日志文件          */
	int          syslog_debug;	/* send output to both logfile and
					 * syslog */
	char         *spooldir;		/* SlurmdSpoolDir		   */
	char         *pidfile;		/* PidFile location		   */
	char         *health_check_program; /* run on RPC request or at start */
	uint64_t     health_check_interval; /* Interval between runs       */
	char         *tmpfs;		/* directory of tmp FS             */
	char         *pubkey;		/* location of job cred public key */
	char         *epilog;		/* Path to Epilog script	   */
	char         *prolog;		/* Path to prolog script           */
	char         *select_type;	/* SelectType                      */
	char         *stepd_loc;	/* slurmstepd path                 */
	char         *task_prolog;	/* per-task prolog script          */
	char         *task_epilog;	/* per-task epilog script          */
	int           port;		/* slurmd 端口号               */
	int           lfd;		/* slurmd 监听文件描述符   */
	pid_t         pid;		/* server pid                      */
	log_options_t log_opts;         /* current logging options         */
	uint16_t      log_fmt;          /* Log file timestamp format flag  */
	int           debug_level;	/* logging detail level            */
	uint16_t      debug_level_set;	/* debug_level set on command line */
	uint64_t      debug_flags;	/* DebugFlags configured           */
	int	      boot_time:1;      /* Report node boot time now (-b)  */
	int           daemonize:1;	/* daemonize flag (-D)		   */
	bool          def_config;       /* We haven't read in the config yet */
	int	      cleanstart:1;     /* clean start requested (-c)      */
	int           mlock_pages:1;	/* mlock() slurmd  */

	slurm_cred_ctx_t vctx;          /* slurm_cred_t verifier context   */

	uint16_t	slurmd_timeout;	/* SlurmdTimeout                   */
	uid_t           slurm_user_id;	/* UID that slurmctld runs as      */
	pthread_mutex_t config_mutex;	/* lock for slurmd_config access   */
	uint16_t        acct_freq_task;
	char           *job_acct_gather_freq;
	char           *job_acct_gather_type; /* job accounting gather type */
	char           *job_acct_gather_params; /* job accounting gather params */
	char           *acct_gather_energy_type; /*  */
	char           *acct_gather_filesystem_type; /*  */
	char           *acct_gather_interconnect_type; /*  */
	char           *acct_gather_profile_type; /*  */
	char           *msg_aggr_params;      /* 消息聚合参数 */
	uint64_t        msg_aggr_window_msgs; /* msg aggr window size in msgs */
	uint64_t        msg_aggr_window_time; /* msg aggr window size in time */
	uint16_t	use_pam;
	uint32_t	task_plugin_param; /* TaskPluginParams, expressed
					 * using cpu_bind_type_t flags */
	uint16_t	propagate_prio;	/* PropagatePrioProcess flag       */

	List		starting_steps; /* steps that are starting but cannot
					   receive RPCs yet */
	pthread_cond_t	starting_steps_cond;
	List		prolog_running_jobs;
	pthread_cond_t	prolog_running_cond;
	char         *plugstack;	/* path to SPANK config file	*/
	uint16_t      kill_wait;	/* seconds between SIGXCPU to SIGKILL
					 * on job termination */
	char           *x11_params;	/* X11Parameters */
} slurmd_conf_t;

extern slurmd_conf_t * conf;
extern int fini_job_cnt;
extern uint32_t *fini_job_id;
extern pthread_mutex_t fini_job_mutex;

/* Send node registration message with status to controller
 * IN status - same values slurm error codes (for node shutdown)
 * IN startup - non-zero if slurmd just restarted
 */
int send_registration_msg(uint32_t status, bool startup);

/*
 * save_cred_state - save the current credential list to a file
 * IN list - list of credentials
 * RET int - zero or error code
 */
int save_cred_state(slurm_cred_ctx_t vctx);

/* Run the health check program if configured */
int run_script_health_check(void);

/* Handler for SIGTERM; can also be called to shutdown the slurmd. */
void slurmd_shutdown(int signum);

#endif /* !_SLURMD_H */
