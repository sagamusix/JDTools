// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 - 2024 by Johannes Schultz
// License: BSD 3-clause

#include "SVZ.hpp"
#include "JD-08.hpp"
#include "Utils.hpp"

#include "miniz.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <tuple>

namespace
{
	struct SVZHeader
	{
		std::array<char, 4> SVZa = { 'S', 'V', 'Z', 'a' };
		uint8_t numChunks = 1;
		uint8_t numChunksRepeated = 1;  // Plugin (BIN) files don't repeat this value, hardware (SVZ) files do.
		std::array<char, 6> ID = { 'R', 'C', '0', '0', '1', 1 };
		uint32le null = 0;

		bool IsValid() const noexcept
		{
			static_assert(sizeof(SVZHeader) == 16);

			const SVZHeader expected{};
			return SVZa == expected.SVZa
				&& (numChunksRepeated == 0 || numChunksRepeated == numChunks)
				&& null == expected.null;
		}
	};

	struct SVZHeaderEntry
	{
		static constexpr std::array<char, 4> DIFa{ 'D', 'I', 'F', 'a' };
		static constexpr std::array<char, 4> MDLa{ 'M', 'D', 'L', 'a' };
		static constexpr std::array<char, 4> EXTa{ 'E', 'X', 'T', 'a' };

		std::array<char, 4> type;
		std::array<char, 4> ZCOR = { 'Z', 'C', 'O', 'R' };
		uint32le offset;
		uint32le size;

		bool IsValid() const noexcept
		{
			static_assert(sizeof(SVZHeaderEntry) == 16);

			return ZCOR == SVZHeaderEntry{}.ZCOR;
		}
	};

	// Hardware
	struct SVZChunkHeaderMDLa
	{
		uint32le numPatches = 0;
		uint32le unknown1 = 0x800;
		uint32le chunkSizeTruncated;  // Only the lower 9 bits?!
		uint32le unknown2 = 0;

		bool IsValid(const SVZHeaderEntry &chunkHeader) const noexcept
		{
			static_assert(sizeof(SVZChunkHeaderMDLa) == 16);

			const SVZChunkHeaderMDLa expected{};
			return chunkHeader.size >= sizeof(*this)
				&& unknown1 == expected.unknown1
				&& chunkSizeTruncated == (chunkHeader.size & 0x1FF)
				&& unknown2 == expected.unknown2;
		}
	};

	// Plugin
	struct SVZChunkHeaderEXTa
	{
		std::array<uint32le, 6> unknown1 = { 1, 0, 32, 0, 1, 32 };
		uint32le compressedSize;
		uint32le compressedCRC32;
		std::array<char, 8> RC = { 'R', 'C', '0', '0', '1', 1, 0, 0 };
		uint32le uncompressedSize;
		std::array<uint32le, 5> unknown2 = { 0, 0, 0, 0, 0 };

		bool IsValid(const SVZHeaderEntry &chunkHeader) const noexcept
		{
			static_assert(sizeof(SVZChunkHeaderEXTa) == 64);

			const SVZChunkHeaderEXTa expected{};
			return chunkHeader.size >= sizeof(*this)
				&& unknown1 == expected.unknown1
				&& unknown2 == expected.unknown2;
		}
	};

	struct SVZChunkHeaderDIFa
	{
		uint32le unknown1 = 0x01;
		uint32le unknown2 = 0x20;
		uint32le unknown3 = 0x14;
		uint32le unknown4 = 0x00;
		std::array<uint8_t, 20> unknown5 = {
			0x42, 0x09, 0x5C, 0xA1,
			0x03, 0x00, 0x86, 0xC8,
			0xE5, 0x4C, 0xA5, 0x48,
			0x08, 0x0C, 0x00, 0x48,
			0x00, 0x48, 0x00, 0x48,
		};
		std::array<uint32le, 4> unknown6 = {0, 0, 0, 0};

		bool IsValid() const noexcept
		{
			static_assert(sizeof(SVZChunkHeaderDIFa) == 52);

			const SVZChunkHeaderDIFa expected{};
			return std::tie(unknown1, unknown2, unknown3, unknown4, unknown5, unknown6)
				== std::tie(expected.unknown1, expected.unknown2, expected.unknown3, expected.unknown4, expected.unknown5, expected.unknown6);
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
			static_assert(sizeof(SVDxHeader) == 32);

			const SVDxHeader expected{};
			return SVDx == expected.SVDx && headerSize == expected.headerSize
				&& patchSize == expected.patchSize && numPatches > 0
				&& unknown == expected.unknown;
		}
	};
}

std::vector<PatchVST> ReadSVZ(std::istream &inFile)
{
	SVZHeader fileHeader;
	if (!Read(inFile, fileHeader))
		return {};

	if (!fileHeader.IsValid())
	{
		std::cerr << "Not a valid SVZ file!" << std::endl;
		return {};
	}

	for (uint32_t chunk = 0; chunk < fileHeader.numChunks; chunk++)
	{
		SVZHeaderEntry entry;
		if (!Read(inFile, entry))
			return {};
		if (entry.type == SVZHeaderEntry::MDLa)
		{
			inFile.seekg(entry.offset, std::ios::beg);
			SVZChunkHeaderMDLa chunkHeader;
			if (!Read(inFile, chunkHeader))
				return {};

			if (!chunkHeader.IsValid(entry))
			{
				std::cerr << "Not a valid SVZ file!" << std::endl;
				return {};
			}

			if (entry.size != 16 + (sizeof(uint32le) + 2048) * chunkHeader.numPatches)
			{
				std::cerr << "SVZ file has unexpected length!" << std::endl;
				return {};
			}

			const uint32_t numPatches = chunkHeader.numPatches;
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
				if (patch.empty[29] != 1)
				{
					std::cerr << "Patches appear to be for different synth model!" << std::endl;
					return {};
				}
				patch.zenHeader = PatchVST::DEFAULT_ZEN_HEADER;
				patch.empty.fill(0);
			}
			return vstPatches;
		}
		else if (entry.type == SVZHeaderEntry::EXTa)
		{
			inFile.seekg(entry.offset, std::ios::beg);
			SVZChunkHeaderEXTa chunkHeader;
			if (!Read(inFile, chunkHeader))
				return {};

			if (!chunkHeader.IsValid(entry))
			{
				std::cerr << "Not a valid SVZ file!" << std::endl;
				return {};
			}

			if (entry.size - 0x20 != chunkHeader.compressedSize)
			{
				std::cerr << "Compressed data has unexpected length!" << std::endl;
				return {};
			}

			uint32_t compressedSize = entry.size - 0x40;
			std::vector<unsigned char> compressed;
			if (!ReadVector(inFile, compressed, compressedSize))
			{
				std::cerr << "Can't read compressed data!" << std::endl;
				return {};
			}
			if (mz_crc32(0, compressed.data(), compressedSize) != chunkHeader.compressedCRC32)
			{
				std::cerr << "Compressed data CRC32 mismatch!" << std::endl;
				return {};
			}

			mz_ulong uncompressedSize = chunkHeader.uncompressedSize;
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
	}
	return {};
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
	SVDxHeader &svdHeader = *reinterpret_cast<SVDxHeader *>(uncompressed.data());
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

	SVZHeader fileHeader{};
	fileHeader.numChunks = 1;
	fileHeader.numChunksRepeated = 0;
	Write(outFile, fileHeader);

	SVZHeaderEntry entryEXTa;
	entryEXTa.type = SVZHeaderEntry::EXTa;
	entryEXTa.offset = 0x20;
	entryEXTa.size = compressedSize + 0x40;
	Write(outFile, entryEXTa);

	SVZChunkHeaderEXTa chunkHeader;
	chunkHeader.compressedSize = compressedSize + 0x20;
	chunkHeader.compressedCRC32 = compressedCRC32;
	chunkHeader.uncompressedSize = uncompressedSize;
	Write(outFile, chunkHeader);

	WriteVector(outFile, compressed);
}

void WriteSVZforHardware(std::ostream &outFile, const std::vector<PatchVST> &vstPatches)
{
	SVZHeader fileHeader{};
	fileHeader.numChunks = 2;
	fileHeader.numChunksRepeated = 2;
	Write(outFile, fileHeader);

	const uint32_t numPatches = static_cast<uint32_t>(vstPatches.size());

	SVZHeaderEntry entryDIFa;
	entryDIFa.type = SVZHeaderEntry::DIFa;
	entryDIFa.offset = 0x30;
	entryDIFa.size = sizeof(SVZChunkHeaderDIFa);
	Write(outFile, entryDIFa);

	SVZHeaderEntry entryMDLa;
	entryMDLa.type = SVZHeaderEntry::MDLa;
	entryMDLa.offset = 0x30 + sizeof(SVZChunkHeaderDIFa);
	entryMDLa.size = static_cast<uint32_t>(16 + (sizeof(uint32le) + 2048) * numPatches);
	Write(outFile, entryMDLa);

	Write(outFile, SVZChunkHeaderDIFa{});

	SVZChunkHeaderMDLa chunkHeader;
	chunkHeader.numPatches = numPatches;
	chunkHeader.chunkSizeTruncated = entryMDLa.size & 0x1FF;
	Write(outFile, chunkHeader);

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
			if(entry.offset < originalSVDfile.size() && entry.size <= originalSVDfile.size() - entry.offset)
			{
				outFile.write(originalSVDfile.data() + entry.offset, entry.size);
			}
			else
			{
				entry.size = 0;
				std::cerr << "Dropping an SVD chunk, it appears to be truncated!" << std::endl;
			}
		}
		entry.offset = offset;
		offset += entry.size;
	}

	outFile.seekp(sizeof(fileHeader));
	WriteVector(outFile, entries);
}

