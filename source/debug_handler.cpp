#include "debug_handler.hpp"

DebugHandler *DebugHandler::instance = nullptr;

DebugHandler *DebugHandler::GetInstance()
{
    if (instance == nullptr)
    {
        instance = new DebugHandler();
    }
    return instance;
}

Result DebugHandler::Attach()
{
    // detach if already attached
    if (attached)
    {
        R_TRY(Detach());
        attached = false;
    }
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

Result DebugHandler::Detach()
{
    if (!attached)
    {
        R_SUCCEED();
    }
    R_TRY(DisableAllBreakpoints());
    R_TRY(svcCloseHandle(debug_handle));
    debug_handle = 0;
    attached = false;

    R_SUCCEED();
}

Result DebugHandler::Start()
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

Result DebugHandler::WriteBreakpoint(u64 address, u32 *instr_out)
{
    // ARMv8 'BRK #0'
    u32 bp_instr = 0xD4200000;
    // store original instruction
    R_TRY(ReadMainMemory(instr_out, address, sizeof(bp_instr)));
    // write software breakpoint
    R_TRY(WriteMainMemory(&bp_instr, address, sizeof(bp_instr)));

    R_SUCCEED();
}

Result DebugHandler::SetBreakpoint(u64 address, u64 *idx_out, std::function<void(ThreadContext *)> on_break)
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

Result DebugHandler::DisableBreakpoint(u64 idx)
{
    R_TRY(WriteMainMemory(&breakpoints[idx].original_instruction, breakpoints[idx].address, sizeof(breakpoints[idx].original_instruction)));

    R_SUCCEED();
}

Result DebugHandler::EnableBreakpoint(u64 idx)
{
    u32 dummy_instr;
    R_TRY(WriteBreakpoint(breakpoints[idx].address, &dummy_instr));
    R_SUCCEED();
}

Result DebugHandler::DisableAllBreakpoints()
{
    for (int i = 0; i < breakpoints.size(); i++)
    {
        R_TRY(DisableBreakpoint(i));
    }

    R_SUCCEED();
}

Result DebugHandler::GetCurrentBreakpoint(DebugEventInfo *event_info, Breakpoint *breakpoint_out)
{
    ThreadContext thread_context;
    R_TRY(GetThreadContext(&thread_context, event_info->thread_id, RegisterGroup_All));
    u64 break_address = thread_context.pc.x - main_base;
    auto bp = std::find_if(breakpoints.begin(), breakpoints.end(), [&break_address](const Breakpoint &bp)
                           { return bp.address == break_address; });
    if (bp == breakpoints.end())
    {
        R_THROW(1);
    }
    *breakpoint_out = *bp;
    R_SUCCEED();
}

Result DebugHandler::Continue()
{
    u64 thread_ids[] = {0};
    R_TRY(svcContinueDebugEvent(debug_handle, ContinueFlag_ExceptionHandled | ContinueFlag_EnableExceptionEvent | ContinueFlag_ContinueAll, thread_ids, 1));
    broken = false;
    R_SUCCEED();
}

Result DebugHandler::Stall()
{
    s32 dummy_index;
    return svcWaitSynchronization(&dummy_index, &debug_handle, 1, -1);
}

Result DebugHandler::ResumeFromBreakpoint(Breakpoint breakpoint)
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
    // stall until break
    R_TRY(Stall());
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

Result DebugHandler::GetProcessDebugEvent(DebugEventInfo *out)
{
    R_TRY(svcGetDebugEvent(out, debug_handle));

    if (out->flags & DebugEventFlag_Stopped)
    {
        broken = true;
    }

    R_SUCCEED();
}

Result DebugHandler::GetThreadContext(ThreadContext *out, u64 thread_id, u32 flags)
{
    return svcGetDebugThreadContext(out, debug_handle, thread_id, flags);
}

Result DebugHandler::Poll()
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
        if (R_SUCCEEDED(GetCurrentBreakpoint(&event_info, &bp)))
        {
            ThreadContext thread_context;
            R_TRY(GetThreadContext(&thread_context, event_info.thread_id, RegisterGroup_All));
            bp.on_break(&thread_context);
            R_TRY(ResumeFromBreakpoint(bp));
        }
        else
        {
            R_TRY(Continue());
        }
    }
    R_SUCCEED()
}

Result DebugHandler::ReadMemory(void *out, u64 address, u64 size)
{
    return svcReadDebugProcessMemory(out, debug_handle, address, size);
}

Result DebugHandler::ReadMainMemory(void *out, u64 address, u64 size)
{
    return ReadMemory(out, main_base + address, size);
}

Result DebugHandler::ReadHeapMemory(void *out, u64 address, u64 size)
{
    return ReadMemory(out, heap_base + address, size);
}

Result DebugHandler::WriteMemory(void *buffer, u64 address, u64 size)
{
    return svcWriteDebugProcessMemory(debug_handle, buffer, address, size);
}

Result DebugHandler::WriteMainMemory(void *buffer, u64 address, u64 size)
{
    return WriteMemory(buffer, main_base + address, size);
}

Result DebugHandler::WriteHeapMemory(void *buffer, u64 address, u64 size)
{
    return WriteMemory(buffer, heap_base + address, size);
}

u64 DebugHandler::normalizeMain(u64 address)
{
    return address - main_base;
}

u64 DebugHandler::normalizeHeap(u64 address)
{
    return address - heap_base;
}

bool DebugHandler::isAttached()
{
    return attached;
}

bool DebugHandler::isBroken()
{
    return broken;
}
