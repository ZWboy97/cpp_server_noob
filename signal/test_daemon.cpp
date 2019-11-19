#include <iostream>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argcs[])
{
    /* Put the program in the background, and dissociate from the controlling
    terminal.  If NOCHDIR is zero, do `chdir ("/")'.  If NOCLOSE is zero,
    redirects stdin, stdout, and stderr to /dev/null.  */
    if (daemon(0, 0) == -1)
    {
        std::cout << "error" << std::endl;
        exit(1);
    }
    while (1)
    {
        sleep(1);
    }
    return 0;
}