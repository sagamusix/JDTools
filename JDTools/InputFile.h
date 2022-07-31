// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 by Johannes Schultz
// License: BSD 3-clause

#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

class InputFile
{
public:
	enum class Type
	{
		SYX,
		MID,
		SVZplugin,
		SVZhardware,
	};

	InputFile(std::istream& file);

	std::vector<uint8_t> NextSysExMessage();

	Type GetType() const { return m_type; }

private:
	uint32_t ReadVarInt();
	uint32_t ReadUint32BE();
	uint16_t ReadUint16BE();
	uint8_t ReadUint8();
	void Skip(uint32_t bytes);

	std::istream &m_file;
	Type m_type = Type::SYX;
	uint32_t m_trackBytesRemain = 0;
	uint8_t m_lastCommand = 0;
};
