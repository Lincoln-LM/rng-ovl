#include <bit>
#include "common.hpp"

#ifndef XOROSHIRO_HPP
#define XOROSHIRO_HPP

class Xoroshiro
{
public:
    Xoroshiro(u32 seed)
    {
        state[0] = seed;
        state[1] = 0x82A2B175229D6A5B;
    }
    u64 next()
    {
        u64 s0 = state[0];
        u64 s1 = state[1];
        u64 result = s0 + s1;

        s1 ^= s0;
        state[0] = std::rotl(s0, 24) ^ s1 ^ (s1 << 16);
        state[1] = std::rotl(s1, 37);

        return result;
    }
    template <u32 max>
    u32 rand()
    {
        auto bit_mask = [](u32 x) constexpr
        {
            x--;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            return x;
        };

        constexpr u32 mask = bit_mask(max);
        if constexpr ((max - 1) == mask)
        {
            return next() & mask;
        }
        else
        {
            u32 result;
            do
            {
                result = next() & mask;
            } while (result >= max);
            return result;
        }
    }

private:
    u64 state[2];
};

#endif