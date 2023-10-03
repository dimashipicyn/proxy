#include "Log.h"
#include "Server.h"

#include <getopt.h>

enum Options
{
    Host,
    Port,
    DbHost,
    DbPort
};

int main(int argc, char** argv)
{
    const char* host = "0.0.0.0";
    int port = 5433;
    const char* dbHost = "0.0.0.0";
    int dbPort = 5432;


    int c = -1;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"host", required_argument, 0, Host},
            {"port", required_argument, 0, Port},
            {"db_host", required_argument, 0, DbHost},
            {"db_port", required_argument, 0, DbPort},
            {"help", no_argument, 0, 0},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case Host:
            host = optarg;
            break;

        case Port:
            port = atoi(optarg);
            break;

        case DbHost:
            dbHost = optarg;
            break;

        case DbPort:
            dbPort = atoi(optarg);
            break;

        default:
            LOG_ERROR("%s\n", "Undefined option!");
        }
    }

    LOG_DEBUG("Start server with host: %s port: %d dbHost: %s dbPort: %d\n", host, port, dbHost, dbPort);
    Server server(host, port, dbHost, dbPort);

    server.run();
}
