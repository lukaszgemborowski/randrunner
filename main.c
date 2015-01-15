#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

void generate_and_run()
{
    // init rand generator
    srand(time(NULL));

    // generate code
    const int CODE_SIZE = 2;
    int i;
    int ret;

    char *rand_code = mmap(0, CODE_SIZE + 1, PROT_EXEC | PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    for (i = 0; i < CODE_SIZE; ++i)
    {
        rand_code[i] = rand()%256;
    }

    // put return in code (retq)
    rand_code[CODE_SIZE] = 0xc3;

    // call generated code
    typedef void (func)(void);
    func* f = (func *)rand_code;
    f();

    // call exit with contents of eax register
    asm("movl %%eax,%0" : "=r"(ret));
    exit(ret);
}

void wait_and_print_status(pid_t p)
{
    int status = 0;
    pid_t event_on_pid = waitpid(p, &status, 0);

    if (event_on_pid > 0)
    {
        if (WIFEXITED(status))
        {
            // normal exit
            printf("Exit code: %d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Signal code: %d\n", WTERMSIG(status));
        }
        else
        {
            printf("Unknown reason of process death.\n");
        }
    }
    else
    {
        // error
        perror("waitpid");
    }
}

int main(void)
{
    const int N = 45;
    int i;

    for (i = 0; i < N; ++i)
    {
        // create child process
        pid_t p = fork();

        if (0 == p)
        {
            // generate and run some random code
            generate_and_run();
        }
        else if (-1 == p)
        {
            // error
            perror("fork");
            exit(-errno);
        }
        else
        {
            wait_and_print_status(p);
        }
    }

    return 0;
}

