#include "server.h"

#include <iostream>

using namespace ModbusTCP;

#define NB_CONNECTION 10

// These functions are needed for the signals and service in unix systems

static std::atomic<bool> __exit{false};

static void __signalCallbackHandler(int signum)
{
    std::cout << std::endl << "[SIGINT caught]" << std::endl;
    __exit = true;
}

#ifdef WIN32
// Windows specific definitions
// I should probably add some WSA stuff
using socklen_t = int; // socklen_t is defined in Unix systems.
// Windows uses int.

// closesocket vs close
inline int __close(int socket)
{
    return closesocket(socket);
}

#else
// Unix specific definitions
#include "config.h"

inline int __close(int socket)
{
    return close(socket);
}

// Used to work with the notificatiion socket in unix systems
void __notifySystemdReady()
{
    // We call directly the notification socket as uwsgi does:
    // https://github.com/unbit/uwsgi/blob/master/core/notify.c

    struct sockaddr_un sd_sun
    {
    };
    struct msghdr msg
    {
    };
    const char *state;

    int sd_notif_fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (sd_notif_fd < 0)
    {
        std::cout << "Cannot open notification socket" << std::endl;
        return;
    }

    int len = strlen(config::sd_notify_socket);
    memset(&sd_sun, 0, sizeof(struct sockaddr_un));
    sd_sun.sun_family = AF_UNIX;
    strncpy(sd_sun.sun_path, config::sd_notify_socket, std::min(static_cast<size_t>(len), sizeof(sd_sun.sun_path)));

    if (sd_sun.sun_path[0] == '@')
    {
        sd_sun.sun_path[0] = 0;
    }

    memset(&msg, 0, sizeof(struct msghdr));
    msg.msg_iov = reinterpret_cast<struct iovec *>(malloc(sizeof(struct iovec) * 3));

    if (msg.msg_iov == nullptr)
    {
        std::cout << "Cannot allocate msg.msg_iov" << std::endl;

        if (::shutdown(sd_notif_fd, SHUT_RDWR) < 0)
            std::cout << "Cannot shutdown notification socket" << std::endl;
        close(sd_notif_fd);
        return;
    }

    memset(msg.msg_iov, 0, sizeof(struct iovec) * 3);
    msg.msg_name = &sd_sun;
    msg.msg_namelen = sizeof(struct sockaddr_un) - (sizeof(sd_sun.sun_path) - static_cast<unsigned int>(len));

    state = "STATUS=Modbus TCP gateway server is ready\nREADY=1\n";
    msg.msg_iov[0].iov_base = const_cast<char *>(state);
    msg.msg_iov[0].iov_len = strlen(state);
    msg.msg_iovlen = 1;

    if (sendmsg(sd_notif_fd, &msg, MSG_NOSIGNAL) < 0)
        std::cout << "Cannot send notification to systemd." << std::endl;
    free(msg.msg_iov);

    if (::shutdown(sd_notif_fd, SHUT_RDWR) < 0)
        std::cout << "Cannot shutdown notification socket" << std::endl;

    close(sd_notif_fd);
}

#endif



// Now the server class

Server::Server() : m_ctx(nullptr),
    m_port(0),
    m_mapping(nullptr),
    m_server(0),
    m_headerLength(0)
{
}

Server::~Server()
{
    finalize();
}

void Server::finalize()
{
    std::cout << "Finalize [" << modbus_strerror(errno) << "]" << std::endl;

    // Closing server socket
    if (m_server != -1)
        __close(m_server);

    // Free Modbus resources
    modbus_mapping_free(m_mapping);
    modbus_close(m_ctx);
    modbus_free(m_ctx);
}

bool Server::configure(int regsize, int port)
{
    m_port = port;
    m_regsize = regsize;

    m_ctx = modbus_new_tcp("0.0.0.0", m_port);
    if (m_ctx == nullptr)
    {
        std::cerr << "Can't initialize modbus tcp context for " << m_port << std::endl;
        std::cerr << "Modbus error: " << modbus_strerror(errno) << std::endl;
        return false;
    }

    m_headerLength = modbus_get_header_length(m_ctx);

    m_mapping = modbus_mapping_new(0, 0, m_regsize, 0);
    if (m_mapping == nullptr)
    {
        std::cerr << "Failed to allocate mapping: " << modbus_strerror(errno) << std::endl;
        modbus_free(m_ctx);
        return false;
    }

    return true;
}

bool Server::listenAndAccept()
{
    std::cout << "Start listening on port " << m_port << std::endl;
    m_server = modbus_tcp_listen(m_ctx, NB_CONNECTION); // One conection only
    if (m_server == -1)
    {
        std::cerr << "Failed to listen to socket: " << modbus_strerror(errno) << std::endl;
        return false;
    }

    /* Clear the reference set of socket */
    FD_ZERO(&m_refset);
    /* Add the server socket */
    FD_SET(m_server, &m_refset);
    /* Keep track of the max file descriptor */
    m_fdmax = m_server;

    std::cout << "Listening!" << std::endl;
    return true;
}

int Server::serveForever(bool notify_systemd)
{
    // Initializing the signal handler to catch SIGINT
    signal(SIGINT, __signalCallbackHandler);

    // Sending to systemd that the service is ready
    std::cout << "Serving forever..." << std::endl;

#ifdef WIN32
    (void)notify_systemd;
#else
    if (notify_systemd)
        __notifySystemdReady();
#endif

    int rc;
    int header;
    int masterSocket;

    for (;;)
    {
        // Check connections readiness?
        m_rdset = m_refset;
        if (select(m_fdmax + 1, &m_rdset, NULL, NULL, NULL) == -1)
        {
            if (__exit)
                std::cout << "Server select interrupted by SIGINT" << std::endl;
            else
                std::cerr << "Server select() failure." << std::endl;
            break; /* Quit */
        }

        // Handle SIGINT
        if (__exit)
            break;

        // Run through the existing connections looking for data to read
        for (masterSocket = 0; masterSocket <= m_fdmax; masterSocket++)
        {

            if (!FD_ISSET(masterSocket, &m_rdset))
            {
                continue;
            }

            if (masterSocket == m_server)
            {
                /* A client is asking a new connection */
                socklen_t addrlen;
                struct sockaddr_in clientaddr;
                int newfd;

                /* Handle new connections */
                addrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, sizeof(clientaddr));
                newfd = accept(m_server, (struct sockaddr *)&clientaddr, &addrlen);
                if (newfd == -1)
                {
                    std::cout << "Server accept() error" << std::endl;
                }
                else
                {
                    FD_SET(newfd, &m_refset);

                    if (newfd > m_fdmax)
                    {
                        /* Keep track of the maximum */
                        m_fdmax = newfd;
                    }
                    std::cout << "New connection from " << inet_ntoa(clientaddr.sin_addr)
                              << ":" << clientaddr.sin_port << " on socket " << newfd << std::endl;
                }
            }
            else
            {
                modbus_set_socket(m_ctx, masterSocket);
                rc = modbus_receive(m_ctx, m_queries);

                /* If a receive returns -1, we close the connection */
                if (rc == -1)
                {
                    std::cout << "Connection closed on socket " << masterSocket << std::endl;
                    __close(masterSocket);

                    /* Remove from reference set */
                    FD_CLR(masterSocket, &m_refset);
                    if (masterSocket == m_fdmax)
                        m_fdmax--;
                }
                else if (rc > 0)
                {

                    header = m_queries[m_headerLength];
                    if (header != 0x03 && header != 0x06 && header != 0x10)
                    {
                        std::cout << "Function code not supported: " << m_queries[m_headerLength] << std::endl;
                        std::cout << "It may have an unwanted behavior." << std::endl;
                    }

                    rc = modbus_reply(m_ctx, m_queries, rc, m_mapping);
                    if (rc == -1)
                    {
                        break; /* Quit on reply error? */
                    }
                }
            }
        }
    }

    return __exit ? EXIT_SUCCESS : EXIT_FAILURE;
}
