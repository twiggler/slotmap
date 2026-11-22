#pragma once

// Shared helpers: a reference model of a slotmap's live contents, comparison against that model in both
// iteration directions, and a reproducible seed for randomized tests.

#include <slotmap/filtered.hpp>
#include <slotmap/slotmap.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace slotmap_test {

// Seed for randomized tests. Set SLOTMAP_TEST_SEED to replay a failure or to explore other sequences.
inline std::uint32_t testSeed() {
	if (const char* seed = std::getenv("SLOTMAP_TEST_SEED"))
		return static_cast<std::uint32_t>(std::stoul(seed));
	return 20170910u;
}

template<class T>
T makeValue(int n) {
	if constexpr (std::is_same_v<T, std::string>)
		return "value " + std::to_string(n);
	else
		return static_cast<T>(n);
}

// Live contents of a slotmap, keyed by slot index so that it iterates in the same order as the slotmap.
template<class TSlotmap>
using Model = std::map<std::size_t, std::pair<typename TSlotmap::Id, typename TSlotmap::value_type>>;

template<class Range>
auto collectForward(Range&& range) {
	std::vector<std::remove_cvref_t<decltype(*range.begin())>> values;
	for (auto it = range.begin(); it != range.end(); ++it)
		values.push_back(*it);
	return values;
}

template<class Range>
auto collectBackward(Range&& range) {
	std::vector<std::remove_cvref_t<decltype(*range.begin())>> values;
	auto it = range.end();
	while (it != range.begin()) {
		--it;
		values.push_back(*it);
	}
	return values;
}

// Checks size, forward and backward filtered iteration, id lookup by value, and find() against the model.
template<class TSlotmap>
void expectEqualsModel(const TSlotmap& slotmap, const Model<TSlotmap>& model) {
	ASSERT_EQ(slotmap.size(), model.size());

	std::vector<typename TSlotmap::value_type> expected;
	for (const auto& [index, entry] : model) {
		const auto& [id, value] = entry;
		expected.push_back(value);

		const auto* found = slotmap.find(id);
		ASSERT_NE(found, nullptr) << "find() lost the element in slot " << index;
		EXPECT_EQ(*found, value);
		EXPECT_TRUE(slotmap.id(*found) == id);
	}

	auto filtered = make_filtered(slotmap);
	EXPECT_EQ(collectForward(filtered), expected);

	std::vector<typename TSlotmap::value_type> expectedBackward(expected.rbegin(), expected.rend());
	EXPECT_EQ(collectBackward(filtered), expectedBackward);
}

} // namespace slotmap_test
