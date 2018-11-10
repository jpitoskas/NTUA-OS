#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *node)
	//{	
	//	/*
	//	 * Start
	//	 */
	//	printf("PID = %ld, name %s, starting...\n",
	//			(long)getpid(), root->name);
	//	change_pname(root->name);
	//
	//	/* ... */
	//
	//	/*
	//	 * Suspend Self
	//	 */
	//	raise(SIGSTOP);
	//	printf("PID = %ld, name = %s is awake\n",
	//		(long)getpid(), root->name);
	//
	//	/* ... */
	//
	//	/*
	//	 * Exit
	//	 */
	//	exit(0);
	//}		 
{
	pid_t pid;
	int status;

	change_pname(node->name);

	int i;
	/*      printf("%s %d\n", node->name, node->nr_children);       */
	for (i=0; i < node->nr_children; ++i)
	{
		printf("%s: Forking child No.%d...\n", node->name, i+1);
		pid = fork();

		if (pid < 0) {
			perror("fork");
			exit(1);
		}

		if (pid == 0) {
			/*      printf("%s: I just born!\n", node->name);       */
			create_tree(node->children+i);
		}
	}

	pid_t wait_pid;
	for (i=0; i < node->nr_children; ++i)
	{
		printf("%s: Waiting...\n", node->name);
		wait_pid = wait(&status);
		/*sleep(SLEEP_TREE_SEC); */
		explain_wait_status(wait_pid, status);
		if (i == (node->nr_children) - 1) {
			printf("%s: Exiting...\n", node->name);
			exit(3);
		}
	}

	if (node->nr_children == 0) {
		printf("%s: Sleeping...\n", node->name);
		sleep(SLEEP_PROC_SEC);
		printf("%s: Exiting...\n", node->name);
		exit(2);node
	/* printf("%s: Exiting...\n", node->name); */
}


/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
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

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs(root);
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	wait_for_ready_children(1);

	/* for ask2-{fork, tree} */
	/* sleep(SLEEP_TREE_SEC); */

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	kill(pid, SIGCONT);

	/* Wait for the root of the process tree to terminate */
	wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
