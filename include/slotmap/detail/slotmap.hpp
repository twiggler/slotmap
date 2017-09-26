#pragma once

#include "nullskipfield.hpp" 
#include "skipfield.hpp"
#include <algorithm>
#include <boost/integer.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>

namespace Twig::Container::detail {

struct OutOfSlots : public std::runtime_error {
	explicit OutOfSlots() :
		std::runtime_error("Flatmap out of slots.") { }
};

template<class TVector>
using ValueIter = decltype(makeValueIter(std::declval<typename TVector::value_type*>()));

template<class TVector>
using ConstValueIter = decltype(makeValueIter(std::declval<const typename TVector::value_type*>()));

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

template<unsigned IdBits, unsigned GenerationBits>
struct Id {
	static_assert(GenerationBits > 0);
	static_assert(GenerationBits < IdBits); // It follows that IdBits > 0	 
			
	static constexpr unsigned IndexBits = IdBits - GenerationBits;
	using UInt = typename boost::uint_t<std::max(GenerationBits, IndexBits)>::fast; // the value of a bit field is limited by the type
	
	static constexpr Id limits() {
		return { UInt(-1), UInt(-1) };
	};

	UInt index : IndexBits;
	UInt generation : GenerationBits;
};

template<unsigned IdBits, unsigned GenerationBits>
bool operator==(const Id<IdBits, GenerationBits>& a, const Id<IdBits, GenerationBits>& b) {
	return a.index == b.index &&
		a.generation == b.generation;
}

template<class T, unsigned IdBits, unsigned GenerationBits>
struct Item {
	T value;	// value should be the first field to allow reinterpret_cast from T* to Item* and vice-versa.
	Id<IdBits, GenerationBits> id;
};

template<bool UseSkipfield,
		template<class> class Vector,
		unsigned IdBits,
		unsigned GenerationBits>
struct SlotmapTraits {
	using UInt = typename Id<IdBits, GenerationBits>::UInt;
	using Skipfield = std::conditional_t<UseSkipfield,
		Skipfield<Vector<UInt>>,
		NullSkipfield<UInt>
	>;
};

}