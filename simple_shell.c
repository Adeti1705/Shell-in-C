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
        if (check_for_pipes(line)) {
            char **command_1 = break_pipes_1(line);
            char ***command_2 = break_pipes_2(command_1);
            executePipe(command_2);
        } else {
            char **command_1 = break_spaces(line);
            executeCommand(command_1);
        }
    }
    fclose(fobj);
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
    }
}