// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#include "SVZ.hpp"

#include "miniz.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace
{
	struct uint32le
	{
		std::array<uint8_t, 4> bytes;

		constexpr uint32le(const uint32_t value = 0) noexcept
		{
			bytes[0] = static_cast<uint8_t>(value);
			bytes[1] = static_cast<uint8_t>(value >> 8);
			bytes[2] = static_cast<uint8_t>(value >> 16);
			bytes[3] = static_cast<uint8_t>(value >> 24);
		}

		constexpr operator uint32_t() const noexcept
		{
			return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
		}
	};

	struct SVZHeaderPlugin
	{
		std::array<char, 4> SVZa = { 'S', 'V', 'Z', 'a' };
		uint16le unknown1 = 1;
		std::array<char, 10> RC001_1 = { 'R', 'C', '0', '0', '1', 1, 0, 0, 0, 0};
		std::array<char, 4> EXTa = { 'E', 'X', 'T', 'a' };
		std::array<char, 4> ZCOR = { 'Z', 'C', 'O', 'R' };
		uint32le unknown2 = 0x20;
		uint32le compressedSize1;
		std::array<uint32le, 6> unknown3 = { 1, 0, 32, 0, 1, 32};
		uint32le compressedSize2;
		uint32le compressedCRC32;
		std::array<char, 8> RC001_2 = { 'R', 'C', '0', '0', '1', 1, 0, 0 };
		uint32le uncompressedSize;
		std::array<uint32le, 5> unknown4 = { 0, 0, 0, 0, 0 };

		bool IsValid() const noexcept
		{
			const SVZHeaderPlugin expected{};
			return SVZa == expected.SVZa && unknown1 == expected.unknown1
				&& RC001_1 == expected.RC001_1 && EXTa == expected.EXTa
				&& ZCOR == expected.ZCOR && unknown2 == expected.unknown2
				&& unknown3 == expected.unknown3 && RC001_2 == expected.RC001_2
				&& unknown4 == expected.unknown4;
		}
	};

	struct SVZHeaderHardware
	{
		std::array<char, 4> SVZa = { 'S', 'V', 'Z', 'a' };
		uint16le unknown1 = 0x0202;
		std::array<char, 10> RC001_1 = { 'R', 'C', '0', '0', '1', 1, 0, 0, 0, 0 };
		std::array<char, 4> DIFa = { 'D', 'I', 'F', 'a' };
		std::array<char, 4> ZCOR_1 = { 'Z', 'C', 'O', 'R' };
		uint32le unknown2 = 0x30;
		uint32le unknown3 = 0x34;
		std::array<char, 4> MDLa = { 'M', 'D', 'L', 'a' };
		std::array<char, 4> ZCOR_2 = { 'Z', 'C', 'O', 'R' };
		uint32le unknown4 = 0x64;
		uint32le bankSize = 0;  // Starting from number of patches
		std::array<uint8_t, 52> unknown5 = { 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x09, 0x5C, 0xA1, 0x03, 0x00, 0x86, 0xC8, 0xE5, 0x4C, 0xA5, 0x48, 0x08, 0x0C, 0x00, 0x48, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		uint32le numPatches = 0;
		uint32le unknown6 = 0x800;
		uint32le bankSizeTruncated;  // Only the lower 9 bits?!
		uint32le unknown7 = 0;

		bool IsValid() const noexcept
		{
			const SVZHeaderHardware expected{};
			return SVZa == expected.SVZa && unknown1 == 0x0202
				&& RC001_1 == expected.RC001_1 && DIFa == expected.DIFa
				&& ZCOR_1 == expected.ZCOR_1 && unknown2 == 0x30
				&& unknown3 == 0x34 && MDLa == expected.MDLa
				&& ZCOR_2 == expected.ZCOR_2 && unknown4 == expected.unknown4
				&& unknown5 == expected.unknown5 && unknown6 == expected.unknown6
				&& bankSizeTruncated == (bankSize & 0x1FF) && unknown7 == expected.unknown7;
		}
	};

	struct SVDHeader
	{
		uint16le headerSize;
		std::array<char, 14> magic = { 'S', 'V', 'D', '5', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	};

	struct SVDHeaderEntry
	{
		std::array<char, 4> type;
		std::array<char, 4> dd07 = { 'D', 'D', '0', '7' };
		uint32le offset;
		uint32le size;
	};

	struct SVDPatchHeader
	{
		uint32le numPatches;
		uint32le patchSize = 2048;
		uint32le unknown1 = 16;
		uint32le unknown2 = 0;
	};

	struct SVDxHeader
	{
		std::array<char, 4> SVDx = { 'S', 'V', 'D', 'x' };
		uint32le headerSize = 32;
		uint32le patchSize = sizeof(PatchVST);
		uint32le numPatches = 64;
		std::array<uint32le, 4> unknown = { 2, 0, 0, 0 };

		bool IsValid() const noexcept
		{
			const SVDxHeader expected{};
			return SVDx == expected.SVDx && headerSize == expected.headerSize
				&& patchSize == expected.patchSize && numPatches > 0
				&& unknown == expected.unknown;
		}
	};
}

template<typename T>
static void WriteStruct(std::ostream& outFile, const T &value)
{
	outFile.write(reinterpret_cast<const char *>(&value), sizeof(T));
}

template<typename T>
static void WriteVector(std::ostream &outFile, const std::vector<T> &value)
{
	outFile.write(reinterpret_cast<const char *>(value.data()), value.size() * sizeof(T));
}

std::vector<PatchVST> ReadSVZforPlugin(std::istream &inFile)
{
	static_assert(sizeof(SVZHeaderPlugin) == 96);
	static_assert(sizeof(SVDxHeader) == 32);

	SVZHeaderPlugin fileHeader;
	if (!inFile.read(reinterpret_cast<char *>(&fileHeader), sizeof(fileHeader)))
		return {};

	if (!fileHeader.IsValid())
	{
		std::cerr << "Not a valid SVZ file!" << std::endl;
		return {};
	}
	if (fileHeader.compressedSize1 <= 0x40 || fileHeader.compressedSize1 - 0x20 != fileHeader.compressedSize2)
	{
		std::cerr << "Compressed data has unexpected length!" << std::endl;
		return {};
	}

	uint32_t compressedSize = fileHeader.compressedSize1 - 0x40;
	std::vector<unsigned char> compressed(compressedSize);
	if (!inFile.read(reinterpret_cast<char *>(compressed.data()), compressed.size()))
	{
		std::cerr << "Can't read compressed data!" << std::endl;
		return {};
	}
	if (mz_crc32(0, compressed.data(), compressedSize) != fileHeader.compressedCRC32)
	{
		std::cerr << "Compressed data CRC32 mismatch!" << std::endl;
		return {};
	}

	mz_ulong uncompressedSize = fileHeader.uncompressedSize;
	std::vector<unsigned char> uncompressed(uncompressedSize);
	if (mz_uncompress(uncompressed.data(), &uncompressedSize, compressed.data(), compressedSize) != Z_OK)
	{
		std::cerr << "Error during decompression!" << std::endl;
		return {};
	}

	const SVDxHeader &svdHeader = *reinterpret_cast<const SVDxHeader *>(uncompressed.data());
	if (!svdHeader.IsValid())
	{
		std::cerr << "Unexpected header after decompression!" << std::endl;
		return {};
	}

	std::vector<PatchVST> vstPatches(svdHeader.numPatches);
	std::memcpy(vstPatches.data(), uncompressed.data() + sizeof(svdHeader), vstPatches.size() * sizeof(PatchVST));
	return vstPatches;
}

std::vector<PatchVST> ReadSVZforHardware(std::istream &inFile)
{
	static_assert(sizeof(SVZHeaderHardware) == 116);
	SVZHeaderHardware fileHeader;
	if (!inFile.read(reinterpret_cast<char *>(&fileHeader), sizeof(fileHeader)))
		return {};

	if (!fileHeader.IsValid())
	{
		std::cerr << "Not a valid SVZ file!" << std::endl;
		return {};
	}
	
	if (fileHeader.bankSize != 16 + (sizeof(uint32le) + 2048) * fileHeader.numPatches)
	{
		std::cerr << "SVZ file has unexpected length!" << std::endl;
		return {};
	}

	const uint32_t numPatches = fileHeader.numPatches;
	std::vector<uint32le> patchesCRC32(numPatches);
	inFile.read(reinterpret_cast<char *>(patchesCRC32.data()), numPatches * sizeof(uint32le));

	std::vector<PatchVST> vstPatches(numPatches);
	for (uint32_t i = 0; i < numPatches; i++)
	{
		PatchVST &patch = vstPatches[i];
		inFile.read(reinterpret_cast<char *>(&patch.name), 2048);
		const auto patchCRC32 = mz_crc32(0, reinterpret_cast<unsigned char *>(&patch.name), 2048);
		if (patchCRC32 != patchesCRC32[i])
			std::cerr << "Warning, CRC32 mismatch for patch " << (i + 1) << std::endl;
		patch.zenHeader = { 3, 5, 0, 100, {} };
		patch.empty.fill(0);
	}
	return vstPatches;
}

std::vector<PatchVST> ReadSVD(std::istream &inFile)
{
	static_assert(sizeof(SVDHeader) == 16);
	static_assert(sizeof(SVDHeaderEntry) == 16);
	
	SVDHeader fileHeader;
	if (!inFile.read(reinterpret_cast<char *>(&fileHeader), sizeof(fileHeader)))
		return {};

	if (fileHeader.magic != SVDHeader{}.magic || fileHeader.headerSize < 30)
	{
		std::cerr << "Not a valid SVD file!" << std::endl;
		return {};
	}

	uint32_t headerOffset = 16, patchOffset = 0, patchSize = 0;
	while (headerOffset < fileHeader.headerSize)
	{
		SVDHeaderEntry entry;
		if (!inFile.read(reinterpret_cast<char *>(&entry), sizeof(entry)))
			return {};
		headerOffset += sizeof(entry);
		if (entry.type == std::array<char, 4>{ 'P', 'A', 'T', 'a' } && entry.dd07 == SVDHeaderEntry{}.dd07)
		{
			patchOffset = entry.offset;
			patchSize = entry.size;
			break;
		}
	}

	if (patchOffset == 0 || patchSize < 16)
	{
		std::cerr << "SVD file does not contain any patches!" << std::endl;
		return {};
	}

	inFile.seekg(patchOffset);
	SVDPatchHeader patchHeader;
	if (!inFile.read(reinterpret_cast<char *>(&patchHeader), sizeof(patchHeader)))
		return {};

	if (patchHeader.patchSize != 2048)
	{
		std::cerr << "SVD file has unexpected patch size!" << std::endl;
		return {};
	}

	if (patchHeader.unknown1 != SVDPatchHeader{}.unknown1 || patchHeader.unknown2 != SVDPatchHeader{}.unknown2)
	{
		std::cerr << "SVD file has unexpected patch header!" << std::endl;
		return {};
	}

	std::vector<PatchVST> vstPatches(patchHeader.numPatches);
	for (uint32_t i = 0; i < patchHeader.numPatches; i++)
	{
		PatchVST &patch = vstPatches[i];
		inFile.read(reinterpret_cast<char *>(&patch.zenHeader), 2048);
		patch.zenHeader = { 3, 5, 0, 100, {} };
		patch.empty.fill(0);
	}
	return vstPatches;
}

void WriteSVZforPlugin(std::ostream &outFile, const std::vector<PatchVST> &vstPatches)
{
	std::vector<unsigned char> uncompressed(sizeof(SVDxHeader) + vstPatches.size() * sizeof(PatchVST));
	SVDxHeader &svdHeader = *reinterpret_cast<SVDxHeader*>(uncompressed.data());
	svdHeader = SVDxHeader{};
	svdHeader.numPatches = static_cast<uint32_t>(vstPatches.size());
	std::memcpy(uncompressed.data() + sizeof(SVDxHeader), vstPatches.data(), vstPatches.size() * sizeof(PatchVST));

	mz_ulong uncompressedSize = static_cast<mz_ulong>(uncompressed.size());
	mz_ulong compressedSize = mz_compressBound(uncompressedSize);
	std::vector<unsigned char> compressed(compressedSize);
	if (mz_compress2(compressed.data(), &compressedSize, uncompressed.data(), uncompressedSize, MZ_BEST_COMPRESSION) != Z_OK)
	{
		std::cerr << "Error during compression!" << std::endl;
		return;
	}
	compressed.resize(compressedSize);
	const auto compressedCRC32 = mz_crc32(0, compressed.data(), compressedSize);

	SVZHeaderPlugin fileHeader{};
	fileHeader.compressedSize1 = compressedSize + 0x40;
	fileHeader.compressedSize2 = compressedSize + 0x20;
	fileHeader.compressedCRC32 = compressedCRC32;
	fileHeader.uncompressedSize = uncompressedSize;

	WriteStruct(outFile, fileHeader);
	WriteVector(outFile, compressed);
}

void WriteSVZforHardware(std::ostream &outFile, const std::vector<PatchVST> &vstPatches)
{
	SVZHeaderHardware fileHeader{};
	const uint32_t numPatches = static_cast<uint32_t>(vstPatches.size());
	fileHeader.numPatches = numPatches;
	fileHeader.bankSize = static_cast<uint32_t>(16 + (sizeof(uint32le) + 2048) * numPatches);
	fileHeader.bankSizeTruncated = fileHeader.bankSize & 0x1FF;
	outFile.write(reinterpret_cast<const char *>(&fileHeader), sizeof(fileHeader));

	std::vector<uint32le> patchesCRC32(numPatches);
	std::vector<std::array<unsigned char, 2048>> patches(numPatches);
	for (uint32_t i = 0; i < numPatches; i++)
	{
		auto &patch = patches[i];
		std::memcpy(patch.data(), &vstPatches[i].name, 2048);
		patch[2042] = 0x44;
		patch[2045] = 0x01;
		patch[2046] = 0x09;
		patchesCRC32[i] = mz_crc32(0, patch.data(), patch.size());
	}

	WriteVector(outFile, patchesCRC32);
	WriteVector(outFile, patches);
}

void WriteSVD(std::ostream &outFile, const std::vector<PatchVST> &vstPatches)
{
	SVDHeader fileHeader{};
	fileHeader.headerSize = sizeof(SVDHeader) + sizeof(SVDHeaderEntry) - 2;
	
	SVDHeaderEntry entry{};
	entry.type = std::array<char, 4>{ 'P', 'A', 'T', 'a'};
	entry.offset = sizeof(SVDHeader) + sizeof(SVDHeaderEntry);
	entry.size = static_cast<uint32_t>(16 + 2048 * vstPatches.size() + 6);
	
	SVDPatchHeader patchHeader{};
	patchHeader.numPatches = static_cast<uint32_t>(vstPatches.size());

	outFile.write(reinterpret_cast<const char *>(&fileHeader), sizeof(fileHeader));
	outFile.write(reinterpret_cast<const char *>(&entry), sizeof(entry));
	outFile.write(reinterpret_cast<const char *>(&patchHeader), sizeof(patchHeader));

	std::array<char, 2048> patchData{};
	patchData[4] = 1;
	patchData[5] = 1;
	patchData[6] = 5;
	patchData[8] = 15;
	patchData[2044] = 8;
	for (auto &patch : vstPatches)
	{
		std::memcpy(patchData.data() + 16, &patch.name, 2016);
		outFile.write(patchData.data(), patchData.size());
	}

	std::array<char, 6> footer{ 0, 0, 0, 0, 1, 1 };
	outFile.write(footer.data(), footer.size());
}
