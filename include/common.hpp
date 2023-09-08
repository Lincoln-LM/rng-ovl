#include <switch.h>

enum ProcessExitReason : u32
{
    ProcessExitReason_ExitProcess = 0,
    ProcessExitReason_TerminateProcess = 1,
    ProcessExitReason_Exception = 2,
};

enum DebugEvent : u32
{
    DebugEvent_CreateProcess = 0,
    DebugEvent_CreateThread = 1,
    DebugEvent_ExitProcess = 2,
    DebugEvent_ExitThread = 3,
    DebugEvent_Exception = 4,
};

enum ContinueFlag : u32
{
    ContinueFlag_ExceptionHandled = (1u << 0),
    ContinueFlag_EnableExceptionEvent = (1u << 1),
    ContinueFlag_ContinueAll = (1u << 2),
    ContinueFlag_ContinueOthers = (1u << 3),

    ContinueFlag_AllMask = (1u << 4) - 1,
};

enum DebugException : u32
{
    DebugException_UndefinedInstruction = 0,
    DebugException_InstructionAbort = 1,
    DebugException_DataAbort = 2,
    DebugException_AlignmentFault = 3,
    DebugException_DebuggerAttached = 4,
    DebugException_BreakPoint = 5,
    DebugException_UserBreak = 6,
    DebugException_DebuggerBreak = 7,
    DebugException_UndefinedSystemCall = 8,
    DebugException_MemorySystemError = 9,
};

enum ThreadExitReason : u32
{
    ThreadExitReason_ExitThread = 0,
    ThreadExitReason_TerminateThread = 1,
    ThreadExitReason_ExitProcess = 2,
    ThreadExitReason_TerminateProcess = 3,
};

enum BreakPointType : u32
{
    BreakPointType_HardwareInstruction = 0,
    BreakPointType_HardwareData = 1,
};

struct DebugInfoCreateProcess
{
    u64 program_id;
    u64 process_id;
    char name[0xC];
    u32 flags;
    u64 user_exception_context_address; /* 5.0.0+ */
};

struct DebugInfoCreateThread
{
    u64 thread_id;
    u64 tls_address;
};

struct DebugInfoExitProcess
{
    ProcessExitReason reason;
};

struct DebugInfoExitThread
{
    ThreadExitReason reason;
};

struct DebugInfoUndefinedInstructionException
{
    u32 insn;
};

struct DebugInfoDataAbortException
{
    u64 address;
};

struct DebugInfoAlignmentFaultException
{
    u64 address;
};

struct DebugInfoBreakPointException
{
    BreakPointType type;
    u64 address;
};

struct DebugInfoUserBreakException
{
    BreakReason break_reason;
    u64 address;
    u64 size;
};

struct DebugInfoDebuggerBreakException
{
    u64 active_thread_ids[4];
};

struct DebugInfoUndefinedSystemCallException
{
    u32 id;
};

union DebugInfoSpecificException
{
    DebugInfoUndefinedInstructionException undefined_instruction;
    DebugInfoDataAbortException data_abort;
    DebugInfoAlignmentFaultException alignment_fault;
    DebugInfoBreakPointException break_point;
    DebugInfoUserBreakException user_break;
    DebugInfoDebuggerBreakException debugger_break;
    DebugInfoUndefinedSystemCallException undefined_system_call;
    u64 raw;
};

struct DebugInfoException
{
    DebugException type;
    u64 address;
    DebugInfoSpecificException specific;
};

union DebugInfo
{
    DebugInfoCreateProcess create_process;
    DebugInfoCreateThread create_thread;
    DebugInfoExitProcess exit_process;
    DebugInfoExitThread exit_thread;
    DebugInfoException exception;
};

struct DebugEventInfo
{
    DebugEvent type;
    u32 flags;
    u64 thread_id;
    DebugInfo info;
};
static_assert(sizeof(DebugEventInfo) >= 0x40);

enum DebugEventFlag : u32
{
    DebugEventFlag_Stopped = (1u << 0),
};

enum ThreadContextFlag : u32
{
    ThreadContextFlag_General = (1 << 0),
    ThreadContextFlag_Control = (1 << 1),
    ThreadContextFlag_Fpu = (1 << 2),
    ThreadContextFlag_FpuControl = (1 << 3),

    ThreadContextFlag_All = (ThreadContextFlag_General | ThreadContextFlag_Control | ThreadContextFlag_Fpu | ThreadContextFlag_FpuControl),

    ThreadContextFlag_SetSingleStep = (1u << 30),
    ThreadContextFlag_ClearSingleStep = (1u << 31),
};

typedef struct
{
    u32 original_instruction;
    u64 address;
    std::function<void(ThreadContext *)> on_break;
} Breakpoint;
