/**************************************
**               HEADER
***************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define IN 0
#define OUT 1

enum rel
{
    END = 0,
    PIPE = 1,
    SEMICOL = 2
};

typedef struct	s_cmd
{
	char     **args;
	int      pipe[2];
	int      prev;
	int      type[2];
}               t_cmd;

/**************************************
**               UTILS
***************************************/

int     strlen(const char *str)
{
	int i;
	
	i = 0;
	while (str && str[i])
		i++;
	return (i);
}

int     putstr_fd(char *str, int fd)
{
    return (write(fd, str, strlen(str)));
}

void    print_error(char *msg, char *arg)
{
    putstr_fd("error: ", STDERR_FILENO);
    putstr_fd(msg, STDERR_FILENO);
    if (arg)
    {
        putstr_fd(" ", STDERR_FILENO);
        putstr_fd(arg, STDERR_FILENO);
    }
    putstr_fd("\n", STDERR_FILENO);
}

void    fatal_error()
{
    print_error("fatal", 0);
	exit(1);
}

/**************************************
**               ALGO
***************************************/

void   exec_pipeline(t_cmd *cmd, char **envp)
{
	pid_t  	pid;
	int     ex;

	if (cmd->type[1] == PIPE || cmd->type[0] == PIPE)
	{
		if (pipe(cmd->pipe) < 0)
			fatal_error();
	}
	pid = fork();
	if (pid < 0)
		fatal_error();
	if (pid == 0)
	{
		if (cmd->type[1] == PIPE)
		{
			if (dup2(cmd->pipe[OUT], OUT) < 0)
				fatal_error();
		}
		if (cmd->type[0] == PIPE)
		{
			if (dup2(cmd->prev_pipe, IN) < 0)
				fatal_error();
		}
		if ((ex = execve(cmd->args[0], cmd->args, envp)) < 0)
            print_error("cannot execute", cmd->args[0]);
	
		exit(ex);
	}
	else
	{
		waitpid(pid, 0, 0);

		if (cmd->type[1] != PIPE && cmd->type[0] == PIPE)
			close(cmd->pipe[IN]);
		
		if (cmd->type[1] == PIPE || cmd->type[0] == PIPE)
			close(cmd->pipe[OUT]);
	
		if (cmd->type[0] == PIPE)
			close(cmd->prev_pipe);
	}
}

int cmd_next(t_cmd *list, int start, char **argv)
{
	int end;

	end = start;
	while (argv[end] && strcmp(argv[end], ";") && strcmp(argv[end], "|"))
		end++;
	if (end - start > 0)
	{
		list->type[0] = list->type[1];
		if (argv[end] == NULL)
			list->type[1] = END;
		else if (!strcmp(argv[end], "|"))
			list->type[1] = PIPE;
		else if (!strcmp(argv[end], ";"))
			list->type[1] = SEMICOL;
		argv[end] = NULL;
		list->prev_pipe = list->pipe[IN];
	}
	return (end);
}

int main(int argc, char **argv, char **envp)
{
	t_cmd	cmd;
	int		i;
	int		start;

	i = 0;
	if (argc > 1)
	{
		while (i < argc && argv[i])
		{
			start = i;
			i = cmd_next(&cmd, i, argv);
			if (!strcmp(argv[start], "cd"))
			{
				if (i - start != 2)
                    print_error("cd: bad arguments", 0);
				else if (chdir(cmd.args[1]) == -1)
                    print_error("cd: cannot change directory to", cmd.args[1]);
			}
			else if (i > start)
			{
				cmd.args = &argv[start];
				exec_pipeline(&cmd, envp);
			}
		}
	}
	return 0;
}
