//
// pch.h
// Header for standard system include files.
//

#pragma once

#include "gtest/gtest.h"
// TODO(me): Workaround for https://github.com/google/googletest/issues/1616 till
// MS GTest package maintainers update to 1.8.1+.
namespace testing::internal {
	inline void PrintTo(std::nullptr_t, ::std::ostream* os) { *os << "(nullptr)"; }
}  // namespace testing::internal

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
