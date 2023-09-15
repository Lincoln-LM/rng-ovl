#include <memory>
#include <functional>
#include <vector>
#include "common.hpp"

#ifndef DEBUG_HANDLER_HPP
#define DEBUG_HANDLER_HPP
class DebugHandler
{
public:
    // singleton methods
    DebugHandler(DebugHandler &other) = delete;
    void operator=(const DebugHandler &) = delete;
    static DebugHandler *GetInstance();

    // hooking methods
    // Attach to the currently running application.
    Result Attach();
    // Detach from the currently running application,
    Result Detach();
    // Process attach debug event.
    Result Start();

    Result GetTitleId(u64 *title_id_out);

    // breakpoint methods
    // Write a software breakpoint instruction to the given address and read the original instruction.
    Result WriteBreakpoint(u64 address, u32 *instr_out);
    // Set a software breakpoint to the provided address and register a function to be called on break.
    Result SetBreakpoint(u64 address, u64 *idx_out, std::function<void(ThreadContext *)> on_break);
    // Disable the breakpoint with the given idx.
    Result DisableBreakpoint(u64 idx);
    // Ensable the breakpoint with the given idx.
    Result EnableBreakpoint(u64 idx);
    // Disable all breakpoints.
    Result DisableAllBreakpoints();
    // Find the currently broken breakpoint based on the program counter.
    Result GetCurrentBreakpoint(DebugEventInfo *event_info, Breakpoint *breakpoint_out);
    // Stall until the next debug event
    Result Stall();
    // Continue from a debug event.
    Result Continue();
    // Resume from the given breakpoint.
    Result ResumeFromBreakpoint(Breakpoint breakpoint);

    // Get and process a debug event.
    Result GetProcessDebugEvent(DebugEventInfo *out);
    // Get the context of the provided thread.
    Result GetThreadContext(ThreadContext *out, u64 thread_id, u32 flags);
    // Poll for debug events and handle breakpoint callbacks.
    Result Poll();

    // memory operations
    // Read process memory into a buffer from an absolute address.
    Result ReadMemory(void *out, u64 address, u64 size);
    // Read process memory into a buffer from a main-relative address.
    Result ReadMainMemory(void *out, u64 address, u64 size);
    // Read process memory into a buffer from a heap-relative address.
    Result ReadHeapMemory(void *out, u64 address, u64 size);

    // Write process memory from an absolute addres
    Result WriteMemory(void *buffer, u64 address, u64 size);
    // Write process memory from a main-relative address.
    Result WriteMainMemory(void *buffer, u64 address, u64 size);
    // Write process memory from a heap-relative address.
    Result WriteHeapMemory(void *buffer, u64 address, u64 size);

    // address normalization
    // Convert an absolute address to a main-relative address.
    u64 normalizeMain(u64 address);
    // Convert an absolute address to a heap-relative address.
    u64 normalizeHeap(u64 address);

    // instance state operations
    // Check whether or not the debug handler is attached to a process.
    bool isAttached();
    // Check whether or not the debug handler is paused at a breakpoint.
    bool isBroken();

private:
    bool attached = false;
    bool broken = false;

    Handle debug_handle;
    u64 process_id;
    u64 main_base;
    u64 heap_base;
    std::vector<Breakpoint> breakpoints;

    // singleton instance
    DebugHandler(){};
    static DebugHandler *instance;
};
#endif
