// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#include "InputFile.hpp"
#include "Utils.hpp"

InputFile::InputFile(std::istream &file)
	: m_file{file}
{
	std::array<char, 4> magic{};
	Read(m_file, magic);

	if (CompareMagic(magic, "MThd"))
	{
		m_type = Type::MID;
	}
	else if (CompareMagic(magic, "SVZa"))
	{
		m_file.seekg(16);
		std::array<char, 4> type{};
		Read(m_file, type);
		if (CompareMagic(type, "EXTa"))
			m_type = Type::SVZplugin;
		else if (CompareMagic(type, "DIFa"))
			m_type = Type::SVZhardware;
	}
	else if (magic[2] == 'S' && magic[3] == 'V')
	{
		Read(m_file, magic);
		if (CompareMagic(magic, "D5\x00\x00"))
			m_type = Type::SVD;
	}

	if (m_type == Type::MID)
	{
		uint32_t headerLength = ReadUint32BE();
		m_file.seekg(headerLength, std::ios::cur);
		m_trackBytesRemain = 0;
	}
	else
	{
		m_file.seekg(0);
	}
}

std::vector<uint8_t> InputFile::NextSysExMessage()
{
	if (m_type == Type::MID)
	{
		while (!m_file.eof())
		{
			if (!m_trackBytesRemain)
			{
				std::array<char, 4> magic{};
				Read(m_file, magic);
				if (m_file.eof())
					return {};
				if (!CompareMagic(magic, "MTrk"))
				{
					std::cerr << "Malformed MIDI file? Unexpected track header value" << std::endl;
					return {};
				}
				m_trackBytesRemain = ReadUint32BE();
			}

			// Skip delay value
			ReadVarInt();

			uint8_t data1 = ReadUint8();
			if (data1 == 0xFF)
			{
				Skip(1);
				Skip(ReadVarInt());
				continue;
			}
			uint8_t command = m_lastCommand;
			if (data1 & 0x80)
			{
				// Command byte (if not present, use running status for channel messages)
				command = data1;
				if (data1 < 0xF0)
				{
					m_lastCommand = data1;
					data1 = ReadUint8();
				}
			}

			switch (command & 0xF0)
			{
			case 0x80:
			case 0x90:
			case 0xA0:
			case 0xB0:
			case 0xE0:
				Skip(1);
				break;
			case 0xC0:
			case 0xD0:
				break;
			case 0xF0:
				switch (command & 0x0F)
				{
				case 0x00:
				case 0x07:
				{
					uint32_t sysExLength = ReadVarInt();
					std::vector<uint8_t> message;
					ReadVector(m_file, message, sysExLength);
					m_trackBytesRemain -= sysExLength;
					if (!message.empty() && message.back() != 0xF7)
					{
						std::cerr << "NOT IMPLEMENTED: Continued SysEx message" << std::endl;
					}
					return message;
				}
				case 0x01:
				case 0x03:
					Skip(1);
				case 0x02:
					Skip(2);
				default:
					break;
				}
				break;
			}
		}
	}
	else if (m_type == Type::SYX)
	{
		if (m_file.eof())
			return {};

		uint8_t ch = 0;
		while (!m_file.eof() && ch != 0xF0)
		{
			ch = ReadUint8();
		}
		
		std::vector<uint8_t> message;
		while (!m_file.eof() && ch != 0xF7)
		{
			ch = ReadUint8();
			message.push_back(ch);
		}
		return message;
	}
	return {};
}

uint32_t InputFile::ReadVarInt()
{
	uint8_t b = ReadUint8();
	uint32_t value = (b & 0x7F);

	while (!m_file.eof() && (b & 0x80) != 0)
	{
		b = ReadUint8();
		value <<= 7;
		value |= (b & 0x7F);
	}
	return value;
}

uint32_t InputFile::ReadUint32BE()
{
	std::array<uint8_t, 4> bytes{};
	Read(m_file, bytes);
	m_trackBytesRemain -= 4;
	return (bytes[0] << 24)
		| (bytes[1] << 16)
		| (bytes[2] << 8)
		| bytes[3];
}

uint16_t InputFile::ReadUint16BE()
{
	std::array<uint8_t, 2> bytes;
	Read(m_file, bytes);
	m_trackBytesRemain -= 2;
	return (bytes[0] << 8)
		| bytes[1];
}

uint8_t InputFile::ReadUint8()
{
	m_trackBytesRemain--;
	return static_cast<uint8_t>(m_file.get());
}

void InputFile::Skip(uint32_t bytes)
{
	m_file.ignore(bytes);
	m_trackBytesRemain -= bytes;
}
