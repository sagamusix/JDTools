#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

class InputFile
{
public:
	InputFile(std::istream& file);

	std::vector<uint8_t> NextSysExMessage();

private:
	uint32_t ReadVarInt();
	uint32_t ReadUint32();
	uint16_t ReadUint16();
	uint8_t ReadUint8();
	void Skip(uint32_t bytes);

	std::istream &m_file;
	uint32_t m_trackBytesRemain = 0;
	uint8_t m_lastCommand = 0;
	bool m_isSMF = false;
};
