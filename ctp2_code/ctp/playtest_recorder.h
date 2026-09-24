#pragma once

#include <cstdint>

namespace playtest_recorder
{

void Initialize();
void Shutdown();
bool Enabled();

// Returns the journal sequence number. Durable actions are fsync'd before
// dispatch so a process freeze cannot erase the input that caused it.
std::uint64_t RecordInput(const char *kind, std::uint64_t timeMs, std::int32_t x, std::int32_t y,
                          std::int32_t valueA, std::int32_t valueB, bool durable);
void RecordText(const char *kind, std::uint64_t timeMs, const char *text, bool durable);
void RecordTarget(const char *ldlBlock);

} // namespace playtest_recorder
