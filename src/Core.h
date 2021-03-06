#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <thread>
#include <atomic>
#include <mutex>

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

typedef std::atomic<u32> atomic_u32;

static constexpr size_t MAX_DEBUG_MSG_SIZE = 1024;
thread_local static char g_debugFmtBuffer[MAX_DEBUG_MSG_SIZE];
thread_local static char g_debugMsgBuffer[MAX_DEBUG_MSG_SIZE];

using ScopedLock = std::lock_guard<std::mutex>;

template <typename Invokable>
struct ScopeExit
{
	ScopeExit(Invokable to_invoke) : m_to_invoke(to_invoke) {}
	~ScopeExit() { m_to_invoke(); }

	Invokable m_to_invoke;
};

template <typename Invokable>
ScopeExit<Invokable> MakeScopeExit(Invokable to_invoke)
{
	return ScopeExit<Invokable>(to_invoke);
}

#define JOIN_STRING2(s1, s2) JOIN_STRING2_INTERNAL(s1, s2)
#define JOIN_STRING2_INTERNAL(s1, s2) s1 ## s2

// NOTE(): This captures by ref, so beware lifetimes (hopefully nothing complex goes in here anyways)!
#define ON_SCOPE_EXIT(code) \
	auto JOIN_STRING2(scope_exit, __LINE__)  = MakeScopeExit([&]() {code;});

u64 static constexpr PLATFORM_DEFAULT_ALIGNMENT = alignof(std::max_align_t);

static constexpr u64 Kilobyte(u64 num)
{
	return num * 1024i64;
}

static constexpr u64 Megabyte(u64 num)
{
	return num * 1024i64 * 1024i64;
}

static constexpr u64 Gigabyte(u64 num)
{
	return num * 1024i64 * 1024i64 * 1024i64;
}

static constexpr size_t BytesToKiloBytes(size_t bytes)
{
	return bytes / (1024Ui64);
}

static constexpr size_t BytesToMegaBytes(size_t bytes)
{
	return bytes / (1024Ui64 * 1024Ui64);
}

static constexpr size_t BytesToGigaBytes(size_t bytes)
{
	return bytes / (1024Ui64 * 1024Ui64 * 1024Ui64);
}

template <typename T>
bool IsPow2(T value)
{
	return (value & T(1)) == 0;
}

// Returns the position the null terminator was written to.
int MiniPrintf(char* buffer, size_t bufferLen, const char *fmt, bool appendNewline, ...);

struct Log
{
	enum Category
	{
		Default,
		Assert,
		GfxDevice,
		Win32,
		Input,
		IO,

		EnumCount,
		EnumFirst = Default,
	};

	static constexpr char* CategoryStrings[Category::EnumCount] = 
	{
		"Default",
		"ASSERT",
		"GfxDevice",
		"Win32",
		"Input",
		"IO"
	};
};

void DebugPrintf(char const* file, int line, char const* fmt, Log::Category category, ...);

// Use for very basic 1:1 conversion from c_str to w_str. Doesnt take care of cases where the number
// bytes might be a mismatch (unicode)!
void CStrToWChar(char const* src_c_str, wchar_t* dst_w_str, u32 str_len);

#ifdef _DEBUG
#define LOG(category, format, ...) DebugPrintf(__FILE__, __LINE__, format, category, __VA_ARGS__); 
#else
#define LOG(format, ...)
#endif

#define UNUSED(x) (void)(x)

#ifdef _DEBUG
#define ASSERT(x) assert(x) // TODO(): This should be messagebox so we can actually continue exection
#define ASSERT_F(x, format, ...) if (!(x)) { LOG(Log::Assert, format, __VA_ARGS__); assert(x); }
#define ASSERT_FAIL() assert(false)
#define ASSERT_FAIL_F(format, ...) ASSERT_F(false, format, __VA_ARGS__)
#define DEBUG_CODE(x) x
#else
#define ASSERT(x) 
#define ASSERT_F(x, format, ...)  
#define ASSERT_FAIL()
#define ASSERT_FAIL_F(format, ...)
#define DEBUG_CODE(x)
#endif

#ifdef _DEBUG
	#define VERIFY(x)				\
	{								\
		bool verify_res_res = (x);	\
		ASSERT(verify_res_res);		\
	}
#else
	#define VERIFY(x) x		
#endif

#define ARRAY_SIZE(x) _countof(x)

template <class T>
static T max(const T& a, const T& b)
{
	return (b > a) ? b : a;
}

template <class T>
static T min(const T& a, const T& b)
{
	return (b < a) ? b : a;
}

template<typename T>
static T Clamp(const T& x, const T& low, const T& high)
{
	return x < low ? low : (x > high ? high : x);
}

static bool NearlyEqual(f32 a, f32 b, f32 epsilon = /* 0.00001 */ 0.000000001)
{
	return fabs(a - b) <= epsilon;
}

inline void memzero(void* dst, size_t size)
{
	memset(dst, 0, size);
}

template<typename T>
inline void MemZeroSafe(T& data)
{
	static_assert(std::is_trivially_copyable<T>::value, "Cannot memzero non-trivial type!");
	memzero(&data, sizeof(T));
}

template<typename T>
inline void MemZeroSafe(T* data)
{
	static_assert(std::is_trivially_copyable<T>::value, "Cannot memzero non-trivial type!");
	memzero(data, sizeof(T));
}

template<typename T>
inline void MemZeroUnsafe(T& data)
{
	memzero(&data, sizeof(T));
}