#pragma once

#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>

namespace Twig::Container::detail {

template<class T, class IdT>
struct Item {
	T value;	// value should be the first field to allow reinterpret_cast from T* to Item* and vice-versa.
	IdT id;
};

template<class Iter>
auto makeFilterIter(Iter first, Iter last) {
	return makeValueIter(
		boost::iterators::make_filter_iterator(
			[](const auto& item) { return item.id.generation; },
			first,
			last
		)
	);
}

template<class Iter>
auto makeFilterIter(Iter first) {
	return makeFilterIter(first, first);
}

template<class Iter>
auto makeValueIter(Iter iter) {
	return boost::iterators::make_transform_iterator(
		iter,
		[](auto& item) -> auto& { return item.value; }	// Return by reference.
	);
}

template<template<class> class Vector,
	     class T,
	     class IdT>
class TupleStorage {
public:
	using TItem = Item<T, IdT>;
	using TVector = Vector<TItem>;
	using Index = typename IdT::UInt;
	using Allocator = typename TVector::allocator_type;
	using ValueIterator = decltype(makeValueIter(std::declval<TItem*>()));
	using ConstValueIterator = decltype(makeValueIter(std::declval<const TItem*>()));
	using FilterIterator = decltype(makeFilterIter(std::declval<TItem*>()));
	using ConstFilterIterator = decltype(makeFilterIter(std::declval<const TItem*>()));

	explicit TupleStorage(Index capacity, const Allocator& allocator = {}) :
		_vector(capacity, allocator) {}
	
	IdT id(Index index) const {
		return _vector[index].id;
	}
	
	IdT idByValue(const T& value) const {
		return reinterpret_cast<const TItem&>(value).id;
	}

	void setId(IdT id) {
		_vector[id.index].id = id;
	}

	void setId(Index index, IdT id) {
		_vector[index].id = id;
	}

	T& value(Index index) {
		return _vector[index].value;
	}

	const T& value(Index index) const {
		return _vector[index].value;
	}

	Index grow() {
		_vector.emplace_back();
		auto capacity = Index(std::min(_vector.capacity(), std::size_t(IdT::limits().index)));
		_vector.resize(capacity);
		return capacity;
	}

	ValueIterator begin() {
		return makeValueIter(_vector.data());
	}

	ConstValueIterator begin() const {
		return makeValueIter(_vector.data());
	}

	FilterIterator beginFilter(Index top) {
		return makeFilterIter(_vector.data(), _vector.data() + top);
	}

	FilterIterator endFilter(Index top) {
		return makeFilterIter(_vector.data() + top);
	}

	ConstFilterIterator beginFilter(Index top) const {
		return makeFilterIter(_vector.data(), _vector.data() + top);
	}

	ConstFilterIterator endFilter(Index top) const {
		return makeFilterIter(_vector.data() + top);
	}

private:
	static_assert(std::is_standard_layout_v<T> && offsetof(TItem, value) == 0);

	TVector _vector;
};

}