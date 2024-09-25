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

char history[100][100];
int pid_history[100];
long time_history[100][2];
int c_hist = 0;

// to add a command to history
void add_to_history(const char *command, int pid, long start_time_ms, long end_time_ms)
{
    if (c_hist < 100)
    {
        strcpy(history[c_hist], command);
        pid_history[c_hist] = pid;
        time_history[c_hist][0] = start_time_ms;
        time_history[c_hist][1] = end_time_ms;
        c_hist++;
    }
    else
    {
        printf("Can't add more than 100 commands to history.\n");
    }
}

// to print command history
void print_history()
{
    printf("\nCommand History:\n");
    for (int i = 0; i < c_hist; i++)
    {
        printf("Command %d: %s\n", i + 1, history[i]);
        printf("PID: %d\n", pid_history[i]);
        printf("Start Time: %ld\n", time_history[i][0]);
        printf("Total Duration in microseconds: %ld \n", time_history[i][1] - time_history[i][0]);
        printf("\n");
    }
}

// signal handler for ctrl+c
void my_handler(int signum)
{
    if (signum == SIGINT)
    {
        printf("\n---------------------------------\n");
        print_history();
        exit(0);
    }
}

// sets up signal handling for ctrl+c
void sig_handler()
{
    struct sigaction sig;
    sig.sa_handler = my_handler;
    if (sigaction(SIGINT, &sig, NULL) != 0)
    {
        printf("Signal handling failed.\n");
        exit(1);
    }
}

// returns current time in microseconds(us)
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

// parses command string into array of strings
char **break_delim(char *cmd_line, char *delim)
{
    char **word_array = (char **)malloc(100 * sizeof(char *));
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

// executes command
int launch(char **command_line, bool background)
{
    int pid = fork();
    if (pid < 0)
    {
        printf("Fork failed.\n");
        return -1;
    }
    else if (pid == 0)
    {
        if (strcmp(command_line[0], "history") == 0)
        {
            print_history();
            exit(0);
        }
        execvp(command_line[0], command_line);
        printf("Command not found: %s\n", command_line[0]);
        exit(1);
    }
    else
    {
        if (!background)
        {
            // parent waits for non-& commands
            int status;
            waitpid(pid, &status, 0);
        }
        else
        {
            printf("Started background process with PID: %d\n", pid);
        }
    }
    return pid;
}

// executes commands with pipes
int pipe_execute(char ***commands)
{
    int inputfd = STDIN_FILENO;
    int lastChildPID = -1;
    int i = 0;

    while (commands[i] != NULL)
    {
        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            perror("Pipe failed");
            exit(1);
        }

        int pid = fork();
        if (pid < 0)
        {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // child process
            if (inputfd != STDIN_FILENO)
            {
                dup2(inputfd, STDIN_FILENO);
                close(inputfd);
            }
            if (commands[i + 1] != NULL)
            {
                dup2(pipefd[1], STDOUT_FILENO);
            }
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(commands[i][0], commands[i]);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            close(pipefd[1]);
            if (inputfd != STDIN_FILENO)
            {
                close(inputfd);
            }
            inputfd = pipefd[0]; // uses pipe read end as input for next command
            lastChildPID = pid;
            i++;
        }
    }

    int status;
    while (wait(&status) > 0)
    {
    }
    return lastChildPID;
}

// to split commands into array for each pipe segment
char ***pipe_manager(char **cmds)
{
    char ***commands = (char ***)malloc(sizeof(char **) * 100);
    if (commands == NULL)
    {
        printf("Failed to allocate memory\n");
        exit(1);
    }

    int j = 0;
    for (int i = 0; cmds[i] != NULL; i++)
    {
        commands[j] = break_delim(cmds[i], " \n");
        j++;
    }
    commands[j] = NULL;
    return commands;
}

// checks if command has pipes
bool hasPipes(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (str[i] == '|')
        {
            return true;
        }
    }
    return false;
}

// main shell loop
int main()
{
    sig_handler(); // handles signal
    char *cmd;
    char current_dir[100];

    printf("\n Shell Starting...----------------------------------\n");
    while (1)
    {
        getcwd(current_dir, sizeof(current_dir)); // gets current directory
        printf(">%s>>> ", current_dir);
        cmd = (char *)malloc(100);
        fgets(cmd, 100, stdin);

        long start_time = current_time();
        int pid;
        bool background = false;

        // checks if command ends with '&'
        size_t len = strlen(cmd);
        if (len > 0 && cmd[len - 2] == '&')
        {
            background = true;
            cmd[len - 2] = '\0';
        }

        if (hasPipes(cmd))
        {
            char **command_1 = break_delim(cmd, "|");
            char ***command_2 = pipe_manager(command_1);
            pid = pipe_execute(command_2); // Pipe execution not yet supporting background
        }
        else
        {
            char **command = break_delim(cmd, " \n");
            pid = launch(command, background); // Pass the background flag
        }

        // Only add to history for foreground processes
        if (!background)
        {
            add_to_history(cmd, pid, start_time, current_time());
        }

        free(cmd);
        free(history);
    }
    return 0;
}