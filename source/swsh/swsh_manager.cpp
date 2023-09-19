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
    debug_handler->ReadMemory(&pokemon.init_spec, pokemon_addr, sizeof(pokemon));
    pokemon.generatedFixed(getTsv());
    pokemon.loaded = true;
    new_spawn = true;

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
    if (new_spawn || overworld_pokemon.find(hash) == overworld_pokemon.end())
    {
        new_spawn = false;
        latest_pokemon.unique_hash = hash;
        overworld_pokemon[latest_pokemon.unique_hash] = latest_pokemon;
    }
    else
    {
        overworld_pokemon[hash].loaded = true;
    }
}
void SwShManager::update()
{
    if (!getIsValid())
    {
        DebugHandler::GetInstance()->Detach();
    }
    if (debug_handler->isAttached() && !debug_handler->isBroken() && tryInitialize())
    {
        int version = getVersion();
        for (auto &pair : overworld_pokemon)
        {
            pair.second.loaded = false;
        }

        npc_count = 0;
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
            debug_handler->ReadMemory(&update_function, temp + 0x90, sizeof(temp));
            debug_handler->ReadMemory(&initialization_function, temp + 0xf0, sizeof(temp));
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
}
u16 SwShManager::getTsv()
{
    u32 id32;
    debug_handler->ReadHeapMemory(&id32, 0x45068F18 + 0xA0, sizeof(id32));
    return id32 ^ (id32 >> 16);
}