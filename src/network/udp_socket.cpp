#include "network/udp_socket.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

namespace trading {
namespace network {

UDPSocket::UDPSocket(const std::string& ip, uint16_t port)
    : socket_fd_(-1), is_active_(false), ip_(ip), port_(port) {
    
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to create UDP socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(socket_fd_);
        throw std::system_error(errno, std::system_category(), "Failed to bind UDP socket");
    }

    is_active_ = true;
}

UDPSocket::~UDPSocket() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
    }
}

bool UDPSocket::sendData(const void* data, size_t size, std::error_code& ec) noexcept {
    if (!is_active_) {
        ec = std::make_error_code(std::errc::not_connected);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(ip_.c_str());

    ssize_t sent = sendto(socket_fd_, data, size, MSG_DONTWAIT,
                         reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    
    if (sent < 0) {
        ec = std::error_code(errno, std::system_category());
        return false;
    }

    return true;
}

size_t UDPSocket::receiveData(void* buffer, size_t size, std::error_code& ec, uint32_t timeout_ms) noexcept {
    if (!is_active_) {
        ec = std::make_error_code(std::errc::not_connected);
        return 0;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd_, &read_fds);

    timeval timeout{};
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int ready = select(socket_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
    if (ready < 0) {
        ec = std::error_code(errno, std::system_category());
        return 0;
    }
    
    if (ready == 0) {
        ec = std::make_error_code(std::errc::timed_out);
        return 0;
    }

    sockaddr_in sender_addr{};
    socklen_t sender_addr_len = sizeof(sender_addr);
    
    ssize_t received = recvfrom(socket_fd_, buffer, size, MSG_DONTWAIT,
                               reinterpret_cast<sockaddr*>(&sender_addr), &sender_addr_len);

    if (received < 0) {
        ec = std::error_code(errno, std::system_category());
        return 0;
    }

    return static_cast<size_t>(received);
}

void UDPSocket::setNonBlocking(bool enabled) {
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to get socket flags");
    }

    flags = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (fcntl(socket_fd_, F_SETFL, flags) < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to set socket flags");
    }
}

void UDPSocket::setReceiveBufferSize(int size) {
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to set receive buffer size");
    }
}

void UDPSocket::setSendBufferSize(int size) {
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0) {
        throw std::system_error(errno, std::system_category(), "Failed to set send buffer size");
    }
}

} // namespace network
} // namespace trading
