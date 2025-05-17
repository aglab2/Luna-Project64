#pragma once

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

/**
 * Opens a TCP server with callbacks to send and receive data.
 *
 * This spawns 3 new threads:
 * threadServer:  listens for new connections, kicks connections
 * threadSend:    sends data to the client
 * threadReceive: receives data from the client
 *
 * Each contains it's own loop including sleeps to not use too much CPU.
 * The exception is threadReceive which relies on the blocking recv() call (kernel wakes it up again).
 *
 * Incoming and outgoing data is synchronized using mutexes,
 * and put into buffers that are shared with the main thread.
 * Meaning, the thread that calls 'update()' with also be the one that gets 'onData()' calls.
 * No additional synchronization is needed.
 *
 * NOTE: if you work on the loop/sleeps, make sure to test CPU usage and package-latency.
 */

#include <stdint.h>

#include <atomic>
#include <string>
#include <vector>
#include <mutex>

namespace TCP {

    class Socket {
    public:
        bool open(uint32_t port, bool useIPv4);
        void close(bool notifyHandler = true);

        void disconnectClient();

        bool isStarted() const { return serverRunning; }
        bool hasClient() const { return fdClient.load() >= 0; }

        std::string getURL(uint32_t port, bool useIPv4) const;

        ~Socket() { close(false); }

    protected:
        void update();

        void sendData(const uint8_t* data, uint32_t size);
        virtual void onData(const std::vector<uint8_t>& data) = 0;

        virtual void onConnect() = 0;
        virtual void onDisconnect() = 0;

    private:
        std::atomic<bool> stopServer{ false }; // set to true to let the server-thread know to stop.
        std::atomic<bool> serverRunning{ false }; // signals the current state of the server-thread
        std::atomic<bool> wantKickClient{ false }; // set to true to let server know to disconnect the current client (if conn.)

        std::atomic<int32_t> fdServer{ -1 };
        std::atomic<int32_t> fdClient{ -1 };

        std::vector<uint8_t> receiveBuffer{};
        std::mutex receiveBufferMutex{};

        std::vector<uint8_t> sendBuffer{};
        std::mutex sendBufferMutex{};
    };

}
