#include <slotmap/slotmap.hpp>
#include <slotmap/filtered.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace Twig::Container;

template<class T> using VectorAdapter = vector<T>;

using TSlotmap = Slotmap<string, VectorAdapter, 32, 16, SlotmapFlags::GROW | SlotmapFlags::SKIPFIELD>;

int main() try {
	auto slotmap = TSlotmap(10); // create a slotmap with 10 slots

	auto& first = slotmap.alloc();
	first = "Roel ";

	slotmap.push("de ");
	auto id = slotmap.push("de ");
	auto jong = string("jong");
	slotmap.push(std::move(jong));

	auto element = slotmap.find(id); // lookup the slot containing the first "de "
	auto removedByElement = slotmap.free(*element);

	auto removedById = slotmap.free(id); // conveniently try to remove directly by id

	if (!removedByElement || removedById) {
		cerr << "unexpected result from free()\n";
		return EXIT_FAILURE;
	}

	for (const auto& value : slotmap)
		cout << value;
	cout << '\n'; // Output: Roel de de Jong

	for (const auto& value : make_filtered(slotmap))
		cout << value;
	cout << "\n"; // Output: Roel de Jong

	cout << "Slotmap size: " << slotmap.size() << "\n";
	cout << "Slotmap capacity: " << slotmap.capacity() << "\n"; // Initial capacity depends on vector implementation
} catch (const exception& error) {
	cerr << error.what() << '\n';
	return EXIT_FAILURE;
}
