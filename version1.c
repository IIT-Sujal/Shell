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
//vector* allProcess;
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
        print_history_file_error();
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
    int l=strlen(cmd)-1<strlen(history)?strlen(cmd)-1:strlen(history);
    for(int i=0;i<l;i++)
    {
        if(cmd[i+1]!=history[i])
            return false;
    }
    return true;
}
int executeCommand(char* cmd,vector* historyOfCommands)
{
    char* originalCommand;
    originalCommand=(char*)malloc((strlen(cmd))*sizeof(char)+21);
    strcpy(originalCommand,cmd);
    char *argc=NULL;
    char **argv=(char**)malloc(50*sizeof(char*));
    char *token;
    int childPid,status=0;
    token = strtok(cmd, " ");
    int tokenCount=0;
    bool isBackgroundProcess = false;
    int ampCount=0;
    for(int i=0;i<(int)strlen(originalCommand);i++)
    {
        if(originalCommand[i]=='&')
        {
            ampCount++;
        }
    }
    if(ampCount==1)
    {
        isBackgroundProcess=true;
    }
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
    else if(first_char(argc)=='!')
    {
        int history_size=vector_size(historyOfCommands);
        for(int i=history_size-1;i>=0;i--)
        {
            if(prefix_match(argc,vector_get(historyOfCommands,i))==true)
            {
                print_command(vector_get(historyOfCommands,i));
                executeCommand(vector_get(historyOfCommands,i),historyOfCommands);
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
            int status=kill(pid,SIGTSTP);
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
            char* cmdLine=(char*)(vector_get(historyOfCommands,taskID));
            executeCommand(cmdLine,historyOfCommands);
        }
        else
        {
            print_invalid_index();
        }
        add_to_history(originalCommand,historyOfCommands);
    }
    else
    {
        add_to_history(originalCommand,historyOfCommands);
        childPid = fork();
        if(childPid == 0)
        {
            print_command_executed(getpid());   
            execvp(argc,argv);
            //printf("%s\n", (char*) strerror(errno));
            exit(0);
        }
        else
        {
            if(isBackgroundProcess==false)
            {
                wait(&status);
            }
        }

    }
    free(token);
    free(argc);
    for(int i=0;i<tokenCount+1;i++)
    free(argv[i]);
    free(argv);
    free(originalCommand);
    
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
  // for(int i=0;i<totalProcess;i++)
  // {
  //    if()
  // }
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
    vector_destroy(historyOfCommands);
    vector_destroy(listOfCommands);
    return;
}
int shell(int argc, char *argv[]) {

    signal(SIGINT, sigintHandler);
    signal (SIGCHLD, proc_exit);
    vector *historyOfCommands=vector_create(NULL,NULL,NULL);
    vector *listOfCommands=vector_create(NULL,NULL,NULL);
    char* historyFile=NULL;
    int c;
    while ((c = getopt (argc, argv, "h:f:")) != -1)
    {
        switch (c)
        {
            case 'h':
            historyFile=(char*)(malloc(sizeof(char)*(strlen(optarg)+1)));
            strcpy(historyFile,optarg);
            vector_destroy(historyOfCommands);
            historyOfCommands = read_file(optarg);
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
                add_to_history(cmdLine,historyOfCommands);
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
                sscanf(cmdLine, "%s && %s", cmd1,cmd2);

                int status=executeCommand(cmd1,historyOfCommands);
                if(status==0)
                {
                    executeCommand(cmd2,historyOfCommands);
                }
                free(cmd1);
                free(cmd2);
            }
            else if(x==2)
            {
                char cmd1[100],cmd2[100];
                sscanf(cmdLine, "%s || %s", cmd1,cmd2);
                int status=executeCommand(cmd1,historyOfCommands);
                if(status==0)
                {
                    executeCommand(cmd2,historyOfCommands);
                }
                free(cmd1);
                free(cmd2);
            }
            else if(x==3)
            {
                char *cmd1,*cmd2;
                cmd1 = strtok(cmdLine, ";");
                cmd2 = strtok(NULL,"\n");
                printf("%s\n%s\n",cmd1,cmd2 );
                int status=executeCommand(cmd1,historyOfCommands);
                if(status==0)
                {
                    executeCommand(cmd2,historyOfCommands);
                }
                free(cmd1);
                free(cmd2);
            }
        }
        else
        {
            executeCommand(cmdLine,historyOfCommands);   
        }
        free(cmdLine);
    }
    free_main_memory(historyOfCommands,listOfCommands,historyFile);
    return 0;
}
/*
pending task
1. check when an erronous filename is given for history/command memory leak is handled
2. reimplement ps
3. check for zombie process
*/