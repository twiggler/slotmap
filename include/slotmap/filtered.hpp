#pragma once

#include "detail/slot_iterator.hpp"
#include "slotmap.hpp"

#include <ranges>
#include <type_traits>

namespace Twig::Container {

// View over the allocated slots of a slotmap, skipping free ones. Jumps over runs of free slots when the
// slotmap has a skipfield, and checks each slot otherwise.
template<class TSlotmap, bool FastIterable = TSlotmap::FastIterable>
class Filtered : public std::ranges::view_interface<Filtered<TSlotmap, FastIterable>> {
	static_assert(!FastIterable || TSlotmap::FastIterable, "fast iteration needs a slotmap with SlotmapFlags::SKIPFIELD");

public:
	using iterator = detail::SlotIterator<TSlotmap, std::conditional_t<FastIterable, detail::SkipAdvance, detail::IdCheckAdvance>>;
	using const_iterator = iterator;

	Filtered() = default;

	explicit Filtered(TSlotmap& slotmap) :
		_slotmap(&slotmap) {}

	iterator begin() const {
		return iterator::begin(*_slotmap);
	}

	iterator end() const {
		return iterator::end(*_slotmap);
	}

private:
	TSlotmap* _slotmap = nullptr;
};

template<class TSlotmap>
auto make_filtered(TSlotmap& slotmap) {
	return Filtered<TSlotmap>(slotmap);
}

} // namespace Twig::Container

// Iterators point into the slotmap, not into the view, so they stay valid after the view is gone.
template<class TSlotmap, bool FastIterable>
// NOLINTNEXTLINE(readability-identifier-naming): specialization of a standard-library name
constexpr bool std::ranges::enable_borrowed_range<Twig::Container::Filtered<TSlotmap, FastIterable>> = true;
