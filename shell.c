#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>

int how_many_words(char *command);
void push_to_arg(char **arg, char *command, int num_of_words);
void execute(char **arg);
void pipe_push(char *command);
void pipe_execute(char **left, char **right, int redirection);
int is_pipe(char *command);

int is_redirect(char *command);
void redirect(char **arg, int type);
void redirect_1(char **com, char *f_name); //  "<"
void redirect_2(char **com, char *f_name); //  ">"
void redirect_3(char **com, char *f_name); //  ">>"
void redirect_4(char **com, char *f_name); //  "2>"

int main(int argc, const char *argv[])
{
    struct passwd *user;
    char *name;
    char cwd[1000];
    char command[510];
    char **arg;
    FILE *file;

    int command_counter = 1;
    int total_length = 4; //  "done" is length of 4
    int pipe_command = 0;
    int redirection_command = 0;
    double average_length = 0;

    file = fopen(argv[1], "w");
    if (file == NULL) // if fopen faild
    {
        perror("fopen() error, insert a log file.\n");
        exit(1);
    }

    if ((user = getpwuid(geteuid())) == NULL) // if getpwuid faild
    {
        perror("getpwuid() error\n");
        exit(1);
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL) // if getcwd faild
    {
        perror("getcwd() error\n");
        exit(1);
    }

    name = user->pw_name;
    printf("%s@%s>", name, cwd);
    fgets(command, sizeof(command), stdin);

    while (strcmp(command, "done\n") != 0)
    {
        fprintf(file, "%s", command); // print the command to the log file

        if (strcmp(command, "\n") == 0) //  if empty command
        {
            printf("%s@%s>", name, cwd);
            fgets(command, sizeof(command), stdin);
            continue;
        }

        command_counter++;
        total_length = total_length + strlen(command) - 1;

        command[strlen(command) - 1] = '\0'; // get rid of the "\n"
        char *command_copy = (char *)malloc(sizeof(char) * (strlen(command) + 1));
        char *command_copy2 = (char *)malloc(sizeof(char) * (strlen(command) + 1));
        strcpy(command_copy, command);
        strcpy(command_copy2, command);

        int num_of_words = how_many_words(command);
        arg = (char **)malloc(sizeof(char *) * (num_of_words + 1)); //  initiate the command array. "num of words + 1" to leave space for NULL
        assert(arg != NULL);                                        // if malloc faild

        push_to_arg(arg, command, num_of_words);

        //  if the command is "cd"
        if (strcmp(arg[0], "cd") == 0)
        {
            printf("Command not supported (Yet)\n");

            free(arg);
            free(command_copy);
            free(command_copy2);
            printf("%s@%s>", name, cwd);
            fgets(command, sizeof(command), stdin);
            continue;
        }

        // if the command is pipe
        if (is_pipe(command_copy) == 1)
        {
            pipe_command++;
            pipe_push(command_copy);

            free(command_copy);
            free(command_copy2);
            free(arg);
            printf("%s@%s>", name, cwd);
            fgets(command, sizeof(command), stdin);
            continue;
        }

        // if the command is redirection
        int r = is_redirect(command_copy2);
        if (r != 0)
        {
            redirection_command++;
            redirect(arg, r);
            free(command_copy);
            free(command_copy2);
            free(arg);
            printf("%s@%s>", name, cwd);
            fgets(command, sizeof(command), stdin);
            continue;
        }

        execute(arg); //  execute the command (if regular command)

        free(arg);
        free(command_copy);
        free(command_copy2);
        printf("%s@%s>", name, cwd);
        fgets(command, sizeof(command), stdin);
    }

    fprintf(file, "%s", command); // print "done" to log file
    fclose(file);

    average_length = (double)total_length / command_counter;

    printf("Num of commands: %d\n", command_counter);
    printf("Total length of all commands: %d\n", total_length);
    printf("Average length of all commands: %lf\n", average_length);
    printf("Number of command that include pipe: %d\n", pipe_command);
    printf("Number of command that include redirection: %d\n", redirection_command);
    printf("See you Next time !\n");

    return 0;
}

// *** Functions *** //

int how_many_words(char *command)
{
    int num_of_words = 1;
    int i = 0;
    while (command[i] != '\0')
    {
        if (command[i] == ' ')
            num_of_words++;
        i++;
    }

    return num_of_words;
}

void push_to_arg(char **arg, char *command, int num_of_words)
{
    // insert the words into the command array
    int i = 0;
    arg[i] = strtok(command, " ");
    i = 1;
    while (i <= num_of_words)
    {
        arg[i] = strtok(NULL, " ");
        i++;
    }
}

void execute(char **arg)
{
    //  split to process
    int son;
    son = fork();

    if (son < 0) //  fork fail
    {
        perror("fork :");
        exit(1);
    }

    if (son == 0) //  child process
    {
        execvp(arg[0], arg);
        exit(0);
    }

    wait(NULL); // wait until son process finished
}

// *** Pipe Functions *** //

int is_pipe(char *command)
{
    int i = 0;
    int first_com = -1;
    int second_com = -1;
    int pipe = -1;

    while (command[i] != '\0')
    {
        if (command[i] == '"') // check if the pipe is inside commas
        {
            if (first_com == -1)
                first_com = i;
            else
                second_com = i;
        }

        if (command[i] == '|')
            pipe = i;
        i++;
    }

    if (pipe != -1)
    {
        if (pipe > first_com && pipe < second_com)
            return 0;

        return 1;
    }
    return 0;
}

void pipe_push(char *command)
{
    int length = how_many_words(command);
    char *command_left = (char *)malloc(sizeof(char) * length);
    char *command_right = (char *)malloc(sizeof(char) * length);

    int i = 0;
    while (command[i] != '\0' && command[i] != '|') // copy the left part
    {
        command_left[i] = command[i];
        i++;
    }
    command_left[i] = '\0';

    i++;
    int j = 0;
    while (command[i] != '\0' && command[i] != '\n') // copy the right part
    {
        if (j == 0 && command[i] == ' ')
        {
            i++;
            continue;
        }
        command_right[j] = command[i];
        i++;
        j++;
    }
    command_right[j] = '\0';

    int redirection = is_redirect(command_right); // check if redirection is needed

    //  initiate left and right, push to left and push to right

    char **left;
    char **right;
    int length_left = how_many_words(command_left);
    int length_right = how_many_words(command_right);

    left = (char **)malloc(sizeof(char *) * (length_left + 1));
    right = (char **)malloc(sizeof(char *) * (length_right + 1));
    push_to_arg(left, command_left, length_left);
    push_to_arg(right, command_right, length_right);

    pipe_execute(left, right, redirection);

    free(left);
    free(right);
    free(command_left);
    free(command_right);
}

void pipe_execute(char **left, char **right, int redirection)
{
    int pipefd[2];
    pid_t leftpid, rightpid;

    if (pipe(pipefd) != 0)
    {
        perror("pipe failed");
        exit(1);
    }

    leftpid = fork();
    if (leftpid < 0) // check fork
    {
        perror("fork() failed");
        exit(1);
    }

    if (leftpid == 0) // left son
    {
        close(pipefd[0]);               // close read channel
        dup2(pipefd[1], STDOUT_FILENO); // output channel is now pipefd[1]
        close(pipefd[1]);

        if (execvp(left[0], left) < 0) // execvp failed
        {
            perror("failed to execvp left");
            exit(1);
        }
        exit(0);
    }

    else // parent
    {
        rightpid = fork();
        if (rightpid < 0) // check fork
        {
            perror("fork() failed");
            exit(1);
        }

        if (rightpid == 0) // right son
        {
            close(pipefd[1]);              // close write channel
            dup2(pipefd[0], STDIN_FILENO); // input channel is now pipefd[0]
            close(pipefd[0]);

            if (redirection == 2 || redirection == 3 || redirection == 4) // if redirection is needed
            {
                redirect(right, redirection);
                exit(0);
            }

            else if (execvp(right[0], right) < 0) // execvp failed
            {
                perror("failed to execvp right");
                exit(1);
            }
            exit(0);
        }

        else // parent wait for the 2 sons to finish
        {
            close(pipefd[0]);
            close(pipefd[1]);
            wait(NULL);
            wait(NULL);
            return;
        }
    }
}

// *** Redirection Functions *** //

int is_redirect(char *command)
{
    int i = 0;
    int redirect = 0;
    int where = -1;
    int first_com = -1;
    int second_com = -1;

    while (command[i] != '\0')
    {
        if (command[i] == '"') // check if the redirect is inside commas
        {
            if (first_com == -1)
                first_com = i;
            else
                second_com = i;
        }

        if (command[i] == '<' && command[i - 1] == ' ' && command[i + 1] == ' ') // type 1 "<"
        {
            redirect = 1;
            where = i;
        }

        else if (command[i] == '>' && command[i - 1] == ' ' && command[i + 1] == '>' && command[i + 2] == ' ') // type 3 ">>"
        {
            redirect = 3;
            where = i;
        }
        else if (command[i] == '>' && command[i - 1] == ' ' && command[i + 1] == ' ') // type 2 ">"
        {
            redirect = 2;
            where = i;
        }

        else if (command[i] == '2' && command[i + 1] == '>' && command[i - 1] == ' ' && command[i + 2] == ' ') // type 4 "2>"
        {
            redirect = 4;
            where = i;
        }

        i++;
    }

    if (where != -1)
    {
        if (where > first_com && where < second_com)
            return 0;

        return redirect;
    }
    return 0;
}

void redirect(char **arg, int type)
{
    int words = 0;
    char *f_name;
    char **com;

    while (arg[words] != NULL)
        words++;

    f_name = arg[words - 1]; // file name
    com = (char **)malloc(sizeof(char *) * (words - 1));

    // insert the words comman to com
    int i = 0;
    while (i < words - 2)
    {
        com[i] = arg[i];
        i++;
    }
    com[i] = NULL;

    // call the right function
    if (type == 1) // "<"
        redirect_1(com, f_name);

    else if (type == 2) // ">"
        redirect_2(com, f_name);

    else if (type == 3) // ">>"
        redirect_3(com, f_name);

    else if (type == 4) // "2>"
        redirect_4(com, f_name);

    free(com);
}

void redirect_1(char **com, char *f_name) // "<"
{
    int fd;
    pid_t pid;
    pid = fork();

    if (pid < 0)
    {
        perror("fork failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        fd = open(f_name, O_RDONLY | O_CREAT | O_APPEND, S_IRWXU);
        dup2(fd, STDIN_FILENO);
        if (execvp(com[0], com) < 0)
        {
            perror("failed to execvp\n");
            exit(1);
        }
    }
    else
        wait(NULL);
}

void redirect_2(char **com, char *f_name) // ">"
{
    int fd;
    pid_t pid;
    pid = fork();

    if (pid < 0)
    {
        perror("fork failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        fd = open(f_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        dup2(fd, STDOUT_FILENO);
        if (execvp(com[0], com) < 0)
        {
            perror("failed to execvp\n");
            exit(1);
        }
    }
    else
        wait(NULL);
}

void redirect_3(char **com, char *f_name) // ">>"
{
    int fd;
    pid_t pid;
    pid = fork();

    if (pid < 0)
    {
        perror("fork failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        fd = open(f_name, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
        dup2(fd, STDOUT_FILENO);
        if (execvp(com[0], com) < 0)
        {
            perror("failed to execvp\n");
            exit(1);
        }
    }
    else
        wait(NULL);
}

void redirect_4(char **com, char *f_name) //  "2>"
{
    int fd;
    pid_t pid;
    pid = fork();

    if (pid < 0)
    {
        perror("fork failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        fd = open(f_name, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
        dup2(fd, STDERR_FILENO);
        if (execvp(com[0], com) < 0)
        {
            perror("failed to execvp\n");
            exit(1);
        }
    }
    else
        wait(NULL);
}
