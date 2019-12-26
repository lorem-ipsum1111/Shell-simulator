#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#define ARG_QUANTA 100//taking at max 100 arguements from command line
#define LINE_SIZE 2000//max characters in a line
#define DELIMITERS " \t\n\a\r"
#define N_INBUILT 3//Number of inbuilt functions
#define HIST_SIZE 10//Number of commands to be stored as history


bool exit_call = false;
bool background_exec = false;
char* history[HIST_SIZE];

int position = -1;

void change_dir(char **args);
void help(char **args);
void exit_shell(char **args);
int increment(int increment_by, int arg );

int increment(int increment_by, int arg ){
	return (increment_by + arg)%HIST_SIZE;

}

char** process(char* line);

char *inbuilt_func[] = {"cd", "help", "exit" };

void (*inbuilts[]) (char **) = {
    &change_dir,
    &help,
    &exit_shell
};


/* Built-in function implementations */
void change_dir(char **args)
{
    if(args[1] == NULL){
        fprintf(stderr, "error: argument to \"cd\" is missing\n");
    }else{
        if(chdir(args[1]) != 0){
            fprintf(stderr, "PROBLEM IN CHANGING DIRECTORY\n");
           
        }
    }

    return;
}

void help(char **args)
{
    int i;
    printf("Type commands and press enter.\n");
    printf("Append \"&\" after the arguments for background execution.\n");
    printf("There are three built in functions:\n");

    for(i = 0; i < N_INBUILT; i++){
        printf(" %s\n", inbuilt_func[i]);
    }

    printf("Use man command for info about commands\n");
    return;
}

void exit_shell(char **args)
{
	exit_call = true;	
	return;
}

void launch_cmd(char** args){
	pid_t pid,wpid;
	int stat;
	pid = fork();
	if(pid == 0){//child process
		if(execvp(args[0],args) == -1){
			fprintf(stderr, "PROBLEM IN LAUNCHING COMMAND\n");
			//exit(EXIT_FAILURE);
			return;
		}
	}
	else if(pid > 0){	
		if(!background_exec){
			wait(NULL);
		}	
	}
	
	else{
		fprintf(stderr, "PROBLEM IN FORKING PROCESS\n");	
	}
	background_exec = false;
	return;
}

void history_op(char** args){
	int i;
	if(position == -1 || history[position] == NULL){
		fprintf(stderr, "History is empty\n");
		//exit(EXIT_FAILURE);
		return;
	}
	
	if((strcmp(args[0],"history"))==0){
		int count = 0, current_pos = position, end_position = 0;
		
		if( current_pos != HIST_SIZE && history[current_pos+1]!=NULL){
			end_position = current_pos+1;
		}
		
		count = increment(current_pos-end_position, HIST_SIZE) + 1;
		while(count > 0){
			char* cmd = history[current_pos];
			printf("%d %s\n",count,cmd);
			current_pos = current_pos - 1;
			current_pos = increment(current_pos , HIST_SIZE);
			//printf("\n%d",current_pos);
			count--;
		}		
	}	
	else
	{
		char** args2;
		char* cmd;		
		if(strcmp(args[0],"!!") == 0){
			
			cmd = (char*)malloc(sizeof(history[position]));
			strcat(cmd,history[position]);
			printf("%s\n",cmd);
			args2 = process(cmd);
			
			for(i = 0; i < N_INBUILT; i++ ){	
				if(strcmp(args2[0],inbuilt_func[i])==0){
				return (*inbuilts[i])(args2);
				}
			}	
			return launch_cmd(args2);			
		}
		else if(args[0][0] == '!'){
			
			if(args[0][1] == '\0'){
				fprintf(stderr, "NO ARGUEMENT AFTER  \"!\"\n");
               			//exit(EXIT_FAILURE);
               			return;
			}
			int index = args[0][1]-'0';
			if((increment(position,1)) != 0 && history[position+1] != NULL){
                		index = increment(position,index);
            		}            		
            		else
            		{
                		index--;
            		}
            		if(history[index] == NULL){
				fprintf(stderr, "COMMAND DOES NOT EXIST\n");
				//exit(EXIT_FAILURE);
				return;
			    }			    
			cmd = (char*)malloc(sizeof(history[index]));
			strcat(cmd,history[index]);
			args2 = process(cmd);
			
			for(i = 0; i < N_INBUILT; i++ ){	
				if(strcmp(args2[0],inbuilt_func[i])==0){
				return (*inbuilts[i])(args2);
				}
			}	
			return launch_cmd(args2);			
		}
		
		else{
			fprintf(stderr, "INVALID COMMAND\n");
		}		
	}
}

void execute(char** args){
	int i;
	if(args[0] == NULL){
		return ;
	}//checking empty arguements
	
	if((strcmp(args[0],"history")==0)||(strcmp(args[0],"!!")==0)||(args[0][0]=='!')){		
		return history_op(args);	
	}//managing history requests
	
	position = increment(position,1);
	history[position] = (char*)malloc(LINE_SIZE*sizeof(char));
	char** args2 = args;
	while(*args2!=NULL){
		strcat(history[position],*args2);
		strcat(history[position]," " );
		args2++;
	}//storing history
	
	for(i = 0; i < N_INBUILT; i++ ){
		if(strcmp(args[0],inbuilt_func[i])==0){
			return (*inbuilts[i])(args);
		}
	}
	
	return launch_cmd(args);
}



char** process(char* line){
	int n_args = ARG_QUANTA;
	int count_tokens = 0;
	char* token;
	char** args = (char**)malloc(n_args*sizeof(char*));
	if(!args){
		fprintf(stderr, "PROBLEM IN MEMORY ALLOCATION\n");
		//exit(EXIT_FAILURE);
		return NULL;
	}
	token = strtok(line,DELIMITERS);
	while(token!=NULL){
		args[count_tokens] = token;
		count_tokens++;
		
		if(count_tokens >= n_args){
			n_args += ARG_QUANTA;
			args = realloc(args,n_args);
			if(!args){
				fprintf(stderr, "PROBLEM IN MEMORY ALLOCATION\n");
				//exit(EXIT_FAILURE);
				return NULL;
			}				
		}		
		token = strtok(NULL,DELIMITERS);		
	}
	if(count_tokens > 0 && strcmp(args[count_tokens-1],"&")==0){
		background_exec = true;
		args[count_tokens-1] == NULL;
	}
	args[count_tokens] = NULL;
	return args;	
}



char* read_line(){
	int line_buffer_size = LINE_SIZE;
	char c;
	int pointer = 0;
	char* line_buffer = (char*)malloc(line_buffer_size*sizeof(char));
	if(!line_buffer){
		fprintf(stderr, "PROBLEM IN MEMORY ALLOCATION\n");
		//exit(EXIT_FAILURE);
		return NULL;
	}
	
	while(1){
		c = getchar();
		if (c==EOF||c=='\n'){
			line_buffer[pointer] = '\0';
			break;
		}
		line_buffer[pointer] = c;
		pointer++;
		if(pointer >= line_buffer_size){
			line_buffer_size += LINE_SIZE;
			line_buffer = realloc(line_buffer,line_buffer_size);
			if(!line_buffer){
				fprintf(stderr, "PROBLEM IN MEMORY ALLOCATION\n");
				//exit(EXIT_FAILURE);
				return NULL;
			}	
				
		}
		
	}
	//printf("%s",line_buffer);
	return line_buffer;
	
}



void Start_loop(){
	char* line;
	char** tokens;	
	while(!exit_call){
	
		printf("$");
		line = read_line();
		tokens = process(line);
		execute(tokens);
		free(line);
		free(tokens);	
	}
	
}

int main(){	 
	Start_loop();	
	return 0;	
}



