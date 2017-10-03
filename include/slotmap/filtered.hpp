#pragma once
#include "slotmap.hpp"
#include "detail/skipiterator.hpp"

namespace Twig::Container {

template<class TSlotmap, bool>
class Filtered {
public:
	using iterator = typename TSlotmap::Storage::FilterIterator;
	using const_iterator = typename TSlotmap::Storage::ConstFilterIterator;

	explicit Filtered(TSlotmap& slotmap) :
		slotmap(&slotmap) {}

	iterator begin() {
		return slotmap->_vector.beginFilter(slotmap->_top);
	}

	iterator end() {
		return slotmap->_vector.endFilter(slotmap->_top);
	}

	const_iterator begin() const {
		return slotmap->_vector.beginFilter(slotmap->_top);
	}

	const_iterator end() const {
		return slotmap->_vector.endFilter(slotmap->_top);
	}

private:
	TSlotmap* slotmap;
};

template<class TSlotmap>
class Filtered<TSlotmap, true> {
public:
	using iterator = detail::SkipIterator<typename TSlotmap::Storage::ValueIterator, typename TSlotmap::Skipfield::const_iterator>;
	using const_iterator = detail::SkipIterator<typename TSlotmap::Storage::ConstValueIterator, typename TSlotmap::Skipfield::const_iterator>;
	using Skipfield = typename TSlotmap::Skipfield;

	explicit Filtered(TSlotmap& slotmap) :
		slotmap(&slotmap) {
	}

	iterator begin() {
		return detail::make_skip_iter(slotmap->_vector.begin(), slotmap->Skipfield::begin());
	}

	iterator end() {
		return detail::make_skip_iter(std::next(slotmap->_vector.begin(), slotmap->_top),
									  std::next(slotmap->Skipfield::begin(), slotmap->_top));
	}

	const_iterator begin() const {
		return detail::make_skip_iter(slotmap->_vector.begin(), slotmap->Skipfield::begin());
	}

	const_iterator end() const {
		return detail::make_skip_iter(std::next(slotmap->_vector.begin(), slotmap->_top),
									  std::next(slotmap->Skipfield::begin(), slotmap->_top));
	}

private:
	TSlotmap* slotmap;
};

template<class TSlotmap>
auto make_filtered(TSlotmap& slotmap) {
	return Filtered<TSlotmap, TSlotmap::FastIterable>(slotmap);
}

}