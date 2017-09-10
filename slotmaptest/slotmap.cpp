#include "stdafx.h"
#include <vector>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/equal.hpp>
#include "CppUnitTest.h"
#include <slotmap/slotmap.hpp>
#include <slotmap/filtered.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace boost;
using namespace Twig::Container;

template<class T> using VectorAdapter = vector<T>;

namespace slotmaptest {		
	template<class TSlotmap, class TOracle>
	void assertAreEqual(const TSlotmap& slotmap, const TOracle& oracle) {
		auto transformSlot = [&](const auto& value) { return make_pair(slotmap.id(value), cref(value));  };
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
		oracle[id.index] = { {0, 0}, typename TSlotmap::Value{} };

		return removedElement;
	}

	TEST_CLASS(SlotmapTest)
	{
	public:
		TEST_METHOD(creation)
		{
			constexpr auto indexBits = 2u;
			using TSlotmap = Slotmap<string, VectorAdapter, 2 * indexBits>;

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
			using TSlotmap = Slotmap<string, VectorAdapter, 8>;
			using TOracle = vector<pair<typename TSlotmap::Id, string>>;
			constexpr auto capacity = 10u;

			TSlotmap slotmap(capacity);
			TOracle oracle;
			
			insert(slotmap, oracle, "Roel");
			assertAreEqual(slotmap, oracle);

			auto middle = insert(slotmap, oracle, "Holland");
			assertAreEqual(slotmap, oracle);

			auto last = insert(slotmap, oracle, "Winter");
			assertAreEqual(slotmap, oracle);

			bool didRemove;
			didRemove = remove(slotmap, oracle, middle);
			Assert::IsTrue(didRemove);
			assertAreEqual(slotmap, oracle);

			didRemove = remove(slotmap, oracle, middle);
			Assert::IsFalse(didRemove);
			assertAreEqual(slotmap, oracle);

			didRemove = remove(slotmap, oracle, last);
			Assert::IsTrue(didRemove);
			assertAreEqual(slotmap, oracle);

			insert(slotmap, oracle, "Summer");
			assertAreEqual(slotmap, oracle);
		}

		TEST_METHOD(grow) {
			using TSlotmap = Slotmap<int, VectorAdapter, 32, 16, true>;
			using TOracle = vector<pair<typename TSlotmap::Id, int>>;
			
			TSlotmap slotmap(1);
			TOracle oracle;
			auto capacity = slotmap.capacity();

			auto itemCount = 0;
			do {
				insert(slotmap, oracle, itemCount++);
			} while (slotmap.size() < slotmap.capacity());
			insert(slotmap, oracle, itemCount);
			Assert::IsTrue(slotmap.capacity() > capacity);
			assertAreEqual(slotmap, oracle);
		}
	};
}