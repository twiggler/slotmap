#pragma once

#include <algorithm>
#include <cstddef>

namespace Twig::Container::detail {

// Stores values and ids in parallel arrays (structure of arrays). Works for any T.
template<template<class> class Vector, class T, class IdT>
class SegregateStorage {
public:
	using ValueVector = Vector<T>;
	using IndexVector = Vector<IdT>;
	using Index = typename IdT::UInt;
	using Allocator = typename ValueVector::allocator_type;
	using IndexAllocator = typename IndexVector::allocator_type;

	explicit SegregateStorage(Index capacity, const Allocator& allocator = {}) :
		_values(capacity, allocator),
		_indices(capacity, IndexAllocator{ allocator }) {}

	IdT id(std::size_t index) const {
		return _indices[index];
	}

	IdT idByValue(const T& value) const {
		return _indices[&value - _values.data()];
	}

	void setId(IdT id) {
		_indices[id.index] = id;
	}

	void setId(std::size_t index, IdT id) {
		_indices[index] = id;
	}

	auto& value(this auto& self, std::size_t index) {
		return self._values[index];
	}

	Index grow() {
		_values.emplace_back();
		_values.resize(_values.capacity());
		_indices.resize(_values.capacity());
		auto capacity = Index(std::min(_values.capacity(), std::size_t(IdT::limits().index)));
		return capacity;
	}

private:
	ValueVector _values;
	IndexVector _indices;
};

} // namespace Twig::Container::detail
