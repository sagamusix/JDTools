// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 - 2024 by Johannes Schultz
// License: BSD 3-clause

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

struct uint16le
{
	uint8_t lsb, msb;

	constexpr uint16le(const uint16_t value = 0) noexcept
		: lsb{static_cast<uint8_t>(value)}
		, msb{static_cast<uint8_t>(value >> 8)}
	{
	}

	constexpr operator uint16_t() const noexcept
	{
		return lsb | (msb << 8);
	}

	constexpr uint16_t get() const noexcept
	{
		return *this;
	}
};

struct uint32le
{
	std::array<uint8_t, 4> bytes;

	constexpr uint32le(const uint32_t value = 0) noexcept
		: bytes{{static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 24)}}
	{
	}

	constexpr operator uint32_t() const noexcept
	{
		return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
	}
};

template<typename T>
static bool Read(std::istream &f, T &value)
{
	static_assert(alignof(T) == 1);
	return f.read(reinterpret_cast<char *>(&value), sizeof(value)).good();
}

template<typename T>
static bool ReadVector(std::istream &f, std::vector<T> &value, const size_t numElements)
{
	static_assert(alignof(T) == 1);
	value.resize(numElements);
	return f.read(reinterpret_cast<char *>(value.data()), value.size() * sizeof(T)).good();
}

template<typename T>
static void Write(std::ostream &f, const T &value)
{
	static_assert(alignof(T) == 1);
	f.write(reinterpret_cast<const char *>(&value), sizeof(value));
}

template<typename T>
static void WriteVector(std::ostream &f, const std::vector<T> &value)
{
	static_assert(alignof(T) == 1);
	f.write(reinterpret_cast<const char *>(value.data()), value.size() * sizeof(T));
}

template<typename T, size_t N>
static T SafeTable(const T (&table)[N], uint8_t offset)
{
	if (offset < N)
		return table[offset];
	else
		return table[N - 1];
}

template<size_t Size>
static std::string_view ToString(const std::array<char, Size> &arr)
{
	return std::string_view{ arr.data(), arr.size() };
}

template<size_t N>
static bool CompareMagic(const std::array<char, N> &left, const char(&right)[N + 1])
{
	return !std::memcmp(left.data(), right, N);
}

template<typename T, typename... Targs>
void Reconstruct(T &x, Targs &&... args)
{
	x.~T();
	new (&x) T{ std::forward<Targs>(args)... };
}
