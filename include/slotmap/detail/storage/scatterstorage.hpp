#pragma once

#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <memory>

namespace Twig::Container::detail {

template<class ValueIter, class IndexIter>
auto makeFilterIter(ValueIter firstValue, ValueIter lastValue, IndexIter firstIndex, IndexIter lastIndex) {
	return boost::make_transform_iterator(
		boost::make_filter_iterator(
			[](const auto& zip) { return zip.second.generation; },
			boost::make_zip_iterator(std::make_pair(firstValue, firstIndex)),
			boost::make_zip_iterator(std::make_pair(lastValue, lastIndex))
		),
		[](const auto& filteredZip) -> auto& { return filteredZip.first; }
	);
}

template<class ValueIter, class IndexIter>
auto makeFilterIter(ValueIter valueIter, IndexIter indexIter) {
	return makeFilterIter(valueIter, valueIter, indexIter, indexIter);
}

template<template<class> class Vector,
		 class T,
		 class IdT>
class ScatterStorage {
public:
	using ValueVector = Vector<T>;
	using IndexVector = Vector<IdT>;
	using Index = typename IdT::UInt;
	using Allocator = typename ValueVector::allocator_type;
	using IndexAllocator = typename IndexVector::allocator_type;
	using ValueIterator = typename ValueVector::iterator;
	using ConstValueIterator = typename ValueVector::const_iterator;
	using ConstIndexIterator = typename IndexVector::const_iterator;
	using FilterIterator = decltype(makeFilterIter(std::declval<ValueIterator>(), std::declval<ConstIndexIterator>()));
	using ConstFilterIterator = decltype(makeFilterIter(std::declval<ConstValueIterator>(), std::declval<ConstIndexIterator>()));

	explicit ScatterStorage(Index capacity, const Allocator& allocator = {}) :
		_values(capacity, allocator),
		_indices(capacity, IndexAllocator{allocator}) {}

	IdT id(Index index) const {
		return _indices[index];
	}

	IdT idByValue(const T& value) const {
		return _indices[&value - _values.data()];
	}

	void setId(IdT id) {
		_indices[id.index] = id;
	}

	void setId(Index index, IdT id) {
		_indices[index] = id;
	}

	T& value(Index index) {
		return _values[index];
	}

	const T& value(Index index) const {
		return _values[index];
	}

	Index grow() {
		_values.emplace_back();
		_values.resize(_values.capacity());
		_indices.resize(_values.capacity());
		auto capacity = Index(std::min(_values.capacity(), std::size_t(IdT::limits().index)));
		return capacity;
	}

	ValueIterator begin() {
		return _values.begin();
	}

	ConstValueIterator begin() const {
		return _values.begin();
	}

	FilterIterator beginFilter(Index top) {
		return makeFilterIter(_values.begin(), std::next(_values.begin(), top), 
							   _indices.begin(), std::next(_indices.begin(), top));
	}

	FilterIterator endFilter(Index top) {
		return makeFilterIter(std::next(_values.begin(), top), std::next(_indices.begin(), top));
	}

	ConstFilterIterator beginFilter(Index top) const {
		return makeFilterIter(_values.begin(), std::next(_values.begin(), top),
							   _indices.begin(), std::next(_indices.begin(), top));
	}

	ConstFilterIterator endFilter(Index top) const {
		return makeFilterIter(std::next(_values.begin(), top), std::next(_indices.begin(), top));
	}

private:
	ValueVector _values;
	IndexVector _indices;
};

}