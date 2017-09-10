#pragma once
#include "slotmap.hpp"

namespace Twig::Container {

template<class TSlotmap>
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
auto make_filtered(TSlotmap& slotmap) {
	return Filtered<TSlotmap>(slotmap);
}

}