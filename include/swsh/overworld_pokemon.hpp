#include <string>
#include "species.hpp"
#include "nature.hpp"
#include "xoroshiro.hpp"

#ifndef OVERWORLD_POKEMON_HPP
#define OVERWORLD_POKEMON_HPP

typedef struct
{
    struct __attribute__((packed))
    {
        Species species;
        u8 pad2[6];
        u8 shininess;
        u8 pad9[3];
        Nature nature;
        u8 pad15[1];
        u8 gender;
        u8 pad17[3];
        u8 ability;
        u8 pad21[7];
        u16 held_item;
        u8 pad30[2];
        u16 egg_move;
        u8 pad34[19];
        u8 guaranteed_ivs;
        u8 pad54[18];
        u16 mark;
        u16 brilliant;
        u8 pad76[4];
        u32 seed;
    } init_spec;

    struct
    {
        u32 ec;
        u32 pid;
        u8 ivs[6];
        u8 height;
        u8 weight;
    } fixed_info;

    u64 unique_hash;
    bool loaded;

    void generatedFixed(u16 tsv)
    {
        Xoroshiro rng(init_spec.seed);

        fixed_info.ec = rng.rand<0xFFFFFFFF>();
        fixed_info.pid = rng.rand<0xFFFFFFFF>();
        bool pid_is_shiny = (((fixed_info.pid >> 16) ^ (fixed_info.pid & 0xFFFF)) ^ tsv) >> 4;
        if (init_spec.shininess == 2)
        {
            if (pid_is_shiny)
            {
                fixed_info.pid ^= 0x10000000;
            }
        }
        else
        {
            if (!pid_is_shiny)
            {
                fixed_info.pid = ((tsv ^ (fixed_info.pid & 0xFFFF)) << 16) | (fixed_info.pid & 0xFFFF);
            }
        }
        std::fill(std::begin(fixed_info.ivs), std::end(fixed_info.ivs), 255);
        for (int i = 0; i < init_spec.guaranteed_ivs;)
        {
            int index = rng.rand<6>();
            if (fixed_info.ivs[index] == 255)
            {
                fixed_info.ivs[index] = 31;
                i++;
            }
        }
        for (int i = 0; i < 6; i++)
        {
            if (fixed_info.ivs[i] == 255)
            {
                fixed_info.ivs[i] = rng.rand<32>();
            }
        }
        fixed_info.height = rng.rand<0x81>() + rng.rand<0x80>();
        fixed_info.weight = rng.rand<0x81>() + rng.rand<0x80>();
    }

    std::string toString()
    {
        char pokemon_text[80];
        snprintf(
            pokemon_text,
            80,
            "%s %s%s",
            init_spec.gender == 1 ? "♂" : "♀",
            init_spec.shininess == 1 ? "★ " : "",
            speciesName().c_str());
        return std::string(pokemon_text);
    }

    std::string speciesName()
    {
        return SPECIES_LUT[init_spec.species];
    }

    std::string natureName()
    {
        return NATURE_LUT[init_spec.nature];
    }
} sOverworldPokemon;

#endif