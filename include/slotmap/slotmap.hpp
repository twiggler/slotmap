#pragma once

#include "detail/slotmap.hpp"
#include <random>
#include <cassert>

namespace Twig::Container {

using detail::OutOfSlots;

template<class T,
         template<class> class Vector,
         unsigned IdBits = sizeof(unsigned) * CHAR_BIT ,
         unsigned GenerationBits = IdBits / 2,
         bool Grow = false>
class Slotmap {
public:
	using Value = T;
	using TVector = Vector<detail::Item<T, IdBits, GenerationBits>>;
	using Allocator = typename TVector::allocator_type;
    using Id = detail::Id<IdBits, GenerationBits>;
    using iterator = detail::ValueIter<TVector>;
	using const_iterator = detail::ConstValueIter<TVector>;

    explicit Slotmap(decltype(Id::index) capacity, const Allocator& alloc = Allocator()) :
		_randomEngine(std::random_device{}()),
		_capacity(std::min(capacity, Id::limits().index)),
        _vector(_capacity, alloc),
		_top(0) // prevent call to clear from needless zapping of elements
    {
        clear();
    }

    void clear() {
		for (typename Id::UInt elementIndex = 0; elementIndex < _top; elementIndex++)
			_vector[elementIndex].id.generation = 0;

		_size = 0;
		 std::uniform_int_distribution<unsigned long long> _generationDistribution(1, Id::limits().generation);
        _generation = static_cast<typename Id::UInt>(_generationDistribution(_randomEngine)); // uniform_int_distribution does not support unsigned char
        _top = 0;
        _freeHead = Id::limits().index; // always smaller than _top, indicates that all free slots are at the tail.
    }

    T* find(Id id) {
        assert(id.generation);

        auto& item = _vector[id.index];
        return item.id == id ? &item.value : nullptr;
    }

    const T* find(Id id) const {
        assert(id.generation);

        const auto& item = _vector[id.index];
        return item.id == id ? &item.value : nullptr;
    }

    Id id(const T& element) const {
        auto id = reinterpret_cast<const typename TVector::value_type&>(element).id;
		return id.generation != 0 ? id : Id{0, 0};
    }

    auto capacity() const {
		return _capacity;
    }

	auto size() const {
		return _size;
	}

    T& alloc() {
		typename Id::UInt index;

		if (_freeHead < _top) {
			index = _freeHead;
			_freeHead = _vector[index].id.index;
		}
		else {
			if (_size == _capacity) {
				if (Grow && _capacity != Id::limits().index) {
					_vector.resize(_size + 1);
					_capacity = static_cast<decltype(_capacity)>(std::min(_vector.capacity(), std::size_t(Id::limits().index)));
				}
				else
					throw OutOfSlots();
			}

			index = _top++;
		}
	
		auto& element = _vector[index];
        element.id = { index, _generation };
        _size++;
        _generation = std::max(1, (_generation + 1) & Id::limits().generation); // Generation of zero indicates unused item.

        return element.value;
    }

	template<class U>
	Id push(U&& value) {
		auto& slot = alloc();
		slot = std::forward<U>(value);
		
		return id(slot);
	}

    bool free(T& value) {
        auto& markedItem = reinterpret_cast<typename TVector::value_type&>(value);
		if (markedItem.id.generation == 0)
			return false;

        auto oldFreeHead = _freeHead;    // Cannot use std::swap because it is illegal to form a reference to a bit field.
        _freeHead = markedItem.id.index;
        markedItem.id = { oldFreeHead, 0 };
        _size--;
		
		return true;
    }

	bool free(Id id) {
		assert(id.generation);
		auto value = find(id);
		
		if (value != nullptr)
			return free(*value);
		
		return false;
	}

    iterator begin() {
		return detail::makeValueIter(_vector.data());
    }

    iterator end() {
        return detail::makeValueIter(_vector.data() + _top);
    }

    const_iterator begin() const {
		return detail::makeValueIter(_vector.data());
    }

    const_iterator end() const {
		return detail::makeValueIter(_vector.data() + _top);
    }

private:
	template<class TSlotmap> friend class Filtered;
	using UInt = typename Id::UInt;

	UInt _capacity;
	TVector _vector;
	std::mt19937 _randomEngine;
	UInt _generation;
    UInt _top;
    UInt _freeHead;
    UInt _size;
};

}