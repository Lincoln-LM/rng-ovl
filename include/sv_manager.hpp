#include <map>
#include <tesla.hpp>
#include "debug_handler.hpp"
#include "species.hpp"

#ifndef SV_MANAGER_HPP
#define SV_MANAGER_HPP

typedef struct __attribute__((packed))
{
    s64 ec;
    s64 pid;
    s64 otid;
    s16 species;
    s16 form;
    s16 field4_0x1c;
    s16 level;
    s16 gender;
    s16 nature;
    s16 stat_nature;
    s8 ability;
    s8 shiny_rolls;
    s16 ivs[6];
    s16 evs[6];
    s32 field13_0x40;
    s8 guaranteed_ivs;
    s8 field15_0x45;
    s16 weight;
    s16 height;
    s16 scale;
} InitSpec;

class SVManager
{
public:
    SVManager(SVManager &other) = delete;
    void operator=(const SVManager &) = delete;
    static SVManager *GetInstance();
    void spawnEvent(ThreadContext *thread_context);
    void update();
    int getVersion()
    {
        return 0;
        // u64 title_id;
        // debug_handler->GetTitleId(&title_id);
        // if (title_id == )
        // {
        //     return 0;
        // }
        // if (title_id == )
        // {
        //     return 1;
        // }
        // return -1;
    };
    bool tryInitialize();
    bool getIsValid();
    std::unordered_map<u64, InitSpec> getPokemon() { return pokemon; }

private:
    SVManager();
    static SVManager *instance;
    DebugHandler *debug_handler;
    u64 spawn_breakpoint_idx = -1;
    std::unordered_map<u64, InitSpec> pokemon;
    bool is_valid = false;

    const u64 pokemon_generation_address[2] = {0xD9750C, 0x0};
};
#endif