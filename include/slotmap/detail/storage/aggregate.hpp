#pragma once

#include <ranges>

namespace Twig::Container::detail {

    template<class T, class IdT>
    struct Item {
        T   value;
        IdT id;

        // value should be the first field to allow reinterpret_cast from T* to Item* and vice-versa.
        static_assert(offsetof(Item, value) == 0);
    };

    template<template<class> class Vector, class T, class IdT>
    class AggregateStorage {
    public:
        using TItem   = Item<T, IdT>;
        using TVector = Vector<TItem>;
        using Index   = typename IdT::UInt;

        explicit AggregateStorage(TVector vector) : _vector(std::move(vector)) {}

        template<class F>
        static auto make(F vectorFactory, Index capacity) { 
            auto vector = vectorFactory<T>(capacity);
            if (!vector) {
                return vector;
            }

            return AggregateStorage(std::move(*vector));
        }

        IdT id(Index index) const { return _vector[index].id; }

        IdT idByValue(const T& value) const { return reinterpret_cast<const TItem&>(value).id; }

        void setId(IdT id) { _vector[id.index].id = id; }

        void setId(Index index, IdT id) { _vector[index].id = id; }

        T& value(Index index) { return _vector[index].value; }

        const T& value(Index index) const { return _vector[index].value; }

        Index grow()
        {
            // TODO: Implement grow in a way that is compatible with multiple possible container types
            return capacity;
        }

        auto range()
        {
            return std::ranges::transform_view(_vector, [](auto& iter) {
                return std::tuple(std::ref(iter.value), std::cref(iter.id));
            });
        }

    private:
        static_assert(std::is_standard_layout_v<T> && offsetof(TItem, value) == 0);

        TVector _vector;
    };

} // namespace Twig::Container::detail
