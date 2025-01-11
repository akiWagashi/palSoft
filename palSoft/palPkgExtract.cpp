#define _CRT_SECURE_NO_WARNINGS

#pragma warning(disable:6387 6031 6054)

#include "palFunc.h"

#include "palDefine.h"
#include "palUtils.h"

#include <Windows.h>
#include <vector>
#include <ShlObj.h>
#include <fstream>

bool palPackgeExtract(const std::string_view& targetArchive, const std::string_view& extractPath)
{

	SHCreateDirectoryExA(NULL, extractPath.data(), NULL);

	char* nameSectionPointer = nullptr;

	int extractCount = 0;

	std::ifstream archive(targetArchive.data(), std::ios::out | std::ios::binary);

	if (!archive.is_open())
	{
		printf("ERROR : open file failed\n");
		return false;
	}

	palArchiveHeader fileHeader;
	archive.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));

	archive.seekg(-4, std::ios::end);

	uint8_t fileFooter[4] = { 0 };
	archive.read(reinterpret_cast<char*>(fileFooter), sizeof(fileFooter));

	if (0 != memcmp(fileHeader.magicNumber, archiveHeader, sizeof(archiveHeader))
		|| fileHeader.entryCount == 0
		|| 0 != memcmp(fileFooter, archiveFooter, sizeof(archiveFooter)))
	{
		archive.close();
		printf("ERROR : packge format don't match\n");
		return false;
	}

	char fileFullPath[_MAX_PATH] = { 0 };
	strcpy(fileFullPath, extractPath.data());
	strcat(fileFullPath, "\\");
	nameSectionPointer = strrchr(fileFullPath, '\\') + 1;

	std::vector<palContentsEntry> entries;
	entries.resize(fileHeader.entryCount);
	archive.seekg(PAL_ARCHIVE_CONTENTS_OFFSET, std::ios::beg);
	archive.read(reinterpret_cast<char*>(entries.data()), sizeof(palContentsEntry) * fileHeader.entryCount);

	std::vector<uint8_t> buffer;
	for (const palContentsEntry& entry : entries)
	{
		if (entry.entrySize == 0) continue;

		strcpy(nameSectionPointer, entry.entryName);
		FILE* entryStream = fopen(fileFullPath, "wb+");

		if (!entryStream)
		{
			printf("ERROR : %s create failed\n", entry.entryName);
			continue;
		}

		buffer.resize(entry.entrySize);
		archive.seekg(entry.entryOffset, std::ios::beg);
		archive.read(reinterpret_cast<char*>(buffer.data()), entry.entrySize);

		if (buffer[0] == '$') dataBufferTransform(buffer.data() + 0x10, entry.entrySize - 0x10, true);

		fwrite(buffer.data(), entry.entrySize, 1, entryStream);
		fclose(entryStream);
		extractCount++;
	}

	archive.close();
	printf("COMPELETED : Extracted %d files \n", extractCount);

	return true;
}