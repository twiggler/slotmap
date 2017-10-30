#include "stdafx.h"

#include <slotmap/slotmap.hpp>
#include <slotmap/filtered.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
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

namespace Microsoft::VisualStudio::CppUnitTestFramework {
	template<> inline std::wstring ToString<Element>(const Element& e) { return std::to_wstring(e.x); }
}

namespace slotmaptest {		
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
		Assert::AreEqual(slotmapSize, oracleSize);

		auto slotmapsAreEqual = range::equal(slotmap | adaptors::transformed(transformSlot), oracle, isEqual);
		Assert::IsTrue(slotmapsAreEqual);
		auto rangedIsEqual = range::equal(make_filtered(slotmap) | adaptors::transformed(transformSlot), oracle | adaptors::filtered(isActive), isEqual);
		Assert::IsTrue(rangedIsEqual);

		for (auto oracleElement : oracle | adaptors::filtered(isActive)) {
			const auto* slotmapValue = slotmap.find(oracleElement.first);
			Assert::IsNotNull(slotmapValue);
			Assert::AreEqual(oracleElement.second, *slotmapValue);
		}
	}

	template<class TSkipmap, class TOracle>
	void assertSkipmapEqualsOracle(const TSkipmap& skipmap, const TOracle& oracle) {
		auto nodeIter = skipmap.begin();
		auto oracleIter = oracle.begin();

		while (nodeIter != skipmap.end()) {
			if (*oracleIter++) {
				Assert::AreEqual(0u, *nodeIter++);
				continue;
			}

			auto skipBlockLength = 1u;
			while (oracleIter != oracle.end() && !(*oracleIter)) {
				skipBlockLength++;
				oracleIter++;
			}

			Assert::AreEqual(skipBlockLength, *nodeIter++);
			auto x = 2u;
			while (x <= skipBlockLength) {
				Assert::AreEqual(x++, *nodeIter++);
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

	TEST_CLASS(SlotmapTest)
	{
	public:
		TEST_METHOD(creation)
		{
			constexpr auto indexBits = 2u;
			using TSlotmap = Slotmap<int, VectorAdapter, 2 * indexBits>;

			TSlotmap slotmap(1 << indexBits);
			constexpr auto maxCapacity = (1u << indexBits) - 1;
			Assert::AreEqual(unsigned(slotmap.capacity()), maxCapacity);

			TSlotmap slotmap2(maxCapacity);
			Assert::AreEqual(unsigned(slotmap2.capacity()), maxCapacity);
		}

		TEST_METHOD(outOfSlots)
		{
			using TSlotmap = Slotmap<unsigned, VectorAdapter, 4>;
			TSlotmap slotmap(0);

			Assert::ExpectException<OutOfSlots>(
				[&]() { slotmap.alloc(); }
			);
		}

		TEST_METHOD(generationCycle)
		{
			using TSlotmap = Slotmap<uint8_t, VectorAdapter, 4>;
			TSlotmap slotmap(2);
			auto& first = slotmap.alloc();
			auto initialId = slotmap.id(first);

			for (unsigned i = 0; i < 2; i++) {
				auto& second = slotmap.alloc();
				slotmap.free(second);
			}

			slotmap.free(first);
			auto& firstAfterCycle = slotmap.alloc();
			auto cycledId = slotmap.id(firstAfterCycle);

			Assert::AreEqual(initialId.index, cycledId.index);
			Assert::AreEqual(initialId.generation, cycledId.generation);
		}

		TEST_METHOD(clear) {
			using TSlotmap = Slotmap<uint8_t, VectorAdapter, 2>;
			TSlotmap slotmap(2);
			auto capacity = slotmap.capacity();
			auto& element = slotmap.alloc();
			auto id = slotmap.id(element);

			slotmap.clear();

			auto elementPtr = slotmap.find(id);
			Assert::IsNull(elementPtr);
			auto size = slotmap.size();
			Assert::AreEqual(static_cast<decltype(size)>(0), size);
			Assert::AreEqual(capacity, slotmap.capacity());
		}

		TEST_METHOD(insertDelete) {
			constexpr auto capacity = unsigned char(10);
			constexpr auto elementTypes = hana::tuple_t<const char *, string>;
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
				Assert::IsTrue(didRemove);
				assertSlotmapEqualsOracle(slotmap, oracle);

				didRemove = remove(slotmap, oracle, middle);
				Assert::IsFalse(didRemove);
				assertSlotmapEqualsOracle(slotmap, oracle);

				auto idGermany = insert(slotmap, oracle, "Germany");
				Assert::AreEqual(decltype(idGermany.index)(1), idGermany.index);
				assertSlotmapEqualsOracle(slotmap, oracle);

				didRemove = remove(slotmap, oracle, last);
				Assert::IsTrue(didRemove);
				assertSlotmapEqualsOracle(slotmap, oracle);

				auto idSummer = insert(slotmap, oracle, "Summer");
				Assert::AreEqual(decltype(idSummer.index)(2), idSummer.index);
				assertSlotmapEqualsOracle(slotmap, oracle);
			};

			hana::for_each(slotmapTypes, test);
		}

		TEST_METHOD(grow) {
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
				auto capacity = slotmap.capacity();

				auto itemCount = std::uint32_t(0);
				do {
					insert(slotmap, oracle, Element{itemCount++});
				} while (slotmap.size() < slotmap.capacity());
				insert(slotmap, oracle, itemCount++);
				Assert::IsTrue(slotmap.capacity() > capacity + 1); 
				assertSlotmapEqualsOracle(slotmap, oracle);

				do {
					const auto& item = slotmap.alloc();
					Assert::AreEqual(item.x, Element::magic);
					itemCount++;
				} while (itemCount < slotmap.capacity());
			};

			hana::for_each(slotmapTypes, test);
		}

		TEST_METHOD(allocator) {
			constexpr auto numberOfSlots = uint16_t(10);
			constexpr auto slack = size_t(32);
			using tupleNodeSizes = NodeSizes<uint32_t, 32, 16>;
			using scatterTupleNodeSizes = NodeSizes<uint32_t, 32, 16, SlotmapFlags::SCATTER>;
			
			auto tupleAllocator = memory_stack<fixed_block_allocator<>>(tupleNodeSizes::elementBlock(numberOfSlots) + slack);
			static_assert(scatterTupleNodeSizes::elementSize() == tupleNodeSizes::idSize());  // Slotmap element size is equal to size of id
			auto elementAndIdAllocator = memory_stack<fixed_block_allocator<>>((scatterTupleNodeSizes::elementBlock(numberOfSlots) + slack) * 2);
			auto skipfieldAllocator = memory_stack<fixed_block_allocator<>>(tupleNodeSizes::skipfieldBlock(numberOfSlots) + slack);
			
			auto test = [&](auto flags, auto&& allocator) {
				auto tupleUnwind = hana::transform(
					std::make_tuple(std::ref(tupleAllocator), std::ref(elementAndIdAllocator), std::ref(skipfieldAllocator)),
						[](auto& stackAllocator) { return memory_stack_raii_unwind<std::decay_t<decltype(stackAllocator)>>(stackAllocator); }
				);
				
				using TAllocator = std::decay_t<decltype(allocator)>;
				using TSlotmap = Slotmap<uint32_t, VectorAllocatorAdapter<TAllocator>::VectorAdapter, 32, 16, hana::value(flags)>;
				
				TSlotmap slotmap(numberOfSlots, allocator);
			};

			test(hana::int_c<0>, tupleAllocator);
			test(hana::int_c<SlotmapFlags::SKIPFIELD>,
				make_segregator(
					threshold(tupleNodeSizes::skipfieldBlock(numberOfSlots), make_allocator_reference(skipfieldAllocator)),
					threshold(tupleNodeSizes::elementBlock(numberOfSlots), make_allocator_reference(tupleAllocator)),
					null_allocator{}
				)
			);
			test(hana::int_c<SlotmapFlags::SCATTER>, elementAndIdAllocator);
			test(hana::int_c<SlotmapFlags::SKIPFIELD | SlotmapFlags::SCATTER>,
				make_segregator(
					threshold(scatterTupleNodeSizes::skipfieldBlock(numberOfSlots), make_allocator_reference(skipfieldAllocator)),
					threshold(scatterTupleNodeSizes::elementBlock(numberOfSlots), make_allocator_reference(elementAndIdAllocator)),
					null_allocator{}
				)
			);
		}
	};

	TEST_CLASS(Skipfield) {
	public:
		TEST_METHOD(skipUnskip) {
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
	};
}