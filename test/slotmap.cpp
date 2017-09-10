#include <slotmap/slotmap.hpp>

#include <iostream>
#include <vector>
#include <tuple>

using namespace std;
using namespace Twig::Container;

template<class T> using VectorAdapter = vector<T>;

using TSlotMap = Slotmap<string, VectorAdapter, 32>;
constexpr std::size_t capacity = 10;

int main() {

// TODO: demo.
	
}