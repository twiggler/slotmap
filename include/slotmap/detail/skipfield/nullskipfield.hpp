#pragma once

namespace Twig::Container::detail {

// Stands in for Skipfield when fast iteration is not requested; empty, so it takes no space.
template<class Vector>
class NullSkipfield {
public:
	using Allocator = typename Vector::allocator_type;
	using Index = typename Vector::value_type;

	explicit NullSkipfield(typename Vector::size_type, const Allocator&) {}

	void clear() {}

	void skip(Index) {}

	void unskip(Index) {}

	void grow() {}
};

} // namespace Twig::Container::detail
