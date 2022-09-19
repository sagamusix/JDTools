// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#include "JDTools.hpp"
#include "InputFile.hpp"
#include "SVZ.hpp"
#include "Utils.hpp"

#include "JD-800.hpp"
#include "JD-990.hpp"
#include "JD-08.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	constexpr uint8_t SYSEX_DEVICE_ID = 0x10;
	constexpr uint8_t UNDEFINED_MEMORY = 0xFE;

	constexpr uint32_t BASE_ADDR_800_PATCH_TEMPORARY = (0x00 << 14);
	constexpr uint32_t BASE_ADDR_800_SETUP_TEMPORARY = (0x01 << 14);
	constexpr uint32_t BASE_ADDR_800_SYSTEM = (0x02 << 14);
	constexpr uint32_t BASE_ADDR_800_PART = (0x03 << 14);
	constexpr uint32_t BASE_ADDR_800_SETUP_INTERNAL = (0x04 << 14);
	constexpr uint32_t BASE_ADDR_800_PATCH_INTERNAL = (0x05 << 14);
	constexpr uint32_t BASE_ADDR_800_DISPLAY = (0x07 << 14);
	
	constexpr uint32_t BASE_ADDR_990_SYSTEM = (0x00 << 21);
	constexpr uint32_t BASE_ADDR_990_PERFORMANCE_TEMPORARY = (0x01 << 21);
	constexpr uint32_t BASE_ADDR_990_PERFORMANCE_PATCHES_TEMPORARY = (0x02 << 21);
	constexpr uint32_t BASE_ADDR_990_PATCH_TEMPORARY = (0x03 << 21);
	constexpr uint32_t BASE_ADDR_990_SETUP_TEMPORARY = (0x04 << 21);
	constexpr uint32_t BASE_ADDR_990_PERFORMANCE_INTERNAL = (0x05 << 21);
	constexpr uint32_t BASE_ADDR_990_PATCH_INTERNAL = (0x06 << 21);
	constexpr uint32_t BASE_ADDR_990_SETUP_INTERNAL = (0x07 << 21);
	constexpr uint32_t BASE_ADDR_990_SYSTEM_CARD = (0x08 << 21);
	constexpr uint32_t BASE_ADDR_990_PERFORMANCE_CARD = (0x09 << 21);
	constexpr uint32_t BASE_ADDR_990_PATCH_CARD = (0x0A << 21);
	constexpr uint32_t BASE_ADDR_990_SETUP_CARD = (0x0B << 21);
}

template<size_t Size>
static std::string_view ToString(const std::array<char, Size> &arr)
{
	return std::string_view{ arr.data(), arr.size() };
}

static void PrintUsage()
{
	std::cout <<
R"(JDTools - Patch conversion utility for Roland JD-800 / JD-990

Usage:

JDTools convert syx <input> <output>
  Converts from JD-800 SysEx dump (SYX / MID), JD-990 SysEx dump (SYX / MID),
  JD-800 VST BIN, JD-08 SVD or ZC1 SVZ file
  to JD-800 or JD-990 SysEx dump (SYX).
  Output is a JD-990 SysEx dump if the source file was a JD-800 SysEx dump,
  otherwise it is always a JD-800 SysEx dump.

JDTools convert bin <input> <output>
  Converts from JD-800 SysEx dump (SYX / MID), JD-990 SysEx dump (SYX / MID),
  JD-08 SVD or ZC1 SVZ file to JD-800 VST BIN file.

JDTools convert svd <input> <JD08Backup.svd> <position>
  Converts from JD-800 SysEx dump (SYX / MID), JD-990 SysEx dump (SYX / MID),
  JD-800 VST BIN or ZC1 SVZ file to JD-08 SVD file.
  The output file should be named JD08Backup.svd so that the JD-08 can find it,
  and must be a valid, existing JD-08 backup file to overwrite.
  The last parameter is optional and specifies the starting patch position to
  overwrite. This can just be a bank (A/B/C/D) or a patch number (e.g. B42).

JDTools convert svz <input> <output>
  Converts from JD-800 SysEx dump (SYX / MID), JD-990 SysEx dump (SYX / MID),
  JD-800 VST BIN or JD-08 SVD file to ZC1 SVZ file.

JDTools merge <input1.syx> <input2.syx> <input3.syx> ... <output.syx>
  Merges SYX or MID files containing temporary patches for either JD-800 or
  JD-990 into banks

JDTools list <input.syx>
  Lists all SysEx / BIN / SVD / SVZ contents
)" << std::endl;
}

static void WriteSysEx(std::ostream &f, uint32_t outAddress, const bool isJD990, const uint8_t *data, size_t size)
{
	// debug stuff
	for (size_t i = 0; i < size; i++)
	{
		if (data[i] >= 0x80)
		{
			std::cerr << "invalid byte in SysEx data block at " << i << " - either broken parameter conversion or broken SysEx source!" << std::endl;
		}
	}

	std::vector<uint8_t> outMessage;
	size_t offset = 0;
	while (size)
	{
		const size_t amountToCopy = std::min(size, size_t(256));
		if (isJD990)
			outMessage.assign({ 0xF0, 0x41, SYSEX_DEVICE_ID, 0x57, 0x12, static_cast<uint8_t>((outAddress >> 21) & 0x7F), static_cast<uint8_t>((outAddress >> 14) & 0x7F), static_cast<uint8_t>((outAddress >> 7) & 0x7F), static_cast<uint8_t>(outAddress & 0x7F) });
		else
			outMessage.assign({ 0xF0, 0x41, SYSEX_DEVICE_ID, 0x3D, 0x12, static_cast<uint8_t>((outAddress >> 14) & 0x7F), static_cast<uint8_t>((outAddress >> 7) & 0x7F), static_cast<uint8_t>(outAddress & 0x7F) });
		outMessage.insert(outMessage.end(), data + offset, data + offset + amountToCopy);
		uint8_t checksum = 0;
		for (size_t i = 5; i < outMessage.size(); i++)
		{
			checksum += outMessage[i];
		}
		checksum = (~checksum + 1) & 0x7F;
		outMessage.push_back(checksum);
		outMessage.push_back(0xF7);
		WriteVector(f, outMessage);

		outAddress += static_cast<uint32_t>(amountToCopy);
		size -= amountToCopy;
		offset += amountToCopy;
	}
}

template<typename T>
static void WriteSysEx(std::ofstream &f, uint32_t outAddress, const bool isJD990, const T &object)
{
	WriteSysEx(f, outAddress, isJD990, reinterpret_cast<const uint8_t *>(&object), sizeof(object));
}

static std::vector<PatchVST> MergePatchesIntoSVD(std::vector<PatchVST> patches, const std::vector<PatchVST> &sourceFile, const size_t offset)
{
	patches.insert(patches.begin(), sourceFile.begin(), sourceFile.begin() + std::min(sourceFile.size(), offset));
	if (patches.size() < sourceFile.size())
		patches.insert(patches.end(), sourceFile.begin() + patches.size(), sourceFile.end());
	else if (patches.size() > 256)
		patches.resize(256);
	return patches;
}

static std::string GetPatchIndex(const uint32_t patch, const uint32_t numPatches, const bool isCard = false)
{
	std::string patchIndex;
	if (isCard)
		patchIndex = 'C';
	else if (numPatches <= 64)
		patchIndex = 'I';
	else
		patchIndex = 'A' + static_cast<char>(patch / 64u);
	patchIndex += '1' + ((patch / 8u) % 8u);
	patchIndex += '1' + (patch % 8u);
	return patchIndex;
}


int main(const int argc, char *argv[])
{
	static_assert(sizeof(Patch800) == 384);
	static_assert(sizeof(Patch990) == 486);
	static_assert(sizeof(PatchVST) == 22352);
	static_assert(sizeof(SpecialSetup800) == 5378);
	static_assert(sizeof(SpecialSetup990) == 6524);

	if (argc < 3)
	{
		PrintUsage();
		return 1;
	}

	const std::string_view verb = argv[1];
	int numInputFiles = 1, firstFileParam = 2;
	if (verb != "convert" && verb != "list" && verb != "merge")
	{
		PrintUsage();
		return 1;
	}
	if ((verb == "list" && argc != 3) || (verb == "merge" && argc < 4))
	{
		PrintUsage();
		return 1;
	}
	if (verb == "merge")
	{
		numInputFiles = argc - 3;
	}

	InputFile::Type targetType = InputFile::Type::SYX;
	if (verb == "convert")
	{
		const std::string_view targetStr = argv[2];
		if (targetStr == "syx" || targetStr == "SYX" && argc == 5)
		{
			targetType = InputFile::Type::SYX;
		}
		else if(targetStr == "bin" || targetStr == "BIN" && argc == 5)
		{
			targetType = InputFile::Type::SVZplugin;
		}
		else if (targetStr == "svz" || targetStr == "SVZ" && argc == 5)
		{
			targetType = InputFile::Type::SVZhardware;
		}
		else if ((targetStr == "svd" || targetStr == "SVD") && (argc == 5 || argc == 6))
		{
			targetType = InputFile::Type::SVD;
		}
		else
		{
			PrintUsage();
			return 1;
		}
		firstFileParam = 3;
	}

	enum class DeviceType
	{
		Undetermined,
		JD800,
		JD990,
		JD800VST,
	};
	DeviceType sourceDeviceType = DeviceType::Undetermined;

	std::vector<uint8_t> memory(0x1'800'000, UNDEFINED_MEMORY);  // enough to address JD-990 card setup
	std::vector<Patch800> temporaryPatches800;
	std::vector<Patch990> temporaryPatches990;
	std::vector<PatchVST> vstPatches;
	std::vector<uint8_t> message;

	for (int i = 0; i < numInputFiles; i++)
	{
		const std::string inFilename = argv[firstFileParam + i];
		std::ifstream inFile{inFilename, std::ios::binary};
		if (!inFile)
		{
			std::cout << "Could not open " << inFilename << " for reading!" << std::endl;
			return 2;
		}

		InputFile inputFile{inFile};
		if (inputFile.GetType() == InputFile::Type::SVZplugin)
		{
			vstPatches = ReadSVZforPlugin(inFile);
			if (vstPatches.empty())
				return 2;
			sourceDeviceType = DeviceType::JD800VST;
		}
		else if (inputFile.GetType() == InputFile::Type::SVZhardware)
		{
			vstPatches = ReadSVZforHardware(inFile);
			if (vstPatches.empty())
				return 2;
			sourceDeviceType = DeviceType::JD800VST;
		}
		else if (inputFile.GetType() == InputFile::Type::SVD)
		{
			vstPatches = ReadSVD(inFile);
			if (vstPatches.empty())
				return 2;
			sourceDeviceType = DeviceType::JD800VST;
		}
		else
		{
			do
			{
				message = inputFile.NextSysExMessage();
				if (message.empty())
					break;

				if (message.size() < 6)
				{
					std::cout << "Ignoring SysEx message: Too short" << std::endl;
					continue;
				}

				if (message[0] != 0x41)
				{
					std::cout << "Ignoring SysEx message: Not a Roland device" << std::endl;
					continue;
				}

				uint8_t ch = message[2];
				if (ch != 0x3D && ch != 0x57)
				{
					std::cout << "Ignoring SysEx message: Not a JD-800 or JD-990 message" << std::endl;
					continue;
				}

				if (ch == 0x3D)
				{
					if (sourceDeviceType == DeviceType::JD990)
					{
						std::cout << "WARNING: File contains mixed JD-800 and JD-990 dumps. Only JD-990 dumps will be processed." << std::endl;
						continue;
					}
					sourceDeviceType = DeviceType::JD800;
				}
				else if (ch == 0x57)
				{
					if (sourceDeviceType == DeviceType::JD800)
					{
						std::cout << "WARNING: File contains mixed JD-800 and JD-990 dumps. Only JD-800 dumps will be processed." << std::endl;
						continue;
					}
					sourceDeviceType = DeviceType::JD990;
				}

				if (message[3] != 0x12)
				{
					// TODO: for <list> verb, also show contents of other types?
					std::cout << "Ignoring SysEx message: Not a Data Set message" << std::endl;
					continue;
				}

				// Remove EOX
				message.pop_back();

				uint8_t checksum = 0;
				for (size_t j = 4; j < message.size(); j++)
				{
					checksum += message[j];
				}
				checksum = (~checksum + 1) & 0x7F;
				if (checksum != 0)
				{
					std::cerr << "Invalid SysEx checksum!" << std::endl;
					return 3;
				}

				// Remove checksum byte
				message.pop_back();

				if ((message.size() < 7 && sourceDeviceType == DeviceType::JD800) || (message.size() < 8 && sourceDeviceType == DeviceType::JD990))
				{
					std::cerr << "WARNING! Skipping SysEx, too short!" << std::endl;
					continue;
				}

				uint32_t address = 0;
				if (sourceDeviceType == DeviceType::JD800)
					address = (message[4] << 14) | (message[5] << 7) | message[6];
				else
					address = (message[4] << 21) | (message[5] << 14) | (message[6] << 7) | message[7];

				if (address + message.size() > memory.size())
				{
					std::cerr << "WARNING! Too large address, ignoring SysEx message!" << std::endl;
					continue;
				}

				std::copy(message.begin() + ((sourceDeviceType == DeviceType::JD800) ? 7 : 8), message.end(), memory.begin() + address);

				if (sourceDeviceType == DeviceType::JD800 && address == BASE_ADDR_800_PATCH_TEMPORARY + 256)
					temporaryPatches800.push_back(*reinterpret_cast<const Patch800 *>(memory.data() + BASE_ADDR_800_PATCH_TEMPORARY));
				else if (sourceDeviceType == DeviceType::JD990 && address == BASE_ADDR_990_PATCH_TEMPORARY + 256)
					temporaryPatches990.push_back(*reinterpret_cast<const Patch990 *>(memory.data() + BASE_ADDR_990_PATCH_TEMPORARY));
			} while (!message.empty());
		}
	}

	if (sourceDeviceType == DeviceType::Undetermined)
	{
		std::cout << "Input didn't contain any SysEx messages for either JD-800 or JD-990!" << std::endl;
		return 2;
	}

	if (verb == "convert")
	{
		const std::string_view outFilenameBase = argv[4];
		std::string_view sourceName, targetName, targetExt;
		std::vector<char> originalSVDfile;
		std::vector<PatchVST> svdOutputPatches;
		uint32_t patchOffsetSVD = 0;
		if (sourceDeviceType == DeviceType::JD800)
			sourceName = "JD-800";
		else if (sourceDeviceType == DeviceType::JD990)
			sourceName = "JD-800";
		else if (sourceDeviceType == DeviceType::JD800VST)
			sourceName = "JD-800 VST / JD-08 / ZC1";

		if (targetType == InputFile::Type::SYX)
		{
			targetExt = "syx";
			if (sourceDeviceType == DeviceType::JD800)
				targetName = "JD-990";
			else
				targetName = "JD-800";
		}
		else if (targetType == InputFile::Type::SVZplugin)
		{
			targetExt = "bin";
			targetName = "JD-800 VST";
		}
		else if (targetType == InputFile::Type::SVZhardware)
		{
			targetExt = "svz";
			targetName = "ZC1";
		}
		else if (targetType == InputFile::Type::SVD)
		{
			targetExt = "svd";
			targetName = "JD-08";

			if (argc == 6)
			{
				// Determine write offset
				const std::string_view svdOffset = argv[5];
				if (svdOffset.size() == 1 && svdOffset[0] >= 'A' && svdOffset[0] <= 'D')
					patchOffsetSVD = (svdOffset[0] - 'A') * 64;
				else if (svdOffset.size() == 1 && svdOffset[0] >= 'a' && svdOffset[0] <= 'd')
					patchOffsetSVD = (svdOffset[0] - 'a') * 64;
				else if (svdOffset.size() == 3 && svdOffset[0] >= 'A' && svdOffset[0] <= 'D' && svdOffset[1] >= '1' && svdOffset[1] <= '8' && svdOffset[2] >= '1' && svdOffset[2] <= '8')
					patchOffsetSVD = (svdOffset[0] - 'A') * 64 + (svdOffset[1] - '1') * 8 + (svdOffset[2] - '1');
				else if (svdOffset.size() == 3 && svdOffset[0] >= 'a' && svdOffset[0] <= 'd' && svdOffset[1] >= '1' && svdOffset[1] <= '8' && svdOffset[2] >= '1' && svdOffset[2] <= '8')
					patchOffsetSVD = (svdOffset[0] - 'a') * 64 + (svdOffset[1] - '1') * 8 + (svdOffset[2] - '1');
				else
				{
					std::cout << "Position parameter needs to be a bank (A/B/C/D) or patch number (e.g. B42)!" << std::endl;
					return 2;
				}
			}

			std::ifstream inFile{ std::string{outFilenameBase}, std::ios::binary };
			if (!inFile)
			{
				std::cout << "Could not open " << outFilenameBase << " for reading! An original JD-08 backup file is required to write the patch data into." << std::endl;
				return 2;
			}

			svdOutputPatches = ReadSVD(inFile);
			if (svdOutputPatches.empty())
			{
				std::cout << outFilenameBase << " does not appear to be a valid SVD file! An original JD-08 backup file is required to write the patch data into." << std::endl;
				return 2;
			}

			inFile.seekg(0, std::ios::end);
			const auto size = static_cast<size_t>(inFile.tellg());
			inFile.seekg(0);
			ReadVector(inFile, originalSVDfile, size);
		}

		std::cout << "Converting " << sourceName << " patch format to " << targetName << "..." << std::endl;

		if (sourceDeviceType != DeviceType::JD800VST)
			vstPatches.resize(64);

		const uint32_t numPatches = static_cast<uint32_t>(vstPatches.size());
		const uint32_t bankSize = (targetType == InputFile::Type::SVD) ? 256 : 64;
		const uint32_t numBanks = (numPatches + bankSize - 1) / bankSize;
		uint32_t sourcePatch = 0;
		std::vector<PatchVST> bankPatchesVST(bankSize);

		for (uint32_t bank = 0; bank < numBanks; bank++)
		{
			std::string outFilename{outFilenameBase};
			if (numBanks > 1)
			{
				if (outFilename.size() > 4 && outFilename[outFilename.size() - 4] == '.')
					outFilename = outFilename.substr(0, outFilename.size() - 3) + std::to_string(bank + 1) + outFilename.substr(outFilename.size() - 4);
				else
					outFilename += "." + std::to_string(bank + 1) + "." + std::string{targetExt};
			}

			std::ofstream outFile{ outFilename, std::ios::trunc | std::ios::binary };

			// Convert patches
			for (uint32_t destPatch = 0; destPatch < bankSize; destPatch++, sourcePatch++)
			{
				if (sourcePatch >= numPatches)
					break;

				if (sourceDeviceType == DeviceType::JD800VST && targetType != InputFile::Type::SVZplugin)
				{
					PatchVST &pVST = vstPatches[sourcePatch];
					if (pVST.zenHeader.modelID1 != 3 || pVST.zenHeader.modelID2 != 5)
					{
						std::cerr << "Ignoring patch" << GetPatchIndex(sourcePatch, numPatches) << ", appears to be for another synth model!" << std::endl;
						memset(&pVST, 0, sizeof(pVST));
						pVST.zenHeader = PatchVST::DEFAULT_ZEN_HEADER;
						pVST.name.fill(' ');
					}
				}

				const uint32_t address800src = BASE_ADDR_800_PATCH_INTERNAL + ((sourcePatch * 0x03) << 7);
				const uint32_t address990src = BASE_ADDR_990_PATCH_INTERNAL + (sourcePatch << 14);
				const uint32_t address800dst = BASE_ADDR_800_PATCH_INTERNAL + ((destPatch * 0x03) << 7);
				const uint32_t address990dst = BASE_ADDR_990_PATCH_INTERNAL + (destPatch << 14);
				if (sourceDeviceType == DeviceType::JD800)
				{
					if (memory[address800src] == UNDEFINED_MEMORY)
						continue;
					const Patch800 &p800 = *reinterpret_cast<const Patch800 *>(memory.data() + address800src);
					std::cout << "Converting " << GetPatchIndex(sourcePatch, numPatches) << ": " << ToString(p800.common.name) << std::endl;
					if (targetType == InputFile::Type::SYX)
					{
						Patch990 p990;
						ConvertPatch800To990(p800, p990);
						WriteSysEx(outFile, address990dst, true, p990);
					}
					else
					{
						ConvertPatch800ToVST(p800, bankPatchesVST[destPatch]);
					}
				}
				else if (sourceDeviceType == DeviceType::JD990)
				{
					if (memory[address990src] == UNDEFINED_MEMORY)
						continue;
					const Patch990 &p990 = *reinterpret_cast<const Patch990 *>(memory.data() + address990src);
					std::cout << "Converting " << GetPatchIndex(sourcePatch, numPatches) << ": " << ToString(p990.common.name) << std::endl;
					Patch800 p800;
					ConvertPatch990To800(p990, p800);
					if (targetType == InputFile::Type::SYX)
						WriteSysEx(outFile, address800dst, false, p800);
					else
						ConvertPatch800ToVST(p800, bankPatchesVST[destPatch]);
				}
				else if (sourceDeviceType == DeviceType::JD800VST)
				{
					const PatchVST &pVST = vstPatches[sourcePatch];
					std::cout << "Converting " << GetPatchIndex(sourcePatch, numPatches) << ": " << ToString(pVST.name) << std::endl;
					if (targetType == InputFile::Type::SYX)
					{
						Patch800 p800;
						ConvertPatchVSTTo800(pVST, p800);
						WriteSysEx(outFile, address800dst, false, p800);
					}
					else
					{
						bankPatchesVST[destPatch] = vstPatches[sourcePatch];
					}
				}
			}

			if (targetType == InputFile::Type::SVZplugin)
				WriteSVZforPlugin(outFile, bankPatchesVST);
			else if (targetType == InputFile::Type::SVZhardware)
				WriteSVZforHardware(outFile, bankPatchesVST);
			else if (targetType == InputFile::Type::SVD)
				WriteSVD(outFile, MergePatchesIntoSVD(bankPatchesVST, svdOutputPatches, patchOffsetSVD), originalSVDfile);

			if(bank > 0)
				continue;

			if (targetType != InputFile::Type::SYX && targetType != InputFile::Type::MID)
			{
				// Convert rhythm setup / special setup
				const uint32_t address800 = (memory[BASE_ADDR_800_SETUP_INTERNAL] != UNDEFINED_MEMORY) ? BASE_ADDR_800_SETUP_INTERNAL : BASE_ADDR_800_SETUP_TEMPORARY;
				const uint32_t address990 = (memory[BASE_ADDR_990_SETUP_INTERNAL] != UNDEFINED_MEMORY) ? BASE_ADDR_990_SETUP_INTERNAL : BASE_ADDR_990_SETUP_TEMPORARY;
				std::vector<PatchVST> setupPatches;
				if (sourceDeviceType == DeviceType::JD800 && memory[address800] != UNDEFINED_MEMORY)
				{
					const SpecialSetup800 &s800 = *reinterpret_cast<const SpecialSetup800 *>(memory.data() + address800);
					std::cout << "Converting special setup" << std::endl;
					setupPatches = ConvertSetup800ToVST(s800);
				}
				else if (sourceDeviceType == DeviceType::JD990 && memory[address990] != UNDEFINED_MEMORY)
				{
					const SpecialSetup990 &s990 = *reinterpret_cast<const SpecialSetup990 *>(memory.data() + address990);
					SpecialSetup800 s800;
					std::cout << "Converting special setup: " << ToString(s990.common.name) << std::endl;
					ConvertSetup990To800(s990, s800);
					setupPatches = ConvertSetup800ToVST(s800);
				}

				if (!setupPatches.empty())
				{
					if (outFilename.size() > 4 && outFilename[outFilename.size() - 4] == '.')
						outFilename = outFilename.substr(0, outFilename.size() - 3) + "setup" + outFilename.substr(outFilename.size() - 4);
					else
						outFilename += ".setup." + std::string{ targetExt };

					std::ofstream outFileSetup{ outFilename, std::ios::trunc | std::ios::binary };

					if (targetType == InputFile::Type::SVZplugin)
						WriteSVZforPlugin(outFileSetup, setupPatches);
					else if (targetType == InputFile::Type::SVZhardware)
						WriteSVZforHardware(outFileSetup, setupPatches);
					else if (targetType == InputFile::Type::SVD)
						WriteSVD(outFileSetup, MergePatchesIntoSVD(setupPatches, svdOutputPatches, patchOffsetSVD), originalSVDfile);
				}

				continue;
			}

			// Convert rhythm setup / special setup
			const uint32_t address800 = BASE_ADDR_800_SETUP_INTERNAL;
			const uint32_t address990 = BASE_ADDR_990_SETUP_INTERNAL;
			if (sourceDeviceType == DeviceType::JD800 && memory[address800] != UNDEFINED_MEMORY)
			{
				const SpecialSetup800 &s800 = *reinterpret_cast<const SpecialSetup800 *>(memory.data() + address800);
				SpecialSetup990 s990;
				std::cout << "Converting special setup" << std::endl;
				ConvertSetup800To990(s800, s990);
				WriteSysEx(outFile, address990, true, s990);
			}
			else if (sourceDeviceType == DeviceType::JD990 && memory[address990] != UNDEFINED_MEMORY)
			{
				const SpecialSetup990 &s990 = *reinterpret_cast<const SpecialSetup990 *>(memory.data() + address990);
				SpecialSetup800 s800;
				std::cout << "Converting special setup: " << ToString(s990.common.name) << std::endl;
				ConvertSetup990To800(s990, s800);
				WriteSysEx(outFile, address800, false, s800);
			}

			// Convert temporary patches
			for (const auto &p800 : temporaryPatches800)
			{
				std::cout << "Converting temporary patch: " << ToString(p800.common.name) << std::endl;
				Patch990 p990;
				ConvertPatch800To990(p800, p990);
				WriteSysEx(outFile, BASE_ADDR_990_PATCH_TEMPORARY, true, p990);
			}
			for (const auto &p990 : temporaryPatches990)
			{
				std::cout << "Converting temporary patch: " << ToString(p990.common.name) << std::endl;
				Patch800 p800;
				ConvertPatch990To800(p990, p800);
				WriteSysEx(outFile, BASE_ADDR_800_PATCH_TEMPORARY, false, p800);
			}
			if (sourceDeviceType == DeviceType::JD800 && memory[BASE_ADDR_800_SETUP_TEMPORARY] != UNDEFINED_MEMORY)
			{
				const SpecialSetup800 &s800 = *reinterpret_cast<const SpecialSetup800 *>(memory.data() + BASE_ADDR_800_SETUP_TEMPORARY);
				SpecialSetup990 s990;
				std::cout << "Converting special setup (temporary)" << std::endl;
				ConvertSetup800To990(s800, s990);
				WriteSysEx(outFile, BASE_ADDR_990_SETUP_TEMPORARY, true, s990);
			}
			else if (sourceDeviceType == DeviceType::JD990 && memory[BASE_ADDR_990_SETUP_TEMPORARY] != UNDEFINED_MEMORY)
			{
				const SpecialSetup990 &s990 = *reinterpret_cast<const SpecialSetup990 *>(memory.data() + BASE_ADDR_990_SETUP_TEMPORARY);
				SpecialSetup800 s800;
				std::cout << "Converting special setup (temporary): " << ToString(s990.common.name) << std::endl;
				ConvertSetup990To800(s990, s800);
				WriteSysEx(outFile, BASE_ADDR_800_SETUP_TEMPORARY, false, s800);
			}
		}
	}
	else if (verb == "merge")
	{
		if (sourceDeviceType == DeviceType::JD800)
			std::cout << "Merging " << temporaryPatches800.size() << " JD-800 patches..." << std::endl;
		else if (sourceDeviceType == DeviceType::JD990)
			std::cout << "Merging " << temporaryPatches990.size() << " JD-990 patches..." << std::endl;
		else if (sourceDeviceType == DeviceType::JD800VST)
			std::cout << "Nothing to merge, temporary patches are only supported in JD-800 / JD-990 SysEx dumps..." << std::endl;

		const size_t numPatches = (sourceDeviceType == DeviceType::JD800) ? temporaryPatches800.size() : temporaryPatches990.size();
		const size_t numBanks = (numPatches + 63) / 64;
		size_t sourcePatch = 0;

		for (size_t bank = 0; bank < numBanks; bank++)
		{
			std::string outFilename = argv[firstFileParam + numInputFiles];
			if (numBanks > 1)
			{
				if (outFilename.ends_with(".syx") || outFilename.ends_with(".SYX"))
					outFilename = outFilename.substr(0, outFilename.size() - 3) + std::to_string(bank + 1) + outFilename.substr(outFilename.size() - 4);
				else
					outFilename += "." + std::to_string(bank + 1);
			}

			std::ofstream outFile{outFilename, std::ios::trunc | std::ios::binary};

			for (uint32_t destPatch = 0; destPatch < 64; destPatch++, sourcePatch++)
			{
				if (sourcePatch >= numPatches)
					break;

				if (sourceDeviceType == DeviceType::JD800)
				{
					const uint32_t address800 = BASE_ADDR_800_PATCH_INTERNAL + ((destPatch * 0x03) << 7);
					std::cout << "Adding " << GetPatchIndex(destPatch, 64) << ": " << ToString(temporaryPatches800[sourcePatch].common.name) << std::endl;
					WriteSysEx(outFile, address800, false, temporaryPatches800[sourcePatch]);
				}
				else if (sourceDeviceType == DeviceType::JD990)
				{
					const uint32_t address990 = BASE_ADDR_990_PATCH_INTERNAL + (destPatch << 14);
					std::cout << "Adding " << GetPatchIndex(destPatch, 64) << ": " << ToString(temporaryPatches990[sourcePatch].common.name) << std::endl;
					WriteSysEx(outFile, address990, true, temporaryPatches990[sourcePatch]);
				}
			}
		}

	}
	else if (verb == "list")
	{
		if (sourceDeviceType == DeviceType::JD800)
		{
			std::cout << "Format: JD-800" << std::endl;

			if (memory[BASE_ADDR_800_SYSTEM] != UNDEFINED_MEMORY)
				std::cout << "System data present" << std::endl;
			if (memory[BASE_ADDR_800_PART] != UNDEFINED_MEMORY)
				std::cout << "Part data present" << std::endl;
			if (memory[BASE_ADDR_800_DISPLAY] != UNDEFINED_MEMORY)
			{
				std::cout << "Display data:" << std::endl;
				const char *str = reinterpret_cast<const char *>(memory.data() + BASE_ADDR_800_DISPLAY);
				std::cout << std::string_view{ str, 22 } << std::endl;
				std::cout << std::string_view{ str + 22, 22 } << std::endl;
			}
		}
		else if (sourceDeviceType == DeviceType::JD990)
		{
			std::cout << "Format: JD-990" << std::endl;

			if (memory[BASE_ADDR_990_SYSTEM] != UNDEFINED_MEMORY)
				std::cout << "System data present" << std::endl;
			if (memory[BASE_ADDR_990_PERFORMANCE_TEMPORARY] != UNDEFINED_MEMORY)
				std::cout << "Performance data (temporary) present" << std::endl;
			if (memory[BASE_ADDR_990_PERFORMANCE_PATCHES_TEMPORARY] != UNDEFINED_MEMORY)
				std::cout << "Performance patch data (temporary) present" << std::endl;
			if (memory[BASE_ADDR_990_PERFORMANCE_INTERNAL] != UNDEFINED_MEMORY)
				std::cout << "Performance data (internal) present" << std::endl;
			if (memory[BASE_ADDR_990_SYSTEM_CARD] != UNDEFINED_MEMORY)
				std::cout << "Card system data present" << std::endl;
			if (memory[BASE_ADDR_990_PERFORMANCE_CARD] != UNDEFINED_MEMORY)
				std::cout << "Performance data (card) present" << std::endl;
		}
		else if (sourceDeviceType == DeviceType::JD800VST)
		{
			std::cout << "Format: JD-800 VST / JD-08 / ZC1" << std::endl;
		}

		const uint32_t numPatches = static_cast<uint32_t>((sourceDeviceType == DeviceType::JD800VST) ? vstPatches.size() : 64u);
		for (uint32_t patch = 0; patch < numPatches; patch++)
		{
			const uint32_t address800 = BASE_ADDR_800_PATCH_INTERNAL + ((patch * 0x03) << 7);
			const uint32_t address990 = BASE_ADDR_990_PATCH_INTERNAL + (patch << 14);
			if (sourceDeviceType == DeviceType::JD800)
			{
				if (memory[address800] == UNDEFINED_MEMORY)
					continue;
				const Patch800 &p800 = *reinterpret_cast<const Patch800 *>(memory.data() + address800);
				std::cout << GetPatchIndex(patch, numPatches) << ": " << ToString(p800.common.name) << std::endl;
			}
			else if (sourceDeviceType == DeviceType::JD990)
			{
				if (memory[address990] == UNDEFINED_MEMORY)
					continue;
				const Patch990 &p990 = *reinterpret_cast<const Patch990 *>(memory.data() + address990);
				std::cout << GetPatchIndex(patch, numPatches) << ": " << ToString(p990.common.name) << std::endl;
			}
			else if (sourceDeviceType == DeviceType::JD800VST)
			{
				std::cout << GetPatchIndex(patch, numPatches) << ": " << ToString(vstPatches[patch].name) << std::endl;
			}
		}
		for (uint32_t patch = 0; patch < 64; patch++)
		{
			const uint32_t addressCard990 = BASE_ADDR_990_PATCH_CARD + (patch << 14);
			if (sourceDeviceType == DeviceType::JD990 && memory[addressCard990] != UNDEFINED_MEMORY)
			{
				const Patch990 &p990 = *reinterpret_cast<const Patch990 *>(memory.data() + addressCard990);
				std::cout << GetPatchIndex(patch, 64, true) << ": " << ToString(p990.common.name) << std::endl;
			}
		}
		if (sourceDeviceType == DeviceType::JD800 && memory[BASE_ADDR_800_PATCH_TEMPORARY] != UNDEFINED_MEMORY)
		{
			for (const auto &p800 : temporaryPatches800)
				std::cout << "Temporary patch: " << ToString(p800.common.name) << std::endl;
		}
		else if (sourceDeviceType == DeviceType::JD990 && memory[BASE_ADDR_990_PATCH_TEMPORARY] != UNDEFINED_MEMORY)
		{
			for (const auto &p990 : temporaryPatches990)
				std::cout << "Temporary patch: " << ToString(p990.common.name) << std::endl;
		}

		if (sourceDeviceType == DeviceType::JD800)
		{
			if (memory[BASE_ADDR_800_SETUP_INTERNAL] != UNDEFINED_MEMORY)
			{
				std::cout << "Special setup (internal): JD-800 Drum Set" << std::endl;
			}
			if (memory[BASE_ADDR_800_SETUP_TEMPORARY] != UNDEFINED_MEMORY)
			{
				std::cout << "Special setup (temporary): JD-800 Drum Set" << std::endl;
			}
		}
		else if (sourceDeviceType == DeviceType::JD990)
		{
			if (memory[BASE_ADDR_990_SETUP_INTERNAL] != UNDEFINED_MEMORY)
			{
				const SpecialSetup990 &s990 = *reinterpret_cast<const SpecialSetup990 *>(memory.data() + BASE_ADDR_990_SETUP_INTERNAL);
				std::cout << "Special setup (internal): " << ToString(s990.common.name) << std::endl;
			}
			if (memory[BASE_ADDR_990_SETUP_CARD] != UNDEFINED_MEMORY)
			{
				const SpecialSetup990 &s990 = *reinterpret_cast<const SpecialSetup990 *>(memory.data() + BASE_ADDR_990_SETUP_CARD);
				std::cout << "Special setup (card): " << ToString(s990.common.name) << std::endl;
			}
			if (memory[BASE_ADDR_990_SETUP_TEMPORARY] != UNDEFINED_MEMORY)
			{
				const SpecialSetup990 &s990 = *reinterpret_cast<const SpecialSetup990 *>(memory.data() + BASE_ADDR_990_SETUP_TEMPORARY);
				std::cout << "Special setup (temporary): " << ToString(s990.common.name) << std::endl;
			}
		}
	}

	return 0;
}
