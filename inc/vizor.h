#pragma once

#define WIN32_LEAN_AND_MEAN

#include <functional>
#include <algorithm>
#include <cinttypes>
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <format>
#include <string>
#include <vector>
#include <deque>
#include <regex>
#include <boost/asio.hpp>
#include <Windows.h>

extern "C"
{

#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

}

#ifdef NDEBUG
#define RELEASE
#endif // NDEBUG

#ifndef SOLUTION
#ifdef RELEASE
#pragma comment(lib, "vizor.lib")
#else
#pragma comment(lib, "vizor.debug.lib")
#endif // RELEASE
#endif // SOLUTION

#ifdef RELEASE
#define DebugPrintln_(message)
#else
#define DebugPrintln_(message) std::cout << (message) << std::endl
#endif

#define EmptyBody_ throw std::runtime_error("Empty body");

#define ToString_(token) #token

namespace Vizor
{

typedef intptr_t	iptr;
typedef int64_t		i64;
typedef int32_t		i32;
typedef int16_t		i16;
typedef int8_t		i8;

typedef uintptr_t	uptr;
typedef uint64_t	u64;
typedef uint32_t	u32;
typedef uint16_t	u16;
typedef uint8_t		u8;

typedef double		f64;
typedef float		f32;

enum CodecStatus
{
	Success,
	Again,
	Close,
	Error
};

struct Encoder
{
	Encoder(i32 input_width, i32 input_height, i32 output_width, i32 output_height, i32 bitrate, std::function<void (u8 const *buffer, i32 length)> HandlePacket);
	~Encoder();

	CodecStatus Encode(u32 const *pixels);

private:
	CodecStatus Encode(AVFrame *frame);
	void EnsureCleanUp();

	std::function<void (u8 const *, i32)> HandlePacket;
	AVCodec const *codec;
	AVCodecContext *codec_context;
	SwsContext *sws_context;
	AVPacket *packet;
	AVFrame *frame;
	i32 input_width;
	i32 input_height;
	i32 stride;
	i32 pts;
};

struct Decoder
{
	Decoder(std::function<void (u8 const *plane_y, i32 stride_y, u8 const *plane_uv, i32 stride_uv)> HandleFrame);
	~Decoder();

	CodecStatus Decode(u8 *buffer, i32 length);

private:
	CodecStatus Decode(AVPacket *packet);
	void EnsureCleanUp();

	std::function<void (u8 const *, i32, u8 const *, i32)> HandleFrame;
	AVCodec const *codec;
	AVCodecContext *codec_context;
	AVBufferRef *hw_device_ctx;
	AVFrame *sw_frame;
	AVPacket *packet;
	AVFrame *frame;
};

}

using iptr = Vizor::iptr;
using i64  = Vizor::i64;
using i32  = Vizor::i32;
using i16  = Vizor::i16;
using i8   = Vizor::i8;

using uptr = Vizor::uptr;
using u64  = Vizor::u64;
using u32  = Vizor::u32;
using u16  = Vizor::u16;
using u8   = Vizor::u8;

using f64  = Vizor::f64;
using f32  = Vizor::f32;

namespace Vizor::Convert
{

template <typename Type>
inline Type To(std::string const &string) { EmptyBody_ }

template <>
inline i32 To<i32>(std::string const &string)
{
	return stoi(string);
}

template <>
inline i64 To<i64>(std::string const &string)
{
	return stoll(string);
}

template <>
inline u64 To<u64>(std::string const &string)
{
	return stoull(string);
}

template <typename Type>
inline Type To(char **container, i32 length) { EmptyBody_ }

template <>
inline std::vector<std::string> To<std::vector<std::string>>(char **container, i32 length)
{
	std::vector<std::string> output;

	for (i32 index = 0; index < length; ++index) output.emplace_back(container[index]);

	return output;
}

std::string ToString(u8 const *buffer, i32 length, char separator);

template <typename Type>
inline Type To(u8 const *buffer, i32 length, char separator) { EmptyBody_ }

template <>
inline std::string To<std::string>(u8 const *buffer, i32 length, char separator)
{
	return ToString(buffer, length, separator);
}

std::string ToAddress(std::string const &address);

i32 ToPort(std::string const &port);

}

namespace Vizor::FileSystem
{

std::vector<std::string> ReadLines(std::string const &filepath);

template <typename Type>
inline Type Read(std::string const &filepath) { EmptyBody_ }

template <>
inline std::vector<std::string> Read<std::vector<std::string>>(std::string const &filepath)
{
	return ReadLines(filepath);
}

}

namespace Vizor::Net
{

struct StreamSource
{
	virtual bool Send(u8 const *buffer, i32 amount) = 0;

	virtual ~StreamSource() = default;
};

struct StreamTarget
{
	virtual i32 Read(u8 *buffer, i32 length) = 0;

	virtual ~StreamTarget() = default;
};

typedef StreamSource StreamWritter;
typedef StreamTarget StreamReader;

}

namespace Vizor::Net::Udp
{

struct SourceSocket: StreamSource
{
	SourceSocket(i32 port, std::string const &target_address, i32 target_port);

	bool Send(u8 const *buffer, i32 amount) override;

private:
	boost::asio::io_context io_context;
	std::unique_ptr<boost::asio::ip::udp::socket> socket;
	std::unique_ptr<boost::asio::ip::udp::endpoint> target;
};

struct TargetSocket: StreamTarget
{
	TargetSocket(i32 port, std::string const &source_address, i32 source_port);

	i32 Read(u8 *buffer, i32 length) override;

private:
	boost::asio::io_context io_context;
	std::unique_ptr<boost::asio::ip::udp::socket> socket;
	std::unique_ptr<boost::asio::ip::udp::endpoint> source;
};

}

namespace Vizor::Random
{

void Seed();

i32 Generate(i32 min, i32 max);

}

namespace Vizor::Security
{

template <typename Integer>
inline Integer CheckSum(u8 const *packet, uptr count) { EmptyBody_ };

template <>
inline u64 CheckSum<u64>(u8 const *packet, uptr count)
{
	u64 sum = 0;

	for (uptr index = 0; index < count; ++index) sum += packet[index];

    return sum;
}

}

namespace Vizor::System
{

void SleepFor(i32 ms);

void Copy(std::vector<u8> &target, std::vector<u8> const &source);

void SplitBytes(std::vector<std::vector<u8>> &output_packets, std::vector<u8> const &input_packet, uptr data_limit);

void JoinBytes(std::vector<u8> &output_packet, std::vector<std::vector<u8>> const &input_packets);

}

namespace Vizor::Win32
{

std::tuple<i32, i32> GetScreenSize();

struct ScreenGrabber
{
	ScreenGrabber();
	~ScreenGrabber();

	// RGB buffer with size width * height
	// buffer[y * width + x] -> data
	// (data & 0x0000ff) >>  0 == red
	// (data & 0x00ff00) >>  8 == green
	// (data & 0xff0000) >> 16 == blue
	u32 *CaptureFrame();

private:
	BITMAPINFOHEADER bitmap_header;
	HBITMAP bitmap;
	HDC window_context;
	HDC memory_context;
	HWND window;

	i32 width;
	i32 height;

	std::vector<u32> pixels;
};

}
