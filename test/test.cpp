#include "pch.h"

#include "../include/slotmap/slotmap.hpp"
#include "../include/slotmap/filtered.hpp"

using namespace std;
using namespace boost;
using namespace Twig::Container;
using namespace hana::literals;
using namespace foonathan::memory;

template<class T> using VectorAdapter = vector<T>;

template<class RawAllocator>
struct VectorAllocatorAdapter {
	template<class T>
	using VectorAdapter = vector<T, std_allocator<T, RawAllocator>>;
};

struct Element { // Used in grow test
	static constexpr auto magic = std::uint32_t(0xFEFEFEFE);
	Element() : x(magic) {}
	Element(std::uint32_t v) : x(v) {}

	bool operator==(const Element& other) const { return x == other.x; }

	std::uint32_t x;
};

template<class TSlotmap, class TOracle>
void assertSlotmapEqualsOracle(const TSlotmap& slotmap, const TOracle& oracle) {
	auto transformSlot = [&](const auto& value) { return make_pair(slotmap.id(value), std::cref(value));  };
	auto isActive = [](const auto& value) { return value.first.generation != 0; };
	auto isEqual = [](const auto& a, const auto& b) {
		if (a.first == b.first) { // Assume free slots have id equal to { 0, 0 }
			if (a.first.generation != 0)
				return a.second == b.second;
			else
				return true;
		} else
			return false;
	};

	auto oracleSize = distance(oracle | adaptors::filtered(isActive));
	auto slotmapSize = static_cast<decltype(oracleSize)>(slotmap.size());
	EXPECT_EQ(slotmapSize, oracleSize);

	auto slotmapsAreEqual = range::equal(slotmap | adaptors::transformed(transformSlot), oracle, isEqual);
	EXPECT_TRUE(slotmapsAreEqual);
	auto rangedIsEqual = range::equal(make_filtered(slotmap) | adaptors::transformed(transformSlot), oracle | adaptors::filtered(isActive), isEqual);
	EXPECT_TRUE(rangedIsEqual);

	for (auto oracleElement : oracle | adaptors::filtered(isActive)) {
		const auto* slotmapValue = slotmap.find(oracleElement.first);
		ASSERT_NE(slotmapValue, nullptr);
		EXPECT_EQ(oracleElement.second, *slotmapValue);
	}
}

template<class TSkipmap, class TOracle>
void assertSkipmapEqualsOracle(const TSkipmap& skipmap, const TOracle& oracle) {
	auto nodeIter = skipmap.begin();
	auto oracleIter = oracle.begin();

	while (nodeIter != skipmap.end()) {
		if (*oracleIter++) {
			EXPECT_EQ(0u, *nodeIter++);
			continue;
		}

		auto skipBlockLength = 1u;
		while (oracleIter != oracle.end() && !(*oracleIter)) {
			skipBlockLength++;
			oracleIter++;
		}

		EXPECT_EQ(skipBlockLength, *nodeIter++);
		auto x = 2u;
		while (x <= skipBlockLength) {
			EXPECT_EQ(x++, *nodeIter++);
		}
	}
}

template<class TSlotmap, class TOracle, class T>
auto insert(TSlotmap& slotmap, TOracle& oracle, T value) {
	auto id = slotmap.push(value);

	auto element = make_pair(id, value);
	if (id.index < oracle.size())
		oracle[id.index] = std::move(element);
	else
		oracle.push_back(std::move(element));

	return id;
}

template<class TSlotmap, class TOracle>
bool remove(TSlotmap& slotmap, TOracle& oracle, typename TSlotmap::Id id) {
	auto removedElement = slotmap.free(id);
	oracle[id.index] = { {0, 0}, typename TSlotmap::value_type{} };

	return removedElement;
}

TEST(Slotmap, HandlesGenerationCycle) {
	using TSlotmap = Slotmap<uint8_t, VectorAdapter, 2>; 
	TSlotmap slotmap(1);
	auto& element = slotmap.alloc();
	auto beforeCycleId = slotmap.id(element);

	slotmap.free(element);
	auto& elementAfterCycle = slotmap.alloc();
	auto afterCycleId = slotmap.id(elementAfterCycle);

	EXPECT_EQ(beforeCycleId.index, afterCycleId.index);
	EXPECT_EQ(beforeCycleId.generation, afterCycleId.generation);
}

TEST(Slotmap, HandleOutOfSlots) {
	using TSlotmap = Slotmap<unsigned, VectorAdapter, 4>;
	TSlotmap slotmap(0);

	ASSERT_THROW(slotmap.alloc(), OutOfSlots);
}

TEST(Slotmap, HandleClear) {
	using TSlotmap = Slotmap<uint8_t, VectorAdapter, 2>;
	TSlotmap slotmap(2);
	auto capacity = slotmap.capacity();
	auto& element = slotmap.alloc();
	auto id = slotmap.id(element);

	slotmap.clear();

	auto elementPtr = slotmap.find(id);
	EXPECT_EQ(elementPtr, nullptr);
	EXPECT_EQ(slotmap.size(), 0);
	EXPECT_EQ(slotmap.capacity(), capacity);
}

TEST(Slotmap, HandleGrow) {
	constexpr auto FlagTypes = hana::make_tuple(
		hana::int_c<SlotmapFlags::GROW>,
		hana::int_c<SlotmapFlags::GROW | SlotmapFlags::SKIPFIELD>,
		hana::int_c<SlotmapFlags::GROW | SlotmapFlags::SCATTER>,
		hana::int_c<SlotmapFlags::GROW | SlotmapFlags::SKIPFIELD | SlotmapFlags::SCATTER>
	);

	auto slotmapTypes = hana::transform(
		FlagTypes,
		[](auto flag) { return hana::type_c<Slotmap<Element, VectorAdapter, 32, 16, hana::value(flag)>>; }
	);

	auto test = [](auto slotmapType) {
		using TSlotmap = decltype(slotmapType)::type;
		using TId = typename TSlotmap::Id;
		static_assert(TSlotmap::Scattering != is_same_v<typename TSlotmap::Storage, Twig::Container::detail::TupleStorage<VectorAdapter, typename TSlotmap::value_type, TId>>);
		using TOracle = vector<pair<TId, Element>>;

		TSlotmap slotmap(4);
		TOracle oracle;
		auto initialCapacity = slotmap.capacity();

		auto itemCount = std::uint32_t(0);
		do {
			insert(slotmap, oracle, Element{ itemCount++ });
		} while (slotmap.size() < slotmap.capacity());
		insert(slotmap, oracle, itemCount++);
		EXPECT_GT(slotmap.capacity(), initialCapacity + 1);
		assertSlotmapEqualsOracle(slotmap, oracle);

		do {
			const auto& item = slotmap.alloc();
			EXPECT_EQ(item.x, Element::magic);
			itemCount++;
		} while (itemCount < slotmap.capacity());
	};

	hana::for_each(slotmapTypes, test);
}

TEST(Slotmap, InsertDelete) {
	constexpr auto capacity = unsigned char(10);
	constexpr auto elementTypes = hana::tuple_t<const char*, string>;
	constexpr auto FlagTypes = hana::make_tuple(0_c, hana::int_c<SlotmapFlags::SKIPFIELD>);
	auto slotmapTypes = hana::transform(
		hana::cartesian_product(
			hana::make_tuple(elementTypes, FlagTypes)
		),
		[](auto params) {
			return hana::type_c<
				Slotmap<
				decltype(+params[0_c])::type,
				VectorAdapter,
				8, 4,
				hana::value(params[1_c])
				>
			>;
		}
	);

	auto test = [=](auto slotmapType) {
		using TSlotmap = decltype(slotmapType)::type;
		using TId = typename TSlotmap::Id;
		static_assert(is_same_v<typename TSlotmap::value_type, const char*> !=
			is_same_v<typename TSlotmap::Storage, Twig::Container::detail::ScatterStorage<VectorAdapter, typename TSlotmap::value_type, TId>>);
		TSlotmap slotmap(capacity);
		using TOracle = vector<pair<TId, typename TSlotmap::value_type>>;
		TOracle oracle;

		insert(slotmap, oracle, "Roel");
		assertSlotmapEqualsOracle(slotmap, oracle);

		auto middle = insert(slotmap, oracle, "Holland");
		assertSlotmapEqualsOracle(slotmap, oracle);

		auto last = insert(slotmap, oracle, "Winter");
		assertSlotmapEqualsOracle(slotmap, oracle);

		bool didRemove;
		didRemove = remove(slotmap, oracle, middle);
		EXPECT_TRUE(didRemove);
		assertSlotmapEqualsOracle(slotmap, oracle);

		didRemove = remove(slotmap, oracle, middle);
		EXPECT_FALSE(didRemove);
		assertSlotmapEqualsOracle(slotmap, oracle);

		auto idGermany = insert(slotmap, oracle, "Germany");
		EXPECT_EQ(decltype(idGermany.index)(1), idGermany.index);
		assertSlotmapEqualsOracle(slotmap, oracle);

		didRemove = remove(slotmap, oracle, last);
		EXPECT_TRUE(didRemove);
		assertSlotmapEqualsOracle(slotmap, oracle);

		auto idSummer = insert(slotmap, oracle, "Summer");
		EXPECT_EQ(decltype(idSummer.index)(2), idSummer.index);
		assertSlotmapEqualsOracle(slotmap, oracle);
	};

	hana::for_each(slotmapTypes, test);
}

TEST(Slotmap, HandlesCustomAllocator) {
	using TSlotmap = Slotmap<
		uint8_t,
		VectorAllocatorAdapter<temporary_allocator>::VectorAdapter,
		2,
		1,
		SlotmapFlags::SKIPFIELD
	>;
	temporary_allocator alloc;
	TSlotmap slotmap(10, alloc);
	SUCCEED();
}

TEST(Skipfield, HandleSkipAndUnskip) {
	constexpr auto n = 100u;
	constexpr auto t = 10000u;
	using Vector = vector<unsigned>;
	using Skipfield = Twig::Container::detail::Skipfield<Vector>;
	random_device rd;
	mt19937 gen(rd());
	bernoulli_distribution intitialDistribution(.5);
	uniform_int_distribution<unsigned> toggleDistribution(0, n - 1);

	auto skipfield = Skipfield(n);
	vector<bool> oracle(n);

	for (auto i = 0u; i < n; i++) {
		auto bit = intitialDistribution(gen);
		oracle[i] = bit;
		if (!bit)
			skipfield.skip(i);
	}
	assertSkipmapEqualsOracle(skipfield, oracle);

	for (auto t = 0u; t < n; t++) {
		auto i = toggleDistribution(gen);
		oracle[i] = !oracle[i];
		auto node = next(skipfield.begin(), i);
		if (oracle[i])
			skipfield.unskip(i);
		else
			skipfield.skip(i);

		assertSkipmapEqualsOracle(skipfield, oracle);
	}
}
