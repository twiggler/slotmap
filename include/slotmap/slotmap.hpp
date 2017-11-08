#pragma once

#include "detail/slotmap.hpp"
#include <random>
#include <cassert>

namespace Twig::Container {

using detail::OutOfSlots;

struct SlotmapFlags {
	static constexpr auto GROW = 1u;
	static constexpr auto SKIPFIELD = 1u << 1;
	static constexpr auto SCATTER = 1u << 2;
};

template<class T,
	unsigned IdBits = sizeof(unsigned) * CHAR_BIT,
	unsigned GenerationBits = IdBits / 2,
	unsigned Flags = 0>
struct NodeSizes {
	using Id = detail::Id<IdBits, GenerationBits>;
	using UInt = typename Id::UInt;

	static constexpr auto elementSize() {
		if (!std::is_standard_layout_v<T> || Flags & SlotmapFlags::SCATTER)
			return sizeof(T);
		else
			return sizeof(detail::Item<T, Id>);
	}

	static constexpr auto idSize() {
		return sizeof(Id);
	}

	static constexpr auto skipnodeSize() {
		return sizeof(UInt);
	}

	static constexpr auto elementBlock(UInt capacity) {
		return capacity * elementSize();
	}

	static constexpr auto idBlock(UInt capacity) {
		return capacity * idSize();
	}

	static constexpr auto skipfieldBlock(UInt capacity) {
		return (capacity + 1) * skipnodeSize();
	}
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
	static constexpr auto Scattering = bool(Flags & SlotmapFlags::SCATTER);
	
	using value_type = T;
	using NodeSizes = NodeSizes<T, IdBits, GenerationBits, Flags>;
	using Id = detail::Id<IdBits, GenerationBits>;
	using Storage = typename detail::SelectStorage<Vector, T, Id, bool(Flags & SlotmapFlags::SCATTER)>::type;
	using iterator = typename Storage::ValueIterator;
	using const_iterator = typename Storage::ConstValueIterator;
	using Allocator = typename Storage::Allocator;
	using Skipfield = typename detail::SelectSkipfield<FastIterable, Vector, IdBits, GenerationBits>::type;
		
	explicit Slotmap(decltype(Id::index) capacity, const Allocator& allocator = {}) :
		Skipfield(std::min(capacity, Id::limits().index) + 1u, typename Skipfield::Allocator(allocator)),
		_randomEngine(std::random_device{}()),
		_capacity(std::min(capacity, Id::limits().index)),
		_vector(_capacity, allocator),
		_top(0),
		_freeHead(Id::limits().index),
		_size(0)
	{
		_sampleGeneration();
	}

	void clear() {
		for (typename Id::UInt elementIndex = 0; elementIndex < _top; elementIndex++)
			_vector.id(elementIndex).generation = 0;

		Skipfield::clear();
		_size = 0;
		_sampleGeneration();
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
				if (Resizable && _capacity != Id::limits().index)
					_capacity = _vector.grow();	// Assume the allocator value-initializes the item.
				else
					throw OutOfSlots();
			}

			index = _top++;
			this->grow();
		}

		auto& id = _vector.id(index);
		id = { index, _generation };
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
		auto& id = _vector.idByValue(value);
		if (id.generation == 0)
			return false;

		auto oldFreeHead = _freeHead;    // Cannot use std::swap because it is illegal to form a reference to a bit field.
		_freeHead = id.index;
		this->skip(id.index);
		id = { oldFreeHead, 0 };
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

	void _sampleGeneration() {
		std::uniform_int_distribution<unsigned long long> _generationDistribution(1, Id::limits().generation);
		_generation = static_cast<typename Id::UInt>(_generationDistribution(_randomEngine)); // uniform_int_distribution does not support unsigned char
	}

	UInt _capacity;
	Storage _vector;
	std::mt19937 _randomEngine;
	UInt _generation;
	UInt _top;
	UInt _freeHead;
	UInt _size;
};

}