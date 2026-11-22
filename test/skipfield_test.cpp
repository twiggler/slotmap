#include "test_support.hpp"

#include <slotmap/detail/skipfield/skipfield.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <random>
#include <string>
#include <vector>

using Skipfield = Twig::Container::detail::Skipfield<std::vector<unsigned>>;
using slotmap_test::testSeed;

namespace {

// Expected encoding: live slots hold 0. A run of n free slots holds n in its first slot and
// 2, 3, ..., n in the following ones, so both ends of the run hold its length.
std::vector<unsigned> expectedEncoding(const std::vector<bool>& live) {
	std::vector<unsigned> encoding;
	std::size_t i = 0;
	while (i < live.size()) {
		if (live[i]) {
			encoding.push_back(0);
			i++;
			continue;
		}

		unsigned runLength = 0;
		while (i + runLength < live.size() && !live[i + runLength])
			runLength++;

		encoding.push_back(runLength);
		for (unsigned x = 2; x <= runLength; x++)
			encoding.push_back(x);
		i += runLength;
	}
	return encoding;
}

void expectSkipfieldEqualsModel(const Skipfield& skipfield, const std::vector<bool>& live) {
	EXPECT_EQ(std::vector<unsigned>(skipfield.begin(), skipfield.end()), expectedEncoding(live));
}

void toggle(Skipfield& skipfield, std::vector<bool>& live, std::size_t i) {
	live[i] = !live[i];
	if (live[i])
		skipfield.unskip(static_cast<unsigned>(i));
	else
		skipfield.skip(static_cast<unsigned>(i));
}

// Every pattern of free and live slots for a small skipfield, and every single toggle from each pattern.
// Skip and unskip only look at neighbouring runs, so this covers every case of the update logic.
TEST(Skipfield, ExhaustiveToggles) {
	constexpr std::size_t n = 10;
	for (unsigned pattern = 0; pattern < (1u << n); pattern++) {
		Skipfield skipfield(n);
		std::vector<bool> live(n, true);
		for (std::size_t i = 0; i < n; i++)
			if (!(pattern & (1u << i)))
				toggle(skipfield, live, i);
		expectSkipfieldEqualsModel(skipfield, live);

		for (std::size_t i = 0; i < n; i++) {
			auto toggled = skipfield;
			auto toggledLive = live;
			toggle(toggled, toggledLive, i);
			expectSkipfieldEqualsModel(toggled, toggledLive);
		}
		if (::testing::Test::HasFailure())
			FAIL() << "pattern " << pattern;
	}
}

TEST(Skipfield, RandomToggles) {
	constexpr std::size_t n = 1000;
	constexpr int toggles = 10000;
	const auto seed = testSeed();
	SCOPED_TRACE("SLOTMAP_TEST_SEED=" + std::to_string(seed));
	std::mt19937 random(seed);
	std::bernoulli_distribution initiallyLive(.5);
	std::uniform_int_distribution<std::size_t> position(0, n - 1);

	Skipfield skipfield(n);
	std::vector<bool> live(n, true);
	for (std::size_t i = 0; i < n; i++)
		if (!initiallyLive(random))
			toggle(skipfield, live, i);
	expectSkipfieldEqualsModel(skipfield, live);

	for (int t = 0; t < toggles && !::testing::Test::HasFailure(); t++) {
		toggle(skipfield, live, position(random));
		expectSkipfieldEqualsModel(skipfield, live);
	}
}

} // namespace
