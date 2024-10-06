#pragma once

#include "detail/slotmap.hpp"
#include "detail/storage/aggregate.hpp"
#include "detail/storage/segregate.hpp"
#include "detail/skipfield/skipfield.hpp"
#include <random>
#include <expected>
#include <cassert>

namespace Twig::Container {

    using detail::OutOfSlots;

    template<class, bool>
    class Filtered;

    struct SlotmapFlags {
        static constexpr auto GROW      = 1u;
        static constexpr auto SKIPFIELD = 1u << 1;
        static constexpr auto SEGREGATE = 1u << 2;
    };

    template<
        class T,
        template<class>
        class Vector,
        unsigned IdBits         = sizeof(unsigned) * CHAR_BIT,
        unsigned GenerationBits = IdBits / 2,
        unsigned Flags          = 0,
        bool     UseExceptions  = true>
    class Slotmap {
    public:
        using value_type = T;
        using Id         = detail::Id<IdBits, GenerationBits>;
        using Storage    = std::conditional_t<
               !hasStandardLayout || Scatter,
               SegregateStorage<Vector, T, IdT>,
               AggregateStorage<Vector, T, IdT>>;

        using Allocator = typename Storage::Allocator;

        explicit Slotmap(
            decltype(Id::index) capacity, decltype(Id::generation) generation = 1, const Allocator& allocator = {}
        )
        requires(UseExceptions)
            :
            skipfield(std::min(capacity, Id::limits().index) + 1u, typename Skipfield::Allocator(allocator)),
            _generation(generation),
            _capacity(std::min(capacity, Id::limits().index)),
            storage(_capacity, allocator),
            _top(0),
            _freeHead(Id::limits().index),
            _size(0)
        {
            assert((Id{1, generation}.valid()));
        }

        Slotmap(
            decltype(Id::index) capacity, decltype(Id::generation) generation = 1, Storage storage, SkipfieldT skipfield
        ) :
            storage(std::move(storage)),
            skipfield(std::move(skipfield)),
            _capacity(std::min(capacity, Id::limits().index)),
            _generation(generation),
            _top(0),
            _freeHead(Id::limits().index),
            _size(0)
        {
            assert((Id{1, generation}.valid()));
        }

        template<class F>
        static auto make(F storageFactory, decltype(Id::index) capacity, decltype(Id::generation) generation = 1)
        {
            auto storage = Storage::make(capacity);
            if (!storage) {
                return storage;
            }


            if (constexpr !(Flags & SlotmapFlags::SKIPFIELD)) {
                return Slotmap(capacity, generation, std::move(*storage), EmptySkipfield{});
            }

            auto skipfield = SkipfieldT::make(storageFactory, capacity);
            if (!skipfield) {
                return skipfield;
            }

            return Slotmap(capacity, generation, std::move(*storage), std::move(*skipfield));
        }

        void clear()
        {
            for (typename Id::UInt elementIndex = 0; elementIndex < _top; elementIndex++)
                storage.setId(elementIndex, Id::null());

            Skipfield::clear();
            _size     = 0;
            _top      = 0;
            _freeHead = Id::limits().index; // always smaller than _top, indicates that all free slots are at the tail.
        }

        T* find(Id id)
        {
            assert(id.valid());

            return storage.id(id.index) == id ? &storage.value(id.index) : nullptr;
        }

        const T* find(Id id) const
        {
            assert(id.valid());

            return storage.id(id.index) == id ? &storage.value(id.index) : nullptr;
        }

        Id id(const T& value) const
        {
            auto id = storage.idByValue(value);
            return id.valid() ? id : Id::null();
        }

        auto capacity() const { return _capacity; }

        auto size() const { return _size; }

        T* alloc()
        {
            typename Id::UInt index;

            if (_freeHead < _top) {
                index     = _freeHead;
                _freeHead = storage.id(index).index;
                this->unskip(index);
            } else {
                if (_size == _capacity) {
                    if (flags & SlotmapFlags::GROW && _capacity != Id::limits().index) {
                        _capacity = storage.grow(); // Assume the allocator value-initializes the item.
                        this->grow();
                    } else {
                        if constexpr (UseExceptions) {
                            throw OutOfSlots();
                        } else {
                            return nullptr
                        }
                    }
                }

                index = _top++;
            }

            storage.setId({index, _generation});
            _size++;
            _generation = Id::evolve(_generation);

            return &storage.value(index);
        }

        Id push(const T& value)
        {
            auto& slot = alloc();
            slot       = value;

            return id(slot);
        }

        Id push(T&& value)
        {
            auto& slot = alloc();
            slot       = std::move(value);

            return id(slot);
        }

        bool free(T& value)
        {
            auto id = storage.idByValue(value);
            if (!id.valid())
                return false;

            doFree(id);

            return true;
        }

        bool free(Id id)
        {
            using namespace std::rel_ops;
            assert(id.valid());

            if (storage.id(id.index) != id)
                return false;

            doFree(id);

            return true;
        }

        auto begin() { return storage.range().begin(); }

        auto end() { return storage.range().end(); }

    private:
        struct EmptySkipfield {
            constexpr EmptySkipfield(auto&&...) {};
        };

        using UInt = typename Id::UInt;
        using SkipFieldT =
            std::conditional_t<Flags & SlotmapFlags::SKIPFIELD, detail::Skipfield<Vector, UInt>, EmptySkipfield>;

        void doFree(Id id)
        {
            auto oldFreeHead =
                _freeHead; // Cannot use std::swap because it is illegal to form a reference to a bit field.
            _freeHead = id.index;
            this->skip(id.index);
            storage.setId(id.index, Id::free(oldFreeHead));
            _size--;
        }


        UInt                             _capacity;
        Storage                          storage;
        UInt                             _generation;
        UInt                             _top;
        UInt                             _freeHead;
        UInt                             _size;
        [[no_unique_address]] SkipfieldT skipfield;
    };

} // namespace Twig::Container

template<
    class T,
    template<class>
    class Vector,
    unsigned IdBits         = sizeof(unsigned) * CHAR_BIT,
    unsigned GenerationBits = IdBits / 2,
    unsigned Flags          = 0,
    bool     UseExceptions  = true>
class Filtered : public std::ranges::view_interface<Filtered<T, Vector, IdBits, GenerationBits, Flags, UseExceptions>> {
    explicit Filtered(Slotmap<T, Vector, IdBits, GenerationBits, Flags, UseExceptions>& slotmap) : slotmap(&slotmap) {}

    auto range()
    {
        return std::ranges::filter(slotmap, [](auto& item) { return std::get<1>(item).valid() });
    }

    auto begin() { return range().begin(); }

    auto end() { return range().end(); }


private:
    Slotmap<T, Vector, IdBits, GenerationBits, Flags, UseExceptions>* slotmap;
};
