#pragma once

namespace Twig::Container::detail {

template<class Index>
class NullSkipfield {
public:
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