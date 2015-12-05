/* Author: Phillip Stewart
CPSC 351, Project 4, shell
*/


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LEN 120


// #### Function prototypes ###################################################
void cd(char* dir);
void jobs();
void fix_cwd(char* cwd);
void check_bg();
void run_command(char** args, int bg)


// #### Globals ###############################################################
char home[LEN];
int num_bg = 0;
int bg_pids[LEN];
int piping;
int pipes[2];
int input = 0;
int last;


// #### Internal commands #####################################################
void cd(char* dir) {
	if (dir[0] == 0) {
		chdir(home);
	} else {
		chdir(dir);
	}
}


void jobs() {
	check_bg();
	if (num_bg) {
		printf("Running processes:\n");
		int i;
		for (i = 0; i < num_bg; i++) {
			printf("  [%d] %d\n", i+1, bg_pids[i]);
		}
	} else {
		printf("No running jobs.\n");
	}
}


// #### Helper functions ######################################################
void fix_cwd(char* cwd) {
	char temp[LEN];
	getcwd(temp, LEN);
	if (strncmp(home, temp, strlen(home)) == 0) {
		cwd[0] = '~';
		cwd[1] = 0;
		++cwd;
		char* t = temp + strlen(home);
		strncpy(cwd, t, strlen(temp) - strlen(home));
	} else {
		strncpy(cwd, temp, strlen(temp));
		cwd[strlen(temp)] = 0;
	}
}


void check_bg() {
	if (num_bg) {
		int exit_status;
		int i;
		for (i = 0; i < num_bg; i++) {
			int pid = waitpid(bg_pids[i], &exit_status, WNOHANG);
			if (pid == bg_pids[i]) {
				--num_bg;
				int j = i;
				while (j < num_bg) bg_pids[j] = bg_pids[j+1];
				printf("[%d] %d Stopped\n", i+1, pid);
			}

		}
	}
}


// #### Run (fork, execvp) ####################################################
void run_command(char** args, int bg) {
	pid_t pid = fork();
	if (pid == 0) { // Child
		if (piping) {
			if (input != 0) {
				dup2(input, 0);
				close(input);
			}
			if (!last) {
				if (pipes[1] != 1) {
					dup2(pipes[1], 1);
					close(pipes[1]);
				}
			}
		}
		execvp(args[0], args);
		printf("Command not found\n");
		_exit(EXIT_FAILURE);
	} else if (pid > 0) { // Parent
		if (piping) {
			close(pipes[1]);
			close(input);
			input = pipes[0];
		}
		if (bg) {
			bg_pids[num_bg] = pid;
			++num_bg;
			printf("[%d] %d\t%s\n", num_bg, pid, args[0]);
		} else {
			int exit_status;
			waitpid(pid, &exit_status, 0);
			//printf("Exit status: %d\n", exit_status);
		}
	} else {
		perror("fork");
	}
	
}


// #### Main ##################################################################
int main(){
	// Program variables:
	char cwd[LEN];
	getcwd(home, LEN);
	fix_cwd(cwd);

	char command[LEN];
	char* command_tok;
	char* command_args[LEN];
	int args_index;

	int bg;

	int stdin_2 = dup(0);
	int stdout_2 = dup(1);

	// #### Shell loop ########################################################
	while (1) {
		input = 0;
		piping = 0;
		bg = 0;
		last = 0;

		check_bg();

		// #### Prompt ########################################################
		printf("CPSC351Shell:%s$ ", cwd);
		if (!fgets(command, LEN, stdin)) {
			// EOF (ctrl+D)
			printf("\n");
			exit(0);
		}

		// #### Clean command string ##########################################
		if (command[0] == '\n') {
			// If no command entered, re-propmt.
			continue;
		} else {
			// Clean the end, check for &
			command[LEN-1] = 0; //just in case...
			int end = strcspn(command, "\n");
			command[strcspn(command, "\n")] = 0;
			if (command[end-1] == '&') {
				bg = 1;
				end -= 2;
				while (end > 0 && command[end] == ' ') {
					--end;
				}
				command[end+1] = 0;
			}
		}

		// #### Internal commands #############################################
		if (strncmp(command, "cd", 2) == 0) {
			if (command[2] == 0) {
				cwd[0] = 0;
				cd(cwd);
				fix_cwd(cwd);
				continue;
			} else if (command[2] == ' '){
				char* c = command + 3;
				while (*c == ' ') ++c; //in case multiple spaces...
				strncpy(cwd, c, LEN-3);
				cwd[LEN-1] = 0; //just in case
				cd(cwd);
				fix_cwd(cwd);
				continue;
			}// else it wasn't really 'cd'
		} else if (strncmp(command, "exit", 4) == 0) {
			//printf("\n");
			exit(0);
		} else if (strncmp(command, "jobs", 4) == 0) {
			jobs();
			continue;
		}

		// #### Tokenize and run commands #####################################
		command_tok = strtok(command, " ");
		for (args_index = 0; args_index < LEN-1; args_index++) {
			command_args[args_index] = command_tok;
			command_tok = strtok(NULL, " ");
			if (command_tok == NULL) {// End of command string
				if (piping) {
					last = 1;
				}
				command_args[args_index + 1] = NULL;
				run_command(command_args, bg);
				break;
			} else if (command_tok[0] == '|') {// reached a pipe
				piping = 1;
				if (pipe(pipes) == -1) {
					perror("pipe");
					exit(1);
				}

				command_args[args_index + 1] = NULL;
				run_command(command_args, 0);

				//prep for next command
				command_tok = strtok(NULL, " ");
				args_index = -1;
				close(pipes[1]);
				input = pipes[0];
			}
		}
		
		if (piping) { //reset streams
			close(input);
			close(pipes[0]);
			close(pipes[1]);
			dup2(stdin_2, 0);
			dup2(stdout_2, 1);
		}
	}

	return 0;
}