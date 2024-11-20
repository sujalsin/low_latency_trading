#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <system_error>

namespace trading {
namespace network {

class UDPSocket {
public:
    UDPSocket(const std::string& ip, uint16_t port);
    ~UDPSocket();

    // Non-copyable
    UDPSocket(const UDPSocket&) = delete;
    UDPSocket& operator=(const UDPSocket&) = delete;

    // Send data with minimal overhead
    bool sendData(const void* data, size_t size, std::error_code& ec) noexcept;
    
    // Receive data with timeout
    size_t receiveData(void* buffer, size_t size, std::error_code& ec, uint32_t timeout_ms = 100) noexcept;

    // Set socket options
    void setNonBlocking(bool enabled);
    void setReceiveBufferSize(int size);
    void setSendBufferSize(int size);

private:
    int socket_fd_;
    std::atomic<bool> is_active_;
    std::string ip_;
    uint16_t port_;
};

} // namespace network
} // namespace trading
