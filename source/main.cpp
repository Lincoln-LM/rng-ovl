#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include <memory>
#include <common.hpp>

// butchered macros from ams
#define R_SUCCEED() \
    {               \
        return 0;   \
    }

#define R_THROW(res_expr) \
    {                     \
        return res_expr;  \
    }

#define R_TRY(res_expr)                                                     \
    {                                                                       \
        if (const auto _tmp_r_try_rc = (res_expr); R_FAILED(_tmp_r_try_rc)) \
        {                                                                   \
            R_THROW(_tmp_r_try_rc);                                         \
        }                                                                   \
    }

// referenced: https://github.com/Atmosphere-NX/Atmosphere/tree/master/stratosphere/dmnt.gen2
class DebugHandler
{
public:
    DebugHandler(){};
    Result Attach()
    {
        // find & hook into application
        u64 pid;
        R_TRY(pmdmntGetApplicationProcessId(&pid));
        R_TRY(svcDebugActiveProcess(&debug_handle, pid));

        // attach debugger
        R_TRY(this->Start())

        // TODO: is there any point in re-grabbing pid after attaching
        u64 pid_value;
        R_TRY(svcGetProcessId(&pid_value, debug_handle));
        process_id = pid_value;

        // get heap base
        R_TRY(svcGetInfo(&heap_base, InfoType_HeapRegionAddress, debug_handle, 0));

        // get main base
        LoaderModuleInfo modules[2];
        s32 module_count = 0;
        R_TRY(ldrDmntGetProcessModuleInfo(pid, modules, 2, &module_count));

        LoaderModuleInfo *proc_module = 0;
        if (module_count == 2)
        {
            main_base = modules[1].base_address;
        }
        else
        {
            main_base = modules[0].base_address;
        }

        R_SUCCEED();
    }
    Result Detach()
    {
        R_TRY(DisableAllBreakpoints());
        R_TRY(svcCloseHandle(debug_handle));
        attached = false;

        R_SUCCEED();
    }
    Result Start()
    {
        while (!attached)
        {
            s32 dummy_index;
            R_TRY(svcWaitSynchronization(&dummy_index, &debug_handle, 1, -1));

            DebugEventInfo debug_event;
            R_TRY(svcGetDebugEvent(&debug_event, debug_handle));

            switch (debug_event.type)
            {
            case DebugEvent_Exception:
                if (debug_event.info.exception.type == DebugException_DebuggerAttached)
                {
                    attached = true;
                }
                break;
            default:
                break;
            }
        }

        broken = true;

        R_SUCCEED()
    }
    Result DisableBreakpoint(u64 idx)
    {
        R_TRY(WriteMainMemory(&breakpoints[idx].original_instruction, breakpoints[idx].address, sizeof(breakpoints[idx].original_instruction)));

        R_SUCCEED();
    }
    Result DisableAllBreakpoints()
    {
        for (int i = 0; i < breakpoints.size(); i++)
        {
            R_TRY(DisableBreakpoint(i));
        }

        R_SUCCEED();
    }
    Result WriteBreakpoint(u64 address, u32 *original_instruction_out)
    {
        u32 bp_instr = 0xD4200000;
        // store original instruction
        R_TRY(ReadMainMemory(original_instruction_out, address, sizeof(bp_instr)));
        // write software breakpoint
        R_TRY(WriteMainMemory(&bp_instr, address, sizeof(bp_instr)));

        R_SUCCEED();
    }
    Result SetBreakpoint(u64 address, u64 *idx_out, std::function<void(ThreadContext *)> on_break)
    {
        *idx_out = static_cast<u64>(breakpoints.size());
        Breakpoint bp = {
            .address = address,
            .on_break = on_break,
        };
        R_TRY(WriteBreakpoint(address, &bp.original_instruction));
        breakpoints.push_back(bp);

        R_SUCCEED();
    }
    Result GetCurrentBreakpoint(DebugEventInfo *event_info, Breakpoint *breakpoint_out)
    {
        ThreadContext thread_context;
        R_TRY(GetThreadContext(&thread_context, event_info->thread_id, RegisterGroup_All));
        u64 break_address = thread_context.pc.x - main_base;
        auto bp = std::find_if(breakpoints.begin(), breakpoints.end(), [&break_address](const Breakpoint &bp)
                                        { return bp.address == break_address; });
        if (bp == breakpoints.end()) {
            R_THROW(1);
        }
        *breakpoint_out = *bp;
        R_SUCCEED();
    }
    Result ResumeFromBreakpoint(Breakpoint breakpoint)
    {
        // TODO: jump instructions where the next instruction to execute is not just +4
        // break at next instruction
        u32 next_instr;
        R_TRY(WriteBreakpoint(breakpoint.address + 4, &next_instr));
        // restore original instruction
        R_TRY(WriteMainMemory(&breakpoint.original_instruction, breakpoint.address, sizeof(breakpoint.original_instruction)));
        // release process from breakpoint
        R_TRY(Continue());
        // catch break at next instruction
        s32 dummy_index;
        // stall until break
        R_TRY(svcWaitSynchronization(&dummy_index, &debug_handle, 1, -1));
        DebugEventInfo next_event_info;
        R_TRY(GetProcessDebugEvent(&next_event_info));
        // TODO: redundancy for if break isnt caught?
        // restore original breakpoint
        u32 dummy_value;
        R_TRY(WriteBreakpoint(breakpoint.address, &dummy_value));
        // restore next instruction
        R_TRY(WriteMainMemory(&next_instr, breakpoint.address + 4, sizeof(next_instr)));
        // release process
        R_TRY(Continue());

        R_SUCCEED();
    }
    Result PollForBreaks()
    {
        if (!attached || broken)
        {
            R_SUCCEED();
        }
        DebugEventInfo event_info;
        R_TRY(GetProcessDebugEvent(&event_info));
        if (broken)
        {
            Breakpoint bp;
            if (R_SUCCEEDED(GetCurrentBreakpoint(&event_info, &bp))) {
                ThreadContext thread_context;
                R_TRY(GetThreadContext(&thread_context, event_info.thread_id, RegisterGroup_All));
                bp.on_break(&thread_context);
                R_TRY(ResumeFromBreakpoint(bp));
            } else {
                R_TRY(Continue());
            }
        }
        R_SUCCEED()
    }
    Result GetProcessDebugEvent(DebugEventInfo *out)
    {
        R_TRY(svcGetDebugEvent(out, debug_handle));

        if (out->flags & DebugEventFlag_Stopped)
        {
            broken = true;
        }

        R_SUCCEED();
    }
    Result Continue()
    {
        u64 thread_ids[] = {0};
        R_TRY(svcContinueDebugEvent(debug_handle, ContinueFlag_ExceptionHandled | ContinueFlag_EnableExceptionEvent | ContinueFlag_ContinueAll, thread_ids, 1));
        broken = false;
        R_SUCCEED();
    }
    Result GetThreadContext(ThreadContext *out, u64 thread_id, u32 flags)
    {
        return svcGetDebugThreadContext(out, debug_handle, thread_id, flags);
    }

    Result ReadMemory(void *out, u64 address, u64 size)
    {
        return svcReadDebugProcessMemory(out, debug_handle, address, size);
    }
    Result ReadMainMemory(void *out, u64 address, u64 size)
    {
        return ReadMemory(out, main_base + address, size);
    }
    Result ReadHeapMemory(void *out, u64 address, u64 size)
    {
        return ReadMemory(out, heap_base + address, size);
    }

    Result WriteMemory(void *buffer, u64 address, u64 size)
    {
        return svcWriteDebugProcessMemory(debug_handle, buffer, address, size);
    }
    Result WriteMainMemory(void *buffer, u64 address, u64 size)
    {
        return WriteMemory(buffer, main_base + address, size);
    }
    Result WriteHeapMemory(void *buffer, u64 address, u64 size)
    {
        return WriteMemory(buffer, heap_base + address, size);
    }

    u64 normalizeMain(u64 address)
    {
        return address - main_base;
    }

    u64 normalizeHeap(u64 address)
    {
        return address - heap_base;
    }

    bool attached = false;
    bool broken = false;

private:
    Handle debug_handle;
    u64 process_id;
    u64 main_base;
    u64 heap_base;
    std::vector<Breakpoint> breakpoints;
};

class RNGGui : public tsl::Gui
{
public:
    RNGGui() {}

    virtual tsl::elm::Element *createUI() override
    {
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
            if (debug_handler.attached) {
                attach_button->setText(R_SUCCEEDED(debug_handler.Detach()) ? "Detached." : "Failure to detach.");
                return true;
            }

            bool succeeded = R_SUCCEEDED(debug_handler.Attach());
            attach_button->setText(succeeded ? "Attached!" : "Failure to attach.");
            if (succeeded) {
                debug_handler.SetBreakpoint(0xD317BC, &overworld_breakpoint_idx, [this](ThreadContext* thread_context){
                    // overworld spawn event

                    u64 pokemon_addr = thread_context->cpu_gprs[20].x;
                    u32 species;
                    u64 fixed_seed;
                    debug_handler.ReadMemory(&species, pokemon_addr, sizeof(species));
                    debug_handler.ReadMemory(&fixed_seed, pokemon_addr + 80, sizeof(fixed_seed));
                    
                    if (species)
                    {
                        char text[23];
                        snprintf(text, 23, "#%d, Seed: 0x%08X", species, fixed_seed);
                        auto new_pokemon = new tsl::elm::ListItem(std::string(text));
                        list->addItem(new_pokemon);
                        overworld_pokemon.push_back(new_pokemon);
                    }
                });
                debug_handler.Continue();
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
        if (debug_handler.attached && !debug_handler.broken)
        {
            u64 npc_count = 0;
            u64 list_start;
            u64 list_end;
            debug_handler.ReadHeapMemory(&list_start, 0x44129298 + 0xb0, sizeof(list_start));
            debug_handler.ReadHeapMemory(&list_end, 0x44129298 + 0xb8, sizeof(list_end));
            for (u64 current_npc = list_start; current_npc < list_end; current_npc += 8)
            {
                u64 temp;
                u64 initialization_function;
                debug_handler.ReadMemory(&temp, current_npc, sizeof(temp));
                debug_handler.ReadMemory(&temp, temp, sizeof(temp));
                debug_handler.ReadMemory(&initialization_function, temp + 0xf0, sizeof(temp));
                initialization_function = debug_handler.normalizeMain(initialization_function);
                if (
                    initialization_function == 0xd600f0 ||
                    initialization_function == 0xd6f6d0 ||
                    initialization_function == 0xdaa010 ||
                    initialization_function == 0xd5ba20)
                {
                    npc_count++;
                }
            }
            char text[11];
            snprintf(text, 11, "NPCs: %d", npc_count);
            npc_counter->setText(std::string(text));

            debug_handler.PollForBreaks();
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
    tsl::elm::List *list;
    tsl::elm::ListItem *attach_button;
    tsl::elm::ListItem *npc_counter;
    tsl::elm::ListItem *clear_button;
    DebugHandler debug_handler;
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
