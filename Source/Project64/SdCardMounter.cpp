// Courtesy of Parallel Launcher project, by falcobuster

#include "stdafx.h"

#include "SdCardMounter.h"

#include <Windows.h>
#include <bcrypt.h>
#include <shellapi.h>

#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include <Project64-core/N64System/SummerCart.h>

#define uint64 uint64_t
#define int64 int64_t
#define ubyte uint8_t
#define uint unsigned int

namespace fs = std::filesystem;

namespace Time {

	inline int64 now() {
		return (int64)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	template<typename Clock, typename Units> int64 toUnixTime(const std::chrono::time_point<Clock, Units>& timePoint) {
		if constexpr (std::is_same_v<Clock, std::chrono::system_clock>) {
			return (int64)std::chrono::duration_cast<std::chrono::seconds>(timePoint.time_since_epoch()).count();
		}
		else {
			return Time::now() + (int64)std::chrono::duration_cast<std::chrono::seconds>(timePoint - std::chrono::time_point<Clock, Units>::clock::now()).count();
		}
	}
}

namespace RNG
{
	void getBytes(void* buffer, size_t count) {
		BCryptGenRandom(nullptr, reinterpret_cast<unsigned char*>(buffer), (ULONG)count, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	}
}

class alignas(uint64_t) Uuid final {

private:
	ubyte m_data[16];

public:
	Uuid() noexcept
	{
		std::memset(m_data, 0, 16);
	}
	Uuid(const Uuid& other) noexcept
	{
		std::memcpy(m_data, other.m_data, 16);
	}
	Uuid(const ubyte* data) noexcept
	{
		std::memcpy(m_data, data, 16);
	}

	Uuid& operator=(const Uuid& other) noexcept;

	inline const ubyte* data() const noexcept {
		return m_data;
	}

	std::string toString() const noexcept;

	static bool tryParse(const std::string& str, Uuid& out) noexcept;
	inline static Uuid parse(const std::string& str) {
		Uuid uuid;
		if (!tryParse(str, uuid)) {
			throw std::runtime_error(str);
		}
		return uuid;
	}

	static int compare(const Uuid& lhs, const Uuid& rhs) noexcept;
	inline static const Uuid& empty() {
		static const Uuid s_zero;
		return s_zero;
	}

	static Uuid Random();

};


Uuid Uuid::Random() {
	Uuid rand;
	RNG::getBytes(rand.m_data, 16);
	rand.m_data[6] = (rand.m_data[6] & 0x0F) | 0x40;
	rand.m_data[8] = (rand.m_data[8] & 0x3F) | 0x80;
	return rand;
}

enum class SdCardFormat : int {
	FAT12 = 0,
	FAT16 = 1,
	FAT32 = 2,
	None = 3
};

struct SdCardInfo {
	SdCardFormat format;
	unsigned int numBlocks;
};

class MountedSdCard {

private:
	const std::string m_path;
	const std::string m_device;
#ifdef _WIN32
	SdCardInfo m_info;
#endif
#ifdef __linux__
	const int m_fd;
#endif
	bool m_mounted;

public:
	MountedSdCard(
		const std::string& mountDirectory,
		const std::string& devicePath
#ifdef __linux__
		, int fd
#endif
#ifdef _WIN32
		, SdCardInfo&& info
#endif
	) :
		m_path(mountDirectory),
		m_device(devicePath),
#ifdef __linux__
		m_fd(fd),
#endif
#ifdef _WIN32
		m_info(info),
#endif
		m_mounted(true)
	{}
	~MountedSdCard();

	MountedSdCard(const MountedSdCard& other) = delete;
	MountedSdCard(MountedSdCard&& other) = delete;

#ifdef _WIN32
	const SdCardInfo& info() const { return m_info; }
#endif

	inline const std::string& path() const { return m_path; }
	bool unmount();

};


static std::string getErrorMessage(DWORD errorCode) {
	if (errorCode == 0) return "Unknown Error";

	const std::string prefix = std::to_string(errorCode) + " - ";
	char* buffer = nullptr;
	const size_t messageSize = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errorCode,
		LANG_USER_DEFAULT,
		(char*)&buffer,
		0,
		nullptr
	);

	if (messageSize == 0) return prefix + "Unknown Error";
	std::string message(buffer, messageSize);

	if (buffer != nullptr) LocalFree(buffer);
	return prefix + message;
}

static inline std::string getLastErrorMessage() {
	return getErrorMessage(GetLastError());
}

static void logWarn(const std::string& message) {
	MessageBoxA(nullptr, message.c_str(), "Warning", MB_ICONWARNING);
	throw std::runtime_error(message);
}

static void logError(const std::string& message) {
	MessageBoxA(nullptr, message.c_str(), "Error", MB_ICONERROR);
	throw std::runtime_error(message);
}

static bool removeVhdFooter(const fs::path& filePath, const SdCardInfo& info, bool rename) {
	const uintmax_t originalSize = (uintmax_t)info.numBlocks * 1024;

	HANDLE fileHandle = CreateFileW(
		filePath.wstring().c_str(),
		GENERIC_WRITE,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (!fileHandle) {
		logWarn("Failed to open SD Card image");
		return false;
	}

	LARGE_INTEGER longFileSize;
	longFileSize.QuadPart = (uint64)originalSize;

	if (!SetFilePointerEx(fileHandle, longFileSize, nullptr, FILE_BEGIN)) {
		logWarn(std::string{ "SD Card I/O Error: " } + getLastErrorMessage());
		return false;
	}

	if (!SetEndOfFile(fileHandle)) {
		CloseHandle(fileHandle);
		logWarn("Failed to remove VHD footer from SD Card image. SD Card image may now be corrupt.");
		return false;
	}

	CloseHandle(fileHandle);

	if (rename) {
		const fs::path newPath = filePath.parent_path() / (filePath.stem().wstring() + L".iso");

		std::error_code err;
		fs::remove(newPath, err);

		err.clear();
		fs::rename(filePath, newPath, err);

		if (err) {
			logError(std::string{ "File operation error: " } + err.message());
			return false;
		}
	}

	return true;
}

struct WritableFifoPipe
{
	void cancel() {}
};
struct ReadableFifoPipe
{
	void cancel() {}
};

struct UnmountProcessInfo {
	SHELLEXECUTEINFOW shell;
	WritableFifoPipe procIn;
	ReadableFifoPipe procOut;
	std::wstring args;
	std::thread thread;
};

static void onHelperExit(UnmountProcessInfo* proc) {
	if (!proc->shell.hProcess) return;
	WaitForSingleObject(proc->shell.hProcess, INFINITE);
	proc->procIn.cancel();
	proc->procOut.cancel();
}

bool MountedSdCard::unmount() {
	if (!m_mounted) return true;

	if (removeVhdFooter(m_device, m_info, true)) {
		m_mounted = false;
		return true;
	}

	return false;
}

MountedSdCard::~MountedSdCard() {
	if (m_mounted) {
		removeVhdFooter(m_device, m_info, true);
	}
}

static inline void writeLong(std::fstream& file, uintmax_t num) {
	file.put((char)(ubyte)(num >> 56));
	file.put((char)(ubyte)((num >> 48) & 0xFF));
	file.put((char)(ubyte)((num >> 40) & 0xFF));
	file.put((char)(ubyte)((num >> 32) & 0xFF));
	file.put((char)(ubyte)((num >> 24) & 0xFF));
	file.put((char)(ubyte)((num >> 16) & 0xFF));
	file.put((char)(ubyte)((num >> 8) & 0xFF));
	file.put((char)(ubyte)(num & 0xFF));
}

static inline SdCardFormat getFilesystemFormat(std::fstream& image) {
	char typeNameBuffer[5];
	image.seekg(0x36, std::ios_base::beg);
	image.read(typeNameBuffer, 5);

	if (!image.good()) return SdCardFormat::None;
	if (std::memcmp(typeNameBuffer, "FAT12", 5) == 0) return SdCardFormat::FAT12;
	if (std::memcmp(typeNameBuffer, "FAT16", 5) == 0) return SdCardFormat::FAT16;

	image.seekg(0x52, std::ios_base::beg);
	image.read(typeNameBuffer, 5);

	if (image.good() && std::memcmp(typeNameBuffer, "FAT32", 5) == 0) return SdCardFormat::FAT32;
	return SdCardFormat::None;
}

static bool writeVhdFooter(fs::path& filePath, SdCardInfo& info) {
	std::fstream image(filePath, std::ios::binary | std::ios::in | std::ios::out | std::ios::ate);
	const auto footerStart = image.tellp();

	ubyte numHeads;
	ubyte sectorsPerTrack;

	image.seekg(0x18, std::ios_base::beg);
	image.read((char*)&sectorsPerTrack, 1);
	image.seekg(0x1A, std::ios_base::beg);
	image.read((char*)&numHeads, 1);

	if (!image.good()) {
		logWarn("SD Card I/O Error");
		return false;
	}

	std::error_code err;
	const uintmax_t fileSize = fs::file_size(filePath, err);
	if (err) {
		logWarn("Failed to get SD card image file size.");
		return false;
	}

	if (numHeads == 0 || sectorsPerTrack == 0) {
		const uintmax_t numSectors = fileSize / 512;

		if (numSectors <= 512 * 1024) {
			sectorsPerTrack = (numSectors <= 4096) ? 16 : 32;

			if (numSectors <= 32 * 1024) {
				numHeads = 2;
			}
			else if (numSectors <= 64 * 1024) {
				numHeads = 4;
			}
			else if (numSectors <= 256 * 1024) {
				numHeads = 8;
			}
			else {
				numHeads = 16;
			}
		}
		else {
			sectorsPerTrack = 63;

			if (numSectors <= 16 * 63 * 1024) {
				numHeads = 16;
			}
			else if (numSectors <= 32 * 16 * 1024) {
				numHeads = 32;
			}
			else if (numSectors <= 64 * 16 * 1024) {
				numHeads = 64;
			}
			else if (numSectors <= 128 * 16 * 1024) {
				numHeads = 128;
			}
			else {
				numHeads = 255;
			}
		}
	}

	const uintmax_t numCylinders = 1 + ((fileSize - 1) / (uintmax_t)512 / (uintmax_t)numHeads / (uintmax_t)sectorsPerTrack);
	if (numCylinders > (uintmax_t)0xFFFF) {
		logWarn("Sorry, but Windows cannot mount this type of disk if it is larger than 2032 GiB.");
		return false;
	}

	info.format = getFilesystemFormat(image);
	info.numBlocks = (uint)(fileSize / 1024);

	image.seekg(0x1BE, std::ios_base::beg);
	if (image.get() != (char)0x80) {
		// Write the partition table to the fake MBR because Windows needs it
		const uint numSectors = (uint)(fileSize / 512);

		if (info.format == SdCardFormat::None) {
			logWarn("SD card does not appear to have an FAT filesystem");
			return false;
		}

		Uuid randomUuid = Uuid::Random();
		image.seekp(0x1B8, std::ios_base::beg);
		image.write((const char*)randomUuid.data(), 4);

		image.seekp(0x1BE, std::ios_base::beg);
		image.put((char)0x80);
		image.put(0x00);
		image.put(0x01);
		image.put(0x00);

		switch (info.format) {
		case SdCardFormat::FAT12: image.put(0x01); break;
		case SdCardFormat::FAT32: image.put(0x0C); break;
		default: image.put((numSectors < (uint)numHeads * (uint)sectorsPerTrack * 1024) ? 0x06 : 0x0E); break;
		}
		if (numSectors >= (uint)sectorsPerTrack * (uint)numHeads * 1024) {
			image.put((char)0xFE);
			image.put((char)0xFF);
			image.put((char)0xFF);
		}
		else {
			image.put((char)((ubyte)(numSectors / sectorsPerTrack) % numHeads));
			image.put((char)(ubyte)(((1 + (uint)numSectors % (uint)sectorsPerTrack) & 0x3F) | (((numSectors / ((uint)numHeads * (uint)sectorsPerTrack)) >> 8) << 6)));
			image.put((char)(ubyte)((numSectors / ((uint)numHeads * (uint)sectorsPerTrack)) & 0xFF));
		}

		const uint zero = 0;
		image.write((const char*)&zero, 4);

		image.put((char)(ubyte)(numSectors & 0xFF));
		image.put((char)(ubyte)((numSectors >> 8) & 0xFF));
		image.put((char)(ubyte)((numSectors >> 16) & 0xFF));
		image.put((char)(ubyte)(numSectors >> 24));
	}

	image.seekp(footerStart);
	image.write("conectix", 8);

	const ubyte data1[] = {
		0x00, 0x00, 0x00, 0x02,
		0x00, 0x01, 0x00, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	image.write((const char*)data1, 16);

	const auto lastModified = fs::last_write_time(filePath, err);
	const int64 time = (err ? Time::now() : Time::toUnixTime(lastModified)) - 946684800ull;

	image.put((char)(ubyte)((time >> 24) & 0xFFull));
	image.put((char)(ubyte)((time >> 16) & 0xFFull));
	image.put((char)(ubyte)((time >> 8) & 0xFFull));
	image.put((char)(ubyte)(time & 0xFFull));

	image.write("pl  ", 4);

	image.put((char)(ubyte)(6 >> 8));
	image.put((char)(ubyte)(6 & 0xFF));
	image.put((char)(ubyte)(22 >> 8));
	image.put((char)(ubyte)(22 & 0xFF));

	image.write("Wi2k", 4);

	writeLong(image, fileSize);
	writeLong(image, fileSize);

	image.put((char)(ubyte)(numCylinders >> 8));
	image.put((char)(ubyte)(numCylinders & 0xff));
	image.put((char)numHeads);
	image.put((char)sectorsPerTrack);

	image.write((const char*)data1, 4);

	const auto checksumStart = image.tellp();
	image.put(0); image.put(0); image.put(0); image.put(0);

	const Uuid uuid = Uuid::Random();
	image.write((const char*)uuid.data(), 16);

	for (int i = 0; i < 428; i++) image.put(0);

	image.flush();

	image.seekg(footerStart);
	uint checksum = 0;
	for (int i = 0; i < 84; i++) {
		checksum += (uint)image.get();
	}
	checksum ^= 0xFFFFFFFFu;

	image.seekp(checksumStart);
	image.put((char)(ubyte)(checksum >> 24));
	image.put((char)(ubyte)((checksum >> 16) & 0xFFu));
	image.put((char)(ubyte)((checksum >> 8) & 0xFFu));
	image.put((char)(ubyte)(checksum & 0xFFu));

	image.flush();

	if (image.bad()) {
		logWarn("SD Card I/O Error");
		removeVhdFooter(filePath, info, false);
		return false;
	}

	image.close();

	const fs::path newPath = filePath.parent_path() / (filePath.stem().wstring() + L".vhd");
	fs::remove(newPath, err);

	err.clear();
	fs::rename(filePath, newPath, err);

	if (err) {
		logWarn(std::string{ "File operation error: " } + err.message());
		removeVhdFooter(filePath, info, false);
		return false;
	}

	filePath = newPath;
	return true;
}

namespace SdCardMounter
{
	void switchStates()
	{
		fs::path isoPath = CSummerCart::SdPath();
		fs::path vhdPath = CSummerCart::VhdPath();
		std::wstring cfgPath = isoPath.wstring() + L".cfg";
		if (fs::exists(isoPath)) {
			SdCardInfo info;
			writeVhdFooter(isoPath, info);
			{
				std::error_code ec;
				fs::remove(cfgPath, ec);
				std::fstream cfgFile(cfgPath, std::ios::out | std::ios::binary);
				cfgFile.write((char*)&info, sizeof(info));
			}
		}
		else
		{
			SdCardInfo info;
			{
				std::fstream cfgFile(cfgPath, std::ios::in | std::ios::binary);
				cfgFile.read((char*)&info, sizeof(info));
			}
			removeVhdFooter(vhdPath, info, true);

			if (!fs::exists(vhdPath)) {
				std::error_code ec;
				fs::remove(cfgPath, ec);
			}
		}
	}

	State getState()
	{
		fs::path isoPath = CSummerCart::SdPath();
		fs::path vhdPath = CSummerCart::VhdPath();
		bool isoExists = fs::exists(isoPath);
		bool vhdExists = fs::exists(vhdPath);
		if (fs::exists(isoPath))
		{
			return State::ISO;
		}
		
		if (fs::exists(vhdPath))
		{
			return State::VHD;
		}

		// Something weird is happening, make a new image
		CSummerCart::MakeInitialImage();
		return State::ISO;
	}
}
