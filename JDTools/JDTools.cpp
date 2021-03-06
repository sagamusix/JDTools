// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "convert.h"
#include "InputFile.h"

#include "jd800.h"
#include "jd990.h"

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
static std::string_view toString(const std::array<char, Size>& arr)
{
	return std::string_view{ arr.data(), arr.size() };
}

static void PrintUsage()
{
	std::cout <<
R"(JDTools - Patch conversion utility for Roland JD-800 / JD-990

Usage:

JDTools convert <input.syx> <output.syx>
Converts from JD-800 to JD-990 SysEx dump or vice versa

JDTools merge <input1.syx> <input2.syx> <input3.syx> ... <output.syx>
Merges files containing temporary patches into banks

JDTools list <input.syx>
Lists all SysEx contents
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
	while(size)
	{
		const size_t amountToCopy = std::min(size , size_t(256));
		if(isJD990)
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
		f.write(reinterpret_cast<const char*>(outMessage.data()), outMessage.size());

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


int main(const int argc, char *argv[])
{
	if (argc < 3)
	{
		PrintUsage();
		return 1;
	}
	static_assert(sizeof(Patch800) == 384);
	static_assert(sizeof(Patch990) == 486);
	static_assert(sizeof(SpecialSetup800) == 5378);
	static_assert(sizeof(SpecialSetup990) == 6524);

	const std::string_view verb = argv[1];
	int numInputFiles = 1;
	if (verb != "convert" && verb != "list" && verb != "merge")
	{
		PrintUsage();
		return 1;
	}
	if ((verb == "convert" && argc != 4) || (verb == "list" && argc != 3) || (verb == "merge" && argc < 4))
	{
		PrintUsage();
		return 1;
	}
	if (verb == "merge")
	{
		numInputFiles = argc - 3;
	}

	enum class DeviceType
	{
		Undetermined,
		JD800,
		JD990,
	};
	DeviceType sourceDeviceType = DeviceType::Undetermined;

	std::vector<uint8_t> memory(0x1'800'000, UNDEFINED_MEMORY);  // enough to address JD-990 card setup
	std::vector<Patch800> temporaryPatches800;
	std::vector<Patch990> temporaryPatches990;
	std::vector<uint8_t> message;

	for (int i = 0; i < numInputFiles; i++)
	{
		const std::string inFilename = argv[2 + i];
		std::ifstream inFile(inFilename, std::ios::binary);
		if (!inFile)
		{
			std::cout << "Could open " << inFilename << " for reading!" << std::endl;
			return 2;
		}

		InputFile inputFile{inFile};
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
			for (size_t i = 4; i < message.size(); i++)
			{
				checksum += message[i];
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
				temporaryPatches800.push_back(*reinterpret_cast<const Patch800*>(memory.data() + BASE_ADDR_800_PATCH_TEMPORARY));
			else if (sourceDeviceType == DeviceType::JD990 && address == BASE_ADDR_990_PATCH_TEMPORARY + 256)
				temporaryPatches990.push_back(*reinterpret_cast<const Patch990*>(memory.data() + BASE_ADDR_990_PATCH_TEMPORARY));
		} while (!message.empty());
	}

	if (sourceDeviceType == DeviceType::Undetermined)
	{
		std::cout << "Input didn't contain any SysEx messages for either JD-800 or JD-990!" << std::endl;
		return 2;
	}

	if(verb == "convert")
	{
		if (sourceDeviceType == DeviceType::JD800)
			std::cout << "Converting JD-800 patch format to JD-990..." << std::endl;
		else
			std::cout << "Converting JD-990 patch format to JD-800..." << std::endl;

		std::string outFilename = argv[3];
		std::ofstream outFile(outFilename, std::ios::trunc | std::ios::binary);
		// Convert patches
		for (uint32_t patch = 0; patch < 64; patch++)
		{
			const uint32_t address800 = BASE_ADDR_800_PATCH_INTERNAL + ((patch * 0x03) << 7);
			const uint32_t address990 = BASE_ADDR_990_PATCH_INTERNAL + (patch << 14);
			if (sourceDeviceType == DeviceType::JD800)
			{
				if (memory[address800] == UNDEFINED_MEMORY)
					continue;
				const Patch800 &p800 = *reinterpret_cast<const Patch800 *>(memory.data() + address800);
				Patch990 p990;
				std::cout << "Converting I" << (patch / 8 + 1) << (patch % 8 + 1) << ": " << toString(p800.common.name) << std::endl;
				ConvertPatch800To990(p800, p990);
				WriteSysEx(outFile, address990, true, p990);
			} else
			{
				if (memory[address990] == UNDEFINED_MEMORY)
					continue;
				const Patch990 &p990 = *reinterpret_cast<const Patch990 *>(memory.data() + address990);
				Patch800 p800;
				std::cout << "Converting I" << (patch / 8 + 1) << (patch % 8 + 1) << ": " << toString(p990.common.name) << std::endl;
				ConvertPatch990To800(p990, p800);
				WriteSysEx(outFile, address800, false, p800);
			}
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
			std::cout << "Converting special setup: " << toString(s990.common.name) << std::endl;
			ConvertSetup990To800(s990, s800);
			WriteSysEx(outFile, address800, false, s800);
		}

		// Convert temporary patches
		for (const auto &p800 : temporaryPatches800)
		{
			std::cout << "Converting temporary patch: " << toString(p800.common.name) << std::endl;
			Patch990 p990;
			ConvertPatch800To990(p800, p990);
			WriteSysEx(outFile, BASE_ADDR_990_PATCH_TEMPORARY, true, p990);
		}
		for (const auto &p990 : temporaryPatches990)
		{
			std::cout << "Converting temporary patch: " << toString(p990.common.name) << std::endl;
			Patch800 p800;
			ConvertPatch990To800(p990, p800);
			WriteSysEx(outFile, BASE_ADDR_800_PATCH_TEMPORARY, false, p800);
		}
		if (sourceDeviceType == DeviceType::JD800 && memory[BASE_ADDR_800_SETUP_TEMPORARY] != UNDEFINED_MEMORY)
		{
			const SpecialSetup800 &s800 = *reinterpret_cast<const SpecialSetup800*>(memory.data() + BASE_ADDR_800_SETUP_TEMPORARY);
			SpecialSetup990 s990;
			std::cout << "Converting special setup (temporary)" << std::endl;
			ConvertSetup800To990(s800, s990);
			WriteSysEx(outFile, BASE_ADDR_990_SETUP_TEMPORARY, true, s990);
		}
		else if (sourceDeviceType == DeviceType::JD990 && memory[BASE_ADDR_990_SETUP_TEMPORARY] != UNDEFINED_MEMORY)
		{
			const SpecialSetup990& s990 = *reinterpret_cast<const SpecialSetup990*>(memory.data() + BASE_ADDR_990_SETUP_TEMPORARY);
			SpecialSetup800 s800;
			std::cout << "Converting special setup (temporary): " << toString(s990.common.name) << std::endl;
			ConvertSetup990To800(s990, s800);
			WriteSysEx(outFile, BASE_ADDR_800_SETUP_TEMPORARY, false, s800);
		}
	}
	else if (verb == "merge")
	{
		if (sourceDeviceType == DeviceType::JD800)
			std::cout << "Merging " << temporaryPatches800.size() << " JD-800 patches..." << std::endl;
		else
			std::cout << "Merging " << temporaryPatches990.size() << " JD-990 patches..." << std::endl;

		const size_t numPatches = ((sourceDeviceType == DeviceType::JD800) ? temporaryPatches800.size() : temporaryPatches990.size());
		const size_t numBanks = (numPatches + 63) / 64;
		size_t sourcePatch = 0;

		for (size_t bank = 0; bank < numBanks; bank++)
		{
			std::string outFilename = argv[2 + numInputFiles];
			if (numBanks > 1)
			{
				if (outFilename.ends_with(".syx") || outFilename.ends_with(".SYX"))
					outFilename = outFilename.substr(0, outFilename.size() - 3) + std::to_string(bank + 1) + outFilename.substr(outFilename.size() - 4);
				else
					outFilename += "." + std::to_string(bank + 1);
			}

			std::ofstream outFile(outFilename, std::ios::trunc | std::ios::binary);

			for (uint32_t destPatch = 0; destPatch < 64; destPatch++, sourcePatch++)
			{
				if(sourcePatch >= numPatches)
					break;

				if (sourceDeviceType == DeviceType::JD800)
				{
					const uint32_t address800 = BASE_ADDR_800_PATCH_INTERNAL + ((destPatch * 0x03) << 7);
					std::cout << "Adding I" << (destPatch / 8 + 1) << (destPatch % 8 + 1) << ": " << toString(temporaryPatches800[sourcePatch].common.name) << std::endl;
					WriteSysEx(outFile, address800, false, temporaryPatches800[sourcePatch]);
				}
				else if (sourceDeviceType == DeviceType::JD990)
				{
					const uint32_t address990 = BASE_ADDR_990_PATCH_INTERNAL + (destPatch << 14);
					std::cout << "Adding I" << (destPatch / 8 + 1) << (destPatch % 8 + 1) << ": " << toString(temporaryPatches990[sourcePatch].common.name) << std::endl;
					WriteSysEx(outFile, address990, true, temporaryPatches990[sourcePatch]);
				}
			}
		}

	}
	else if(verb == "list")
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
				const char* str = reinterpret_cast<const char*>(memory.data() + BASE_ADDR_800_DISPLAY);
				std::cout << std::string_view{ str, str + 22 } << std::endl;
				std::cout << std::string_view{ str + 22, str + 44 } << std::endl;
			}
		}
		else
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

		for (uint32_t patch = 0; patch < 64; patch++)
		{
			const uint32_t address800 = BASE_ADDR_800_PATCH_INTERNAL + ((patch * 0x03) << 7);
			const uint32_t address990 = BASE_ADDR_990_PATCH_INTERNAL + (patch << 14);
			if (sourceDeviceType == DeviceType::JD800)
			{
				if (memory[address800] == UNDEFINED_MEMORY)
					continue;
				const Patch800 &p800 = *reinterpret_cast<const Patch800 *>(memory.data() + address800);
				std::cout << "I" << (patch / 8 + 1) << (patch % 8 + 1) << ": " << toString(p800.common.name) << std::endl;
			}
			else
			{
				if (memory[address990] == UNDEFINED_MEMORY)
					continue;
				const Patch990 &p990 = *reinterpret_cast<const Patch990 *>(memory.data() + address990);
				std::cout << "I" << (patch / 8 + 1) << (patch % 8 + 1) << ": " << toString(p990.common.name) << std::endl;
			}
		}
		for (uint32_t patch = 0; patch < 64; patch++)
		{
			const uint32_t addressCard990 = BASE_ADDR_990_PATCH_CARD + (patch << 14);
			if (sourceDeviceType == DeviceType::JD990 && memory[addressCard990] != UNDEFINED_MEMORY)
			{
				const Patch990 &p990 = *reinterpret_cast<const Patch990*>(memory.data() + addressCard990);
				std::cout << "C" << (patch / 8 + 1) << (patch % 8 + 1) << ": " << toString(p990.common.name) << std::endl;
			}
		}
		if (sourceDeviceType == DeviceType::JD800 && memory[BASE_ADDR_800_PATCH_TEMPORARY] != UNDEFINED_MEMORY)
		{
			for (const auto &p800 : temporaryPatches800)
				std::cout << "Temporary patch: " << toString(p800.common.name) << std::endl;
		}
		else if (sourceDeviceType == DeviceType::JD990 && memory[BASE_ADDR_990_PATCH_TEMPORARY] != UNDEFINED_MEMORY)
		{
			for (const auto &p990 : temporaryPatches990)
				std::cout << "Temporary patch: " << toString(p990.common.name) << std::endl;
		}

		if (sourceDeviceType == DeviceType::JD800)
		{
			if (memory[BASE_ADDR_800_SETUP_INTERNAL] != UNDEFINED_MEMORY)
			{
				const SpecialSetup800 &s800 = *reinterpret_cast<const SpecialSetup800 *>(memory.data() + BASE_ADDR_800_SETUP_INTERNAL);
				std::cout << "Special setup (internal): JD-800 Drum Set" << std::endl;
			}
			if (memory[BASE_ADDR_800_SETUP_TEMPORARY] != UNDEFINED_MEMORY)
			{
				const SpecialSetup800 &s800 = *reinterpret_cast<const SpecialSetup800 *>(memory.data() + BASE_ADDR_800_SETUP_TEMPORARY);
				std::cout << "Special setup (temporary): JD-800 Drum Set" << std::endl;
			}
		}
		else if (sourceDeviceType == DeviceType::JD990)
		{
			if (memory[BASE_ADDR_990_SETUP_INTERNAL] != UNDEFINED_MEMORY)
			{
				const SpecialSetup990 &s990 = *reinterpret_cast<const SpecialSetup990 *>(memory.data() + BASE_ADDR_990_SETUP_INTERNAL);
				std::cout << "Special setup (internal): " << toString(s990.common.name) << std::endl;
			}
			if (memory[BASE_ADDR_990_SETUP_CARD] != UNDEFINED_MEMORY)
			{
				const SpecialSetup990 &s990 = *reinterpret_cast<const SpecialSetup990 *>(memory.data() + BASE_ADDR_990_SETUP_CARD);
				std::cout << "Special setup (card): " << toString(s990.common.name) << std::endl;
			}
			if (memory[BASE_ADDR_990_SETUP_TEMPORARY] != UNDEFINED_MEMORY)
			{
				const SpecialSetup990 &s990 = *reinterpret_cast<const SpecialSetup990*>(memory.data() + BASE_ADDR_990_SETUP_TEMPORARY);
				std::cout << "Special setup (temporary): " << toString(s990.common.name) << std::endl;
			}
		}
	}

	return 0;
}
