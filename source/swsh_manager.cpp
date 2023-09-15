#include "swsh_manager.hpp"

SwShManager *SwShManager::instance = nullptr;

SwShManager::SwShManager()
{
    debug_handler = DebugHandler::GetInstance();
    // end of rng generation for overworld pokemon
    debug_handler->SetBreakpoint(0xD317BC, &overworld_breakpoint_idx, [this](ThreadContext *thread_context)
                                 { overworldSpawnEvent(thread_context); });
    // equivalent of npc_vector.push_back(newest_npc)
    debug_handler->SetBreakpoint(0xEA2FE4, &object_creation_breakpoint_idx, [this](ThreadContext *thread_context)
                                 { objectCreationEvent(thread_context); });
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
    debug_handler->ReadMemory(&pokemon.generation_data, pokemon_addr, sizeof(pokemon));
    pokemon.loaded = true;
    new_spawn = true;

    if (pokemon.generation_data.species)
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
    if (debug_handler->isAttached() && !debug_handler->isBroken())
    {
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
            u64 initialization_function;
            u64 update_function;
            debug_handler->ReadMemory(&temp, current_npc, sizeof(temp));
            debug_handler->ReadMemory(&temp, temp, sizeof(temp));
            debug_handler->ReadMemory(&update_function, temp + 0x90, sizeof(temp));
            debug_handler->ReadMemory(&initialization_function, temp + 0xf0, sizeof(temp));
            update_function = debug_handler->normalizeMain(update_function);
            initialization_function = debug_handler->normalizeMain(initialization_function);
            // is PokemonObject
            if (update_function == 0xd5efe0)
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
                initialization_function == 0xd600f0 ||
                initialization_function == 0xd6f6d0 ||
                initialization_function == 0xdaa010)
            // TODO: 0xd5ba20 can cause menu close advances
            {
                npc_count++;
            }
        }
        debug_handler->Poll();
    }
}
