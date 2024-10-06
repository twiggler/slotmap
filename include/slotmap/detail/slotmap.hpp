#pragma once

#include <cstdint>

namespace Twig::Container::detail {

    struct OutOfSlots : public std::runtime_error {
        explicit OutOfSlots() : std::runtime_error("Slotmap out of slots.") {}
    };

    template<unsigned IdBits, unsigned GenerationBits>
    struct Id {
        static_assert(GenerationBits > 0);
        static_assert(GenerationBits < IdBits); // It follows that IdBits > 0

        static constexpr unsigned IndexBits = IdBits - GenerationBits;

        // Helper type trait to select the appropriate uint_leastN_t type
        template<unsigned Bits>
        struct uint_least_t;

        template<>
        struct uint_least_t<8> {
            using type = std::uint_least8_t;
        };

        template<>
        struct uint_least_t<16> {
            using type = std::uint_least16_t;
        };

        template<>
        struct uint_least_t<32> {
            using type = std::uint_least32_t;
        };

        template<>
        struct uint_least_t<64> {
            using type = std::uint_least64_t;
        };

        using UInt = typename uint_least_t<std::max(GenerationBits, IndexBits)>::type;

        static constexpr Id limits() { return {std::numeric_limits<UInt>::max(), std::numeric_limits<UInt>::max()}; }

        static constexpr Id null() { return {0, 0}; }

        static Id free(UInt index) { return {index, 0}; }

        static UInt evolve(UInt generation) { return std::max<UInt>(1, (generation + 1) & limits().generation); }

        bool valid() const { return generation != 0; }

        UInt index : IndexBits;
        UInt generation : GenerationBits;
    };

    template<unsigned IdBits, unsigned GenerationBits>
    bool operator==(const Id<IdBits, GenerationBits>& a, const Id<IdBits, GenerationBits>& b)
    {
        return a.index == b.index && a.generation == b.generation;
    }

} // namespace Twig::Container::detail
