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

#include "TCPTextServer.h"

namespace GDB {

    enum class WatchpointType : uint32_t {
        WRITE, READ, ACCESS
    };

    struct Watchpoint {
        uint64_t addressStart{ 0 };
        uint64_t addressEnd{ 0 };
        uint64_t addressStartOrg{ 0 }; // un-normalized address, GDB needs this
        WatchpointType type{};

        auto operator==(const Watchpoint& w) const {
            return addressStart == w.addressStart && addressEnd == w.addressEnd
                && addressStartOrg == w.addressStartOrg && type == w.type;
        }

        auto hasOverlap(uint64_t start, uint64_t end) const {
            return (end >= addressStart) && (start <= addressEnd);
        }

        std::string getTypePrefix() const {
            if (type == WatchpointType::WRITE)return "watch:";
            if (type == WatchpointType::READ)return "rwatch:";
            return "awatch:";
        }
    };
}
