#include "test_support.hpp"

#include <slotmap/detail/skipfield/skipfield.hpp>

#include <gtest/gtest.h>

#include <bitset>
#include <cstddef>
#include <functional>
#include <random>
#include <ranges>
#include <string>
#include <vector>

using Skipfield = Twig::Container::detail::Skipfield<std::vector<unsigned>>;
using slotmap_test::testSeed;

namespace {

// Expected encoding: live slots hold 0. A run of n free slots holds n in its first slot and
// 2, 3, ..., n in the following ones, so both ends of the run hold its length.
std::vector<unsigned> expectedEncoding(const std::vector<bool>& live) {
	std::vector<unsigned> encoding;
	for (auto run : live | std::views::chunk_by(std::equal_to{})) {
		const auto length = static_cast<unsigned>(std::ranges::size(run));
		if (run.front()) {
			encoding.insert(encoding.end(), length, 0u);
		} else {
			encoding.push_back(length);
			auto rest = std::views::iota(2u, length + 1);
			encoding.insert(encoding.end(), rest.begin(), rest.end());
		}
	}
	return encoding;
}

// A skipfield together with the model it should match: one flag per slot, true while the slot is live.
struct ModeledSkipfield {
	explicit ModeledSkipfield(std::size_t size) :
		skipfield(size),
		live(size, true) {}

	void toggle(std::size_t slot) {
		live[slot] = !live[slot];
		if (live[slot])
			skipfield.unskip(static_cast<unsigned>(slot));
		else
			skipfield.skip(static_cast<unsigned>(slot));
	}

	std::vector<unsigned> encoding() const {
		return { skipfield.begin(), skipfield.end() };
	}

	Skipfield skipfield;
	std::vector<bool> live;
};

// A skipfield in which exactly the slots set in `freeSlots` are free.
template<std::size_t Size>
ModeledSkipfield withFreeSlots(const std::bitset<Size>& freeSlots) {
	ModeledSkipfield result(Size);
	for (std::size_t slot = 0; slot < Size; slot++)
		if (freeSlots.test(slot))
			result.toggle(slot);
	return result;
}

// The skipfield's state is just its pattern of free and live slots (checked after every step), and each operation
// toggles one slot. So testing every possible single toggle from every pattern covers every sequence of operations.
// Ten slots suffice because skip() and unskip() only look at the runs next to the toggled slot.
TEST(Skipfield, ExhaustiveToggles) {
	constexpr std::size_t size = 10;
	// Every number below 2^size, read as bits, is one pattern of free slots.
	for (unsigned long long pattern = 0; pattern < (1ull << size); pattern++) {
		const auto start = withFreeSlots(std::bitset<size>(pattern));
		ASSERT_EQ(start.encoding(), expectedEncoding(start.live));

		for (std::size_t slot = 0; slot < size; slot++) {
			auto toggled = start;
			toggled.toggle(slot);
			ASSERT_EQ(toggled.encoding(), expectedEncoding(toggled.live)) << "after toggling slot " << slot;
		}
	}
}

TEST(Skipfield, RandomToggles) {
	constexpr std::size_t size = 1000;
	constexpr int toggles = 10000;
	const auto seed = testSeed();
	SCOPED_TRACE("SLOTMAP_TEST_SEED=" + std::to_string(seed));
	std::mt19937 random(seed);
	std::bernoulli_distribution startsFree(.5);
	std::uniform_int_distribution<std::size_t> anySlot(0, size - 1);

	std::bitset<size> freeSlots;
	for (std::size_t slot = 0; slot < size; slot++)
		freeSlots[slot] = startsFree(random);
	auto skipfield = withFreeSlots(freeSlots);
	ASSERT_EQ(skipfield.encoding(), expectedEncoding(skipfield.live));

	for (int step = 0; step < toggles; step++) {
		const auto slot = anySlot(random);
		skipfield.toggle(slot);
		ASSERT_EQ(skipfield.encoding(), expectedEncoding(skipfield.live)) << "after toggle " << step << ", of slot " << slot;
	}
}

} // namespace
