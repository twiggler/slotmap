#pragma once

#include <algorithm>
#include <numeric>

namespace Twig::Container::detail {

template<template<class> class Vector, class IdT>
class Skipfield {
public:
	explicit Skipfield(Vector<IdT> indices) :
		indices(std::move(indices)) { }

	template<class F>
	static auto make(F factory, IdT capacity) {
		auto indices = factory(capacity);
		if (!indices) {
			return indices;
		}
		
		return Skipfield(std::move(indices));
	}

	void clear() {
		std::ranges::fill(indices, 0);
	}

	void skip(IdT index)  { 
		const unsigned char leftBlock = index && indices[index - 1];
		const unsigned char rightBlock = index < indices.size() - 1 && indices[index + 1];

		switch (leftBlock | rightBlock << 1) {
			case 0: // No block to the left nor right
				{
					indices[index] = 1;
					break;
				}
			case 1: // Block to the left; index is a tail node.
				{
					const auto blockStartOffset = indices[index - 1];
					const auto newBlockSize = blockStartOffset + 1;
					indices[index] = newBlockSize;
					indices[index - blockStartOffset] = newBlockSize;
					break;
				}
			case 2: // Block to the right; index is a head node.
				{	
					const auto blockSize = indices[index + 1] + 1;
					createSkipBlock(index, blockSize);
					break;
				}
			case 3:	// Block to the left and right.
				{
					const auto blockStartOffset = indices[index - 1];
					const auto combinedBlockSize = blockStartOffset + indices[index + 1] + 1;
					indices[index - blockStartOffset] = combinedBlockSize;
					fillSkipBlock(index, combinedBlockSize, blockStartOffset + 1);
				}
		}
	}

	void unskip(IdT index)  {
		const unsigned char leftBlock = index && indices[index - 1];
		const unsigned char rightBlock = index < indices.size() - 1 && indices[index + 1];

		switch (leftBlock | rightBlock << 1) {
			case 0:	// No block to the left nor right
				{
					indices[index] = 0;
					break;
				}
			case 1:	// Block to the left, index is a tail node.
				{
					const auto leftBlockLength = indices[index] - 1;
					indices[index] = 0;
					indices[index - leftBlockLength] = leftBlockLength;
					break;
				}
			case 2: // Block to the right, index is a head node.
				{
					auto blockLength = indices[index] - 1;
					indices[index] = 0;
					createSkipBlock(index + 1, blockLength);
					break;
				}
			case 3: // Node is within a skip block
				{
					auto x = indices[index];
					auto leftBlockLength =  x - 1;
					const auto rightBlockLength = indices[index - leftBlockLength] - x;
					indices[index - leftBlockLength] = leftBlockLength;
					indices[index] = 0;
					createSkipBlock(index + 1, rightBlockLength);
					break;
				}
		}

	}

	void grow() {
		// TODO: Implement grow.
	}

private:
	void createSkipBlock(IdT index, IdT length)  {
		indices[index] = length;
		fillSkipBlock(index + 1, length);
		
	}

	void fillSkipBlock(IdT index, IdT length, IdT start = 2)  {
		auto i = index;
		auto x = start;

		while (x <= length) {
			indices[i++] = x++;
		}
	}
	
	Vector indices;
};

}