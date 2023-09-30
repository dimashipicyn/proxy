#include "Server.h"

int main(int, char**)
{
    Server server("127.0.0.1", 5433);

    server.run();
}
