#include "Server.h"

int main(int, char**)
{
    Server server("0.0.0.0", 5433);

    server.run();
}
