#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "tree.h"
#include "proc-common.h"

void create_tree_pipes(struct tree_node *node, int fd[2])
{
	change_pname(node->name);
/*	printf("(PID = %ld) %s: Starting...\n", (long)getpid(), node->name);	*/

	int num;
	int status;
	pid_t pid[node->nr_children];
	pid_t wait_pid;

	/*
	   int fd[2];

	   if (pipe(fd) < 0) 
	   {
	   perror("pipe");
	   }
	   */

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
			create_tree_pipes(node->children+i, fd);
		}
	}
	/*
	printf("%s: Waiting for ready children...\n", node->name);
	*/
	wait_for_ready_children(node->nr_children);
	raise(SIGSTOP);

	if (node->nr_children == 0) {
		num = atoi(node->name);
		if ( write(fd[1], &num, sizeof(num)) != sizeof(num) )
		{
			perror("write to pipe");
			exit(1);
		}
		printf("%s: Writing %d to pipe\n", node->name, num);
	}

/*	printf("(PID = %ld) %s: Woke up!!\n", (long)getpid(), node->name);	*/

	int afla[2];

	for (i=0; i < node->nr_children; ++i)
	{	
		kill(pid[i], SIGCONT);
	/*	printf("%s: Waiting...\n", node->name);	*/
		wait_pid = wait(&status);
		explain_wait_status(wait_pid, status);

		if ( read(fd[0], &afla[i], sizeof(afla[i])) != sizeof(afla[i]) ) {
			perror("read from pipe");
			exit(1);
		}

	}

	int result;
	if ( strcmp(node->name , "+") == 0 ) {
		result = afla[0] + afla[1];
		printf("Result = %d %s %d => Result = %d\n", afla[0], node->name, afla[1], result);
	/*	printf("%s: Exiting...\n", node->name);	*/
		if ( write(fd[1], &result, sizeof(result)) != sizeof(result) ){
			perror("write to pipe");
			exit(1);
		}
		exit(2);
	}
	else if ( strcmp(node->name , "*") == 0 ) {
		result = afla[0] * afla[1];
		printf("Result = %d %s %d => Result = %d\n", afla[0], node->name, afla[1], result);
	/*	printf("%s: Exiting...\n", node->name);*/
		if ( write(fd[1], &result, sizeof(result)) != sizeof(result) ){
			perror("write to pipe");
			exit(1);
		}
		exit(2);

	}

	/*printf("%s: Exiting...\n", node->name);*/
	exit(0);
}


int main(int argc, char *argv[])
{
	struct tree_node *root;
	int status;
	int fd[2];
	int temp;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
		exit(1);
	}

	if (pipe(fd) < 0)
	{
		perror("pipe");
		exit(1);
	}

	root = get_tree_from_file(argv[1]);
	print_tree(root);

	pid_t pid = fork();

	if ( pid < 0 ) {
		perror("fork");
		exit(1);
	}

	if ( pid == 0 ) {
		create_tree_pipes(root, fd);
		exit(1);
	}
	
	wait_for_ready_children(1);
	show_pstree(pid);
	kill(pid, SIGCONT);
	pid = wait(&status);
	explain_wait_status(pid, status);

	if ( read(fd[0], &temp, sizeof(int))!=sizeof(int) ){
		perror("read pipe"); 
	}
	printf("\nThe final result is: %d\n\n", temp);
	return 0;
}
