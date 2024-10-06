#pragma once

#include <memory>
#include <ranges>

namespace Twig::Container::detail {

    template<class ValueIter, class IndexIter>
    auto makeFilterIter(ValueIter firstValue, ValueIter lastValue, IndexIter firstIndex, IndexIter lastIndex)
    {
        // Convert to function - pointer because lambda cannot be default - constructed.
        using V = std::pair<
            typename std::iterator_traits<ValueIter>::reference,
            typename std::iterator_traits<IndexIter>::reference>;

        return boost::make_transform_iterator(
            boost::make_filter_iterator(
                +[](V zip) { return zip.second.valid(); },
                boost::make_zip_iterator(std::make_pair(firstValue, firstIndex)),
                boost::make_zip_iterator(std::make_pair(lastValue, lastIndex))
            ),
            +[](V filteredZip) -> auto& { return filteredZip.first; }
        );
    }

    template<class ValueIter, class IndexIter>
    auto makeFilterIter(ValueIter valueIter, IndexIter indexIter)
    {
        return makeFilterIter(valueIter, valueIter, indexIter, indexIter);
    }

    template<template<class> class Vector, class T, class IdT>
    class SegregateStorage {
    public:
        using ValueVector = Vector<T>;
        using IndexVector = Vector<IdT>;
        using Index       = typename IdT::UInt;

        SegregateStorage(ValueVector values, IndexVector indices) :
            _values(std::move(values)), _indices(std::move(indices))
        {}

        template<class F>
		static auto make(F vectorFactory, Index capacity)
		{
			auto values = vectorFactory<T>(capacity);
			if (!values) {
				return values;
			}

			auto indices = vectorFactory<Index>(capacity);
			if (!indices) {
				return indices;
			}

			return SegregateStorage(std::move(*values), std::move(*indices));
		}

		IdT id(Index index) const { return _indices[index]; }

		IdT idByValue(const T& value) const { return _indices[&value - _values.data()]; }

		void setId(IdT id) { _indices[id.index] = id; }

		void setId(Index index, IdT id) { _indices[index] = id; }

		T& value(Index index) { return _values[index]; }

		const T& value(Index index) const { return _values[index]; }

		Index grow()
		{
            // TODO: Implement grow in a way that is compatible with multiple possible container types
			
			return capacity;
		}

		auto range()
		{
			return std::ranges::transform_view(
				makeFilterIter(_values.begin(), _values.end(), _indices.begin(), _indices.end()),
				[](auto& value) { return std::ref(value); }
			);
		}

	private:
		ValueVector _values;
		IndexVector _indices;
	};

	template<template<class> class Vector, class T, class IdT>
	class AggregateStorage {
	public:
		using ValueVector = Vector<T>;
		using IndexVector = Vector<IdT>;
		using Index       = typename IdT::UInt;
		using IndexAllocator = typename IndexVector::allocator_type;

		AggregateStorage(Index capacity, const IndexAllocator& allocator) :
			_values(capacity, allocator), _indices(capacity, IndexAllocator{allocator}) {}

		template<class F>
		static auto make(F vectorFactory, Index capacity)
		{
			auto values = vectorFactory(capacity);
			if (!values) {
				return values;
			}

			auto indices = vectorFactory(capacity);
			if (!indices) {
				return indices;
			}

			return AggregateStorage(std::move(*values), std::move(*indices));
		}

		IdT id(Index index) const { return _indices[index]; }

		Id
		_values(capacity, allocator), _indices(capacity, IndexAllocator{allocator}) {}

        IdT id(Index index) const { return _indices[index]; }

        IdT idByValue(const T& value) const { return _indices[&value - _values.data()]; }

        void setId(IdT id) { _indices[id.index] = id; }

        void setId(Index index, IdT id) { _indices[index] = id; }

        T& value(Index index) { return _values[index]; }

        const T& value(Index index) const { return _values[index]; }

        Index grow()
        {
            _values.emplace_back();
            _values.resize(_values.capacity());
            _indices.resize(_values.capacity());
            auto capacity = Index(std::min(_values.capacity(), std::size_t(IdT::limits().index)));
            return capacity;
        }

        auto range() { return std::ranges::zip_view(_values, static_cast<const IndexVector&>(_indices)); }

    private:
        ValueVector _values;
        IndexVector _indices;
    };


} // namespace Twig::Container::detail
