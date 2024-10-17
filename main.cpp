#include "server.h"

#include <string>
#include <iostream>
#include <vector>
#include <fstream>

void print_help()
{
    std::cout << "Modbus TCP Gateway Server" << std::endl
              << "\tShare registers from anywhere to anywhere using Modbus TCP" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: modbusgatewayserver [--daemon] [-p PORT] REGSIZE" << std::endl;
    std::cout << std::endl;
    std::cout << "\tREGSIZE\t\t Number of holding registers to instantiate." << std::endl;
    std::cout << "\tPORT\t\t The socket server port. Default is 502 but it requires root priviledges." << std::endl;
    std::cout << std::endl;
    std::cout << "Optional arguments:" << std::endl;
    std::cout << "\t--daemon\t\t Will communicate with the Unix notify socket and gracefully handle signals. Only supported on Linux." << std::endl;
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    int port = 502; // Need root priviliedges because less than 1024!
    // Still, I'd use this from a service (which obv has root priviledges)
    // though as it's the Modbus service standard port.
    bool notify_systemd = false;

    // Quick and dirty cowboy command line arguments parsing
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        print_help();
        return EXIT_FAILURE;
    }

    auto it = args.begin();
    while (it != args.end() - 1)
    {
        // Check arguments
        if (*it == "--help" || *it == "-h")
        {
            print_help();
            return EXIT_SUCCESS;
        }
        else if (*it == "--port" || *it == "-p")
        {
            port = std::stoi(*(++it));
        }
        else if (*it == "--daemon")
        {
            notify_systemd = true;
        }
        else
        {
            print_help();
            return EXIT_FAILURE;
        }
        ++it; // Next
    }

    int regsize = std::stoi(args.back());

    std::cout << "### Modbus TCP Gateway Server ###" << std::endl;
    ModbusTCP::Server server;

    if (!server.configure(regsize, port))
        return EXIT_FAILURE;

    if (!server.listenAndAccept())
        return EXIT_FAILURE;

    return server.serveForever(notify_systemd);
}
