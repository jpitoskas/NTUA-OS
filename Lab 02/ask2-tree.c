#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3


void create_tree(struct tree_node *node)
{
	pid_t pid;
	int status;

	change_pname(node->name);

	int i;
/*	printf("%s %d\n", node->name, node->nr_children); 	*/
	for (i=0; i < node->nr_children; ++i)
	{
		printf("%s: Forking child No.%d...\n", node->name, i+1);
		pid = fork();
		
		if (pid < 0) {
			perror("fork");
			exit(1);
		}

		if (pid == 0) {
		/*	printf("%s: I just born!\n", node->name);	*/
			create_tree(node->children+i);
		}
	}

	pid_t wait_pid;
	for (i=0; i < node->nr_children; ++i)
	{
		printf("%s: Waiting...\n", node->name);
		wait_pid = wait(&status);
		//sleep(SLEEP_TREE_SEC);
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
		exit(2);
	}
	
	/* printf("%s: Exiting...\n", node->name); */
}


int main(int argc, char** argv)
{
	struct tree_node *root;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
		exit(1);
	}

	root = get_tree_from_file(argv[1]);
	print_tree(root);

	pid_t pid;
	int status;

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}
	if (pid == 0) {
		create_tree(root);
		exit(1);
	}

	sleep(SLEEP_TREE_SEC);

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
