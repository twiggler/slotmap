#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace Twig::Container::detail {

// Gives iterators and views access to a slotmap's internals; Slotmap befriends it.
struct SlotmapAccess {
	template<class Map>
	static auto& storage(Map& map) {
		return map._storage;
	}

	template<class Map>
	static const auto& skipfield(const Map& map) {
		return map._skipfield;
	}

	template<class Map>
	static std::size_t top(const Map& map) {
		return map._top;
	}
};

// Advance policies decide which slots in [0, top) an iterator visits.

// Visits every slot, including free ones.
struct RawAdvance {
	static std::size_t first(const auto& /*map*/, std::size_t /*end*/) {
		return 0;
	}

	static std::size_t next(const auto& /*map*/, std::size_t index, std::size_t /*end*/) {
		return index + 1;
	}

	static std::size_t prev(const auto& /*map*/, std::size_t index) {
		return index - 1;
	}
};

// Visits slots whose stored id is valid, checking each slot.
struct IdCheckAdvance {
	static bool live(const auto& map, std::size_t index) {
		return SlotmapAccess::storage(map).id(index).valid();
	}

	static std::size_t first(const auto& map, std::size_t end) {
		std::size_t index = 0;
		while (index < end && !live(map, index))
			index++;
		return index;
	}

	static std::size_t next(const auto& map, std::size_t index, std::size_t end) {
		do
			index++;
		while (index < end && !live(map, index));
		return index;
	}

	static std::size_t prev(const auto& map, std::size_t index) {
		do
			index--;
		while (!live(map, index));
		return index;
	}
};

// Jumps over runs of free slots using the skipfield, where both ends of a run hold its length.
// The skipfield has a sentinel entry at index top, which is never part of a run.
struct SkipAdvance {
	static std::size_t first(const auto& map, std::size_t /*end*/) {
		return SlotmapAccess::skipfield(map)[0];
	}

	static std::size_t next(const auto& map, std::size_t index, std::size_t /*end*/) {
		index++;
		return index + SlotmapAccess::skipfield(map)[index];
	}

	static std::size_t prev(const auto& map, std::size_t index) {
		index--;
		return index - SlotmapAccess::skipfield(map)[index];
	}
};

// Bidirectional iterator over the values of a slotmap. Map is the slotmap type, const-qualified for a
// const iterator. Only the index differs between iterators of the same range, so only it is compared.
template<class Map, class Advance>
class SlotIterator {
public:
	using reference = decltype(SlotmapAccess::storage(std::declval<Map&>()).value(std::size_t{}));
	using value_type = std::remove_cvref_t<reference>;
	using pointer = std::add_pointer_t<reference>;
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::bidirectional_iterator_tag;
	using iterator_concept = std::bidirectional_iterator_tag;

	SlotIterator() = default;

	// A const iterator can be made from a mutable one.
	template<class MutableMap>
		requires(std::is_const_v<Map> && std::is_same_v<const MutableMap, Map>)
	SlotIterator(const SlotIterator<MutableMap, Advance>& other) :
		_map(other._map),
		_index(other._index),
		_end(other._end) {}

	static SlotIterator begin(Map& map) {
		const auto end = SlotmapAccess::top(map);
		return SlotIterator(map, Advance::first(map, end), end);
	}

	static SlotIterator end(Map& map) {
		const auto end = SlotmapAccess::top(map);
		return SlotIterator(map, end, end);
	}

	reference operator*() const {
		return SlotmapAccess::storage(*_map).value(_index);
	}

	pointer operator->() const {
		return std::addressof(**this);
	}

	SlotIterator& operator++() {
		_index = Advance::next(*_map, _index, _end);
		return *this;
	}

	SlotIterator operator++(int) {
		auto previous = *this;
		++*this;
		return previous;
	}

	SlotIterator& operator--() {
		_index = Advance::prev(*_map, _index);
		return *this;
	}

	SlotIterator operator--(int) {
		auto previous = *this;
		--*this;
		return previous;
	}

	friend bool operator==(const SlotIterator& a, const SlotIterator& b) {
		return a._index == b._index;
	}

	// Slot index the iterator points at.
	std::size_t index() const {
		return _index;
	}

private:
	template<class, class>
	friend class SlotIterator;

	SlotIterator(Map& map, std::size_t index, std::size_t end) :
		_map(&map),
		_index(index),
		_end(end) {}

	Map* _map = nullptr;
	std::size_t _index = 0;
	std::size_t _end = 0;
};

} // namespace Twig::Container::detail
