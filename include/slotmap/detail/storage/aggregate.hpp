#pragma once

#include <algorithm>
#include <cstddef>
#include <type_traits>

namespace Twig::Container::detail {

template<class T, class IdT>
struct Item {
	T value; // value must be the first member, so that an Item and its value are pointer-interconvertible.
	IdT id;
};

// Stores each value next to its id (array of structures). Requires a standard-layout T.
template<template<class...> class Vector, class T, class IdT>
class AggregateStorage {
public:
	using TItem = Item<T, IdT>;
	using TVector = Vector<TItem>;
	using Index = typename IdT::UInt;
	using Allocator = typename TVector::allocator_type;

	explicit AggregateStorage(Index capacity, const Allocator& allocator = {}) :
		_vector(capacity, allocator) {}

	IdT id(std::size_t index) const {
		return _vector[index].id;
	}

	IdT idByValue(const T& value) const {
		return reinterpret_cast<const TItem&>(value).id;
	}

	void setId(IdT id) {
		_vector[id.index].id = id;
	}

	void setId(std::size_t index, IdT id) {
		_vector[index].id = id;
	}

	auto& value(this auto& self, std::size_t index) {
		return self._vector[index].value;
	}

	Index grow() {
		_vector.emplace_back();
		auto capacity = Index(std::min(_vector.capacity(), std::size_t(IdT::limits().index)));
		_vector.resize(capacity);
		return capacity;
	}

private:
	static_assert(std::is_standard_layout_v<TItem> && offsetof(TItem, value) == 0);

	TVector _vector;
};

} // namespace Twig::Container::detail
