/**
* Shell Lab
* CS 241 - Fall 2018
*/

#include "format.h"
#include "shell.h"
#include "vector.h"
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>


typedef struct process {
	char *command;
	char *status;
	pid_t pid;
} process;

static vector* allProcess;
static vector* listOfCommands;
static char* historyFile;
process* blank_process()
{
	process* x = (process*)(malloc(sizeof(process)));
	x->status=(char*)malloc(sizeof(char)*8);
	x->pid = -1;
	x->command=NULL;
	return x;
}
void free_main_memory(vector* historyOfCommands, vector* listOfCommands,char* historyFile)
{
	if(historyFile!=NULL)
	{
		FILE *file = fopen(historyFile, "w");

		for(int i=0;i<(int)vector_size(historyOfCommands);i++)
		{
			fputs(vector_get(historyOfCommands,i),file);
			fputs("\n",file);
		}
		fclose(file);		
	}
	free(historyFile);
	for(int i=0;i<(int)vector_size(historyOfCommands);i++)
	{
		//printf("%s\n\n\n",vector_get(historyOfCommands,i) );
		free(vector_get(historyOfCommands,i));
	}
	for(int i=0;i<(int)vector_size(listOfCommands);i++)
	{
		//printf("%s\n\n\n",vector_get(historyOfCommands,i) );
		free(vector_get(listOfCommands,i));
	}
	for(int i=0;i<(int)vector_size(allProcess);i++)
	{
		//printf("%s\n\n\n",vector_get(historyOfCommands,i) );
		process* x=vector_get(allProcess,i);
		free(x->command);
		free(x->status);
		free(vector_get(allProcess,i));

	}
	vector_destroy(allProcess);
	vector_destroy(historyOfCommands);
	vector_destroy(listOfCommands);
	return;
}
vector* read_file(char* fileName)
{
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	vector* fileLines = vector_create(NULL,NULL,NULL);
	fp = fopen(fileName, "r");

	if (fp == NULL)
	{
		print_script_file_error();
		exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		line = strtok(line, "\n");
		char* x =(char*) malloc((strlen(line)+1)*(sizeof(char)));
		strcpy(x,line);
		vector_push_back(fileLines,x);
	}

	fclose(fp);
	if (line)
		free(line);
	return fileLines;
}

void call_prompt()
{
	char *directory=get_current_dir_name();
	pid_t pid=getpid();
	print_prompt(directory, pid);
	free(directory); 
}

void print_history(vector* historyOfCommands)
{
	size_t sizeOfHistory=vector_size(historyOfCommands);
	for(size_t i=0;i<sizeOfHistory;i++)
	{
		print_history_line(i, (char*)(vector_get(historyOfCommands,i)));
	}
}

char* read_input(void)
{
	int buffer_size = 2048;
	char *input = (char*)malloc(buffer_size * sizeof(char));
	int i = 0;
	char c;
	while ((c = getchar()) != '\n') {
        /* did user enter ctrl+d ?*/
		if (c == EOF) {
			free(input);
			return NULL;
		}
		input[i++] = c;
	}
	input[i] = '\0';
	input=(char*)realloc(input,(strlen(input)+1)*sizeof(char));
	return input;
}
void add_to_history(char *cmd,vector* historyOfCommands)
{
	char * s = malloc((strlen(cmd)+1)*sizeof(char));
	strcpy(s,cmd);
	vector_push_back(historyOfCommands,s);
	return;
}
char first_char(char *c)
{
	int l=strlen(c);
	for(int i=0;i<l;i++)
	{
		if(c[i]==' ')
		{
			continue;
		}
		else
		{
			return c[i];
		}
	}
	return ' ';
}
bool prefix_match(char* cmd,char* history)
{
	int l=strlen(cmd)-1;
	if(l>(int)strlen(history))
	{
		return false;
	}
	for(int i=0;i<l;i++)
	{
		if(cmd[i+1]!=history[i])
			return false;
	}
	return true;
}
void print_process()
{
		for(int i=0;i<(int)vector_size(allProcess);i++)
		{
			process* x = vector_get(allProcess,i);
/*			fflush(stdout);
			int value=(int)x->pid;
			//printf(" %d \n",value);
			if(value>0)
			{
				printf("ok\nji");
			}
			fflush(stdout);
			printf("%s\n",x->command);
			printf("%s\n",x->status);*/
			print_process_info(x->status, x->pid,x->command);
		}
}
int executeCommand(char* cmd,vector* historyOfCommands)
{
	char* originalCommand;
	originalCommand=(char*)malloc((strlen(cmd))*sizeof(char)+21);
	strcpy(originalCommand,cmd);
	bool isBackgroundProcess = false;
	int ampCount=0;
	for(int i=0;i<(int)strlen(originalCommand);i++)
	{
		if(originalCommand[i]=='&')
		{
			cmd[i]=' ';
			ampCount++;
		}
	}
	if(ampCount==1)
	{
		isBackgroundProcess=true;
	}
	char *argc=NULL;
	char **argv=(char**)malloc(50*sizeof(char*));
	char *token;
	int childPid,status=0;
	token = strtok(cmd, " ");
	int tokenCount=0;
	
	while(token!=NULL)
	{
		if(argc==NULL)
		{
			argc=(char*)malloc(strlen(token)*sizeof(char)+1);
			strcpy(argc,token);
		}
		argv[tokenCount]=(char*)malloc((strlen(token)+1)*sizeof(char));
		strcpy(argv[tokenCount],token);
		token = strtok(NULL, " ");
		tokenCount++;
	}
	argv=(char**)realloc(argv,(tokenCount+1)*sizeof(char*));
	argv[tokenCount]=NULL;
	if(strcmp(argc,"")==0)
	{		
		free(token);
		free(argc);
		free(argv);
		free(originalCommand);
		return 0;
	}
	if(strcmp(argc,"history")==0 || strcmp(argc,"!history")==0)
	{
		print_history(historyOfCommands);
	}
	else if(strcmp(argc,"cd")==0)
	{
		status=chdir(argv[1]);
		if(status==-1)
		{
			print_no_directory(argv[1]);
		}
		add_to_history(originalCommand,historyOfCommands);
	}
	else if(strcmp(argc,"ps")==0)
	{
		print_process();
		add_to_history(originalCommand,historyOfCommands);	
	}
	else if(first_char(argc)=='!')
	{
		int history_size=vector_size(historyOfCommands);
		for(int i=history_size-1;i>=0;i--)
		{
			if(prefix_match(originalCommand,vector_get(historyOfCommands,i)))
			{
				print_command(vector_get(historyOfCommands,i));
				char *x=malloc(strlen(vector_get(historyOfCommands,i))+1);
				strcpy(x,vector_get(historyOfCommands,i));
				executeCommand(x,historyOfCommands);
				break;
			}
			else if(i==0)
			{
				print_no_history_match();
			}
		}
	}
	else if(strcmp(argc,"kill")==0)
	{
		int pid=0;
		int n=0;

		if(tokenCount>1)
			n = sscanf(argv[1], "%d", &pid);
		if(n==1)
		{   
			int status=kill(pid,SIGTERM);
			if(status==-1)
			{
				print_no_process_found(pid);                
			}
		}
		else
		{
			print_invalid_command(originalCommand);
		}
		add_to_history(originalCommand,historyOfCommands);
	}
	else if(strcmp(argc,"stop")==0)
	{
		int pid=0;
		int n=0;

		if(tokenCount>1)
			n = sscanf(argv[1], "%d", &pid);
		if(n==1)
		{
			printf("to stop %d",pid);
			int status=kill(pid,SIGSTOP);
			//for(int)
		
			if(status==-1)
			{
				print_no_process_found(pid);                
			}
			else
			{

				for(int i=0;i<(int)vector_size(allProcess);i++)
				{
				
					printf("size is %d\n",(int)vector_size(allProcess));
					printf("hi sujal kk you reach here\n\n" );
					process* x = vector_get(allProcess,i);
					printf("hi sujal k0 you reach here\n\n" );
					printf("%s\n",x->command );
					printf("hi sujal k1 you reach here\n\n" );
				
/*					if(x->pid == pid)
					{
						strcpy(x->status,STATUS_STOPPED);
					}*/

					printf("hi sujal atleast you reach here\n\n" );

				}
			}
		}
		else
		{
			print_invalid_command(originalCommand);
		}
					
		add_to_history(originalCommand,historyOfCommands);
	}
	else if(strcmp(argc,"cont")==0)
	{
		int pid=0;
		int n=0;

		if(tokenCount>1)
			n = sscanf(argv[1], "%d", &pid);
		if(n==1)
		{   
			int status=kill(pid,SIGCONT);
			if(status==-1)
			{
				print_no_process_found(pid);                
			}
			else
			{
				for(int i=0;i<(int)vector_size(allProcess);i++)
				{
					process *x=vector_get(allProcess,i);
					if(x->pid==pid)
					{
						strcpy(x->status,STATUS_RUNNING);
					}
				}
			}
		}
		else
		{
			print_invalid_command(originalCommand);
		}
		add_to_history(originalCommand,historyOfCommands);
	}
	else if(argc[0]=='#')
	{
		int taskID;
		int n = sscanf(argc, "#%d", &taskID);
		if(n==1 && (int)vector_size(historyOfCommands)>taskID)
		{
			char *x=malloc(strlen(vector_get(historyOfCommands,taskID))+1);
			strcpy(x,vector_get(historyOfCommands,taskID));
			executeCommand(x,historyOfCommands);
		}
		else
		{
			print_invalid_index();
		}
	}
	else
	{
		//printf("oc-->%s\n\n",originalCommand);
		add_to_history(originalCommand,historyOfCommands);
		childPid = fork();
		if(childPid == 0)
		{
			print_command_executed(getpid());
			execvp(argc,argv);
			print_invalid_command(originalCommand) ;
			free_main_memory(historyOfCommands,listOfCommands,historyFile);
			//printf("%s\n", (char*) strerror(errno));
			exit(-1);
		}
		else
		{
			if(!isBackgroundProcess)
			{
				wait(&status);
			}
			else
			{
/*				if (childPid > 0) 
				{
					if (setpgid(childPid, childPid) == -1) 
					{
						print_setpgid_failed();
						exit(1);
					}
				}			*/
				process* new_process=blank_process();
				printf("copying this%s\n",originalCommand);
				new_process->command=(char*)malloc(sizeof(originalCommand)+1);
				new_process->pid=childPid;
				strcpy(new_process->command,originalCommand);
				strcpy(new_process->status,STATUS_RUNNING);
				vector_push_back(allProcess,new_process);	
				//print_process();
			}
		}

	}
	free(token);
	free(argc);
	for(int i=0;i<tokenCount+1;i++)
	free(argv[i]);
	free(argv);
	free(originalCommand);
	//print_process();
	return status;
}
// returns 3 for ';', returns 2 for '||', returns 1 if && is found and returns 0 is no logial operator found
int hasLogicalOperator(char* cmd) 
{
	int l=strlen(cmd);
	for(int i=0;i<l-1;i++)
	{
		if(cmd[i]=='&' && cmd[i+1]=='&')
		{
			return 1;
		}
		if(cmd[i]=='|' && cmd[i+1]=='|')
		{
			return 2;
		}
		if(cmd[i]==';')
		{
			return 3;
		}
	}
	return 0;
}
void sigintHandler() 
{ 
    signal(SIGINT, sigintHandler); 
    fflush(stdout); 
} 
void proc_exit()
{
	pid_t pid;
	pid = wait(NULL);
	for(int i=0;i<(int)vector_size(allProcess);i++)
	{
		process*x = vector_get(allProcess,i);
		if(x->pid==pid)
		{
			free(x->command);
			free(x->status);
			free(vector_get(allProcess,i));
			vector_erase(allProcess, i);
			return;
		}
	}

}
int shell(int argc, char *argv[]) {

	//signal(SIGINT, sigintHandler);
	signal (SIGCHLD, proc_exit);
	vector *historyOfCommands=vector_create(NULL,NULL,NULL);
	listOfCommands=vector_create(NULL,NULL,NULL);
	allProcess = vector_create(NULL,NULL,NULL);
	process* main_shell=blank_process();
	main_shell->pid=getpid();
	main_shell->command=(char*)malloc(sizeof(char)*6);
	strcpy(main_shell->status,STATUS_RUNNING);
	strcpy(main_shell->command,"shell");
	vector_push_back(allProcess,main_shell);
	historyFile=NULL;
	int c;
	while ((c = getopt (argc, argv, "h:f:")) != -1)
	{
		switch (c)
		{
			case 'h':
			historyFile=(char*)(malloc(sizeof(char)*(strlen(optarg)+1)));
			strcpy(historyFile,optarg);
			FILE * fp;
			fp = fopen(optarg, "r");
			if (fp == NULL)
			{
				print_history_file_error();
			}
			else
			{
				vector_destroy(historyOfCommands);
				historyOfCommands = read_file(optarg);			
			}
	
			break;
			case 'f':
			vector_destroy(listOfCommands);
			listOfCommands = read_file(optarg);
			break;
			default:
			print_usage();
			break;
		}
	}

	if(vector_size(listOfCommands)>0)
	{
		size_t sizeOfCommand=vector_size(listOfCommands);
		for(size_t i=0;i<sizeOfCommand;i++)
		{
			call_prompt();
			char* cmdLine=(char*)(vector_get(listOfCommands,i));
			print_command(cmdLine);
			executeCommand(cmdLine,historyOfCommands);
		}
		free_main_memory(historyOfCommands,listOfCommands,historyFile);
		return 0;
	}

	while (1)
	{
		printf("1\n");
		print_process();
		char * cmdLine;
		call_prompt();
		cmdLine = read_input(); 
		if(cmdLine[0]=='\0')
		{
			free(cmdLine);
			continue;
		}
		int x=hasLogicalOperator(cmdLine);
		if(cmdLine==NULL||strcmp(cmdLine,"exit")==0)
		{
			if(strcmp(cmdLine,"exit")==0)
			{
				free(cmdLine);
			}
			free_main_memory(historyOfCommands,listOfCommands,historyFile);
			return 0;
		}
		else if(x>0)
		{
			if(x==1)
			{
				char cmd1[100],cmd2[100];
				sscanf(cmdLine, "%[^&] && %[^\n]", cmd1,cmd2);
				int status=executeCommand(cmd1,historyOfCommands);
				if(status==0)
				{
					executeCommand(cmd2,historyOfCommands);
				}
			}
			else if(x==2)
			{
				char cmd1[100],cmd2[100];
				sscanf(cmdLine, "%[^|] || %[^\n]", cmd1,cmd2);
				int status=executeCommand(cmd1,historyOfCommands);
				if(status!=0)
				{
					executeCommand(cmd2,historyOfCommands);
				}
			}
			else if(x==3)
			{
				char cmd1[100],cmd2[100];
				sscanf(cmdLine, "%[^;]; %[^\n]", cmd1,cmd2);
				int status=executeCommand(cmd1,historyOfCommands);
				if(status==0)
				{
					executeCommand(cmd2,historyOfCommands);
				}
			}
		}
		else
		{
			executeCommand(cmdLine,historyOfCommands);   
		}
		free(cmdLine);
	}
}
