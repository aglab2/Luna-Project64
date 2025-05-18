#include "stdafx.h"
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

#include "TCPTextServer.h"

namespace TCPText
{
    void Server::sendText(const std::string& text)
    {
        sendData((const uint8_t*)text.data(), text.size());
    }

    void Server::onData(const std::vector<uint8_t>& data)
    {
        std::string_view dataStr((const char*)data.data(), data.size());

        if (!hadHandshake) {
            hadHandshake = true;

            // This is a security check for browsers.
            // Any website can request localhost via JS or HTML, while it can't see the result, 
            // GDB will receive the data and commands could be injected (true for all GDB-servers).
            // Since all HTTP requests start with headers, we can simply block anything that doesn't start like a GDB client.
            if (dataStr[0] != '+') {
                printf("Non-GDB client detected (message: %s), disconnect client\n", dataStr.data());
                disconnectClient();
                return;
            }

            onConnect();
        }

        onText(dataStr);
    }
}