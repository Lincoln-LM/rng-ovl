#include <map>
#include <tesla.hpp>
#include "overworld_pokemon.hpp"
#include "debug_handler.hpp"
#include "weather.hpp"

#ifndef SWSH_MANAGER_HPP
#define SWSH_MANAGER_HPP

class SwShManager
{
public:
    SwShManager(SwShManager &other) = delete;
    void operator=(const SwShManager &) = delete;
    static SwShManager *GetInstance();
    void startRainCalibration();
    void startFlyCalibration();
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
    Weather getWeather() { return weather; }
    int getRainCalibration() { return rain_calibration; }
    int getFlyCalibration() { return fly_calibration; }
    int getFlyNPCCalibration() { return fly_npc_calibration; }
    std::unordered_map<u64, sOverworldPokemon> getOverworldPokemon() { return overworld_pokemon; }

private:
    SwShManager();
    static SwShManager *instance;
    DebugHandler *debug_handler;
    void overworldSpawnEvent(ThreadContext *thread_context);
    void objectCreationEvent(ThreadContext *thread_context);
    u64 overworld_breakpoint_idx = -1;
    u64 object_creation_breakpoint_idx = -1;
    u64 rain_breakpoint_idx = -1;
    u64 fly_breakpoint_0_idx = -1;
    u64 fly_breakpoint_1_idx = -1;
    u64 fly_npc_breakpoint_idx = -1;
    std::unordered_map<u64, sOverworldPokemon> overworld_pokemon;
    sOverworldPokemon latest_pokemon;
    bool new_spawn = false;
    int npc_count = 0;
    int rain_calibration = 0;
    int fly_calibration = 0;
    int fly_npc_calibration = 0;
    Weather weather = Weather::INVALID;
    bool is_valid = false;

    const u64 overworld_generation_address[2] = {0xD317BC - 0x30, 0xD317BC};
    const u64 object_creation_address[2] = {0xEA2FE4 - 0x30, 0xEA2FE4};
    const u64 pokemon_object_update_address[2] = {0xD5EFE0 - 0x30, 0xD5EFE0};
    const u64 rain_rand_address[2] = {0xEB6BF8 - 0x30, 0xEB6BF8};
    const u64 fly_rand_address_0[2] = {0x1471CC0 - 0x30, 0x1471CC0}; // GetPublicRand
    const u64 fly_rand_address_1[2] = {0xD5AD40 - 0x30, 0xD5AD40};
    const u64 npc_rand_address[2] = {0xceec00 - 0x30, 0xceec00};
    const u64 npc_init_address_0[2] = {0xD600F0 - 0x30, 0xD600F0};
    const u64 npc_init_address_1[2] = {0xD6F6D0 - 0x30, 0xD6F6D0};
    const u64 npc_init_address_2[2] = {0xDAA010 - 0x30, 0xDAA010};
    const u64 npc_init_address_3[2] = {0xDB4A40 - 0x30, 0xDB4A40};

    const u64 npc_sub_address_0[2] = {0xCF1A20 - 0x30, 0xCF1A20};
    const u64 npc_sub_address_1[2] = {0xDB9140 - 0x30, 0xDB9140};
};
#endif