/*
----------------------------------------------------------------------
ares

Copyright (c) 2004-2025 ares team, Near et al

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
----------------------------------------------------------------------
Removed nall references and weird C++ style by aglab2
*/

#include "TCPSocket.h"

#include <inttypes.h>
#include <memory>
#include <thread>

#include <ws2tcpip.h>

struct sockaddr_in;
struct sockaddr_in6;

namespace {
    constexpr bool TCP_LOG_MESSAGES = false;

    constexpr uint32_t TCP_BUFFER_SIZE = 1024 * 16;
    constexpr uint32_t CLIENT_SLEEP_MS = 10; // ms to sleep while checking for new clients
    constexpr uint32_t CYCLES_BEFORE_SLEEP = 100; // how often to do a send/receive check before a sleep
    constexpr uint32_t RECEIVE_TIMEOUT_SEC = 1; // only important for latency of disconnecting clients, reads are blocming anyways

    // A few platform specific socket functions:
    // (In general, windows+linux share the same names, yet they behave differenly)
    bool socketSetBlockingMode(int32_t socket, bool isBlocking)
    {
        if (socket < 0)return false;
        u_long state = isBlocking ? 0 : 1;
        return ioctlsocket(socket, FIONBIO, &state) == NO_ERROR;
    }

    auto socketShutdown(int32_t socket)
    {
        if (socket < 0)return;
        ::shutdown(socket, SD_BOTH);
    }

    auto socketClose(int32_t socket)
    {
        if (socket < 0)return;
        ::closesocket(socket);
    }
}

namespace TCP
{
    std::string Socket::getURL(uint32_t port, bool useIPv4) const
    {
        return { useIPv4 ? "127.0.0.1:" : "[::1]:", port };
    }

    bool Socket::open(uint32_t port, bool useIPv4)
    {
        stopServer = false;

        auto url = getURL(port, useIPv4);
        printf("Opening TCP-server on %s\n", url.data());

        auto threadServer = std::thread([this, port, useIPv4]() {
            serverRunning = true;

            while (!stopServer) {
                fdServer = socket(useIPv4 ? AF_INET : AF_INET6, SOCK_STREAM, 0);
                if (fdServer < 0)
                    break;

                {
                    const char valueOn = 1;
                    setsockopt(fdServer, SOL_SOCKET, SO_REUSEADDR, &valueOn, sizeof(int32_t));
                    setsockopt(fdServer, IPPROTO_TCP, TCP_NODELAY, &valueOn, sizeof(int32_t));

                    if (!socketSetBlockingMode(fdServer, true)) {
                        OutputDebugStringA("TCP: failed to set to blocking mode!\n");
                    }

                    DWORD rcvTimeMs = 1000 * RECEIVE_TIMEOUT_SEC;
                    setsockopt(fdServer, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcvTimeMs, sizeof(rcvTimeMs));
                }

                int32_t bindRes;
                if (useIPv4) {
                    sockaddr_in serverAddrV4{};
                    serverAddrV4.sin_family = AF_INET;
                    serverAddrV4.sin_addr.s_addr = htonl(INADDR_ANY);
                    serverAddrV4.sin_port = htons(port);

                    bindRes = ::bind(fdServer, (sockaddr*)&serverAddrV4, sizeof(serverAddrV4)) < 0;
                }
                else {
                    sockaddr_in6 serverAddrV6{};
                    serverAddrV6.sin6_family = AF_INET6;
                    serverAddrV6.sin6_addr = in6addr_loopback;
                    serverAddrV6.sin6_port = htons(port);

                    bindRes = ::bind(fdServer, (sockaddr*)&serverAddrV6, sizeof(serverAddrV6)) < 0;
                }

                if (bindRes < 0 || listen(fdServer, 1) < 0) {
                    printf("error binding socket on port %d! (%s)\n", port, strerror(errno));
                    break;
                }

                // scan for new connections
                while (fdClient < 0) {
                    fdClient = ::accept(fdServer, nullptr, nullptr);
                    if (fdClient < 0) {
                        if (errno != EAGAIN) {
                            if (!stopServer)
                                printf("error accepting connection! (%s)\n", strerror(errno));
                            break;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(CLIENT_SLEEP_MS));
                    }
                }
                if (fdClient < 0) {
                    break;
                }

                // close the server socket, we only want one client
                socketClose(fdServer);
                fdServer = -1;

                while (!stopServer && fdClient >= 0) {
                    // Kick client if we need to
                    if (wantKickClient) {
                        socketClose(fdClient);
                        fdClient = -1;
                        wantKickClient = false;
                        onDisconnect();
                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(CLIENT_SLEEP_MS));
                }
            }

            printf("Stopping TCP-server...\n");

            socketClose(fdClient);
            fdClient = -1;

            wantKickClient = false;

            printf("TCP-server stopped\n");
            serverRunning = false;
            });

        auto threadSend = std::thread([this]()
            {
                std::vector<uint8_t> localSendBuffer{};
                uint32_t cycles = 0;

                while (!stopServer)
                {
                    if (fdClient < 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(CLIENT_SLEEP_MS));
                        continue;
                    }

                    { // copy send-data to minimize lock time
                        std::lock_guard guard{ sendBufferMutex };
                        if (sendBuffer.size() > 0) {
                            localSendBuffer = sendBuffer;
                            sendBuffer.resize(0);
                        }
                    }

                    // send data
                    if (localSendBuffer.size() > 0) {
                        auto bytesWritten = send(fdClient, (const char*)localSendBuffer.data(), localSendBuffer.size(), 0);
                        if (bytesWritten < localSendBuffer.size()) {
                            printf("Error sending data! (%s)\n", strerror(errno));
                        }

                        localSendBuffer.resize(0);
                        cycles = 0; // sending once has a good chance of sending more -> reset sleep timer
                    }

                    if (cycles++ >= CYCLES_BEFORE_SLEEP) {
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                        cycles = 0;
                    }
                }
            });

        auto threadReceive = std::thread([this]()
            {
                uint8_t packet[TCP_BUFFER_SIZE]{ 0 };

                while (!stopServer)
                {
                    if (fdClient < 0 || wantKickClient) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(CLIENT_SLEEP_MS));
                        continue;
                    }

                    // receive data from connected clients
                    int32_t length = recv(fdClient, (char*) packet, TCP_BUFFER_SIZE, 0);
                    if (length > 0) {
                        std::lock_guard guard{ receiveBufferMutex };
                        auto oldSize = receiveBuffer.size();
                        receiveBuffer.resize(oldSize + length);
                        memcpy(receiveBuffer.data() + oldSize, packet, length);
                    }
                    else if (length == 0) {
                        disconnectClient();
                    }
                    else {
                        if (WSAGetLastError() != WSAETIMEDOUT) {
                            printf("TCP server: error receiving data from client: %s\n", strerror(errno));
                            disconnectClient();
                        }
                    }
                }
            });

        threadServer.detach();
        threadSend.detach();
        threadReceive.detach();

        return true;
    }

    void Socket::close(bool notifyHandler)
    {
        stopServer = true;

        // we have to forcefully shut it down here, since otherwise accept() would hang causing a UI crash
        socketShutdown(fdServer);
        socketClose(fdClient);
        socketClose(fdServer);
        fdServer = -1;
        fdClient = -1;

        while (serverRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250)); // wait for other threads to stop
        }

        if (notifyHandler) {
            onDisconnect(); // don't call this in destructor, it's virtual
        }
    }

    void Socket::update()
    {
        std::vector<uint8_t> data{};

        { // local copy, minimize lock time
            std::lock_guard guard{ receiveBufferMutex };
            if (receiveBuffer.size() > 0) {
                data = receiveBuffer;
                receiveBuffer.resize(0);
            }
        }

        if (data.size() > 0) {
            onData(data);
        }
    }

    void Socket::disconnectClient()
    {
        wantKickClient = true;
    }

    void Socket::sendData(const uint8_t* data, uint32_t size)
    {
        std::lock_guard guard{ sendBufferMutex };
        uint32_t oldSize = sendBuffer.size();
        sendBuffer.resize(oldSize + size);
        memcpy(sendBuffer.data() + oldSize, data, size);
    }
}