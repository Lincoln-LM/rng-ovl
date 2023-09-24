#include "swsh.hpp"

SwShManager *SwShManager::instance = nullptr;

SwShManager::SwShManager()
{
    debug_handler = DebugHandler::GetInstance();
}

bool SwShManager::tryInitialize()
{
    int version = getVersion();
    bool is_swsh = version != -1;
    if (!is_valid && is_swsh)
    {
        // end of rng generation for overworld pokemon
        debug_handler->SetBreakpoint(overworld_generation_address[version], &overworld_breakpoint_idx, [this](ThreadContext *thread_context)
                                     { overworldSpawnEvent(thread_context); });
        // equivalent of npc_vector.push_back(newest_npc)
        debug_handler->SetBreakpoint(object_creation_address[version], &object_creation_breakpoint_idx, [this](ThreadContext *thread_context)
                                     { objectCreationEvent(thread_context); });
        // run before the two rand(20001) that are used during rain/thunderstorm, disabled by default
        debug_handler->SetBreakpoint(rain_rand_address[version], &rain_breakpoint_idx, [this](ThreadContext *thread_context)
                                     { rain_calibration += 2; });
        // GetPublicRand, called with x0=100 for an unknown purpose while flying
        debug_handler->SetBreakpoint(fly_rand_address_0[version], &fly_breakpoint_0_idx, [this](ThreadContext *thread_context)
                                     { fly_calibration++; });
        // unknown rand(100) that happens when flying
        debug_handler->SetBreakpoint(fly_rand_address_1[version], &fly_breakpoint_1_idx, [this](ThreadContext *thread_context)
                                     { fly_calibration++; });
        // npc rand(91), called additional times upon fly
        debug_handler->SetBreakpoint(npc_rand_address[version], &fly_npc_breakpoint_idx, [this](ThreadContext *thread_context)
                                     { fly_npc_calibration++; });

        // only enable when calibrating
        debug_handler->DisableBreakpoint(fly_breakpoint_0_idx);
        debug_handler->DisableBreakpoint(fly_breakpoint_1_idx);
        debug_handler->DisableBreakpoint(fly_npc_breakpoint_idx);
        debug_handler->DisableBreakpoint(rain_breakpoint_idx);
    }
    is_valid = is_swsh;
    return is_valid;
}

bool SwShManager::getIsValid()
{
    return is_valid;
}

SwShManager *SwShManager::GetInstance()
{
    if (instance == nullptr)
    {
        instance = new SwShManager();
    }
    return instance;
}
void SwShManager::overworldSpawnEvent(ThreadContext *thread_context)
{
    u64 pokemon_addr = thread_context->cpu_gprs[20].x;
    sOverworldPokemon pokemon;
    debug_handler->ReadMemory(&pokemon.init_spec, pokemon_addr, sizeof(pokemon.init_spec));
    pokemon.generatedFixed(getTsv());
    pokemon.loaded = true;
    new_spawn = true;
    debug_handler->DisableBreakpoint(fly_breakpoint_0_idx);
    debug_handler->DisableBreakpoint(fly_breakpoint_1_idx);
    debug_handler->DisableBreakpoint(fly_npc_breakpoint_idx);
    debug_handler->DisableBreakpoint(rain_breakpoint_idx);

    if (pokemon.init_spec.species)
    {
        latest_pokemon = pokemon;
        debug_handler->EnableBreakpoint(object_creation_breakpoint_idx);
    }
}
void SwShManager::objectCreationEvent(ThreadContext *thread_context)
{
    u64 temp;
    u64 hash;
    debug_handler->ReadMemory(&temp, thread_context->cpu_gprs[8].x, sizeof(temp));
    debug_handler->ReadMemory(&hash, temp + 0x1a0, sizeof(hash));
    if (new_spawn)
    {
        new_spawn = false;
        latest_pokemon.unique_hash = hash;
        overworld_pokemon[latest_pokemon.unique_hash] = latest_pokemon;
    }
    else if (overworld_pokemon.find(hash) != overworld_pokemon.end())
    {
        overworld_pokemon[hash].loaded = true;
    }
}
void SwShManager::startRainCalibration()
{
    rain_calibration = 0;
    debug_handler->EnableBreakpoint(rain_breakpoint_idx);
}
void SwShManager::startFlyCalibration()
{
    fly_calibration = 0;
    fly_npc_calibration = 0;
    debug_handler->EnableBreakpoint(fly_breakpoint_0_idx);
    debug_handler->EnableBreakpoint(fly_breakpoint_1_idx);
    debug_handler->EnableBreakpoint(fly_npc_breakpoint_idx);
}
void SwShManager::update()
{
    if (debug_handler->isAttached() && !debug_handler->isBroken() && tryInitialize())
    {
        int version = getVersion();
        for (auto &pair : overworld_pokemon)
        {
            pair.second.loaded = false;
        }

        npc_count = 0;
        u64 temp;
        debug_handler->ReadMainMemory(&temp, 0x26365B8, sizeof(temp));
        debug_handler->ReadMemory(&temp, temp + 0x90, sizeof(temp));
        debug_handler->ReadMemory(&temp, temp + 0x408, sizeof(temp));
        debug_handler->ReadMemory(&temp, temp + 0x3F8, sizeof(temp));
        debug_handler->ReadMemory(&weather, temp + 0x60, sizeof(weather));
        if (weather > Weather::MIST)
        {
            weather = Weather::INVALID;
        }
        u64 list_start;
        u64 list_end;
        debug_handler->ReadHeapMemory(&list_start, 0x44129298 + 0xb0, sizeof(list_start));
        debug_handler->ReadHeapMemory(&list_end, 0x44129298 + 0xb8, sizeof(list_end));
        for (u64 current_npc = list_start; current_npc < list_end; current_npc += 8)
        {
            u64 temp;
            u64 arg0;
            u64 initialization_function;
            u64 update_function;
            debug_handler->ReadMemory(&arg0, current_npc, sizeof(arg0));
            debug_handler->ReadMemory(&temp, arg0, sizeof(temp));
            debug_handler->ReadMemory(&update_function, temp + 0x90, sizeof(update_function));
            debug_handler->ReadMemory(&initialization_function, temp + 0xf0, sizeof(initialization_function));
            update_function = debug_handler->normalizeMain(update_function);
            initialization_function = debug_handler->normalizeMain(initialization_function);
            // is PokemonObject
            if (update_function == pokemon_object_update_address[version])
            {
                u64 unique_hash;
                debug_handler->ReadMemory(&temp, current_npc, sizeof(temp));
                debug_handler->ReadMemory(&unique_hash, temp + 0x1a0, sizeof(unique_hash));
                if (overworld_pokemon.find(unique_hash) != overworld_pokemon.end())
                {
                    overworld_pokemon[unique_hash].loaded = true;
                }
            }
            if (
                initialization_function == npc_init_address_0[version] ||
                initialization_function == npc_init_address_1[version] ||
                initialization_function == npc_init_address_2[version])
            // TODO: 0xd5ba20 (shield) can cause menu close advances
            {
                npc_count++;
            }
            else if (initialization_function == npc_init_address_3[version])
            {
                u64 sub_list_start;
                u64 sub_list_end;
                debug_handler->ReadMemory(&sub_list_start, arg0 + 0x1e8, sizeof(sub_list_start));
                debug_handler->ReadMemory(&sub_list_end, arg0 + 0x1f0, sizeof(sub_list_end));
                // actual logic near 0xdb4af8
                for (u64 current_item = sub_list_start; current_item < sub_list_end; current_item += 8)
                {
                    debug_handler->ReadMemory(&temp, current_item, sizeof(temp));
                    debug_handler->ReadMemory(&temp, temp + 0x50, sizeof(temp));
                    debug_handler->ReadMemory(&temp, temp, sizeof(temp));
                    u64 some_func = debug_handler->normalizeMain(temp);
                    if (
                        some_func == npc_sub_address_0[version] ||
                        some_func == npc_sub_address_1[version])
                    {
                        npc_count++;
                    }
                }
            }
        }
        debug_handler->Poll();
    }
    if (!getIsValid())
    {
        DebugHandler::GetInstance()->Detach();
    }
}
u16 SwShManager::getTsv()
{
    u32 id32;
    debug_handler->ReadHeapMemory(&id32, 0x45068F18 + 0xA0, sizeof(id32));
    return id32 ^ (id32 >> 16);
}
