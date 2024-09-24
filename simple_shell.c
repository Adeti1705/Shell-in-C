#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

void my_handler(int signum) { // check
    if (signum == SIGINT) {
        printf("\n-------------------------------\n");
        display_history();        
        exit(0);
    }
}

void my_handler(int signum) { // check
    if (signum == SIGINT) {
        printf("\n-------------------------------\n");
        display_history();        
        exit(0);
    }
}

void sig_handler()
{
    struct sigaction sig;
    sig.sa_handler = my_handler;
    if (sigaction(SIGINT, &sig, NULL) != 0)
    {
        printf("Signal handling failed.\n");
        exit(1);
    }
    sigaction(SIGINT, &sig, NULL);
}

bool inputFlag = true;
// takes user input and returns string of the same
char *User_Input()
{
    char *inp = (char *)malloc(100);
    if (inp == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    fgets(inp, 100, stdin);
    inputFlag = false;

    if (inp[0] == ' ' || inp[0] == '\n' || strlen(inp) <= 0)
    {
        inputFlag = true;
    }
    return inp;
}

long current_time()
{
    struct timeval t;
    if (gettimeofday(&t, NULL) != 0)
    {
        printf("Error in getting the time.\n");
        exit(1);
    }
    long epoch_microsec = t.tv_sec * 1000000;
    return epoch_microsec + t.tv_usec;
}

bool hasPipes(char *str)
{
    char *eof = '\0';
    char *pipe = '|';
    bool hasPipes = false;
    for (int i = 0; str[i] != eof; i++)
    {
        if (str[i] == pipe)
        {
            hasPipes = true;
            break;
        }
    }
    return hasPipes;
}
void launch(char** argv) {  // check
    int status = fork();
  
    if (status < 0) {
        printf("Forking child failed.\n");
        exit(1);
    }
    else if (status == 0) {
        execvp(argv[0], argv); 
        printf("Command failed.\n");
        exit(1);
    }
    else { 
        int ret;
        int pid = wait(&ret);
        if (WIFEXITED(ret)) {
            if (WEXITSTATUS(ret) == -1)
            {
                printf("Exit = -1\n");
            }
        } 
        else {
            printf("\nAbnormal termination of :%d\n" , pid);
        }
    }
}
char ** break_delim(char *cmd_line, char* delim){
    char **word_array = (char **)malloc(100*sizeof(char *));
    if (word_array == NULL)
    {
        printf("Error in allocating memory for command.\n");
        exit(1);
    }
    char *word = strtok(cmd_line, delim);
    int i = 0;
    while (word != NULL)
    {
        word_array[i] = word;
        i++;
        word = strtok(NULL, delim);
    }
    word_array[i] = NULL;
    return word_array;
}

void pipe_execute(char ***commands) {  
    int i = 0, pid;
    int inputfd = STDIN_FILENO;  

    while (commands[i] != NULL) {
        int fd[2];
        pipe(fd);
        pid = fork();

        if (pid < 0) {
            printf("Forking child failed.\n");
            exit(1);

        } else if (pid == 0) {

            close(fd[0]); // closing read end
            if (inputfd != STDIN_FILENO) {
                dup2(inputfd, STDIN_FILENO);
                close(inputfd);
            }
            if (commands[i + 1] != NULL) {
                dup2(fd[1], STDOUT_FILENO);
            }
            execvp(commands[i][0], commands[i]);
            exit(1);
        } else {
            close(fd[1]); // closing write end
            if (inputfd != STDIN_FILENO) {
                close(inputfd);
            }
            inputfd = fd[0];
            i++;
        }
    }
    int wait_process;
    
    do
    {
        wait_process = wait(NULL);
    } while (wait_process > 0);
    
}

char*** pipe_manager(char** cmds){
    char*** commands=(char***)malloc(sizeof(char**)*100);
    if(commands==NULL){
        printf("Failed to allocate memory");
        exit(1);
    }
    int len=0;
    for(int i=0; cmds[i]!=NULL; i++){
        len=i;
    }
    int j=0;
    for(j=0; j<len; j++){
        commands[j]=break_delim(cmds[j],' \n');
    }
    commands[j]=NULL;
    return commands;
}

void execute(char *name){
    FILE*fobj=fopen(name,'r');
    if(fobj==NULL){
        printf("Error in opening the file\n");
        return;
    }
    char line[100];
    while (fgets(line,100,fobj)!=NULL){
        int len = strlen(line);
        if (len>0 &&(line[len-1]=='\n' || line[len-1]=='\r')){
            line[len-1]='\0';
        }
        if (len==0){
            continue;
        }
        //need to edit
        if (hasPipes(line)) {
            char **command_1 = break_delim(line,'|');
            char ***command_2 = pipe_manager(command_1);
            pipe_execute(command_2);
        } else {
            char **command = break_delim(line, ' \n');
            launch(command);
        }
    }
    fclose(fobj);
}

char history[100][100];
int pid_history[100], child_pid;
long time_history[100][2], start_time;
int c_hist = 0;

int add_to_history(const char *command, int pid, long start_time_ms, long end_time_ms) {
    if (c_hist < 100) { 
        strcpy(history[c_hist], command);   
        pid_history[c_hist] = pid;         
        time_history[c_hist][0] = start_time_ms;  
        time_history[c_hist][1] = end_time_ms;    
        c_hist++;  
    } else {
        printf("History is full. Cannot add more entries.\n");
    }
}

void print_history() {
    printf("\n Command History:\n");
    for (int i = 0; i < c_hist; i++) {
        printf("Command: %s\n", history[i]);
        printf("PID: %d\n", pid_history[i]);
        printf("Start Time: %ld\n", time_history[i][0]);
        printf("Total duration: %ld\n", time_history[i][1]-time_history[i][0]);
    }
}

int main(int argc, char const *argv[]){
    sig_handler();
    char *history = (char *)malloc(100);
    char *cmd;
    if (history == NULL)
    {
        printf("Error in allocating histroy memory");
        exit(1);
    }
    char current_dir[100];
    printf("\n Shell Starting...--------------------------------\n");
    while (1)
    {
        getcwd(current_dir, sizeof(current_dir));
        printf(">%s>>> ", current_dir);
        cmd = User_Input();
        if (inputFlag == true)
        {
            strcpy(history, cmd);
            // check for &
            long start_time = current_time();
            if (cmd[0] == '@')
            {   
                cmd[strlen(cmd) - 1] = '\0';     
                execute_cmd(++cmd);
            }





            c_hist=add_to_history(history,pid_history,start_time,current_time());
        }

        
    }
}