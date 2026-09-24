#include "ctp/playtest_recorder.h"
#include "ctp/c3.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
namespace playtest_recorder
{
namespace
{

FILE *s_journal = nullptr;
std::uint64_t s_sequence = 0;
std::uint64_t s_lastInputSequence = 0;
std::chrono::steady_clock::time_point s_started;

std::uint64_t s_lastTargetInputSequence = 0;
std::string Escape(const char *text)
{
	std::string escaped;
	if (!text)
		return escaped;
	for (unsigned char ch : std::string(text))
	{
		switch (ch)
		{
		case '\\':
			escaped += "\\\\";
			break;
		case '"':
			escaped += "\\\"";
			break;
		case '\n':
			escaped += "\\n";
			break;
		case '\r':
			escaped += "\\r";
			break;
		case '\t':
			escaped += "\\t";
			break;
		default:
			if (ch >= 0x20)
				escaped += static_cast<char>(ch);
			break;
		}
	}
	return escaped;
}

std::uint64_t ElapsedMs()
{
	return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
	                                      std::chrono::steady_clock::now() - s_started)
	                                      .count());
}

void Flush(bool durable)
{
	if (!s_journal)
		return;
	std::fflush(s_journal);
#ifdef _WIN32
	if (durable)
		_commit(_fileno(s_journal));
#else
	if (durable)
		::fsync(fileno(s_journal));
#endif
}

} // namespace

void Initialize()
{
	if (s_journal)
		return;
	const char *directory = std::getenv("CTP2_PLAYTEST_DIR");
	if (!directory || !*directory)
		return;
	std::string path = std::string(directory) + "/input.jsonl";
	s_journal = std::fopen(path.c_str(), "a");
	if (!s_journal)
		return;
	s_started = std::chrono::steady_clock::now();
	std::fprintf(s_journal, "{\"seq\":%llu,\"elapsed_ms\":0,\"kind\":\"recorder_started\"}\n",
	             static_cast<unsigned long long>(++s_sequence));
	Flush(true);
}

void Shutdown()
{
	if (!s_journal)
		return;
	std::fprintf(s_journal, "{\"seq\":%llu,\"elapsed_ms\":%llu,\"kind\":\"recorder_stopped\"}\n",
	             static_cast<unsigned long long>(++s_sequence),
	             static_cast<unsigned long long>(ElapsedMs()));
	Flush(true);
	std::fclose(s_journal);
	s_journal = nullptr;
}

bool Enabled() { return s_journal != nullptr; }

std::uint64_t RecordInput(const char *kind, std::uint64_t timeMs, std::int32_t x, std::int32_t y,
                          std::int32_t valueA, std::int32_t valueB, bool durable)
{
	if (!s_journal)
		return 0;
	s_lastInputSequence = ++s_sequence;
	std::fprintf(s_journal,
	             "{\"seq\":%llu,\"elapsed_ms\":%llu,\"sdl_time_ms\":%llu,"
	             "\"kind\":\"%s\",\"x\":%d,\"y\":%d,\"a\":%d,\"b\":%d}\n",
	             static_cast<unsigned long long>(s_lastInputSequence),
	             static_cast<unsigned long long>(ElapsedMs()),
	             static_cast<unsigned long long>(timeMs), Escape(kind).c_str(), static_cast<int>(x),
	             static_cast<int>(y), static_cast<int>(valueA), static_cast<int>(valueB));
	Flush(durable);
	return s_lastInputSequence;
}

void RecordText(const char *kind, std::uint64_t timeMs, const char *text, bool durable)
{
	if (!s_journal)
		return;
	s_lastInputSequence = ++s_sequence;
	std::fprintf(s_journal,
	             "{\"seq\":%llu,\"elapsed_ms\":%llu,\"sdl_time_ms\":%llu,"
	             "\"kind\":\"%s\",\"text\":\"%s\"}\n",
	             static_cast<unsigned long long>(s_lastInputSequence),
	             static_cast<unsigned long long>(ElapsedMs()),
	             static_cast<unsigned long long>(timeMs), Escape(kind).c_str(),
	             Escape(text).c_str());
	Flush(durable);
}

void RecordTarget(const char *ldlBlock)
{
	if (!s_journal || s_lastInputSequence == 0 || s_lastTargetInputSequence == s_lastInputSequence)
		return;
	s_lastTargetInputSequence = s_lastInputSequence;
	std::fprintf(s_journal,
	             "{\"seq\":%llu,\"elapsed_ms\":%llu,\"kind\":\"dispatch\","
	             "\"input_seq\":%llu,\"target\":\"%s\"}\n",
	             static_cast<unsigned long long>(++s_sequence),
	             static_cast<unsigned long long>(ElapsedMs()),
	             static_cast<unsigned long long>(s_lastInputSequence),
	             Escape(ldlBlock ? ldlBlock : "").c_str());
	Flush(false);
}

} // namespace playtest_recorder
