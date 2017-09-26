#pragma once
#include "slotmap.hpp"
#include "detail/skipiterator.hpp"

namespace Twig::Container {

template<class TSlotmap, bool>
class Filtered {
public:
	using iterator = decltype(detail::makeFilterIter(std::declval<typename TSlotmap::TVector::value_type*>()));
	using const_iterator = decltype(detail::makeFilterIter(std::declval<const typename TSlotmap::TVector::value_type*>()));

	explicit Filtered(TSlotmap& slotmap) :
		slotmap(&slotmap) {}

	iterator begin() {
		return detail::makeFilterIter(slotmap->_vector.data(), slotmap->_vector.data() + slotmap->_top);
	}

	iterator end() {
		return detail::makeFilterIter(slotmap->_vector.data() + slotmap->_top);
	}

	const_iterator begin() const {
		return detail::makeFilterIter(slotmap->_vector.data(), slotmap->_vector.data() + slotmap->_top);
	}

	const_iterator end() const {
		return detail::makeFilterIter(slotmap->_vector.data() + slotmap->_top);
	}

private:
	TSlotmap* slotmap;
};

template<class TSlotmap>
class Filtered<TSlotmap, true> {
public:
	using SkipIter = detail::SkipIterator<typename TSlotmap::TVector::value_type*, typename TSlotmap::Skipfield::const_iterator>;
	using ConstSkipIter = detail::SkipIterator<const typename TSlotmap::TVector::value_type*, typename TSlotmap::Skipfield::const_iterator>;
	using iterator = decltype(detail::makeValueIter(std::declval<SkipIter>()));
	using const_iterator = decltype(detail::makeValueIter(std::declval<ConstSkipIter>()));
	using Skipfield = typename TSlotmap::Skipfield;

	explicit Filtered(TSlotmap& slotmap) :
		slotmap(&slotmap) {
	}

	iterator begin() {
		return detail::makeValueIter(detail::make_skip_iter(slotmap->_vector.data(), slotmap->Skipfield::begin()));
	}

	iterator end() {
		return detail::makeValueIter(detail::make_skip_iter(slotmap->_vector.data() + slotmap->_top, std::next(slotmap->Skipfield::begin(), slotmap->_top)));
	}

	const_iterator begin() const {
		return detail::makeValueIter(detail::make_skip_iter(slotmap->_vector.data(), slotmap->Skipfield::begin()));
	}

	const_iterator end() const {
		return detail::makeValueIter(detail::make_skip_iter(slotmap->_vector.data() + slotmap->_top, std::next(slotmap->Skipfield::begin(), slotmap->_top)));
	}

private:
	TSlotmap* slotmap;
};

template<class TSlotmap>
auto make_filtered(TSlotmap& slotmap) {
	return Filtered<TSlotmap, TSlotmap::FastIterable>(slotmap);
}

}