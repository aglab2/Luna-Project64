#include "stdafx.h"

#include "ff.h"
#include "diskio.h"

namespace FF
{
	static FILE* Disk = NULL;

	DSTATUS disk_initialize(BYTE pdrv)
	{
		if (Disk)
			return 0;

		CPath isoPath(g_Settings->LoadStringVal(Cmd_AppdataDirectory).c_str(), "AUTO0.iso");
		const char* isoStr = isoPath;

		Disk = fopen(isoStr, "r+b");
		return Disk ? 0 : 1;
	}

	static void disk_uninitialize()
	{
		if (Disk) {
			fclose(Disk);
			Disk = nullptr;
		}
	}

	DSTATUS disk_status(BYTE pdrv)
	{
		return 0;
	}

	DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
	{
		uint64_t offset = sector * 512;
		fseek(Disk, offset, SEEK_SET);
		size_t bytesRead = fread(buff, 1, count * 512, Disk);
		return bytesRead == count * 512 ? RES_OK : RES_ERROR;
	}

	DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
	{
		uint64_t offset = sector * 512;
		fseek(Disk, offset, SEEK_SET);
		size_t bytesWritten = fwrite(buff, 1, count * 512, Disk);
		return bytesWritten == count * 512 ? RES_OK : RES_ERROR;
	}

	DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
	{
		switch (cmd)
		{
		case CTRL_SYNC:
		{
			fflush(Disk);
			return RES_OK;
		}
		case CTRL_EJECT:
			disk_uninitialize();
			return RES_OK;
		}

		return RES_PARERR;
	}
}
