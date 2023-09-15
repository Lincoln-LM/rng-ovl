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
    u16 getTsv();
    int getVersion()
    {
        u64 title_id;
        debug_handler->GetTitleId(&title_id);
        if (title_id == 0x0100ABF008968000)
        {
            return 0;
        }
        if (title_id == 0x01008DB008C2C000)
        {
            return 1;
        }
        return -1;
    };
    bool tryInitialize();
    bool getIsValid();
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
    bool is_valid = false;

    const u64 overworld_generation_address[2] = {0xD317BC - 0x30, 0xD317BC};
    const u64 object_creation_address[2] = {0xEA2FE4 - 0x30, 0xEA2FE4};
    const u64 pokemon_object_update_address[2] = {0xD5EFE0 - 0x30, 0xD5EFE0};
    const u64 npc_init_address_0[2] = {0xD600F0 - 0x30, 0xD600F0};
    const u64 npc_init_address_1[2] = {0xD6F6D0 - 0x30, 0xD6F6D0};
    const u64 npc_init_address_2[2] = {0xDAA010 - 0x30, 0xDAA010};
};
#endif