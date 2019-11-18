/**
 * main.cpp entry
 * @auther ljc
 * @data 2019-11-18
 */

#include <iostream>
#include "server.h"

int main(int arg, char *argv[])
{
    avdance::Server *server = new avdance::Server();
    if (server)
    {
        server->run();
    }
    return 0;
}