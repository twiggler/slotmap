#pragma once

#include "detail/slot_iterator.hpp"
#include "detail/slotmap.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstddef>
#include <utility>

#if defined(_MSC_VER)
#define SLOTMAP_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define SLOTMAP_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

namespace Twig::Container {

using detail::OutOfSlots;

struct SlotmapFlags {
	static constexpr auto GROW = 1u;
	static constexpr auto SKIPFIELD = 1u << 1;
	static constexpr auto SEGREGATE = 1u << 2;
};

template<class T, template<class> class Vector, unsigned IdBits = sizeof(unsigned) * CHAR_BIT, unsigned GenerationBits = IdBits / 2,
		 unsigned Flags = 0>
class Slotmap {
public:
	static constexpr bool Resizable = (Flags & SlotmapFlags::GROW) != 0;
	static constexpr bool FastIterable = (Flags & SlotmapFlags::SKIPFIELD) != 0;
	static constexpr bool Segregating = (Flags & SlotmapFlags::SEGREGATE) != 0;

	using value_type = T;
	using Id = detail::Id<IdBits, GenerationBits>;
	using Storage = typename detail::SelectStorage<Vector, T, Id, Segregating>::type;
	using Skipfield = typename detail::SelectSkipfield<FastIterable, Vector, IdBits, GenerationBits>::type;
	using Allocator = typename Storage::Allocator;
	using iterator = detail::SlotIterator<Slotmap, detail::RawAdvance>;
	using const_iterator = detail::SlotIterator<const Slotmap, detail::RawAdvance>;

	explicit Slotmap(decltype(Id::index) capacity, decltype(Id::generation) generation = 1, const Allocator& allocator = {}) :
		_capacity(std::min(capacity, Id::limits().index)),
		_storage(std::max(_capacity, UInt{ 1 }), allocator),                  // at least one slot, so that Id::null() has one to miss
		_skipfield(_capacity + 1u, typename Skipfield::Allocator(allocator)), // + 1: sentinel that iterators read at top
		_generation(generation),
		_top(0),
		_freeHead(Id::limits().index),
		_size(0) {
		assert((Id{ 1, generation }.valid()));
		markUnused(0, storageSize());
	}

	void clear() {
		markUnused(0, _top);

		_skipfield.clear();
		_size = 0;
		_top = 0;
		_freeHead = Id::limits().index; // not below _top: no free slots to reuse, allocate from the tail.
	}

	// Returns the element with this id, or nullptr if its slot was freed. Id::null() finds nothing.
	// Precondition: the id comes from this slotmap.
	auto* find(this auto& self, Id id) {
		assert(id.index < self.storageSize());

		auto* element = &self._storage.value(id.index);
		return self._storage.id(id.index) == id ? element : nullptr;
	}

	Id id(const T& value) const {
		auto id = _storage.idByValue(value);
		return id.valid() ? id : Id::null();
	}

	auto capacity() const {
		return _capacity;
	}

	auto size() const {
		return _size;
	}

	T& alloc() {
		UInt index;

		if (_freeHead < _top) {
			index = _freeHead;
			_freeHead = _storage.id(index).index;
			_skipfield.unskip(index);
		} else {
			if (_size == _capacity) {
				if (Resizable && _capacity != Id::limits().index) {
					const auto oldCapacity = _capacity;
					_capacity = _storage.grow(); // Assume the allocator value-initializes the item.
					_skipfield.grow();
					markUnused(oldCapacity, _capacity);
				} else
					throw OutOfSlots();
			}

			index = _top++;
		}

		_storage.setId({ index, _generation });
		_size++;
		_generation = Id::evolve(_generation);

		return _storage.value(index);
	}

	Id push(const T& value) {
		auto& slot = alloc();
		slot = value;

		return id(slot);
	}

	Id push(T&& value) {
		auto& slot = alloc();
		slot = std::move(value);

		return id(slot);
	}

	bool free(T& value) {
		auto id = _storage.idByValue(value);
		if (!id.valid())
			return false;

		doFree(id);

		return true;
	}

	// Returns false if the slot was already freed. Id::null() frees nothing.
	// Precondition: the id comes from this slotmap.
	bool free(Id id) {
		assert(id.index < storageSize());

		if (_storage.id(id.index) != id)
			return false;

		doFree(id);

		return true;
	}

	// Iteration visits every slot below the high-water mark, including free ones; see make_filtered.
	iterator begin() {
		return iterator::begin(*this);
	}

	iterator end() {
		return iterator::end(*this);
	}

	const_iterator begin() const {
		return const_iterator::begin(*this);
	}

	const_iterator end() const {
		return const_iterator::end(*this);
	}

private:
	friend struct detail::SlotmapAccess;

	using UInt = typename Id::UInt;

	// Slots that were never allocated (or were cleared) store an id that no key can equal: the sentinel index
	// belongs to no slot, because the capacity is clamped below it. Freed slots store the free-list link
	// instead, which never equals their own index. So a lookup is a single comparison, also for Id::null().
	static constexpr Id UnusedId = Id::free(Id::limits().index);

	std::size_t storageSize() const {
		return std::max<std::size_t>(_capacity, 1);
	}

	void markUnused(std::size_t first, std::size_t last) {
		for (auto index = first; index < last; index++)
			_storage.setId(index, UnusedId);
	}

	void doFree(Id id) {
		auto oldFreeHead = _freeHead; // Cannot use std::swap because it is illegal to form a reference to a bit field.
		_freeHead = id.index;
		_skipfield.skip(id.index);
		_storage.setId(id.index, Id::free(oldFreeHead));
		_size--;
	}

	UInt _capacity;
	Storage _storage;
	SLOTMAP_NO_UNIQUE_ADDRESS Skipfield _skipfield;
	UInt _generation;
	UInt _top;
	UInt _freeHead;
	UInt _size;
};

} // namespace Twig::Container
