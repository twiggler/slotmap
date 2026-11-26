#pragma once

#include "skipfield/nullskipfield.hpp"
#include "skipfield/skipfield.hpp"
#include "storage/aggregate.hpp"
#include "storage/segregate.hpp"

#include <algorithm>
#include <compare>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace Twig::Container::detail {

struct OutOfSlots : public std::runtime_error {
	explicit OutOfSlots() :
		std::runtime_error("Slotmap out of slots.") {}
};

// Smallest unsigned integer type with at least Bits bits.
template<unsigned Bits>
using UIntLeast = std::conditional_t<
	(Bits <= 8), std::uint_least8_t,
	std::conditional_t<(Bits <= 16), std::uint_least16_t, std::conditional_t<(Bits <= 32), std::uint_least32_t, std::uint_least64_t>>>;

template<unsigned IdBits, unsigned GenerationBits>
struct Id {
	static_assert(GenerationBits > 0);
	static_assert(GenerationBits < IdBits); // It follows that IdBits > 0

	static constexpr unsigned IndexBits = IdBits - GenerationBits;
	static constexpr unsigned FieldBits = std::max(GenerationBits, IndexBits);
	static_assert(FieldBits < 64, "the index and generation fields can be at most 63 bits wide");

	// Type of both fields; the value of a bit field is limited by its type. A field of 32 or more bits gets a wider
	// type than it needs, because MSVC warns about full-width bit fields of that size (C4293).
	using UInt = UIntLeast<(FieldBits >= 32 ? FieldBits + 1 : FieldBits)>;

	static constexpr Id limits() {
		return { allOnes(IndexBits), allOnes(GenerationBits) };
	}

	static constexpr Id null() {
		return { 0, 0 };
	}

	static Id free(UInt index) {
		return { index, 0 };
	}

	// Next generation: cycles through 1 … max, skipping 0, which marks invalid ids.
	static constexpr UInt evolve(UInt generation) {
		return static_cast<UInt>(generation % limits().generation + 1);
	}

	bool valid() const {
		return generation != 0;
	}

	friend constexpr bool operator==(const Id&, const Id&) = default;
	friend constexpr auto operator<=>(const Id&, const Id&) = default;

	UInt index : IndexBits;
	UInt generation : GenerationBits;

private:
	static constexpr UInt allOnes(unsigned bits) {
		return std::numeric_limits<UInt>::max() >> (std::numeric_limits<UInt>::digits - bits);
	}
};

template<bool UseSkipfield, template<class...> class Vector, unsigned IdBits, unsigned GenerationBits>
struct SelectSkipfield {
	using UInt = typename Id<IdBits, GenerationBits>::UInt;
	using type = std::conditional_t<UseSkipfield, Skipfield<Vector<UInt>>, NullSkipfield<Vector<UInt>>>;
};

template<template<class...> class Vector, class T, class IdT, bool Segregate>
struct SelectStorage {
	static constexpr bool HasStandardLayout = std::is_standard_layout_v<T>;
	using type = std::conditional_t<!HasStandardLayout || Segregate, SegregateStorage<Vector, T, IdT>, AggregateStorage<Vector, T, IdT>>;
};

} // namespace Twig::Container::detail
