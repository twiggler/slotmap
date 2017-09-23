#pragma once

#include <algorithm>
#include <numeric>

namespace Twig::Container::detail {

template<class Vector>
class Skipfield {
public:
	using Allocator = typename Vector::allocator_type;
	using Index = typename Vector::value_type;
	using const_iterator = typename Vector::const_iterator;

	explicit Skipfield(typename Vector::size_type capacity, const Allocator& allocator = Allocator()) :
		_data(capacity, allocator) { }

	void clear() {
		std::fill(_data.begin(), _data.end(), 0);
	}

	void skip(Index index) noexcept { 
		const unsigned char leftBlock = index && _data[index - 1];
		const unsigned char rightBlock = index < _data.size() - 1 && _data[index + 1];

		switch (leftBlock | rightBlock << 1) {
			case 0: // No block to the left nor right
				{
					_data[index] = 1;
					break;
				}
			case 1: // Block to the left; index is a tail node.
				{
					const auto blockStartOffset = _data[index - 1];
					const auto newBlockSize = blockStartOffset + 1;
					_data[index] = newBlockSize;
					_data[index - blockStartOffset] = newBlockSize;
					break;
				}
			case 2: // Block to the right; index is a head node.
				{	
					const auto blockSize = _data[index + 1] + 1;
					createSkipBlock(index, blockSize);
					break;
				}
			case 3:	// Block to the left and right.
				{
					const auto blockStartOffset = _data[index - 1];
					const auto combinedBlockSize = blockStartOffset + _data[index + 1] + 1;
					_data[index - blockStartOffset] = combinedBlockSize;
					fillSkipBlock(index, combinedBlockSize, blockStartOffset + 1);
				}
		}
	}

	void unskip(Index index) noexcept {
		const unsigned char leftBlock = index && _data[index - 1];
		const unsigned char rightBlock = index < _data.size() - 1 && _data[index + 1];

		switch (leftBlock | rightBlock << 1) {
			case 0:	// No block to the left nor right
				{
					_data[index] = 0;
					break;
				}
			case 1:	// Block to the left, index is a tail node.
				{
					const auto leftBlockLength = _data[index] - 1;
					_data[index] = 0;
					_data[index - leftBlockLength] = leftBlockLength;
					break;
				}
			case 2: // Block to the right, index is a head node.
				{
					auto blockLength = _data[index] - 1;
					_data[index] = 0;
					createSkipBlock(index + 1, blockLength);
					break;
				}
			case 3: // Node is within a skip block
				{
					auto x = _data[index];
					auto leftBlockLength =  x - 1;
					const auto rightBlockLength = _data[index - leftBlockLength] - x;
					_data[index - leftBlockLength] = leftBlockLength;
					_data[index] = 0;
					createSkipBlock(index + 1, rightBlockLength);
					break;
				}
		}

	}

	void grow() {
		_data.push_back(0);
	}

	const_iterator begin() const noexcept {
		return _data.begin();
	}

	const_iterator end() const noexcept {
		return _data.end();
	}

private:
	void createSkipBlock(Index index, Index length) noexcept {
		_data[index] = length;
		fillSkipBlock(index + 1, length);
		
	}

	void fillSkipBlock(Index index, Index length, Index start = 2) noexcept {
		auto i = index;
		auto x = start;

		while (x <= length) {
			_data[i++] = x++;
		}
	}
	
	Vector _data;
};

}