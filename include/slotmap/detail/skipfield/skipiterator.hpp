#pragma once

namespace Twig::Container::detail {

template<class NodeIter, class SkipfieldIter>
class SkipIterator {
public:
    using value_type        = typename std::iterator_traits<NodeIter>::value_type;
    using difference_type   = typename std::iterator_traits<NodeIter>::difference_type;
    using iterator_category = std::bidirectional_iterator_tag;

    SkipIterator() = default;

    explicit SkipIterator(NodeIter node, SkipfieldIter skipfieldNode)
        : _node(node), _skipfieldNode(skipfieldNode) {
        advance();
    }

    template<class OtherIter>
    SkipIterator(const SkipIterator<OtherIter, SkipfieldIter>& other)
        : _node(other._node), _skipfieldNode(other._skipfieldNode) {}

    reference operator*() const {
        return *_node;
    }

    pointer operator->() const {
        return &(*_node);
    }

    SkipIterator& operator++() {
        ++_skipfieldNode;
        advance();
        return *this;
    }

    SkipIterator operator++(int) {
        SkipIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    SkipIterator& operator--() {
        --_skipfieldNode;
        retreat();
        return *this;
    }

    SkipIterator operator--(int) {
        SkipIterator tmp = *this;
        --(*this);
        return tmp;
    }

    bool operator==(const SkipIterator& rhs) const {
        return _node == rhs._node;
    }

    bool operator!=(const SkipIterator& rhs) const {
        return !(*this == rhs);
    }

private:
    void advance() {
        auto skipCount = *_skipfieldNode;
        _skipfieldNode += skipCount;
        _node += skipCount + 1;
    }

    void retreat() {
        auto skipCount = *_skipfieldNode;
        _skipfieldNode -= skipCount;
        _node -= skipCount - 1;
    }

    NodeIter _node;
    SkipfieldIter _skipfieldNode;
};

}