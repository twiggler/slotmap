// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// Headers for CppUnitTest
#include "CppUnitTest.h"

// TODO: reference additional headers your program requires here
#define _ITERATOR_DEBUG_LEVEL 0

#include <vector>
#include <random>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/cartesian_product.hpp>
#include <boost/hana/transform.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/unpack.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/front.hpp>
#include <boost/hana/type.hpp>
#include <foonathan/memory/memory_arena.hpp>
#include <foonathan/memory/memory_stack.hpp>
#include <foonathan/memory/segregator.hpp>
#include <foonathan/memory/std_allocator.hpp>