#include "test_support.hpp"

#include <slotmap/detail/slotmap.hpp>
#include <slotmap/detail/storage/aggregate.hpp>
#include <slotmap/filtered.hpp>
#include <slotmap/slotmap.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <cstdint>
#include <memory_resource>
#include <random>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

using namespace Twig::Container;
using namespace slotmap_test;

namespace {

// ---------------------------------------------------------------------------------------------------------
// Typed tests over every storage layout and iteration mode.
// ---------------------------------------------------------------------------------------------------------

template<class T, unsigned Flags>
using TestSlotmap = Slotmap<T, std::vector, 32, 16, Flags>;

using SlotmapTypes =
	::testing::Types<TestSlotmap<int, 0>, TestSlotmap<int, SlotmapFlags::SKIPFIELD>, TestSlotmap<int, SlotmapFlags::SEGREGATE>,
					 TestSlotmap<int, SlotmapFlags::SKIPFIELD | SlotmapFlags::SEGREGATE>, TestSlotmap<std::string, 0>,
					 TestSlotmap<std::string, SlotmapFlags::SKIPFIELD>,
					 // Wide ids, whose skipfield entries are not promoted to int: a 24-bit index (32-bit entries),
					 // 32-bit fields (64-bit entries, see Id::UInt) and a 48-bit index (64-bit entries).
					 Slotmap<int, std::vector, 32, 8, SlotmapFlags::SKIPFIELD>, Slotmap<int, std::vector, 64, 32, SlotmapFlags::SKIPFIELD>,
					 Slotmap<int, std::vector, 64, 16, SlotmapFlags::SKIPFIELD>>;

template<class TSlotmap>
class SlotmapTest : public ::testing::Test {};
TYPED_TEST_SUITE(SlotmapTest, SlotmapTypes);

TYPED_TEST(SlotmapTest, SelectsStorageLayout) {
	using TSlotmap = TypeParam;
	using T = typename TSlotmap::value_type;
	using Aggregate = detail::AggregateStorage<std::vector, T, typename TSlotmap::Id>;
	constexpr bool expectSegregate = TSlotmap::Segregating || !std::is_standard_layout_v<T>;
	static_assert(expectSegregate != std::is_same_v<typename TSlotmap::Storage, Aggregate>);
}

TYPED_TEST(SlotmapTest, InsertDelete) {
	using TSlotmap = TypeParam;
	using TId = typename TSlotmap::Id;
	using T = typename TSlotmap::value_type;
	TSlotmap slotmap(10);
	Model<TSlotmap> model;

	auto first = slotmap.push(makeValue<T>(1));
	EXPECT_TRUE((first == TId{ 0, 1 }));
	model[0] = { first, makeValue<T>(1) };
	expectEqualsModel(slotmap, model);

	auto middle = slotmap.push(makeValue<T>(2));
	EXPECT_TRUE((middle == TId{ 1, 2 }));
	model[1] = { middle, makeValue<T>(2) };
	expectEqualsModel(slotmap, model);

	auto& last = slotmap.alloc();
	last = makeValue<T>(3);
	model[2] = { TId{ 2, 3 }, makeValue<T>(3) };
	expectEqualsModel(slotmap, model);

	EXPECT_TRUE(slotmap.free(middle));
	model.erase(1);
	expectEqualsModel(slotmap, model);

	EXPECT_FALSE(slotmap.free(middle));
	expectEqualsModel(slotmap, model);

	auto reused = slotmap.push(makeValue<T>(4));
	EXPECT_TRUE((reused == TId{ 1, 4 }));
	model[1] = { reused, makeValue<T>(4) };
	expectEqualsModel(slotmap, model);

	EXPECT_TRUE(slotmap.free(last));
	model.erase(2);
	expectEqualsModel(slotmap, model);

	EXPECT_FALSE(slotmap.free(last));
	expectEqualsModel(slotmap, model);

	auto tail = slotmap.push(makeValue<T>(5));
	EXPECT_TRUE((tail == TId{ 2, 5 }));
	model[2] = { tail, makeValue<T>(5) };
	expectEqualsModel(slotmap, model);
}

// Regression: decrementing a skipfield iterator over a run of free slots used to move the element and the
// skipfield positions apart.
TYPED_TEST(SlotmapTest, DecrementSkipsFreeRun) {
	using TSlotmap = TypeParam;
	using T = typename TSlotmap::value_type;
	TSlotmap slotmap(6);
	std::vector<typename TSlotmap::Id> ids;
	ids.reserve(6);
	for (int n = 0; n < 6; n++)
		ids.push_back(slotmap.push(makeValue<T>(n)));
	slotmap.free(ids[2]);
	slotmap.free(ids[3]);

	auto filtered = make_filtered(slotmap);
	auto it = filtered.begin();
	++it;
	++it; // slot 4
	ASSERT_EQ(*it, makeValue<T>(4));
	--it;
	EXPECT_EQ(*it, makeValue<T>(1));
}

// Every sequence of alloc/free operations up to a fixed depth on a small slotmap, compared with the model
// after each step. Frees alternate between by-id and by-reference to cover both paths.
template<class TSlotmap>
struct ExplorationState {
	TSlotmap slotmap{ 4 };
	Model<TSlotmap> model;
	std::vector<typename TSlotmap::Id> stale;
	int nextValue = 0;
};

template<class TSlotmap>
void explore(const ExplorationState<TSlotmap>& state, int depth) {
	using T = typename TSlotmap::value_type;
	expectEqualsModel(state.slotmap, state.model);
	for (auto id : state.stale)
		EXPECT_EQ(state.slotmap.find(id), nullptr);
	if (::testing::Test::HasFailure() || depth == 0)
		return;

	if (state.model.size() < state.slotmap.capacity()) {
		auto next = state;
		auto value = makeValue<T>(next.nextValue++);
		auto id = next.slotmap.push(value);
		next.model[id.index] = { id, value };
		explore(next, depth - 1);
	}

	for (const auto& [index, entry] : state.model) {
		auto next = state;
		auto id = entry.first;
		bool freed = index % 2 == 0 ? next.slotmap.free(id) : next.slotmap.free(*next.slotmap.find(id));
		EXPECT_TRUE(freed);
		EXPECT_FALSE(next.slotmap.free(id));
		next.model.erase(index);
		next.stale.push_back(id);
		explore(next, depth - 1);
	}
}

TYPED_TEST(SlotmapTest, ExhaustiveOperationSequences) {
	explore(ExplorationState<TypeParam>{}, 10);
}

TYPED_TEST(SlotmapTest, RandomOperationSequences) {
	using TSlotmap = TypeParam;
	using T = typename TSlotmap::value_type;
	const auto seed = testSeed();
	SCOPED_TRACE("SLOTMAP_TEST_SEED=" + std::to_string(seed));
	std::mt19937 random(seed);
	std::uniform_int_distribution<int> operation(0, 9);

	TSlotmap slotmap(64);
	Model<TSlotmap> model;
	std::vector<typename TSlotmap::Id> stale;
	// Generations come from one counter that wraps. Once it has, a stale id may legitimately match a reused slot,
	// so stale ids are only checked before that.
	const std::size_t distinctGenerations = TSlotmap::Id::limits().generation;
	std::size_t allocations = 0;

	for (int step = 0; step < 20000 && !::testing::Test::HasFailure(); step++) {
		const int op = operation(random);
		if (op < 5 && model.size() < slotmap.capacity()) {
			auto value = makeValue<T>(step);
			auto id = slotmap.push(value);
			model[id.index] = { id, value };
			allocations++;
		} else if (op < 9 && !model.empty()) {
			auto entry = std::next(model.begin(), std::uniform_int_distribution<std::size_t>(0, model.size() - 1)(random));
			auto id = entry->second.first;
			EXPECT_TRUE(op % 2 == 0 ? slotmap.free(id) : slotmap.free(*slotmap.find(id)));
			stale.push_back(id);
			model.erase(entry);
		} else if (!stale.empty() && allocations < distinctGenerations) {
			auto id = stale[std::uniform_int_distribution<std::size_t>(0, stale.size() - 1)(random)];
			EXPECT_FALSE(slotmap.free(id));
			EXPECT_EQ(slotmap.find(id), nullptr);
		}

		if (step % 128 == 0)
			expectEqualsModel(slotmap, model);
	}
	expectEqualsModel(slotmap, model);
}

TYPED_TEST(SlotmapTest, WorksWithStdRanges) {
	using TSlotmap = TypeParam;
	using T = typename TSlotmap::value_type;
	using FilteredRange = decltype(make_filtered(std::declval<TSlotmap&>()));
	static_assert(std::bidirectional_iterator<typename TSlotmap::iterator>);
	static_assert(std::convertible_to<typename TSlotmap::iterator, typename TSlotmap::const_iterator>);
	static_assert(std::ranges::bidirectional_range<TSlotmap> && std::ranges::common_range<TSlotmap>);
	static_assert(std::ranges::bidirectional_range<const TSlotmap>);
	static_assert(std::ranges::bidirectional_range<FilteredRange> && std::ranges::view<FilteredRange>);
	static_assert(std::ranges::borrowed_range<FilteredRange>);

	TSlotmap slotmap(8);
	std::vector<typename TSlotmap::Id> ids;
	ids.reserve(6);
	for (int n = 0; n < 6; n++)
		ids.push_back(slotmap.push(makeValue<T>(n)));
	slotmap.free(ids[1]);
	slotmap.free(ids[2]);
	slotmap.free(ids[4]);

	const std::vector<T> live{ makeValue<T>(0), makeValue<T>(3), makeValue<T>(5) };
	EXPECT_TRUE(std::ranges::equal(make_filtered(slotmap), live));
	EXPECT_TRUE(std::ranges::equal(make_filtered(slotmap) | std::views::reverse, live | std::views::reverse));
	EXPECT_EQ(std::ranges::distance(make_filtered(slotmap)), 3);

	auto found = std::ranges::find(make_filtered(slotmap), makeValue<T>(3)); // borrowed: outlives the view
	ASSERT_NE(found, make_filtered(slotmap).end());
	EXPECT_EQ(slotmap.id(*found), ids[3]);

	typename TSlotmap::const_iterator first = slotmap.begin();
	EXPECT_TRUE(first == slotmap.begin());
}

TYPED_TEST(SlotmapTest, IteratesConstSlotmap) {
	using TSlotmap = TypeParam;
	using T = typename TSlotmap::value_type;
	TSlotmap slotmap(4);
	auto id = slotmap.push(makeValue<T>(1));
	slotmap.push(makeValue<T>(2));
	slotmap.free(id);

	const TSlotmap& constSlotmap = slotmap;
	EXPECT_EQ(collectForward(constSlotmap).size(), 2u); // raw iteration includes the free slot
	EXPECT_EQ(collectForward(make_filtered(constSlotmap)), std::vector<T>{ makeValue<T>(2) });
}

// ---------------------------------------------------------------------------------------------------------
// Growth.
// ---------------------------------------------------------------------------------------------------------

struct Element {
	static constexpr std::uint32_t Magic = 0xFEFEFEFE;
	Element() :
		x(Magic) {}
	Element(std::uint32_t v) :
		x(v) {}

	bool operator==(const Element& other) const = default;

	std::uint32_t x;
};

template<unsigned Flags>
using GrowSlotmap = Slotmap<Element, std::vector, 32, 16, SlotmapFlags::GROW | Flags>;

using GrowSlotmapTypes = ::testing::Types<GrowSlotmap<0>, GrowSlotmap<SlotmapFlags::SKIPFIELD>, GrowSlotmap<SlotmapFlags::SEGREGATE>,
										  GrowSlotmap<SlotmapFlags::SKIPFIELD | SlotmapFlags::SEGREGATE>>;

template<class TSlotmap>
class GrowSlotmapTest : public ::testing::Test {};
TYPED_TEST_SUITE(GrowSlotmapTest, GrowSlotmapTypes);

TYPED_TEST(GrowSlotmapTest, GrowsWhenFull) {
	using TSlotmap = TypeParam;
	TSlotmap slotmap(4);
	const auto initialCapacity = slotmap.capacity();

	std::uint32_t itemCount = 0;
	do {
		slotmap.push(itemCount++);
	} while (slotmap.size() < slotmap.capacity());
	slotmap.push(itemCount++);
	EXPECT_GT(slotmap.capacity(), initialCapacity + 1);

	std::uint32_t i = 0;
	for (auto&& element : slotmap) {
		auto id = slotmap.id(element);
		EXPECT_EQ(id.index, i);
		EXPECT_EQ(element, Element(i));
		EXPECT_EQ(id.generation, ++i);
	}

	do {
		const auto& item = slotmap.alloc();
		EXPECT_EQ(item.x, Element::Magic); // new slots are value-initialized
		itemCount++;
	} while (itemCount < slotmap.capacity());
}

// ---------------------------------------------------------------------------------------------------------
// Individual behaviors.
// ---------------------------------------------------------------------------------------------------------

// Ids stay compact: both fields share one integer of the smallest suitable type.
TEST(Slotmap, IdLayout) {
	using detail::Id;
	static_assert(std::is_same_v<Id<16, 8>::UInt, std::uint_least8_t> && sizeof(Id<16, 8>) == 2);
	static_assert(std::is_same_v<Id<32, 16>::UInt, std::uint_least16_t> && sizeof(Id<32, 16>) == 4);
	static_assert(std::is_same_v<Id<32, 8>::UInt, std::uint_least32_t> && sizeof(Id<32, 8>) == 4);
	static_assert(std::is_same_v<Id<64, 32>::UInt, std::uint_least64_t> && sizeof(Id<64, 32>) == 8);
	static_assert(std::is_same_v<Id<64, 16>::UInt, std::uint_least64_t> && sizeof(Id<64, 16>) == 8);
	using ThirtyTwoBitFields = Id<64, 32>;
	using FortyEightBitIndex = Id<64, 16>;
	EXPECT_EQ(ThirtyTwoBitFields::limits().index, 0xFFFFFFFFu);
	EXPECT_EQ(FortyEightBitIndex::limits().index, 0xFFFFFFFFFFFFu);
}

TEST(Slotmap, HandlesGenerationCycle) {
	using TSlotmap = Slotmap<std::uint8_t, std::vector, 2>; // one index bit, one generation bit
	TSlotmap slotmap(1);
	auto& element = slotmap.alloc();
	auto beforeCycleId = slotmap.id(element);

	slotmap.free(element);
	auto& elementAfterCycle = slotmap.alloc();
	auto afterCycleId = slotmap.id(elementAfterCycle);

	EXPECT_EQ(beforeCycleId.index, afterCycleId.index);
	EXPECT_EQ(beforeCycleId.generation, afterCycleId.generation);
}

TEST(Slotmap, ThrowsWhenOutOfSlots) {
	using TSlotmap = Slotmap<unsigned, std::vector, 4>;
	TSlotmap slotmap(0);

	EXPECT_THROW(slotmap.alloc(), OutOfSlots);
}

TEST(Slotmap, Clear) {
	using TSlotmap = Slotmap<std::uint8_t, std::vector, 2>;
	TSlotmap slotmap(2);
	auto capacity = slotmap.capacity();
	auto id = slotmap.id(slotmap.alloc());

	slotmap.clear();

	EXPECT_EQ(slotmap.find(id), nullptr);
	EXPECT_EQ(slotmap.size(), 0u);
	EXPECT_EQ(slotmap.capacity(), capacity);
}

// Memory resource that counts allocations, to check that the slotmap allocates through the given allocator.
class CountingResource : public std::pmr::memory_resource {
public:
	std::size_t bytesAllocated = 0;

private:
	void* do_allocate(std::size_t bytes, std::size_t alignment) override {
		bytesAllocated += bytes;
		return std::pmr::new_delete_resource()->allocate(bytes, alignment);
	}

	void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
		std::pmr::new_delete_resource()->deallocate(p, bytes, alignment);
	}

	bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
		return this == &other;
	}
};

TEST(Slotmap, UsesCustomAllocator) {
	using TSlotmap = Slotmap<std::uint8_t, std::pmr::vector, 16, 8, SlotmapFlags::SKIPFIELD>;
	CountingResource resource;
	TSlotmap slotmap(4, 1, &resource);
	EXPECT_GT(resource.bytesAllocated, 0u);

	auto id = slotmap.push(7);
	ASSERT_NE(slotmap.find(id), nullptr);
	EXPECT_EQ(*slotmap.find(id), 7);
}

TEST(Slotmap, IteratorsAreDefaultConstructible) {
	using AggregateSlotmap = Slotmap<std::uint8_t, std::vector, 2>;
	using SegregateSlotmap = Slotmap<std::string, std::vector, 2, 1, SlotmapFlags::SEGREGATE>;
	using SkipfieldSlotmap = Slotmap<std::uint8_t, std::vector, 2, 1, SlotmapFlags::SKIPFIELD>;

	static_assert(std::default_initializable<typename AggregateSlotmap::iterator>);
	static_assert(std::default_initializable<typename Filtered<AggregateSlotmap, false>::iterator>);
	static_assert(std::default_initializable<typename SegregateSlotmap::iterator>);
	static_assert(std::default_initializable<typename Filtered<SegregateSlotmap, false>::iterator>);
	static_assert(std::default_initializable<typename Filtered<SkipfieldSlotmap, true>::iterator>);
}

} // namespace
