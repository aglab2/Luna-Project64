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

#include <GDBServer.h>

#include <inttypes.h>

namespace {
    constexpr bool GDB_LOG_MESSAGES = false;

    constexpr uint32_t MAX_REQUESTS_PER_UPDATE = 10;
    constexpr uint32_t MAX_PACKET_SIZE = 0x4096;
    constexpr uint32_t DEF_BREAKPOINT_SIZE = 64;
    constexpr bool NON_STOP_MODE = false; // broken for now, mainly useful for multi-thread debugging, which we can't really support

    uint8_t gdbCalcChecksum(const std::string& payload) {
        uint8_t checksum = 0;
        for (char c : payload)checksum += c;
        return checksum;
    }

    template<typename T>
    std::string hex(T value, long precision = 0, char padchar = '0') {
        std::string buffer;
        buffer.resize(sizeof(T) * 2);
        char* p = buffer.data();

        //create a mask to clear the upper four bits after shifting right in case T is a signed type
        T mask = 1;
        mask <<= sizeof(T) * 8 - 4;
        mask -= 1;

        uint32_t size = 0;
        do {
            uint32_t n = value & 15;
            p[size++] = n < 10 ? '0' + n : 'a' + n - 10;
            value = value >> 4 & mask;
        } while (value);
        buffer.resize(size);
        std::reverse(buffer.begin(), buffer.end());
        for (int i = 0; i < precision; i++)
            buffer[i] = padchar;

        return buffer;
    }

    template<typename T>
    inline auto addOrRemoveEntry(std::vector<T>& data, T value, bool shouldAdd) {
        if (shouldAdd) {
            data.emplace_back(value);
        }
        else {
            data.erase(std::remove(data.begin(), data.end(), value), data.end());
        }
    }

    std::string build(std::initializer_list<std::string_view> list)
    {
        size_t len;
        for (const auto& str : list)
        {
            len += str.size();
        }

        std::string output;
        output.reserve(len);
        for (const auto& str : list)
        {
            output += str;
        }

        return output;
    }

    std::vector<std::string_view> split(const std::string& input, char divider)
    {
        std::vector<std::string_view> result;
        auto start = input.begin();
        for (auto it = input.begin(); it != input.end(); ++it)
        {
            if (*it == divider)
            {
                result.emplace_back(&*start, it - start);
                start = it + 1;
            }
        }
        result.emplace_back(&*start, input.end() - start);

        return result;
    }

    uint64_t toBinary_(const char* s, size_t left, uint64_t sum = 0)
    {
        if (0 == left)
            return 0;

        return (
            *s == '0' || *s == '1' ? toBinary_(s + 1, left - 1, (sum << 1) | *s - '0') :
            *s == '\'' ? toBinary_(s + 1, left - 1, sum) :
            sum
            );
    }

    uint64_t toOctal_(const char* s, size_t left, uint64_t sum = 0)
    {
        if (0 == left)
            return 0;

        return (
            *s >= '0' && *s <= '7' ? toOctal_(s + 1, left - 1, (sum << 3) | *s - '0') :
            *s == '\'' ? toOctal_(s + 1, left - 1, sum) :
            sum
            );
    }

    uint64_t toDecimal_(const char* s, size_t left, uint64_t sum = 0)
    {
        if (0 == left)
            return 0;

        return (
            *s >= '0' && *s <= '9' ? toDecimal_(s + 1, left - 1, (sum * 10) + *s - '0') :
            *s == '\'' ? toDecimal_(s + 1, left - 1, sum) :
            sum
            );
    }

    uint64_t toHex_(const char* s, size_t left, uint64_t sum = 0)
    {
        return (
            !left ? sum :
            *s >= 'A' && *s <= 'F' ? toHex_(s + 1, left - 1, (sum << 4) | *s - 'A' + 10) :
            *s >= 'a' && *s <= 'f' ? toHex_(s + 1, left - 1, (sum << 4) | *s - 'a' + 10) :
            *s >= '0' && *s <= '9' ? toHex_(s + 1, left - 1, (sum << 4) | *s - '0') :
            *s == '\'' ? toHex_(s + 1, left - 1, sum) :
            sum
            );
    }

    uint64_t hex(const std::string_view& sv)
    {
        size_t left = sv.length();
        if (0 == left)
            return 0;

        const char* s = sv.data();
        return (
            *s == '0' && left > 1 && (*(s + 1) == 'X' || *(s + 1) == 'x') ? toHex_(s + 2, left - 2) :
            *s == '$' ? toHex_(s + 1, left - 1) : toHex_(s, left)
            );
    }

    uint64_t toNatural(const char* s, size_t left)
    {
        if (0 == left)
            return 0;

        return (
            *s == '0' && left > 1 && (*(s + 1) == 'B' || *(s + 1) == 'b') ? toBinary_(s + 2, left - 2) :
            *s == '0' && left > 1 && (*(s + 1) == 'O' || *(s + 1) == 'o') ? toOctal_(s + 2, left - 2) :
            *s == '0' && left > 1 && (*(s + 1) == 'X' || *(s + 1) == 'x') ? toHex_(s + 2, left - 2) :
            *s == '%' ? toBinary_(s + 1, left - 1) : *s == '$' ? toHex_(s + 1, left - 1) : toDecimal_(s, left)
            );
    }

    int64_t integer(const std::string_view& sv) {
        size_t left = sv.length();
        if (0 == left)
            return 0;

        const char* s = sv.data();
        return (
            *s == '+' ? +toNatural(s + 1, left - 1) : *s == '-' ? -toNatural(s + 1, left - 1) : toNatural(s, left)
            );
    }
}

namespace GDB {
    Server server{};

    bool Server::reportSignal(Signal sig, uint64_t originPC) {
        if (!hasActiveClient || !handshakeDone)return true; // no client -> no error
        if (forceHalt)return false; // Signals can only happen while the game is running, ignore others

        pcOverride = originPC;

        forceHalt = true;
        haltSignalSent = true;
        sendSignal(sig);

        return true;
    }

    void Server::reportWatchpoint(const Watchpoint& wp, uint64_t address) {
        auto orgAddress = wp.addressStartOrg + (address - wp.addressStart);
        forceHalt = true;
        haltSignalSent = true;
        sendSignal(Signal::TRAP, build({ wp.getTypePrefix(), hex(orgAddress), ";" }));
    }

    void Server::reportMemRead(uint64_t address, uint32_t size) {
        if (watchpointRead.empty())return;

        if (hooks.normalizeAddress) {
            address = hooks.normalizeAddress(address);
        }

        uint64_t addressEnd = address + size - 1;
        for (const auto& wp : watchpointRead) {
            if (wp.hasOverlap(address, addressEnd)) {
                return reportWatchpoint(wp, address);
            }
        }
    }

    void Server::reportMemWrite(uint64_t address, uint32_t size) {
        if (watchpointWrite.empty())return;

        if (hooks.normalizeAddress) {
            address = hooks.normalizeAddress(address);
        }

        uint64_t addressEnd = address + size - 1;
        for (const auto& wp : watchpointWrite) {
            if (wp.hasOverlap(address, addressEnd)) {
                return reportWatchpoint(wp, address);
            }
        }
    }

    bool Server::reportPC(uint64_t pc) {
        if (!hasActiveClient)return true;

        currentPC = pc;
        bool needHalts = forceHalt || breakpoints.end() != std::find(breakpoints.begin(), breakpoints.end(), pc);

        if (needHalts) {
            forceHalt = true; // breakpoints may get deleted after a signal, but we have to stay stopped

            if (!haltSignalSent) {
                haltSignalSent = true;
                sendSignal(Signal::TRAP);
            }
        }

        if (singleStepActive) {
            singleStepActive = false;
            forceHalt = true;
        }

        return !needHalts;
    }

    /**
     * NOTE: please read the comment in the header server.hpp file before making any changes here!
     */
    std::string Server::processCommand(const std::string& cmd, bool& shouldReply)
    {
        auto cmdParts = split(cmd, ':');
        auto cmdName = cmdParts[0];
        char cmdPrefix = cmdName.size() > 0 ? cmdName[0] : ' ';

        switch (cmdPrefix)
        {
        case '!': return "OK"; // informs us that "extended remote-debugging" is used

        case '?': // handshake: why did we halt?
            haltProgram();
            haltSignalSent = true;
            return "T05"; // needs to be faked, otherwise the GDB-client hangs up and eats 100% CPU

        case 'c': // continue
        case 'C': // continue (with signal, signal itself can be ignored)
            // normal stop-mode is only allowed to respond once a signal was raised, non-stop must return OK immediately
            handshakeDone = true; // good indicator that GDB is done, also enables exception sending
            shouldReply = NON_STOP_MODE;
            resumeProgram();
            return "OK";

        case 'D': // client wants to detach (Note: VScode doesn't seem to use this, uses vKill instead)
            requestDisconnect = true;
            return "OK";
            break;

        case 'g': // dump all general registers
            if (hooks.regReadGeneral) {
                return hooks.regReadGeneral();
            }
            else {
                return "0000000000000000000000000000000000000000";
            }
            break;

        case 'G': // set all general registers
            if (hooks.regWriteGeneral) {
                hooks.regWriteGeneral(cmd.substr(1));
                return "OK";
            }
            break;

        case 'H': // set which thread a 'c' command that may follow belongs to (can be ignored in stop-mode)
            if (cmdName == "Hc0")currentThreadC = 0;
            if (cmdName == "Hc-1")currentThreadC = -1;
            return "OK";

        case 'k':  // old version of vKill
            if (handshakeDone) { // sometimes this gets send during handshake (to reset the program?) -> ignore
                requestDisconnect = true;
            }
            return "OK";
            break;

        case 'm': // read memory (e.g.: "m80005A00,4")
        {
            if (!hooks.read) {
                return "";
            }

            auto sepIdxMaybe = cmdName.find(",");
            uint32_t sepIdx = std::string::npos != sepIdxMaybe ? sepIdxMaybe : 1;

            uint64_t address = hex(cmdName.substr(1, sepIdx - 1));
            uint64_t count = hex(cmdName.substr(sepIdx + 1, cmdName.size() - sepIdx));
            return hooks.read(address, count);
        }
        break;

        case 'M': // write memory (e.g.: "M801ef90a,4:01000000")
        {
            if (!hooks.write) {
                return "";
            }

            auto sepIdxMaybe = cmdName.find(",");
            uint32_t sepIdx = std::string::npos != sepIdxMaybe ? sepIdxMaybe : 1;

            uint64_t address = hex(cmdName.substr(1, sepIdx - 1));
            uint64_t byteSize = hex(cmdName.substr(sepIdx + 1, 1));
            std::string_view hexvalue = cmdParts.size() > 1 ? cmdParts[1] : std::string_view{};
            std::vector<uint8_t> value;
            value.reserve(hexvalue.length() / 2);
            for (int i = 0; i < hexvalue.size(); i += 2) {
                auto hexbyte = hexvalue.substr(i, 2);
                value.push_back((uint8_t)hex(hexbyte));
            }
            hooks.write(address, value);
            return "OK";
        }

        break;

        case 'p': // read specific register (e.g.: "p15")
            if (hooks.regRead) {
                uint32_t regIdx = integer(cmdName.substr(1));
                return hooks.regRead(regIdx);
            }
            else {
                return "00000000";
            }
            break;

        case 'P': // write specific register (e.g.: "P15=FFFFFFFF80001234")
            if (hooks.regWrite) {
                auto sepIdxMaybe = cmdName.find("=");
                uint32_t sepIdx = std::string::npos != sepIdxMaybe ? sepIdxMaybe : 1;

                uint32_t regIdx = static_cast<uint32_t>(hex(cmdName.substr(1, sepIdx - 1)));
                uint64_t regValue = hex(cmdName.substr(sepIdx + 1));

                return hooks.regWrite(regIdx, regValue) ? "OK" : "E00";
            }
            break;

        case 'q':
            // This tells the client what we can and can't do
            if (cmdName == "qSupported") {
                return build({
"PacketSize=", hex(MAX_PACKET_SIZE),
";fork-events-;swbreak+;hwbreak-",
";vContSupported-", // prevent vCont commands (reduces potential GDB variations: some prefer using it, others don't)
NON_STOP_MODE ? ";QNonStop+" : "",
"QStartNoAckMode+",
hooks.targetXML ? ";xmlRegisters+;qXfer:features:read+" : "" // (see: https://marc.info/?l=gdb&m=149901965961257&w=2)
                });
            }

            // handshake-command, most return dummy values to convince gdb to connect
            if (cmdName == "qTStatus")return forceHalt ? "T1" : "";
            if (cmdName == "qAttached")return "1"; // we are always attached, since a game is running
            if (cmdName == "qOffsets")return "Text=0;Data=0;Bss=0";

            if (cmdName == "qSymbol")return "OK"; // client offers us symbol-names -> we don't care

            // client asks us about existing breakpoints (may happen after a re-connect) -> ignore since we clear them on connect
            if (cmdName == "qTfP")return "";
            if (cmdName == "qTsP")return "";

            // extended target features (gdb extension), most return XML data
            if (cmdName == "qXfer" && cmdParts.size() > 4)
            {
                if (cmdParts[1] == "features" && cmdParts[2] == "read") {
                    // informs the client about arch/registers (https://sourceware.org/gdb/onlinedocs/gdb/Target-Description-Format.html#Target-Description-Format)
                    if (cmdParts[3] == "target.xml") {
                        return hooks.targetXML ? build({ "l", hooks.targetXML() }) : std::string{};
                    }
                }
            }

            // Thread-related queries
            if (cmdName == "qfThreadInfo")return { "m1" };
            if (cmdName == "qsThreadInfo")return { "l" };
            if (cmdName == "qThreadExtraInfo,1")return ""; // ignoring this command fixes support for CLion (and VSCode?), otherwise gdb hangs
            if (cmdName == "qC")return { "QC1" };
            // there will also be a "qP0000001f0000000000000001" command depending on the IDE, this is ignored to prevent GDB from hanging up
            break;

        case 'Q':
            if (cmdName == "QNonStop") { // 0=stop, 1=non-stop-mode (this allows for async GDB-communication)
                if (cmdParts.size() <= 1)return "E00";
                nonStopMode = cmdParts[1] == "1";

                if (nonStopMode) {
                    haltProgram();
                }
                else {
                    resumeProgram();
                }
                return "OK";
            }

            if (cmdName == "QStartNoAckMode") {
                if (noAckMode) {
                    return "OK";
                }
                // The final OK has to be sent in ack mode.
                sendPayload("OK");
                shouldReply = false;
                noAckMode = true;
                return "";
            }
            break;

        case 's': {
            if (cmdName.size() > 1) {
                uint64_t address = integer(cmdName.substr(1));
                printf("stepping at address unsupported, ignore (%016" PRIX64 ")\n", address);
            }

            shouldReply = false;
            singleStepActive = true;
            resumeProgram();
            return "";
        } break;

        case 'v': {
            // normalize (e.g. "vAttach;1" -> "vAttach")
            auto vName = cmdName.substr(0, cmdName.find(";"));

            if (vName == "vMustReplyEmpty")return ""; // handshake-command / keep-alive (must return the same as an unknown command would)
            if (vName == "vAttach")return NON_STOP_MODE ? "OK" : "S05"; // attaches to the process, we must return a fake trap-exception to make gdb happy
            if (vName == "vCont?")return ""; // even though "vContSupported-" is set, gdb may still ask for it -> ignore to force e.g. `s` instead of `vCont;s:1;c`
            if (vName == "vStopped")return "";
            if (vName == "vCtrlC") {
                haltProgram();
                return "OK";
            }

            if (vName == "vKill") {
                if (handshakeDone) { // sometimes this gets send during handshake (to reset the program?) -> ignore
                    requestDisconnect = true;
                }
                return "OK";
            }

            if (vName == "vCont") return "E00"; // if GDB completely ignores both "vCont is unsupported" responses, throw an error here

        } break;

        case 'Z': // insert breakpoint (e.g. "Z0,801a0ef4,4")
        case 'z': // remove breakpoint (e.g. "z0,801a0ef4,4")
        {
            bool isInsert = cmdPrefix == 'Z';
            bool isHardware = cmdName[1] == '1'; // 0=software, 1=hardware
            auto sepIdxMaybe = cmdName.find(',', 3);
            uint32_t sepIdx = std::string::npos != sepIdxMaybe ? (sepIdxMaybe + 3) : 0;

            uint64_t address = hex(cmdName.substr(3, sepIdx - 1));
            uint64_t addressStart = address;
            uint64_t addressEnd = address + hex(cmdName.substr(sepIdx + 1)) - 1;

            if (hooks.normalizeAddress) {
                addressStart = hooks.normalizeAddress(addressStart);
                addressEnd = hooks.normalizeAddress(addressEnd);
            }
            Watchpoint wp{ addressStart, addressEnd, address };

            switch (cmdName[1]) {
            case '0': // (hardware/software breakpoints are the same for us)
            case '1': addOrRemoveEntry(breakpoints, address, isInsert); break;

            case '2':
                wp.type = WatchpointType::WRITE;
                addOrRemoveEntry(watchpointWrite, wp, isInsert);
                break;

            case '3':
                wp.type = WatchpointType::READ;
                addOrRemoveEntry(watchpointRead, wp, isInsert);
                break;

            case '4':
                wp.type = WatchpointType::ACCESS;
                addOrRemoveEntry(watchpointRead, wp, isInsert);
                addOrRemoveEntry(watchpointWrite, wp, isInsert);
                break;
            default: return "E00";
            }

            if (hooks.emuCacheInvalidate) { // for re-compiler, otherwise breaks might be skipped
                hooks.emuCacheInvalidate(address);
            }
            return "OK";
        }
        }

        printf("Unknown-Command: %s (data: %s)\n", cmdName.data(), cmdBuffer.data());
        return "";
    }

    void Server::onText(std::string_view text) {

        if (cmdBuffer.size() == 0) {
            cmdBuffer.reserve(text.size());
        }

        for (char c : text)
        {
            switch (c)
            {
            case '$':
                insideCommand = true;
                break;

            case '#': { // end of message + 2-char checksum after that
                insideCommand = false;

                ++messageCount;
                bool shouldReply = true;
                auto cmdRes = processCommand(cmdBuffer, shouldReply);
                if (shouldReply) {
                    sendPayload(cmdRes);
                }
                else if (!noAckMode) {
                    sendText("+");
                }

                cmdBuffer = "";
            } break;

            case '+': break; // "OK" response -> ignore

            case '\x03': // CTRL+C (same as "vCtrlC" packet) -> force halt
                if constexpr (GDB_LOG_MESSAGES) {
                    printf("GDB <: CTRL+C [0x03]\n");
                }
                haltProgram();
                break;

            default:
                if (insideCommand) {
                    cmdBuffer.push_back(c);
                }
            }
        }
    }

    void Server::updateLoop() {
        if (!isStarted())return;

        if (requestDisconnect) {
            requestDisconnect = false;
            if (!noAckMode) {
                sendText("+");
            }
            disconnectClient();
            resumeProgram();
            return;
        }

        // The following code manages the message processing which gets exchanged from the server thread.
        // It was carefully build to balance latency, throughput and CPU usage to let the game still run at full speed
        // while allowing for fast processing once the debugger is halted.

        uint32_t loopFrames = isHalted() ? 20 : 1; // "frames" to check (loops with sleep in-between)
        uint32_t loopCount = isHalted() ? 500 : 100; // loops inside a frame, the more the less latency, but CPU usage goes up
        uint32_t maxLoopResets = 10000; // how many times can a new message reset the counter (prevents infinite loops with misbehaving clients)
        bool wasHalted = isHalted();

        for (uint32_t frame = 0; frame < loopFrames; ++frame) {
            for (uint32_t i = 0; i < loopCount; ++i) {
                messageCount = 0;
                update();

                // if the last message resumed the program, abort (no more messages will be send until the next stop)
                if (wasHalted && !isHalted())return;

                if (messageCount > 0 && maxLoopResets > 0) {
                    i = loopCount; // reset loop here to keep a fast chain of messages going (reduces latency)
                    --maxLoopResets;
                }
            }

            if (wasHalted) _Thrd_yield();
        }
    }

    std::string Server::getStatusText(uint32_t port, bool useIPv4) {
        auto url = getURL(port, useIPv4);
        std::string prefix = isHalted() ? "+" : ">";

        if (hasClient())return build({ prefix, " GDB connected ", url });
        if (isStarted())return build({ "GDB listening ", url });
        return build({ "GDB pending (", url, ")" });
    }

    void Server::sendSignal(Signal code) {
        sendPayload(build({ "S", hex(static_cast<uint8_t>(code), 2) }));
    }

    void Server::sendSignal(Signal code, const std::string& reason) {
        sendPayload(build({ "T", hex(static_cast<uint8_t>(code), 2), reason }));
    }

    void Server::sendPayload(const std::string& payload) {
        std::string msg = build({ noAckMode ? "$" : "+$", payload, "#", hex(gdbCalcChecksum(payload), 2, '0') });
        if constexpr (GDB_LOG_MESSAGES) {
            printf("GDB >: %.*s\n", msg.size() > 100 ? 100 : msg.size(), msg.data());
        }
        sendText(msg);
    }

    void Server::haltProgram() {
        forceHalt = true;
        haltSignalSent = false;
    }

    void Server::resumeProgram() {
        pcOverride.reset();
        forceHalt = false;
        haltSignalSent = false;
    }

    void Server::onConnect() {
        printf("GDB client connected\n");
        resetClientData();
        hasActiveClient = true;
    }

    void Server::onDisconnect() {
        if (hasActiveClient)
            printf("GDB client disconnected\n");
        hadHandshake = false;
        resetClientData();
    }

    void Server::reset() {
        hooks.read = {};
        hooks.write = {};
        hooks.normalizeAddress = {};
        hooks.regReadGeneral = {};
        hooks.regWriteGeneral = {};
        hooks.regRead = {};
        hooks.regWrite = {};
        hooks.emuCacheInvalidate = {};
        hooks.targetXML = {};

        resetClientData();
    }

    void Server::resetClientData() {
        breakpoints.clear();
        breakpoints.reserve(DEF_BREAKPOINT_SIZE);

        watchpointRead.clear();
        watchpointRead.reserve(DEF_BREAKPOINT_SIZE);

        watchpointWrite.clear();
        watchpointWrite.reserve(DEF_BREAKPOINT_SIZE);

        pcOverride.reset();
        insideCommand = false;
        cmdBuffer = "";
        haltSignalSent = false;
        forceHalt = false;
        singleStepActive = false;
        nonStopMode = false;
        noAckMode = false;

        currentThreadC = -1;
        hasActiveClient = false;
        handshakeDone = false;
        requestDisconnect = false;
    }

};