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
#include "Watchpoint.h"

#include <functional>
#include <optional>

namespace GDB {

    enum class Signal : uint8_t {
        HANGUP = 1,
        INT = 2,
        QUIT = 3,
        ILL = 4,
        TRAP = 5,
        ABORT = 6,
        EMT = 7,
        FPE = 8,
        KILL = 9,
        BUS = 10,
        SEGV = 11,
        SYS = 12,
        PIPE = 13,
        ALRM = 14,
        TERM = 15,
        URG = 16,
        STOP = 17,
        TSTP = 18,
        CONT = 19,
        CHLD = 20,
        TTIN = 21,
        TTOU = 22,
        IO = 23,
        PROF = 27,
        WINCH = 28,
        LOST = 29,
    };

    /**
     * This implements a GDB server to handle remote debugging via a GDB client.
     * It is both independent of ares itself and any specific system.
     * Functionality is added by providing system-specific callbacks, as well as using the API inside a system.
     * (See the Readme.md file for more information.)
     *
     * NOTE:
     * Command handling and the overall logic was carefully designed to support as many IDEs and GDB versions as possible.
     * Things can break very easily (and the official documentation may lie), so be very sure of any changes made here.
     * If changes are necessary, please verify that the following gdb-versions / IDEs still work properly:
     *
     * GDB:
     * - gdb-multiarch        (the plain vanilla version exists in most package managers, supports a lot of arches)
     * - mips64-ultra-elf-gdb (special MIPS build of gdb-multiarch, i do NOT recommend it, behaves strangely)
     * - mingw-w64-x86_64-gdb (vanilla build for Windows/MSYS)
     *
     * IDEs/Tools:
     * - GDB's CLI
     * - VSCode
     * - CLion (with bundled gdb-multiarch)
     *
     * For testing, please also check both linux and windows (WSL2).
     * With WSL2, windows-ares is started from within WSL, while the debugger runs in linux.
     * This can be easily tested with VSCode and it's debugger.
     */
    class Server : public TCPText::Server {
    public:

        void reset();

        struct {
            // Memory
            std::function<std::string(uint64_t address, uint32_t byteCount)> read{};
            std::function<void(uint64_t address, std::vector<uint8_t> value)> write{};
            std::function<uint64_t(uint64_t address)> normalizeAddress{};

            // Registers
            std::function<std::string()> regReadGeneral{};
            std::function<void(const std::string& regData)> regWriteGeneral{};
            std::function<std::string(uint32_t regIdx)> regRead{};
            std::function<bool(uint32_t regIdx, uint64_t regValue)> regWrite{};

            // Emulator
            std::function<void(uint64_t address)> emuCacheInvalidate{};
            std::function<std::string()> targetXML{};


        } hooks{};

        // Exception
        bool reportSignal(Signal sig, uint64_t originPC);

        // PC / Memory State Updates
        bool reportPC(uint64_t pc);
        void reportMemRead(uint64_t address, uint32_t size);
        void reportMemWrite(uint64_t address, uint32_t size);

        // Breakpoints / Watchpoints
        auto isHalted() const { return forceHalt && haltSignalSent; }
        auto hasBreakpoints() const {
            return !breakpoints.empty() || singleStepActive || !watchpointRead.empty() || !watchpointWrite.empty();
        }

        auto getPcOverride() const { return pcOverride; };

        void updateLoop();
        std::string getStatusText(uint32_t port, bool useIPv4);

    protected:
        void onText(std::string_view text) override;
        void onConnect() override;
        void onDisconnect() override;

    private:
        bool insideCommand{ false };
        std::string cmdBuffer{ "" };

        bool haltSignalSent{ false }; // marks if a signal as been sent for new halts (force-halt and breakpoints)
        bool forceHalt{ false }; // forces a halt despite no breakpoints being hit
        bool singleStepActive{ false };

        bool noAckMode{ false }; // gets set if lldb prefers no acknowledgements
        bool nonStopMode{ false }; // (NOTE: Not working for now), gets set if gdb wants to switch over to async-messaging
        bool handshakeDone{ false }; // set to true after a few handshake commands, used to prevent exception-reporting until client is ready
        bool requestDisconnect{ false }; // set to true if the client decides it wants to disconnect

        bool hasActiveClient{ false };
        uint32_t messageCount{ 0 }; // message count per update loop
        int32_t currentThreadC{ -1 }; // selected thread for the next 'c' command

        uint64_t currentPC{ 0 };
        std::optional<uint64_t> pcOverride{ 0 }; // temporary override to handle edge-cases for exceptions/watchpoints

        // client-state:
        std::vector<uint64_t> breakpoints{};
        std::vector<Watchpoint> watchpointRead{};
        std::vector<Watchpoint> watchpointWrite{};

        std::string processCommand(const std::string& cmd, bool& shouldReply);
        void resetClientData();

        void reportWatchpoint(const Watchpoint& wp, uint64_t address);

        void sendPayload(const std::string& payload);
        void sendSignal(Signal code);
        void sendSignal(Signal code, const std::string& reason);

        void haltProgram();
        void resumeProgram();
    };

    extern Server server;

}