#pragma once

#include <boost/iterator/iterator_adaptor.hpp>

namespace Twig::Container::detail {

template<class NodeIter, class SkipfieldIter>
class SkipIterator : public boost::iterator_adaptor<
	SkipIterator<NodeIter, SkipfieldIter>,
	NodeIter,
	boost::use_default,
	boost::bidirectional_traversal_tag>
{
public:
	SkipIterator() = default;

	explicit SkipIterator(NodeIter node, SkipfieldIter skipfieldNode) :
		SkipIterator::iterator_adaptor(node),
		_skipfieldNode(skipfieldNode)
	{
		auto skipCount = *skipfieldNode;
		_skipfieldNode += skipCount;
		this->base_reference() += skipCount;
	}

	template<class OtherIter>
	SkipIterator(const SkipIterator<OtherIter, SkipfieldIter>& other) :
		SkipIterator::iterator_adaptor(other.base()),
		_skipfieldNode(other._skipfieldNode) { }

private:
	friend class boost::iterator_core_access;

	void increment() {
		auto skipCount = *(++_skipfieldNode);
		_skipfieldNode += skipCount;
		this->base_reference() += skipCount + 1;
	}

	void decrement() {
		auto skipCount = *(--skipfieldNode);
		_skipfieldNode -= skipCount;
		this->base_reference() -= skipCount - 1;
	}

	SkipfieldIter _skipfieldNode;
};

template<class NodeIter, class SkipfieldIter>
auto make_skip_iter(NodeIter nodeIter, SkipfieldIter skipfieldIter) {
	return SkipIterator<NodeIter, SkipfieldIter>{nodeIter, skipfieldIter};
}

}