#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <cstdint>

namespace config
{
    /// Maximum length of the Unix socket file path
    ///
    /// Note:
    /// 108 is the maximum length on Linux. See:
    /// http://man7.org/linux/man-pages/man7/unix.7.html
    constexpr uint32_t unix_socket_path_len = 108;

    constexpr bool notify_systemd = true;
    constexpr char sd_notify_socket[unix_socket_path_len] = "/run/systemd/notify";

    /// Enable/Disable the Nagle algorithm in the TCP buffer
    constexpr bool tcp_nodelay = true;

    /// Unix socket file path
    constexpr char unix_socket_path[unix_socket_path_len] = "/var/run/modbus-tcp-gateway-server.sock";
    /// Unix socket max parallel connections
    constexpr int unix_socket_worker_connections = 100;
}

#endif // __CONFIG_H__
