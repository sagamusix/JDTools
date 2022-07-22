#include "InputFile.h"

InputFile::InputFile(std::istream &file)
	: m_file{file}
{
	char magic[4] = {};
	m_file.read(magic, 4);
	m_isSMF = !std::memcmp(magic, "MThd", 4);
	if (m_isSMF)
	{
		uint32_t headerLength = ReadUint32();
		m_file.seekg(headerLength, std::ios::cur);
		m_trackBytesRemain = 0;
	}
	else
	{
		file.seekg(0);
	}
}

std::vector<uint8_t> InputFile::NextSysExMessage()
{
	if (m_isSMF)
	{
		while (!m_file.eof())
		{
			if (!m_trackBytesRemain)
			{
				char magic[4] = {};
				m_file.read(magic, 4);
				if (m_file.eof())
					return {};
				if (std::memcmp(magic, "MTrk", 4))
				{
					std::cerr << "Malformed MIDI file? Unexpected track header value" << std::endl;
					return {};
				}
				m_trackBytesRemain = ReadUint32();
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
					std::vector<uint8_t> message(sysExLength);
					m_file.read(reinterpret_cast<char *>(message.data()), message.size());
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
	else
	{
		if(m_file.eof())
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

uint32_t InputFile::ReadUint32()
{
	char bytes[4] = {};
	m_file.read(bytes, 4);
	m_trackBytesRemain -= 4;
	return (static_cast<uint8_t>(bytes[0]) << 24)
		| (static_cast<uint8_t>(bytes[1]) << 16)
		| (static_cast<uint8_t>(bytes[2]) << 8)
		| static_cast<uint8_t>(bytes[3]);
}

uint16_t InputFile::ReadUint16()
{
	char bytes[2] = {};
	m_file.read(bytes, 2);
	m_trackBytesRemain -= 2;
	return (static_cast<uint8_t>(bytes[0]) << 8)
		| static_cast<uint8_t>(bytes[1]);
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
