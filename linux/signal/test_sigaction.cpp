#include <iostream>
#include <signal.h> // for sigaction(), sigfillset
#include <unistd.h> // for pause()
void sig_handler(int sig)
{
    std::cout << "receive signal:" << sig << std::endl;
}
int main(int argc, char *argcs[])
{
    struct sigaction act, oact;
    act.sa_handler = sig_handler;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);
    sigaction(SIGQUIT, &act, &oact);
    pause();
    return 0;
}