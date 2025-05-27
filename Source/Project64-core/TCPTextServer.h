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

#pragma once

#include "TCPSocket.h"

/**
 * Provides text-based TCP server on top of the Socket.
 * This handles incoming messages and can send data back to the client.
 */
namespace TCPText {

    class Server : public TCP::Socket {
    public:
        bool hadHandshake{ false };

    protected:
        void onData(const std::vector<uint8_t>& data) override;

        auto sendText(const std::string& text) -> void;
        virtual auto onText(std::string_view text) -> void = 0;
    };

}
