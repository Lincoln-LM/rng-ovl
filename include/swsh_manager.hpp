#include <map>
#include <tesla.hpp>
#include "debug_handler.hpp"
#include "species.hpp"

#ifndef SWSH_MANAGER_HPP
#define SWSH_MANAGER_HPP

typedef struct
{
    struct __attribute__((packed))
    {
        Species species;
        u8 pad2[6];
        u8 shininess;
        u8 pad9[3];
        u16 nature;
        u8 pad14[2];
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
    } generation_data;
    u64 unique_hash;
    bool loaded;
} sOverworldPokemon;

class SwShManager
{
public:
    SwShManager(SwShManager &other) = delete;
    void operator=(const SwShManager &) = delete;
    static SwShManager *GetInstance();
    void overworldSpawnEvent(ThreadContext *thread_context);
    void objectCreationEvent(ThreadContext *thread_context);
    void update();
    int getNpcCount() { return npc_count; }
    std::unordered_map<u64, sOverworldPokemon> getOverworldPokemon() { return overworld_pokemon; }

private:
    SwShManager();
    static SwShManager *instance;
    DebugHandler *debug_handler;
    u64 overworld_breakpoint_idx = -1;
    u64 object_creation_breakpoint_idx = -1;
    u64 request_deletion_breakpoint_idx = -1;
    std::unordered_map<u64, sOverworldPokemon> overworld_pokemon;
    sOverworldPokemon latest_pokemon;
    bool new_spawn = false;
    int npc_count = 0;
};
#endif