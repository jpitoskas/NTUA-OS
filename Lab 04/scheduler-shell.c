#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"
#include "queue.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

Queue q;
int auto_incr = 0;	/* so that each id stays unique even if we add or kill process(es) */
char program_name[42];

typedef struct PCB {
	pid_t pid;
	int id;
	char* exec;
	
}PCB;

PCB shell;
PCB current;


/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
	/*assert(0 && "Please fill me!");*/
	
	node *temp = q.head;
	int i;
	for (i = 0; i < getQueueSize(&q); i++) 
	{
		PCB *ptr = temp->data;
		PCB val = *ptr;
		printf("\nMr. Meeseeks %d with Meeseeks PID: %ld fulfills request: %s \n\n", val.id, (long)val.pid, val.exec);
		temp=temp->next;
	}
	printf("Current running:\n \t\t id: %d\n \t\t PID: %ld\n \t\t Executable: %s\n", current.id, (long)current.pid, current.exec);
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id)
{
	/*assert(0 && "Please fill me!");*/
	int ret = 1;
	int i;
	int size = getQueueSize(&q);
	for (i = 0; i < size; i++) 
	{
		PCB elem;
		dequeue(&q, &elem);
		if ( elem.id == id ) 
		{
			ret = 0;
			kill(elem.pid, SIGKILL);
	 		printf("\nMeeseeks %d with Meeseeks PID: %ld just died. Cause of Death: Existence's Pain\n\n", id, (long)elem.pid);
		}
		else
		{
			enqueue(&q, &elem);
		}
	}
	if (ret==1)
		{
			if (shell.id == id)
			{
				printf("\nCannot kill the Shell. Press <q> to kill Shell \n\n");
			}
			else
			{
				printf("\nError 404: Meeseeks not found\n\n");
			}
		}	
	return ret;
}


/* Create a new task.  */
static void
sched_create_task(char *executable)
{
	/*assert(0 && "Please fill me!");*/
	char *newargv[] = {executable, NULL};
	char *newenviron[] = { NULL };
	pid_t new_pid = fork();
	
	if ( new_pid > 0 )
	{
		int i=0;
		while (executable[i] != '\0'){
			program_name[i] = executable[i];
			i = i + 1;
		}
		program_name[i] = '\0';
		
		PCB task;
		auto_incr = auto_incr + 1;
		task.pid = new_pid;
		task.id = auto_incr;
		task.exec = program_name;
		printf("\nHEY, I'm new Mr Meeseeks %d LOOK AT ME and my Meeseeks PID is %ld. My Purpose is to execute: %s\n\n", task.id, (long)task.pid, task.exec);
		enqueue(&q, &task);
	}
	if ( new_pid == 0 ) {
		raise(SIGSTOP);
		execve(executable, newargv, newenviron );
		exit(0);
	}	
	if (new_pid < 0)
	{
		perror("fork");
	}
	wait_for_ready_children(1);
}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;

		default:
			return -ENOSYS;
	}
}

/* 
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	/*assert(0 && "Please fill me!");*/
	if (kill(current.pid, SIGSTOP) < 0) {
		perror("kill");
		exit(1);
	}
}

/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
	// assert(0 && "Please fill me!");
	
	pid_t p;
	int status;
	
	for (;;) {
		
		p = waitpid(-1, &status, WUNTRACED | WNOHANG);
		
		if (p < 0) {
			perror("waitpid");
			exit(1);
		}
		if (p == 0)
			break;
		
		explain_wait_status(p, status);
		
		/* A child has died */
		if ((WIFEXITED(status) || WIFSIGNALED(status)) && (strcmp(current.exec, "shell") != 0)){
			PCB temp;
			/* Existence is pain for Meeseeks... */
			printf("Mr Meeseeks %d: \t Stops existing! *POOF!* \n", current.id);
			if (getQueueSize(&q) == 0){
				fprintf(stderr, "Rick (Scheduler):\t All your tasks seem to be complete, now give me back the Meeseeks Box. Exiting...\n");
				exit(42);
			}
			dequeue(&q, &temp);
			current = temp;
		}
		else if ((WIFEXITED(status) || WIFSIGNALED(status)) && (strcmp(current.exec, "shell") == 0)){
			if (getQueueSize(&q) == 0){
				fprintf(stderr, "Rick (Scheduler):\t All your tasks seem to be complete, now give me back the Meeseeks Box. Exiting...\n");
				exit(42);
			}
			PCB temp;
			dequeue(&q, &temp);
			current = temp;
		}
		/* A child has stopped due to SIGSTOP/SIGTSTP, etc */
		if (WIFSTOPPED(status)){
			PCB temp;
			if ( strcmp(current.exec, "shell") == 0 )
			{
				printf("This is the Shell bruv. Nothing to do here! \n");
			}
			else
			{
				printf("Mr Meeseeks %d: \t I need more time to fulfill the request!\n", current.id);
			}
			enqueue(&q, &current);
			dequeue(&q, &temp);
			current = temp;
		}
		if (kill(current.pid, SIGCONT) < 0) {
			perror("kill");
			exit(1);
		}
		else{
			if ( strcmp(current.exec, "shell") == 0 )
			{
				printf("This is the Shell bruv. What up biatch?! \n");
			}
			else
			{
				printf("Mr Meeseeks %d: \t Back to work again!\n", current.id);
			}
			alarm(SCHED_TQ_SEC);
		}
	}
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
	}
}


/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL , NULL};
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static pid_t
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];
	return p;
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: \t giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{	
	int nproc;
	pid_t pid;
	char *newenviron[] = { NULL };
 	queueInit(&q, sizeof(PCB));
	
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;
	
	
	/* TODO: add the shell to the scheduler's tasks */

	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */

	nproc = argc - 1; /* number of proccesses goes here */
	
	// if (nproc == 0) {
		// fprintf(stderr, "Rick (Scheduler): \t You don't seem to have any requests, why did you ask for a Meeseeks Box? Exiting...\n");
		// exit(2);
	// }
	// else {
		// fprintf(stderr, "Rick (Scheduler): \t This is a Meeseeks Box, let me show you how it works. You press this...\n");
	// }
	
	int i;
	for (i = 1; i <= nproc; i++) {
		
		char *newargv[] = { argv[i], NULL };
		PCB camtono;
		pid = fork();
		
		if (pid > 0){
			auto_incr = auto_incr + 1;
			printf("HEY, I'm Mr Meeseeks %d LOOK AT ME and my Meeseeks PID is %ld. \n", auto_incr, (long)pid);
			printf("Rick (Scheduler): \t Mr Meeseeks %d, run process named %s. \n", auto_incr, argv[i]);
			printf("Mr Meeseeks %d: \t YES SIR YI. \n", auto_incr);
			camtono.pid = pid;
			camtono.id = auto_incr;
			camtono.exec = argv[i];
			enqueue(&q, &camtono);
			
		}

		if (pid == 0) {
			raise(SIGSTOP);
			execve(argv[i], newargv, newenviron);
			exit(0);
		}
		
		if (pid < 0) {
			perror("Fork");
		}
    }
	
	/* Create the shell. */
	pid_t shell_pid;
	shell_pid = sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
	
	/* TODO: add the shell to the scheduler's tasks */
	auto_incr = auto_incr + 1;
	shell.pid = shell_pid;
	shell.id = auto_incr;
	shell.exec = "shell";
	enqueue(&q, &shell);
	
	
	
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc+1);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	dequeue(&q, &current);
	printf("First Process PID: %ld, Meeseeks ID: %d\n", (long)current.pid, current.id);
	alarm(SCHED_TQ_SEC);
	kill(current.pid, SIGCONT);
	
	shell_request_loop(request_fd, return_fd);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "HEY Morty, what are you doing here?! You are not supposed to be down here!\n");
	return 1;
	
}
