#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include <memory>
#include "debug_handler.hpp"

// referenced: https://github.com/Atmosphere-NX/Atmosphere/tree/master/stratosphere/dmnt.gen2

class RNGGui : public tsl::Gui
{
public:
    RNGGui() {}

    virtual tsl::elm::Element *createUI() override
    {
        debug_handler = DebugHandler::GetInstance();

        auto frame = new tsl::elm::OverlayFrame("RNG Overlay", "v0.0.0");

        list = new tsl::elm::List();
        attach_button = new tsl::elm::ListItem("Attach...");
        attach_button->setClickListener([this](u64 keys)
                                        {
            if (!is_focused) {
                return false;
            }
            if (!(keys & HidNpadButton_A)) {
                return false;
            }
            if (debug_handler->isAttached()) {
                attach_button->setText(R_SUCCEEDED(debug_handler->Detach()) ? "Detached." : "Failure to detach.");
                return true;
            }

            bool succeeded = R_SUCCEEDED(debug_handler->Attach());
            attach_button->setText(succeeded ? "Detach" : "Failure to attach.");
            if (succeeded) {
                debug_handler->SetBreakpoint(0xD317BC, &overworld_breakpoint_idx, [this](ThreadContext* thread_context){
                    // overworld spawn event

                    u64 pokemon_addr = thread_context->cpu_gprs[20].x;
                    u32 species;
                    u64 fixed_seed;
                    debug_handler->ReadMemory(&species, pokemon_addr, sizeof(species));
                    debug_handler->ReadMemory(&fixed_seed, pokemon_addr + 80, sizeof(fixed_seed));
                    
                    if (species)
                    {
                        char text[23];
                        snprintf(text, 23, "#%d, Seed: 0x%08X", species, fixed_seed);
                        auto new_pokemon = new tsl::elm::ListItem(std::string(text));
                        list->addItem(new_pokemon);
                        overworld_pokemon.push_back(new_pokemon);
                    }
                });
                debug_handler->Continue();
            }
            
            return succeeded; });

        list->addItem(attach_button);
        npc_counter = new tsl::elm::ListItem("NPCs: ?");
        list->addItem(npc_counter);
        clear_button = new tsl::elm::ListItem("Clear Spawn List");
        clear_button->setClickListener([this](u64 keys)
                                       {
            if (keys & HidNpadButton_A) {
                for (auto& item : overworld_pokemon) {
                    list->removeItem(item);
                }
                overworld_pokemon.clear();
                return true;
            } 
            return false; });
        list->addItem(clear_button);
        frame->setContent(list);

        return frame;
    }

    virtual void update() override
    {
        if (debug_handler->isAttached() && !debug_handler->isBroken())
        {
            u64 npc_count = 0;
            u64 list_start;
            u64 list_end;
            debug_handler->ReadHeapMemory(&list_start, 0x44129298 + 0xb0, sizeof(list_start));
            debug_handler->ReadHeapMemory(&list_end, 0x44129298 + 0xb8, sizeof(list_end));
            for (u64 current_npc = list_start; current_npc < list_end; current_npc += 8)
            {
                u64 temp;
                u64 initialization_function;
                debug_handler->ReadMemory(&temp, current_npc, sizeof(temp));
                debug_handler->ReadMemory(&temp, temp, sizeof(temp));
                debug_handler->ReadMemory(&initialization_function, temp + 0xf0, sizeof(temp));
                initialization_function = debug_handler->normalizeMain(initialization_function);
                if (
                    initialization_function == 0xd600f0 ||
                    initialization_function == 0xd6f6d0 ||
                    initialization_function == 0xdaa010)
                {
                    npc_count++;
                }
            }
            char text[11];
            snprintf(text, 11, "NPCs: %d", npc_count);
            npc_counter->setText(std::string(text));

            debug_handler->Poll();
        }
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override
    {
        if (keysDown & HidNpadButton_Plus)
        {
            is_focused = !is_focused;
        }
        tsl::hlp::requestForeground(is_focused);
        return !is_focused;
    }

private:
    DebugHandler *debug_handler;

    tsl::elm::List *list;
    tsl::elm::ListItem *attach_button;
    tsl::elm::ListItem *npc_counter;
    tsl::elm::ListItem *clear_button;

    u64 overworld_breakpoint_idx = -1;
    std::vector<u64> seen;
    std::vector<tsl::elm::ListItem *> overworld_pokemon;
    bool is_focused = true;
};

class RNGOverlay : public tsl::Overlay
{
public:
    virtual void initServices() override
    {
        Result rc = ldrDmntInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);
    }
    virtual void exitServices() override
    {
        DebugHandler::GetInstance()->Detach();
        ldrDmntExit();
    }

    virtual void onShow() override {}
    virtual void onHide() override {}

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override
    {
        return initially<RNGGui>();
    }
};

int main(int argc, char **argv)
{
    return tsl::loop<RNGOverlay>(argc, argv);
}
