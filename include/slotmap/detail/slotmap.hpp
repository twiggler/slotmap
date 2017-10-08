#pragma once

#include "nullskipfield.hpp" 
#include "skipfield.hpp"
#include "tuplestorage.hpp"
#include "scatterstorage.hpp"
#include <algorithm>
#include <boost/integer.hpp>

namespace Twig::Container::detail {

struct OutOfSlots : public std::runtime_error {
	explicit OutOfSlots() :
		std::runtime_error("Flatmap out of slots.") { }
};

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

template<bool UseSkipfield,
		 template<class> class Vector,
		 unsigned IdBits,
		 unsigned GenerationBits>
struct SelectSkipfield {
	using UInt = typename Id<IdBits, GenerationBits>::UInt;
	using type = std::conditional_t<UseSkipfield,
		Skipfield<Vector<UInt>>,
		NullSkipfield<UInt>
	>;
};

template<template<class> class Vector,
		 class T,
		 class IdT,
		 bool Scatter>
struct SelectStorage {
	static constexpr bool hasStandardLayout = std::is_standard_layout_v<T>;
	using type = std::conditional_t<!hasStandardLayout || Scatter,
		ScatterStorage<Vector, T, IdT>,
		TupleStorage<Vector, T, IdT>
	>;
};

}