//Rahul Patel - rpate3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>


//initialize functions and variables
int myshell_run(int i); 
void command_run(char **args);
int command_exec(char **args); 
extern char ** get_args();
int command_inredirect(char **args, int i);
int command_outredirect(char **args, int i);
void command_default(char **args);
int command_pipe(char **args, int i); 
int print_prompt();
char **command_args();

//main function to run shell program
int main(int argc, char **argv){
	printf("Welcome to my  shell \n");

        myshell_run(1);

}

//loop that shell runs, prints out $: to prompt user
int myshell_run(int i){
	int stat = i;
        char *cline;
        char **args;
        int check;
        do {
		print_prompt();
		args = command_args();
		command_run(args);
	}while(stat);

	return (0);
}

//print a prompt
int print_prompt(){
	printf("$:  ");
	return(0); 
}

//split command line into arguments to process
char **command_args(){
	char **args;
	args = get_args();
	return args;
}

//default command execution without any special characters 
int command_exec(char **args){
        int pid;
        int stat;


	
        pid = fork(); //http://man7.org/linux/man-pages/man2/fork.2.html

	if ((pid < 0)){
		printf("Error forking child process");
		exit(1);
	}else if(pid ==0){ 
        	if(execvp(args[0], args) < 0){ //http://linux.die.net/man/3/execvp
			perror("error in executing \n");
			exit(1);
		}
	}else{ 
		pid= wait(&stat);  //http://linux.die.net/man/2/wait
        }
	return(0); 	
}

//handles ">>" special case
int command_out2redirect(char **args, int i){
	int stat;
	int pid; 
	int newfd;

	pid = fork(); 

	if ( pid < 0) {
		perror("forking error");
		exit(1); 
	}else if( pid == 0){
	newfd = open(args[i+1], O_APPEND|O_WRONLY); //http://linux.die.net/man/3/open
	if(dup2(newfd, 1) < 0){ //http://man7.org/linux/man-pages/man2/dup.2.html
		perror("dup2 error");
	}
	close(newfd); 
	args[i] = NULL;
	execvp(args[0], args);
	}else{
		pid = wait(&stat);
	}
	return myshell_run(1);
}


//handles ">" special case
int command_outredirect(char **args, int i){
	int stat;
	int pid; 
	int newfd;
	pid = fork(); 

	if ( pid < 0){
		perror("forking error");
		exit(1);
	}
	else if(pid == 0){
	newfd = open(args[i+1], O_CREAT|O_EXCL|O_RDWR);
	if(dup2(newfd, 1)<0){
		perror("dup2 error");
		exit(1); 
	}
	close(newfd);
	args[i] = NULL;
	execvp(args[0], args);
	}else{
		pid = wait(&stat);
	}

	return myshell_run(1); 
	}	

//handles "<" special case
int command_inredirect(char **args, int i){
	int pid; 
	int newfile;
	pid = fork(); 
	int stat; 
	if (pid < 0){
		perror("error forking"); 
		exit(1);
	}
	else if(pid == 0){
	newfile = open(args[i+1], O_RDONLY);
	if(dup2(newfile, 0)< 0){
		perror("dup2 error");
		exit(1);
	}
	close(newfile);
	args[i] = NULL;

		execvp(args[0], args); 
	}else{
		pid = wait(&stat);
	}
	return myshell_run(1);
 
}

//handles the special case for cd 
int command_cd(char **args){
	int pid;
	int stat;
	int dir;
	char *path;
	char *buffer;
	long size; 
	args[1] = NULL;

	if ( pid < 0){
		perror("error forking");
		exit(1); 
	}else if (pid == 0){
	
	size = pathconf(".", _PC_PATH_MAX);
	
	if((buffer = (char *)malloc((size_t)size)) != NULL)
		path = getcwd(buffer, (size_t)size); //http://man7.org/linux/man-pages/man2/getcwd.2.html
	
	if((dir = chdir(path))== 0){ //http://man7.org/linux/man-pages/man2/chdir.2.html
		return(dir); 
	}else{
		perror("cd failed");
	}

	}else {
		pid = wait(&stat);	
	}
	
	return myshell_run(1);
}


//handles the "|" special case
int command_pipe(char **args, int i){
	int pipefd[2];
	int pid;
	int pid2;
	int stat;
	char  **args1 = args; 
 	char  **args2 = args;	
	
	args[i] = NULL;

	//creates new arrays to store sections of the command line input
	for(int j = 0; args[j] != NULL; j++) {
		args1[j] = args[j];
	} 

	for(int z = i+1; args[z] != NULL; z++){
		args2[z] = args[z]; 
	} 

	pipe(pipefd); //http://linux.die.net/man/2/pipe 
	pid = fork();

	if(pid < 0){
		perror("fork error");
		exit(1); 	
	}else if(pid == 0){
		dup2(pipefd[0], 0); 
		close(pipefd[1]);
		close(pipefd[0]);  
		execvp(args[i+1], args2);
		return myshell_run(1); 
	}else{
		pid2 = fork(); 
		if (pid2 == 0){
		dup2(pipefd[1], 1);
		close(pipefd[0]);
		close(pipefd[1]);
		execvp(args[0], args1);
		return myshell_run(1);
		} 
	}	

	return myshell_run(1); 
	
}

//handles the ";" special case
int command_semi(char **args, int i){
	int count = 0;
	for(int i =0; args[i] != NULL; i++){
		if(strcmp(args[i], ";") == 0){
			count++;
		}
	}

	char **semiargs = args;
	char **semiargs1 = args;


	args[i] = NULL;

	for(int j = 0; args[j] != NULL; j++){
		semiargs[j] = args[j]; 	
	}


	for(int z = i+1; args[z] !=NULL; z++){
		semiargs1[z] = args[z]; 		
	}

	int pid_child; 
	int status_child; 
	int pid_par;
	pid_child = fork(); 

	while ( count > 0){

	if(pid_child ==0){
		if(execvp(semiargs[0], semiargs) < 0) {
		perror("Error in executing command");
		}
		if(execvp(semiargs1[0], semiargs1) < 0){
		perror("error in executing command"); 
		} 
	}else{
		do{
			pid_par = wait(&status_child);
		}while(pid_par != pid_child);

	return status_child;
	}
	count --;
	}		

	return myshell_run(1);
}

//checks the command line for special characters & flags and executes them appropriately 
void command_run(char **args){
	int ret;

	if(args[0] == NULL){
		//return 1;
	}else if (strcmp(args[0], "exit") == 0){
		exit(0);
	}else if(strcmp(args[0], "cd") == 0){
		command_cd(args);
		}else{
		for(int i = 1; args[i] != NULL; i++){
			if((strcmp(args[i], "|") == 0)){
			command_pipe(args, i);
                        }else if(strcmp(args[i], "<") == 0){
			command_inredirect(args, i);
			}else if(strcmp(args[i], ">") == 0){
			command_outredirect(args, i);
			}else if(strcmp(args[i], ">>") == 0){
                       	command_out2redirect(args, i);
			}else if(strcmp(args[i], ";") == 0){
			command_semi(args, i); 
			printf("this is semi colon");
			}
		}
	}

	command_exec(args);

}	
