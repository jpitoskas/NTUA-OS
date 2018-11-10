#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void create_tree_signals(struct tree_node *node)
{
	change_pname(node->name);
	printf("(PID = %ld) %s: Starting...\n", (long)getpid(), node->name);
	
	int status;
	pid_t pid[node->nr_children];
	pid_t wait_pid;
	
	int i;
	for (i=0; i < node->nr_children; ++i) 
	{
		pid[i] = fork();

		if (pid[i] < 0) 
		{
			perror("fork");
			exit(1);
		}

		if (pid[i] == 0) 
		{	
			create_tree_signals(node->children+i);
		}
	}
	
	printf("%s: Waiting for ready children...\n", node->name);
	wait_for_ready_children(node->nr_children);		/* if nr_children = 0 => it just continues immediately */
	raise(SIGSTOP);

	printf("(PID = %ld) %s: Woke up!!\n", (long)getpid(), node->name);
	
	for (i=0; i < node->nr_children; ++i) 
	{
		kill(pid[i], SIGCONT);
		printf("%s: Waiting...\n", node->name);
		wait_pid = wait(&status);
		explain_wait_status(wait_pid, status);
		
		if (i == (node->nr_children) -1)	/* waiting for children to terminate and parent exits */
		{
			printf("%s: Exiting...\n", node->name);
			exit(2);				
		}			
	}

	printf("%s: Exiting...\n", node->name);
	exit(0);
}

/*
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;

	if (argc < 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	root = get_tree_from_file(argv[1]);
	print_tree(root);
	pid = fork();
	if (pid < 0) {
		perror("first fork");
		exit(1);
	}
	if (pid == 0) {
		create_tree_signals(root);
		exit(1);
	}
	
	wait_for_ready_children(1);

	show_pstree(pid);
	
	kill(pid, SIGCONT);

	wait(&status);
	explain_wait_status(pid, status);
	
	return 0;
}
