#include "sv_manager.hpp"

SVManager *SVManager::instance = nullptr;

SVManager::SVManager()
{
    debug_handler = DebugHandler::GetInstance();
}

bool SVManager::tryInitialize()
{
    int version = getVersion();
    bool is_sv = version != -1;
    if (!is_valid && is_sv)
    {
        // end of fixinitspec
        debug_handler->SetBreakpoint(pokemon_generation_address[version], &spawn_breakpoint_idx, [this](ThreadContext *thread_context)
                                     { spawnEvent(thread_context); });
    }
    is_valid = is_sv;
    return is_valid;
}

bool SVManager::getIsValid()
{
    return is_valid;
}

SVManager *SVManager::GetInstance()
{
    if (instance == nullptr)
    {
        instance = new SVManager();
    }
    return instance;
}
void SVManager::spawnEvent(ThreadContext *thread_context)
{
    u64 init_spec_addr = thread_context->sp + 0x18;
    InitSpec init_spec;
    debug_handler->ReadMemory(&init_spec, init_spec_addr, sizeof(init_spec));
    if (init_spec.species)
    {
        // TODO: ec as hash is probably a bad idea
        pokemon[init_spec.ec] = init_spec;
    }
}
void SVManager::update()
{
    if (debug_handler->isAttached() && !debug_handler->isBroken() && tryInitialize())
    {
        int version = getVersion();
        debug_handler->Poll();
    }
}
