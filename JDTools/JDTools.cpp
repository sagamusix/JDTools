// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include "convert.h"

#include "jd800.h"
#include "jd990.h"

namespace
{
	constexpr uint8_t SYSEX_DEVICE_ID = 0x10;
}

static void PrintUsage()
{
	std::cout <<
R"(JDTools - Patch conversion utility for Roland JD-800 / JD-990

Usage: JDTools <verb> <filename.syx>
Verbs:
convert  -  Converts from JD-800 to JD-990 SysEx dump or vice versa
list     -  Lists all SysEx contents
)" << std::endl;
}

static void ReadUntilEOX(std::istream &f)
{
	uint8_t ch = 0;
	while (!f.eof() && ch != 0xF7)
	{
		ch = static_cast<uint8_t>(f.get());
	}
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

int main(const int argc, char *argv[])
{
	if (argc != 3)
	{
		PrintUsage();
		return 1;
	}
	static_assert(sizeof(Patch800) == 384);
	static_assert(sizeof(Patch990) == 486);
	static_assert(sizeof(SpecialSetup800) == 5378);
	static_assert(sizeof(SpecialSetup990) == 6524);

	const std::string_view verb = argv[1];
	if (verb != "convert" && verb != "list")
	{
		PrintUsage();
		return 1;
	}

	const std::string inFilename = argv[2];
	std::ifstream inFile(inFilename, std::ios::binary);
	if (!inFile)
	{
		std::cout << "Could open " << inFilename << " for reading!" << std::endl;
		return 2;
	}

	enum class DeviceType
	{
		Undetermined,
		JD800,
		JD990,
	};
	DeviceType sourceDeviceType = DeviceType::Undetermined;

	std::vector<uint8_t> memory(0x1'800'000);  // enough to address JD-990 card setup
	std::vector<uint8_t> message;

	while (!inFile.eof())
	{
		uint8_t ch = 0;
		while (!inFile.eof() && ch != 0xF0)
		{
			ch = static_cast<uint8_t>(inFile.get());
		}
		if (inFile.eof())
			break;

		if (inFile.get() != 0x41)
		{
			std::cout << "Ignoring SysEx message: Not a Roland device" << std::endl;
			ReadUntilEOX(inFile);
			continue;
		}

		// Ignore device ID
		ch = inFile.get();

		ch = static_cast<uint8_t>(inFile.get());
		if (ch != 0x3D && ch != 0x57)
		{
			std::cout << "Ignoring SysEx message: Not a JD-800 or JD-990 message" << std::endl;
			ReadUntilEOX(inFile);
			continue;
		}

		if (ch == 0x3D)
		{
			if(sourceDeviceType == DeviceType::JD990)
			{
				std::cout << "WARNING: File contains mixed JD-800 and JD-990 dumps. Only JD-990 dumps will be processed." << std::endl;
				ReadUntilEOX(inFile);
				continue;
			}
			sourceDeviceType = DeviceType::JD800;
		}
		else if (ch == 0x57)
		{
			if (sourceDeviceType == DeviceType::JD800)
			{
				std::cout << "WARNING: File contains mixed JD-800 and JD-990 dumps. Only JD-800 dumps will be processed." << std::endl;
				ReadUntilEOX(inFile);
				continue;
			}
			sourceDeviceType = DeviceType::JD990;
		}


		ch = static_cast<uint8_t>(inFile.get());
		if (ch != 0x12)
		{
			// TODO: for <list> verb, also show contents of other types
			std::cout << "Ignoring SysEx message: Not a Data Set message" << std::endl;
			ReadUntilEOX(inFile);
			continue;
		}

		message.clear();
		uint8_t checksum = 0;
		while (!inFile.eof() && ch != 0xF7)
		{
			ch = static_cast<uint8_t>(inFile.get());
			if (ch == 0xF7)
				break;
			checksum += ch;
			message.push_back(ch);
		}
		checksum = (~checksum + 1) & 0x7F;
		if (checksum != 0)
		{
			std::cerr << "Invalid SysEx checksum!" << std::endl;
			return 3;
		}

		if ((message.size() < 4 && sourceDeviceType == DeviceType::JD800) || (message.size() < 5 && sourceDeviceType == DeviceType::JD990))
		{
			std::cerr << "WARNING! Skipping SysEx, too short!" << std::endl;
			continue;
		}

		// Remove checksum byte
		message.pop_back();

		uint32_t address = 0;
		if (sourceDeviceType == DeviceType::JD800)
			address = (message[0] << 14) | (message[1] << 7) | message[2];
		else
			address = (message[0] << 21) | (message[1] << 14) | (message[2] << 7) | message[3];

		if(address + message.size() > memory.size())
		{
			std::cerr << "WARNING! Too large address, ignoring SysEx message!" << std::endl;
			continue;
		}

		std::copy(message.begin() + ((sourceDeviceType == DeviceType::JD800) ? 3 : 4), message.end(), memory.begin() + address);
	}

	if(verb == "convert")
	{
		std::string outFilename = inFilename;
		if (sourceDeviceType == DeviceType::JD800)
		{
			outFilename += ".jd990.syx";
			std::cout << "Converting JD-800 patch format to JD-990..." << std::endl;
		}
		else
		{
			outFilename += ".jd800.syx";
			std::cout << "Converting JD-990 patch format to JD-800..." << std::endl;
		}

		std::ofstream outFile(outFilename, std::ios::trunc | std::ios::binary);
		for (uint32_t patch = 0; patch < 64; patch++)
		{
			const uint32_t address800 = (0x05 << 14) + ((patch * 0x03) << 7);
			const uint32_t address990 = (0x06 << 21) + (patch << 14);
			if (sourceDeviceType == DeviceType::JD800)
			{
				const Patch800 &p800 = *reinterpret_cast<const Patch800 *>(memory.data() + address800);
				Patch990 p990;
				std::cout << "Converting I" << (patch / 8 + 1) << (patch % 8 + 1) << ": " << std::string_view(p800.common.name.data(), p800.common.name.size()) << std::endl;
				ConvertPatch800To990(p800, p990);
				WriteSysEx(outFile, address990, true, reinterpret_cast<const uint8_t *>(&p990), sizeof(p990));
			} else
			{
				const Patch990 &p990 = *reinterpret_cast<const Patch990 *>(memory.data() + address990);
				Patch800 p800;
				std::cout << "Converting I" << (patch / 8 + 1) << (patch % 8 + 1) << ": " << std::string_view(p990.common.name.data(), p990.common.name.size()) << std::endl;
				ConvertPatch990To800(p990, p800);
				WriteSysEx(outFile, address800, false, reinterpret_cast<const uint8_t *>(&p800), sizeof(p800));
			}
		}
	}
	else if(verb == "list")
	{
		if (sourceDeviceType == DeviceType::JD800)
			std::cout << "Format: JD-800" << std::endl;
		else
			std::cout << "Format: JD-990" << std::endl;

		for (uint32_t patch = 0; patch < 64; patch++)
		{
			const uint32_t address800 = (0x05 << 14) + ((patch * 0x03) << 7);
			const uint32_t address990 = (0x06 << 21) + (patch << 14);
			if (sourceDeviceType == DeviceType::JD800)
			{
				const Patch800 &p800 = *reinterpret_cast<const Patch800 *>(memory.data() + address800);
				std::cout << "I" << (patch / 8 + 1) << (patch % 8 + 1) << ": " << std::string_view(p800.common.name.data(), p800.common.name.size()) << std::endl;
			}
			else
			{
				const Patch990 &p990 = *reinterpret_cast<const Patch990 *>(memory.data() + address990);
				std::cout << "I" << (patch / 8 + 1) << (patch % 8 + 1) << ": " << std::string_view(p990.common.name.data(), p990.common.name.size()) << std::endl;
			}
		}
	}

	return 0;
}

/*
                            JD-800      JD-990
Patch Temporary Area        00 00 00
Special Setup Temp Area     01 00 00
System Area                 02 00 00
Part Area                   03 00 00
Special Setup Memory Area   04 00 00
Patch Memory Area           05 00 00    06 00 00 00
*/
