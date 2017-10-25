#pragma once

namespace Twig::Container::detail {

template<class Vector>
class NullSkipfield {
public:
	using Allocator = typename Vector::allocator_type;
	using Index = typename Vector::value_type;

	explicit NullSkipfield(typename Vector::size_type, const Allocator&) { }

	void clear() { }

	void skip(Index) { }

	void unskip(Index)  { }

	void grow() { }

	auto begin() const  {
		return nullptr;
	}

	auto end() const {
		return nullptr;
	}
};

}