#pragma once

#include "detail/slotmap.hpp"
#include <random>
#include <cassert>

namespace Twig::Container {

using detail::OutOfSlots;

template<class, bool> class Filtered;

struct SlotmapFlags {
	static constexpr auto GROW = 1u;
	static constexpr auto SKIPFIELD = 1u << 1;
	static constexpr auto SEGREGATE = 1u << 2;
};

template<class T,
		 template<class> class Vector,
		 unsigned IdBits = sizeof(unsigned) * CHAR_BIT,
		 unsigned GenerationBits = IdBits / 2,
		 unsigned Flags = 0>
class Slotmap : detail::SelectSkipfield<bool(Flags & SlotmapFlags::SKIPFIELD), Vector, IdBits, GenerationBits>::type
{
public:
	static constexpr auto Resizable = bool(Flags & SlotmapFlags::GROW);
	static constexpr auto FastIterable = bool(Flags & SlotmapFlags::SKIPFIELD);
	static constexpr auto Segregating = bool(Flags & SlotmapFlags::SEGREGATE);
	
	using value_type = T;
	using Id = detail::Id<IdBits, GenerationBits>;
	using Storage = typename detail::SelectStorage<Vector, T, Id, bool(Flags & SlotmapFlags::SEGREGATE)>::type;
	using iterator = typename Storage::ValueIterator;
	using const_iterator = typename Storage::ConstValueIterator;
	using Allocator = typename Storage::Allocator;
	using Skipfield = typename detail::SelectSkipfield<FastIterable, Vector, IdBits, GenerationBits>::type;
		
	explicit Slotmap(decltype(Id::index) capacity, decltype(Id::generation) generation = 1, const Allocator& allocator = {}) :
		Skipfield(std::min(capacity, Id::limits().index) + 1u, typename Skipfield::Allocator(allocator)),
			_generation(generation),
			_capacity(std::min(capacity, Id::limits().index)),
			_vector(_capacity, allocator),
			_top(0),
			_freeHead(Id::limits().index),
			_size(0) {}

	void clear() {
		for (typename Id::UInt elementIndex = 0; elementIndex < _top; elementIndex++)
			_vector.setId(elementIndex, { 0, 0 });

		Skipfield::clear();
		_size = 0;
		_top = 0;
		_freeHead = Id::limits().index; // always smaller than _top, indicates that all free slots are at the tail.
	}

	T* find(Id id) {
		assert(id.generation);

		return _vector.id(id.index) == id ? &_vector.value(id.index) : nullptr;
	}

	const T* find(Id id) const {
		assert(id.generation);

		return _vector.id(id.index) == id ? &_vector.value(id.index) : nullptr;
	}

	Id id(const T& value) const {
		auto id = _vector.idByValue(value);
		return id.generation != 0 ? id : Id{ 0, 0 };
	}

	auto capacity() const {
		return _capacity;
	}

	auto size() const {
		return _size;
	}

	T& alloc() {
		typename Id::UInt index;

		if (_freeHead < _top) {
			index = _freeHead;
			_freeHead = _vector.id(index).index;
			 this->unskip(index);
		} else {
			if (_size == _capacity) {
				if (Resizable && _capacity != Id::limits().index) {
					_capacity = _vector.grow();	// Assume the allocator value-initializes the item.
					this->grow();
				}
				else
					throw OutOfSlots();
			}

			index = _top++;
		}

		_vector.setId({ index, _generation });
		_size++;
		_generation = std::max(1, (_generation + 1) & Id::limits().generation); // Generation of zero indicates unused item.

		return _vector.value(index);
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
		auto id = _vector.idByValue(value);
		if (id.generation == 0)
			return false;

		auto oldFreeHead = _freeHead;    // Cannot use std::swap because it is illegal to form a reference to a bit field.
		_freeHead = id.index;
		this->skip(id.index);
		_vector.setId(id.index, { oldFreeHead, 0 });
		_size--;

		return true;
	}

	bool free(Id id) {
		assert(id.generation);
		auto value = find(id);

		if (value != nullptr)
			return free(*value);

		return false;
	}

	iterator begin() {
		return _vector.begin();
	}

	iterator end() {
		return std::next(_vector.begin(), _top);
	}

	const_iterator begin() const {
		return _vector.begin();
	}

	const_iterator end() const {
		return std::next(_vector.begin(), _top);
	}

private:
	using UInt = typename Id::UInt;

	template<class, bool> friend class Filtered;

	UInt _capacity;
	Storage _vector;
	UInt _generation;
	UInt _top;
	UInt _freeHead;
	UInt _size;
};

}