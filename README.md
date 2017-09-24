# Introduction

Slotmap is a sequence container with weak reference keys written in C++17. It is especially useful for writing video games.

A [demo](demo/slotmap.cpp) is provided in `demo/slotmap.cpp`.

Features:
+ Slots are stored linear in memory.
+ Fast iteration.
+ Constant time allocation and deallocation.
+ Zero overhead lookups.
+ Reusing a slot invalidates the key.
+ Stable references to elements.

Slotmap is inspired by [this article](http://greysphere.tumblr.com/post/31601463396/data-arrays) by Jeff Gates, and [this](https://gamedev.stackexchange.com/questions/33888/what-is-the-most-efficient-container-to-store-dynamic-game-objects-in) discussion.  
Fast iteration is available by optionally using [skipfields](https://link.springer.com/epdf/10.1007/s40869-017-0038-3?author_access_token=cyehqAFuAEnx5rF1-5V3sve4RwlQNchNByi7wbcMAY5wgvdWX7FmurWl4trUGoDtyDH43_RnVt3Y6cI8090dRW3kI3LvrU9wCVzxVe_1gAg2Oc0OlIjjsBpTjDEAEr9gpnZenJKZ8SpkipppuHk4PA==).  

## Dependencies
Slotmap requires the Boost header-only libraries `integer` and `iterator`. In addition, the unit-tests require the Boost header only libraries `range` and `hana`. 

# Usage
All library files reside in `include/slotmap`.  
All classes reside in the nested namespace `Twig::Container` 
## Configuration
`Slotmap` is a class template which requires two template parameters:  
`T`, the type to store in the slotmap.  
`Vector<U>`, a template parameter which specifies which underlying vector container the slotmap uses. An example is `template<class T> using Vector = std::vector<T>`. A possible alternative is to use `folly::fbvector`.  

There are three optional template parameters:  
`IdBits`, designates the size of the id in bits. A slotmap id type consists of a public `index` member field and a public `generation` member field. After every allocation, a global generation counter is incremented and assigned to the id of the allocated slot. The default is the size in bits of an unsigned int.   
`GenerationBits`, how many bits to use for the generation part of the id. Key collisions for slot *s* occur after 2^`GenerationBits`-2 allocations of any slot including *s*, followed by a deallocation-allocation of slot *s*. The default value equals `IdBits` divided by two.  
`Flags`, controls grow and interation behavior by combining members of `SlotmapFlags`:   
 Setting `SlotmapFlags::GROW` allows the slotmap to grow and act as a dynamic array. __When allowed to grow, the slotmap loses constant time allocation and stable references to elements.__ The reallocation strategy of the underlying `Vector` container is used. By default, the `Slotmap` is a static array which is unable to grow.  
 Setting `SlotmapFlags::SKIPFIELD` allows for faster iteration by jumping over blocks of removed elements in constant time. This is accomplished by implementation of a [skipfield](https://link.springer.com/epdf/10.1007/s40869-017-0038-3?author_access_token=cyehqAFuAEnx5rF1-5V3sve4RwlQNchNByi7wbcMAY5wgvdWX7FmurWl4trUGoDtyDH43_RnVt3Y6cI8090dRW3kI3LvrU9wCVzxVe_1gAg2Oc0OlIjjsBpTjDEAEr9gpnZenJKZ8SpkipppuHk4PA==). __Because the skipfield needs to updated after every allocation and deallocation, the slotmap loses constant time allocation and deallocation. Moreover, the memory overhead of the slotmap is increased by the storage cost of the skipfield.__ By default, the slotmap uses a boolean flag to skip deallocated elements, which may be slower to iterate due to branching.   

## Methods
### Construction
`SlotMap(<auto> capacity, const Allocator& alloc = {})`:  
Construct a slotmap with a capacity of `capacity`. The allocator is passed to the underlying `Vector`. `capacity` is truncated to the largest number of elements that can be represented by an id. This overload is only available when the slotmap is configured without `SlotmapFlags::SKIPFIELD`.

`SlotMap(<auto> capacity, const SlotAlloc& slotAlloc = {}, const SkipfieldAlloc& skipAlloc = {})`:  
Construct a slotmap with a capacity of `capacity`. The `slotAlloc` is passed to the underlying Vector. The `skipAlloc` is passed to the skipfield.  `capacity` is truncated to the largest number of elements that can be represented by an id. This overload is only available when the slotmap is configured with `SlotmapFlags::SKIPFIELD`.



### Allocation / deallocation
`T& alloc()`:  
Return a reference to a free slot. An effort is made to keep the slotmap compact by recycling slots which have been used.     
Throws a `OutOfSlots` exception if all slots are used and `Grow` is false.  
Throws when `Grow` is true and reallocation fails. 

`Id push(const T& value)`  
`Id push(T&& value)`   
Convenience method to allocate a free slot and assign or move-assign `value`.  
Returns the id associated with the allocated slot.

`bool free(T& value)`:  
Free the slot taken by `value`. Note that the destructor of the element is *not* called, and should not be called manually. Returns true if the slot was used and the element is removed, or false if the slot was already free.

`bool free(Id id)`:  
Convenience method to free a slot by id.

### Lookup
`T* find(Id id)`  
`const T* find(Id id) const`:  
Returns a pointer to the element associated with `id`.  
A null-pointer is returned if there is no element with `id`.

`auto Id id(const T& element) const`  
Returns the id associated with `element`.
If the slot pointed to by `element` is no longer in use, return an invalid id whose `index` and `generation` are zero.

### Iteration
`auto begin()`  
`auto end()`:  
Note that iteration includes free slots. To filter out free slots, using a skipfield if configured, use the `Filtered` adapter in `filtered.hpp`.

# Implementation details
Todo.

