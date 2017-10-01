#include <slotmap/slotmap.hpp>
#include <slotmap/filtered.hpp>

#include <iostream>
#include <string>
#include <cassert>
#include <vector>

using namespace std;
using namespace Twig::Container;

template<class T> using VectorAdapter = vector<T>;

using TSlotmap = Slotmap<char*, VectorAdapter, 32, 16, SlotmapFlags::GROW | SlotmapFlags::SKIPFIELD>;

int main() {
	auto slotmap = TSlotmap(10); // create a slotmap with 10 slots

	auto& first = slotmap.alloc();
	first = "Roel ";

	slotmap.push("de ");
	auto id = slotmap.push("de ");
	slotmap.push("Jong");

	auto element = slotmap.find(id); // lookup the slot containing the first "de "
	auto removedByElement = slotmap.free(*element);
	
	auto removedById = slotmap.free(id); // conveniently try to remove directly by id
	
	assert(removedByElement && !removedById);

	for (auto value : slotmap)
		cout << value;
	cout << '\n'; // Output: Roel de de Jong

	for (auto value : make_filtered(slotmap))
		cout << value;
	cout << "\n"; // Output: Roel de Jong

	cout << "Slotmap size: " << slotmap.size() << "\n";
	cout << "Slotmap capacity: " << slotmap.capacity() << "\n"; // Initial capacity depends on vector implementation
}