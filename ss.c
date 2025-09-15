#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define SYNC 1
#define ASYNC 0

#ifndef strdup
#define strdup(s) memcpy(malloc(strlen((s)) + 1), (s), strlen((s)) + 1)
#endif

char *expand(char *);
void prompt();

char *PROMPT = ">> ";

void
prompt()
{
        printf("%s", expand(PROMPT));
}

char *
expand(char *str)
{
        return str;
}

/* ret and *ret must be free */
char **
split(char *str, char sep)
{
        char **arr = malloc(sizeof(char *));
        int arrlen = 1;
        char *c;

        arr[0] = c = strdup(str);
        for (; *c; c++) {
                if (*c != sep) continue;
                *c = 0;
                arr = realloc(arr, sizeof(char *) * (arrlen + 1));
                arr[arrlen] = c + 1;
                ++arrlen;
        }
        return arr;
}

int
run(char **argv, int fdin, int fdout, bool sync, int *pid)
{
        int child;
        int status = 0;
        int fdinold, fdoutold;

        switch (child = fork()) {
        case -1:
                perror("fork");
                return -1;
        case 0: {
                fdinold = dup(STDIN_FILENO);
                fdoutold = dup(STDOUT_FILENO);
                dup2(fdin, STDIN_FILENO);
                dup2(fdout, STDOUT_FILENO);

                execvp(argv[0], argv);
                perror(argv[0]);

                dup2(STDIN_FILENO, fdinold);
                dup2(STDOUT_FILENO, fdoutold);

                exit(0);
        }
        default:
                if (pid) *pid = child;
                if (sync) waitpid(child, &status, 0);
                return status;
        }
}

int
cd(char **argv)
{
        return chdir(argv[1]);
}

bool
is_builtin(char *cmd)
{
        return !strcmp("cd", cmd);
}

int
run_builtin(char **argv, int fdin, int fdout, bool sync, int *pid)
{
        int child;
        int status = 0;
        int fdinold, fdoutold;

        if (sync == ASYNC)
                fprintf(stderr, "builtin cmd (%s) should be sync\n", *argv);

        fdinold = dup(STDIN_FILENO);
        fdoutold = dup(STDOUT_FILENO);
        dup2(fdin, STDIN_FILENO);
        dup2(fdout, STDOUT_FILENO);

        if (!strcmp("cd", *argv)) status = cd(argv);

        dup2(STDIN_FILENO, fdinold);
        dup2(STDOUT_FILENO, fdoutold);

        return status;
}

int
execute(char *cmd)
{
        char **scmd = split(cmd, ' ');

        if (is_builtin(scmd[0])) {
                run_builtin(scmd, STDIN_FILENO, STDOUT_FILENO, SYNC, NULL);
        } else
                run(scmd, STDIN_FILENO, STDOUT_FILENO, SYNC, NULL);

        free(*scmd);
        free(scmd);
        return 0;
}

char *
getinput()
{
        char *buf = malloc(127);
        while (!fgets(buf, 127, stdin)) {
                prompt();
        }
        buf[strcspn(buf, "\n\r")] = 0;
        return buf;
}

int
main()
{
        bool alive = true;
        while (alive) {
                prompt();
                execute(getinput());
        }
}
