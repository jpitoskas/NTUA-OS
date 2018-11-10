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

Queue q;

typedef struct PCB {
	pid_t pid;
	int id;
	char* exec;
	
}PCB;

PCB current;

/*
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	// assert(0 && "Please fill me!");
	
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
		if (WIFEXITED(status) || WIFSIGNALED(status)){
			PCB temp;
			/* Existence is pain for Meeseeks... */
			printf("Mr Meeseeks %d: Stops existing! *POOF!* \n", current.id);
			if (getQueueSize(&q) == 0){
				fprintf(stderr, "Rick (Scheduler): All your tasks seem to be complete, now give me back the Meeseeks Box. Exiting...\n");
				exit(42);
			}
			dequeue(&q, &temp);
			current = temp;
		}
		/* A child has stopped due to SIGSTOP/SIGTSTP, etc */
		if (WIFSTOPPED(status)){
			PCB temp;
			printf("Mr Meeseeks %d: I need more time to fulfill the request!\n", current.id);
			enqueue(&q, &current);
			dequeue(&q, &temp);
			current = temp;
		}
		if (kill(current.pid, SIGCONT) < 0) {
			perror("kill");
			exit(1);
		}
		else{
			printf("Mr Meeseeks %d: Back to work again!\n", current.id);
			alarm(SCHED_TQ_SEC);
		}
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

int main(int argc, char *argv[])
{
	pid_t pid;
	int nproc = argc - 1; /* number of proccesses goes here */
	char *newenviron[] = { NULL };

 	queueInit(&q, sizeof(PCB));
	
	if (nproc == 0) {
		fprintf(stderr, "Rick (Scheduler): You don't seem to have any requests, why did you ask for a Meeseeks Box? Exiting...\n");
		exit(2);
	}
	else {
		fprintf(stderr, "Rick (Scheduler): This is a Meeseeks Box, let me show you how it works. You press this...\n");
	}
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	
	int i;
	for (i = 1; i <= nproc; i++) {
		
		char *newargv[] = { argv[i], NULL };
		PCB camtono;
		pid = fork();
		
		if (pid > 0){
			printf("HEY, I'm Mr Meeseeks %d LOOK AT ME and my Meeseeks PID is %ld. \n", i, (long)pid);
			printf("Rick (Scheduler): Mr Meeseeks %d, run process named %s. \n", i, argv[i]);
			printf("Mr Meeseeks %d: YES SIR YI. \n", i);
			camtono.pid = pid;
			camtono.id = i;
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
	
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);
	
	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();
	
	dequeue(&q, &current);
	printf("First Process PID: %ld, Meeseeks ID: %d\n", (long)current.pid, current.id);
	alarm(SCHED_TQ_SEC);
	kill(current.pid, SIGCONT);

	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "HEY Morty, what are you doing here?! You are not supposed to be down here!\n");
	return 1;
	
}
