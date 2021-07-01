/**************************************
**               HEADER
***************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

typedef struct  s_cmd
{
    char **env;
    char **args;
    int pipe[2];
    int prev;
    int type[2];
    int len;
}               t_cmd;

typedef enum e_type
{
    END = 0,
    PIPE = 1,
    SEMICOLON = 2
}               t_type;

/**************************************
**               UTILS
***************************************/

int ft_strlen(char *str)
{
    int i;

    i = 0;
    while (str && str[i])
        i++;
    return (i);
}

void    ft_putstr_fd(char *str, int fd)
{
    write(fd, str, ft_strlen(str));
}

void    print_error(char *msg, char *arg)
{
    ft_putstr_fd("error: ", 2);
    ft_putstr_fd(msg, 2);
    if (arg)
    {
        ft_putstr_fd(" ", 2);
        ft_putstr_fd(arg, 2);
    }
    ft_putstr_fd("\n", 2);
}

void    fatal_error(void)
{
    print_error("fatal", 0);
    exit(1);
}

/**************************************
**               EXEC
***************************************/
    
int     cmd_exec(t_cmd *cmd)
{
    pid_t pid;

    if (cmd->type[1] == PIPE)
	{
		if (pipe(cmd->pipe) < 0)
			fatal_error();
	}
    pid = fork();
    if (pid < 0)
    {
        fatal_error();
    }
    else if (pid == 0)
    {
        if (cmd->type[0] == PIPE)
            dup2(cmd->prev, 0);
        if (cmd->type[1] == PIPE)
            dup2(cmd->pipe[1], 1);

        if (execve(cmd->args[0], cmd->args, cmd->env) < 0)
        {
            print_error("cannot execute", cmd->args[0]);
            exit(1);
        }
    }
    else
    {
        waitpid(pid, 0, 0);

		if (cmd->type[1] == PIPE)
			close(cmd->pipe[1]);
    
        if (cmd->type[0] == PIPE)
            close(cmd->prev);
        
        if (cmd->type[0] == PIPE && cmd->type[1] != PIPE)
            close(cmd->pipe[0]);
    }
    return (0);
}

/**************************************
**               ALGO
***************************************/

int cmd_end(char **argv)
{
    int i;

    i = 0;
    while (argv[i] && strcmp(argv[i], ";") && strcmp(argv[i], "|"))
        i++;
    return (i);
}


int    builtin_cd(t_cmd *cmd)
{
    if (cmd->len > 2)
    {
        print_error("cd: bad arguments", 0);
        return (1);
    }
    else if (chdir(cmd->args[1]) == -1)
    {
        print_error("cd: cannot change directory to", cmd->args[1]);
        return (1);
    }
    return (0);
}

int cmd_len(t_cmd *cmd, char **argv)
{
    int len;

    len = cmd_end(argv);
    cmd->args = argv;
    cmd->type[0] = cmd->type[1];
    if (len > 0)
    {
        if (argv[len] == NULL)
            cmd->type[1] = END;
        else if (!strcmp(argv[len], "|"))
            cmd->type[1] = PIPE;
        else if (!strcmp(argv[len], ";"))
            cmd->type[1] = SEMICOLON;
        argv[len] = NULL;
        cmd->prev = cmd->pipe[0];
    }
    return (len);
}

int main(int argc, char **argv, char **envp)
{
    t_cmd cmd;
    int i;
    int res;

    i = 1;
    res = 0;
    cmd.env = envp;
    if (argc <= 1)
        return (0);
    while (i < argc && argv[i])
    {
        cmd.len = cmd_len(&cmd, &argv[i]);
        i += cmd.len;
        if (!strcmp(cmd.args[0], "cd"))
        {
            res = builtin_cd(&cmd);
        }
        else if (cmd.len > 0)
        {
            res = cmd_exec(&cmd);
        }
        i++;
    }
    return (res);
}