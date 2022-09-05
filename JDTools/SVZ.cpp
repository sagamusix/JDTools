// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#include "SVZ.hpp"
#include "Utils.hpp"

#include "miniz.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace
{
	constexpr bool IsDigit(const char digit)
	{
		return digit >= '0' && digit <= '9';
	}
	
	struct SVZHeaderPlugin
	{
		std::array<char, 4> SVZa = { 'S', 'V', 'Z', 'a' };
		uint16le unknown1 = 1;
		std::array<char, 8> RC1 = { 'R', 'C', '0', '0', '1', 1, 0, 0 };
		uint16le unknown2 = 0;
		std::array<char, 4> EXTa = { 'E', 'X', 'T', 'a' };
		std::array<char, 4> ZCOR = { 'Z', 'C', 'O', 'R' };
		uint32le unknown3 = 0x20;
		uint32le compressedSize1;
		std::array<uint32le, 6> unknown4 = { 1, 0, 32, 0, 1, 32};
		uint32le compressedSize2;
		uint32le compressedCRC32;
		std::array<char, 8> RC2 = { 'R', 'C', '0', '0', '1', 1, 0, 0 };
		uint32le uncompressedSize;
		std::array<uint32le, 5> unknown5 = { 0, 0, 0, 0, 0 };

		bool IsValid() const noexcept
		{
			const SVZHeaderPlugin expected{};
			return SVZa == expected.SVZa && unknown1 == expected.unknown1
				&& RC1[0] == 'R' && RC1[1] == 'C' && IsDigit(RC1[2]) && IsDigit(RC1[3]) && IsDigit(RC1[4]) && RC1[5] == 1 && RC1[6] == 0 && RC1[7] == 0
				&& RC1 == RC2 && unknown2 == expected.unknown2
				&& EXTa == expected.EXTa && ZCOR == expected.ZCOR
				&& unknown3 == expected.unknown3 && unknown4 == expected.unknown4
				&& unknown5 == expected.unknown5;
		}
	};

	struct SVZHeaderHardware
	{
		std::array<char, 4> SVZa = { 'S', 'V', 'Z', 'a' };
		uint16le numChunks = 0x0202;  // same value twice
		std::array<char, 10> RC = { 'R', 'C', '0', '0', '1', 1, 0, 0, 0, 0 };
		std::array<char, 4> DIFa = { 'D', 'I', 'F', 'a' };
		std::array<char, 4> ZCOR_1 = { 'Z', 'C', 'O', 'R' };
		uint32le unknown2 = 0x30;
		uint32le unknown3 = 0x34;
		std::array<char, 4> MDLa = { 'M', 'D', 'L', 'a' };
		std::array<char, 4> ZCOR_2 = { 'Z', 'C', 'O', 'R' };
		uint32le bankOffset = 0x64;
		uint32le bankSize = 0;  // Starting from number of patches
		std::array<uint8_t, 52> unknown5 = { 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x09, 0x5C, 0xA1, 0x03, 0x00, 0x86, 0xC8, 0xE5, 0x4C, 0xA5, 0x48, 0x08, 0x0C, 0x00, 0x48, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		uint32le numPatches = 0;
		uint32le unknown6 = 0x800;
		uint32le bankSizeTruncated;  // Only the lower 9 bits?!
		uint32le unknown7 = 0;

		bool IsValid() const noexcept
		{
			const SVZHeaderHardware expected{};
			return SVZa == expected.SVZa && numChunks == 0x0202
				&& RC[0] == 'R' && RC[1] == 'C' && IsDigit(RC[2]) && IsDigit(RC[3]) && IsDigit(RC[4]) && RC[5] == 1 && RC[6] == 0 && RC[7] == 0 && RC[8] == 0 && RC[9] == 0
				&& RC == expected.RC && DIFa == expected.DIFa
				&& ZCOR_1 == expected.ZCOR_1 && unknown2 == 0x30
				&& unknown3 == 0x34 && MDLa == expected.MDLa
				&& ZCOR_2 == expected.ZCOR_2 && bankOffset == expected.bankOffset
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

		static constexpr std::array<char, 4> PATCH_ENTRY{ 'P', 'A', 'T', 'a'};
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

std::vector<PatchVST> ReadSVZforPlugin(std::istream &inFile)
{
	static_assert(sizeof(SVZHeaderPlugin) == 96);
	static_assert(sizeof(SVDxHeader) == 32);

	SVZHeaderPlugin fileHeader;
	if (!Read(inFile, fileHeader))
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
	std::vector<unsigned char> compressed;
	if (!ReadVector(inFile, compressed, compressedSize))
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
	if (!Read(inFile, fileHeader))
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
	std::vector<uint32le> patchesCRC32;
	ReadVector(inFile, patchesCRC32, numPatches);

	std::vector<PatchVST> vstPatches(numPatches);
	for (uint32_t i = 0; i < numPatches; i++)
	{
		PatchVST &patch = vstPatches[i];
		inFile.read(reinterpret_cast<char *>(&patch.name), 2048);
		const auto patchCRC32 = mz_crc32(0, reinterpret_cast<unsigned char *>(&patch.name), 2048);
		if (patchCRC32 != patchesCRC32[i])
			std::cerr << "Warning, CRC32 mismatch for patch " << (i + 1) << std::endl;
		patch.zenHeader = PatchVST::DEFAULT_ZEN_HEADER;
		patch.empty.fill(0);
	}
	return vstPatches;
}

std::vector<PatchVST> ReadSVD(std::istream &inFile)
{
	static_assert(sizeof(SVDHeader) == 16);
	static_assert(sizeof(SVDHeaderEntry) == 16);
	
	SVDHeader fileHeader;
	if (!Read(inFile, fileHeader))
		return {};

	if (fileHeader.magic != SVDHeader{}.magic || fileHeader.headerSize < 30)
	{
		std::cerr << "Not a valid SVD file!" << std::endl;
		return {};
	}

	uint32_t headerOffset = 14, patchOffset = 0, patchSize = 0;
	while (headerOffset < fileHeader.headerSize)
	{
		SVDHeaderEntry entry;
		if (!Read(inFile, entry))
			return {};
		headerOffset += sizeof(entry);
		if (entry.type == SVDHeaderEntry::PATCH_ENTRY && entry.dd07 == SVDHeaderEntry{}.dd07)
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
	if (!Read(inFile, patchHeader))
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
		patch.zenHeader = PatchVST::DEFAULT_ZEN_HEADER;
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

	Write(outFile, fileHeader);
	WriteVector(outFile, compressed);
}

void WriteSVZforHardware(std::ostream &outFile, const std::vector<PatchVST> &vstPatches)
{
	SVZHeaderHardware fileHeader{};
	const uint32_t numPatches = static_cast<uint32_t>(vstPatches.size());
	fileHeader.numPatches = numPatches;
	fileHeader.bankSize = static_cast<uint32_t>(16 + (sizeof(uint32le) + 2048) * numPatches);
	fileHeader.bankSizeTruncated = fileHeader.bankSize & 0x1FF;
	Write(outFile, fileHeader);

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

void WriteSVD(std::ostream &outFile, const std::vector<PatchVST> &vstPatches, const std::vector<char> &originalSVDfile)
{
	// The JD-08 appears to reject SVD files that miss the PRFa, SYSa and/or DIFa chunks.
	// Even if they only consist of the 16-byte header similar to the SVDPatchHeader struct and zeroing out the size fields in that header,
	// the device just starts acting strangely. So the only solution for now is to take an existing SVD file and replace the patch data inside.
	SVDHeader fileHeader = *reinterpret_cast<const SVDHeader *>(originalSVDfile.data());
	if (originalSVDfile.size() < 32 || fileHeader.magic != SVDHeader{}.magic || fileHeader.headerSize < 30 || fileHeader.headerSize > originalSVDfile.size() - 2)
	{
		std::cerr << "Output file must be a valid JD-08 backup SVD file!" << std::endl;
		// File was already opened for writing... preserve original contents
		WriteVector(outFile, originalSVDfile);
		return;
	}

	std::vector<SVDHeaderEntry> entries;
	bool hasPatchEntry = false;
	for (uint32_t headerOffset = 14; headerOffset < fileHeader.headerSize; headerOffset += sizeof(SVDHeaderEntry))
	{
		entries.push_back(*reinterpret_cast<const SVDHeaderEntry *>(originalSVDfile.data() + headerOffset + 2));
		if (entries.back().type == SVDHeaderEntry::PATCH_ENTRY)
			hasPatchEntry = true;
	}
	if (!hasPatchEntry)
	{
		entries.push_back({SVDHeaderEntry::PATCH_ENTRY});
		fileHeader.headerSize = fileHeader.headerSize + sizeof(SVDHeaderEntry);
	}

	Write(outFile, fileHeader);
	WriteVector(outFile, entries);  // Needs to be updated later

	uint32_t offset = static_cast<uint32_t>(outFile.tellp());
	for (SVDHeaderEntry &entry : entries)
	{
		if (entry.type == SVDHeaderEntry::PATCH_ENTRY)
		{
			entry.size = static_cast<uint32_t>(sizeof(SVDPatchHeader) + 2048 * vstPatches.size());

			SVDPatchHeader patchHeader{};
			patchHeader.numPatches = static_cast<uint32_t>(vstPatches.size());
			Write(outFile, patchHeader);

			std::array<char, 2048> patchData{};
			patchData[4] = 1;
			patchData[5] = 1;
			patchData[6] = 5;
			patchData[8] = 15;
			patchData[2044] = 8;
			for (const auto &patch : vstPatches)
			{
				std::memcpy(patchData.data() + 16, &patch.name, 2016);
				Write(outFile, patchData);
			}
		}
		else
		{
			// Just copy the original block
			if(entry.offset < originalSVDfile.size() && entry.size < originalSVDfile.size() - entry.offset)
				outFile.write(originalSVDfile.data() + entry.offset, entry.size);
			else
				entry.size = 0;
		}
		entry.offset = offset;
		offset += entry.size;
	}

	outFile.seekp(sizeof(fileHeader));
	WriteVector(outFile, entries);


}
