"""lldb Python formatters for psprecomp PSP runtime structures.

Provides human-readable display of PspThread, NativeModule,
PspSemaphore, and dlmalloc arena state during debug sessions.

Usage:
    command script import /path/to/psp_lldb.py

Or add to runtime/.lldbinit for automatic loading.
"""

import struct

import lldb


# ---- Status Enum Mapping ----

THREAD_STATUS_NAMES = {
    0: "DORMANT",
    1: "READY",
    2: "RUNNING",
    3: "WAIT",
    4: "DEAD",
    5: "WAIT_SLEEP",
}

PSP_ADDR_MASK = 0x07FFFFFF


# ---- Helper Functions ----


def read_u32(process, addr):
    """Read a little-endian uint32 from process memory."""
    error = lldb.SBError()
    data = process.ReadMemory(addr, 4, error)
    if not error.Success():
        return None
    return struct.unpack_from("<I", data, 0)[0]


def read_cstring(process, addr, max_len=256):
    """Read a NULL-terminated string from process memory."""
    error = lldb.SBError()
    result = process.ReadCStringFromMemory(addr, max_len, error)
    if not error.Success():
        return "<error>"
    return result


def get_rdram_base(target, process):
    """Get the host address of the rdram buffer.

    Tries g_threads[0].rdram first (always available when any
    thread is in use), falls back to EvaluateExpression.
    """
    threads = target.FindGlobalVariables("g_threads", 1)
    if threads.GetSize() > 0:
        arr = threads.GetValueAtIndex(0)
        # g_threads is an array; find first in_use slot
        count = arr.GetNumChildren()
        for i in range(count):
            t = arr.GetChildAtIndex(i)
            in_use = t.GetChildMemberWithName(
                "in_use"
            ).GetValueAsUnsigned(0)
            if in_use:
                rdram_member = t.GetChildMemberWithName("rdram")
                base = rdram_member.GetValueAsUnsigned(0)
                if base != 0:
                    return base

    # Fallback: evaluate expression
    thread = process.GetSelectedThread()
    if thread.IsValid():
        frame = thread.GetSelectedFrame()
        if frame.IsValid():
            val = frame.EvaluateExpression(
                "psp_get_current_thread()->rdram"
            )
            if val.IsValid() and not val.GetError().Fail():
                return val.GetValueAsUnsigned(0)

    return 0


# ---- Type Summary: PspThread ----


def psp_thread_summary(valobj, internal_dict):
    """Type summary function for PspThread.

    Registered with: type summary add PspThread
    Displays tid, status, name, entry_addr, stack_top, priority.
    """
    try:
        in_use = valobj.GetChildMemberWithName(
            "in_use"
        ).GetValueAsUnsigned(0)
        tid = valobj.GetChildMemberWithName(
            "id"
        ).GetValueAsSigned(-1)

        if not in_use:
            return "(unused slot)"

        status_val = valobj.GetChildMemberWithName(
            "status"
        ).GetValueAsSigned(-1)
        status = THREAD_STATUS_NAMES.get(
            status_val, f"UNKNOWN({status_val})"
        )

        name_val = valobj.GetChildMemberWithName("name")
        name = name_val.GetSummary() or '""'

        entry = valobj.GetChildMemberWithName(
            "entry_addr"
        ).GetValueAsUnsigned(0)
        stack = valobj.GetChildMemberWithName(
            "stack_top"
        ).GetValueAsUnsigned(0)
        prio = valobj.GetChildMemberWithName(
            "priority"
        ).GetValueAsSigned(0)
        wakeup = valobj.GetChildMemberWithName(
            "wakeup_count"
        ).GetValueAsSigned(0)

        return (
            f"tid={tid} status={status} name={name} "
            f"entry=0x{entry:08X} stack=0x{stack:08X} "
            f"prio={prio}"
        )
    except Exception as e:
        return f"<error: {e}>"


# ---- Command: psp_thread ----


def psp_current_thread_command(
    debugger, command, result, internal_dict
):
    """Display the current PspThread (calls psp_get_current_thread).

    Usage: psp_thread
    """
    try:
        target = debugger.GetSelectedTarget()
        process = target.GetProcess()
        thread = process.GetSelectedThread()
        frame = thread.GetSelectedFrame()

        if not frame.IsValid():
            result.SetError(
                "No valid frame. Is the process stopped?"
            )
            return

        val = frame.EvaluateExpression(
            "psp_get_current_thread()"
        )
        if not val.IsValid() or val.GetError().Fail():
            err = val.GetError().GetCString() if val.IsValid() else "unknown"
            result.SetError(
                f"Failed to call psp_get_current_thread(): {err}"
            )
            return

        ptr_val = val.GetValueAsUnsigned(0)
        if ptr_val == 0:
            result.AppendMessage(
                "No PSP thread on this OS thread "
                "(main thread or non-PSP thread)"
            )
            return

        thread_val = val.Dereference()
        summary = psp_thread_summary(thread_val, {})
        result.AppendMessage(
            f"Current PspThread at 0x{ptr_val:X}: {summary}"
        )
    except Exception as e:
        result.SetError(str(e))


# ---- Command: psp_threads ----


def psp_threads_command(
    debugger, command, result, internal_dict
):
    """List all active PSP threads.

    Usage: psp_threads
    """
    try:
        target = debugger.GetSelectedTarget()

        threads_var = target.FindGlobalVariables("g_threads", 1)
        if threads_var.GetSize() == 0:
            result.SetError(
                "Cannot find g_threads symbol. "
                "Is this a debug build?"
            )
            return

        arr = threads_var.GetValueAtIndex(0)
        count = arr.GetNumChildren()

        result.AppendMessage("PSP Threads:")
        active = 0
        for i in range(count):
            t = arr.GetChildAtIndex(i)
            in_use = t.GetChildMemberWithName(
                "in_use"
            ).GetValueAsUnsigned(0)
            if not in_use:
                continue
            active += 1
            summary = psp_thread_summary(t, {})
            result.AppendMessage(f"  [{i}] {summary}")

        if active == 0:
            result.AppendMessage("  (no active threads)")
        else:
            result.AppendMessage(f"  ({active} active)")
    except Exception as e:
        result.SetError(str(e))


# ---- Command: psp_module ----


def psp_module_command(
    debugger, command, result, internal_dict
):
    """Display NativeModule at a PSP address.

    Usage: psp_module [psp_addr]
    Default address: 0x08000100 (boot module)
    """
    try:
        target = debugger.GetSelectedTarget()
        process = target.GetProcess()

        psp_addr = 0x08000100
        cmd = command.strip()
        if cmd:
            psp_addr = int(cmd, 0)

        rdram_base = get_rdram_base(target, process)
        if rdram_base == 0:
            result.SetError(
                "Cannot find rdram base address. "
                "Is the process running?"
            )
            return

        host_addr = rdram_base + (psp_addr & PSP_ADDR_MASK)

        error = lldb.SBError()
        data = process.ReadMemory(host_addr, 196, error)
        if not error.Success():
            result.SetError(
                f"Failed to read memory at "
                f"0x{host_addr:X}: {error}"
            )
            return

        # Decode NativeModule fields
        name_bytes = data[0x08:0x08 + 28]
        name = name_bytes.split(b"\x00")[0].decode(
            "ascii", errors="replace"
        )
        attribute = struct.unpack_from("<H", data, 0x04)[0]
        ver_major = data[0x06]
        ver_minor = data[0x07]
        status = struct.unpack_from("<I", data, 0x24)[0]
        modid = struct.unpack_from("<I", data, 0x2C)[0]
        module_start = struct.unpack_from("<I", data, 0x50)[0]
        entry_addr = struct.unpack_from("<I", data, 0x64)[0]
        gp_value = struct.unpack_from("<I", data, 0x68)[0]
        text_addr = struct.unpack_from("<I", data, 0x6C)[0]
        text_size = struct.unpack_from("<I", data, 0x70)[0]
        data_size = struct.unpack_from("<I", data, 0x74)[0]
        bss_size = struct.unpack_from("<I", data, 0x78)[0]
        nsegment = struct.unpack_from("<I", data, 0x7C)[0]

        seg_addrs = []
        seg_sizes = []
        for j in range(min(nsegment, 4)):
            sa = struct.unpack_from("<I", data, 0x80 + j * 4)[0]
            ss = struct.unpack_from("<I", data, 0x90 + j * 4)[0]
            seg_addrs.append(sa)
            seg_sizes.append(ss)

        # Status name
        status_names = {
            0: "NOT_LOADED",
            1: "LOADED",
            4: "STARTING",
            5: "STARTED",
            8: "STOPPING",
            9: "STOPPED",
        }
        status_name = status_names.get(
            status, f"UNKNOWN"
        )

        result.AppendMessage(
            f"NativeModule at 0x{psp_addr:08X}:"
        )
        result.AppendMessage(f"  name:       {name}")
        result.AppendMessage(f"  modid:      {modid}")
        result.AppendMessage(
            f"  attribute:  0x{attribute:04X}"
        )
        result.AppendMessage(
            f"  version:    {ver_major}.{ver_minor}"
        )
        result.AppendMessage(
            f"  status:     {status_name} ({status})"
        )
        result.AppendMessage(
            f"  mod_start:  0x{module_start:08X}"
        )
        result.AppendMessage(
            f"  entry:      0x{entry_addr:08X}"
        )
        result.AppendMessage(
            f"  gp:         0x{gp_value:08X}"
        )
        result.AppendMessage(
            f"  text:       0x{text_addr:08X} "
            f"({text_size} bytes)"
        )
        result.AppendMessage(
            f"  data_size:  {data_size} bytes"
        )
        result.AppendMessage(
            f"  bss_size:   {bss_size} bytes"
        )

        if nsegment > 0:
            segs = ", ".join(
                f"0x{a:08X} ({s})"
                for a, s in zip(seg_addrs, seg_sizes)
            )
            result.AppendMessage(f"  segments:   [{segs}]")
        else:
            result.AppendMessage("  segments:   (none)")
    except Exception as e:
        result.SetError(str(e))


# ---- Command: psp_sema ----


def psp_sema_command(
    debugger, command, result, internal_dict
):
    """Display PspSemaphore state by UID.

    Usage: psp_sema [uid]
    If no uid given, attempts to list all semaphores.

    Note: g_semaphores is file-static in psp_hle_kernel_sema.cpp.
    If FindGlobalVariables fails, the command falls back to
    EvaluateExpression (requires being stopped in a frame where
    g_semaphores is visible).
    """
    try:
        target = debugger.GetSelectedTarget()
        process = target.GetProcess()
        thread = process.GetSelectedThread()
        frame = thread.GetSelectedFrame()

        cmd = command.strip()

        if not frame.IsValid():
            result.SetError(
                "No valid frame. Is the process stopped?"
            )
            return

        if cmd:
            # Lookup specific semaphore by UID
            uid = int(cmd, 0)
            _display_sema_by_uid(
                target, process, frame, uid, result
            )
        else:
            # List all semaphores
            _list_all_semas(
                target, process, frame, result
            )
    except Exception as e:
        result.SetError(str(e))


def _display_sema_by_uid(target, process, frame, uid, result):
    """Display a single semaphore by UID."""
    # Try FindGlobalVariables first
    semas = target.FindGlobalVariables("g_semaphores", 1)
    if semas.GetSize() > 0:
        sema_map = semas.GetValueAtIndex(0)
        # Iterate unordered_map children to find matching uid
        found = _find_sema_in_map(sema_map, uid, result)
        if found:
            return

    # Fallback: EvaluateExpression
    expr = f"g_semaphores.count({uid})"
    count_val = frame.EvaluateExpression(expr)
    if count_val.IsValid() and not count_val.GetError().Fail():
        count = count_val.GetValueAsUnsigned(0)
        if count == 0:
            result.AppendMessage(
                f"No semaphore with uid={uid}"
            )
            return

        # Access the semaphore
        sema_expr = f"*g_semaphores[{uid}]"
        sema_val = frame.EvaluateExpression(sema_expr)
        if sema_val.IsValid() and not sema_val.GetError().Fail():
            _print_sema_fields(sema_val, result)
            return

    result.AppendMessage(
        f"Cannot access semaphore uid={uid}. "
        "Try breaking inside psp_hle_kernel_sema.cpp "
        "scope where g_semaphores is visible."
    )


def _list_all_semas(target, process, frame, result):
    """List all semaphores."""
    semas = target.FindGlobalVariables("g_semaphores", 1)
    if semas.GetSize() > 0:
        sema_map = semas.GetValueAtIndex(0)
        _iterate_sema_map(sema_map, result)
        return

    # Fallback: try EvaluateExpression for size
    size_val = frame.EvaluateExpression(
        "g_semaphores.size()"
    )
    if size_val.IsValid() and not size_val.GetError().Fail():
        size = size_val.GetValueAsUnsigned(0)
        result.AppendMessage(
            f"PspSemaphores ({size} total):"
        )
        if size == 0:
            result.AppendMessage("  (none)")
            return

        result.AppendMessage(
            "  Use 'psp_sema <uid>' to inspect a "
            "specific semaphore."
        )
        result.AppendMessage(
            "  (Full iteration requires breaking "
            "inside psp_hle_kernel_sema.cpp scope)"
        )
        return

    result.AppendMessage(
        "Cannot access g_semaphores. "
        "Try breaking inside psp_hle_kernel_sema.cpp "
        "scope, or use 'psp_sema <uid>' with a known UID."
    )


def _find_sema_in_map(sema_map, uid, result):
    """Search unordered_map for a semaphore by UID.

    Returns True if found and displayed.
    """
    count = sema_map.GetNumChildren()
    for i in range(count):
        child = sema_map.GetChildAtIndex(i)
        if not child.IsValid():
            continue
        # unordered_map children are key-value pairs
        first = child.GetChildMemberWithName("first")
        if first.IsValid():
            key = first.GetValueAsSigned(-1)
            if key == uid:
                second = child.GetChildMemberWithName(
                    "second"
                )
                if second.IsValid():
                    # Dereference unique_ptr
                    sema_val = second.Dereference()
                    if not sema_val.IsValid():
                        # Try GetChildAtIndex(0) for
                        # unique_ptr internal pointer
                        ptr = second.GetChildAtIndex(0)
                        if ptr.IsValid():
                            sema_val = ptr.Dereference()
                    _print_sema_fields(sema_val, result)
                    return True
    return False


def _iterate_sema_map(sema_map, result):
    """Iterate all semaphores in the unordered_map."""
    count = sema_map.GetNumChildren()
    result.AppendMessage(
        f"PspSemaphores ({count} total):"
    )
    if count == 0:
        result.AppendMessage("  (none)")
        return

    for i in range(count):
        child = sema_map.GetChildAtIndex(i)
        if not child.IsValid():
            continue
        first = child.GetChildMemberWithName("first")
        second = child.GetChildMemberWithName("second")
        if first.IsValid() and second.IsValid():
            uid = first.GetValueAsSigned(-1)
            sema_val = second.Dereference()
            if not sema_val.IsValid():
                ptr = second.GetChildAtIndex(0)
                if ptr.IsValid():
                    sema_val = ptr.Dereference()
            if sema_val.IsValid():
                name_val = sema_val.GetChildMemberWithName(
                    "name"
                )
                name = name_val.GetSummary() or '""'
                cur = sema_val.GetChildMemberWithName(
                    "current_count"
                ).GetValueAsSigned(0)
                wc = sema_val.GetChildMemberWithName(
                    "wait_count"
                ).GetValueAsSigned(0)
                result.AppendMessage(
                    f"  uid={uid} name={name} "
                    f"count={cur} waiters={wc}"
                )


def _print_sema_fields(sema_val, result):
    """Print detailed semaphore fields."""
    uid = sema_val.GetChildMemberWithName(
        "uid"
    ).GetValueAsSigned(-1)
    name_val = sema_val.GetChildMemberWithName("name")
    name = name_val.GetSummary() or '""'
    cur = sema_val.GetChildMemberWithName(
        "current_count"
    ).GetValueAsSigned(0)
    max_c = sema_val.GetChildMemberWithName(
        "max_count"
    ).GetValueAsSigned(0)
    init_c = sema_val.GetChildMemberWithName(
        "init_count"
    ).GetValueAsSigned(0)
    wait_c = sema_val.GetChildMemberWithName(
        "wait_count"
    ).GetValueAsSigned(0)

    result.AppendMessage(f"PspSemaphore uid={uid}:")
    result.AppendMessage(f"  name:          {name}")
    result.AppendMessage(f"  current_count: {cur}")
    result.AppendMessage(f"  max_count:     {max_c}")
    result.AppendMessage(f"  init_count:    {init_c}")
    result.AppendMessage(f"  wait_count:    {wait_c}")


# ---- Command: psp_heap ----


def psp_heap_command(
    debugger, command, result, internal_dict
):
    """Display dlmalloc arena and partition allocator state.

    Usage: psp_heap

    Reads g_dlmalloc_pos, g_dlmalloc_end, g_dlmalloc_count,
    g_dlmalloc_arena_ready, g_heap_pos from globals or
    EvaluateExpression.

    Note: These are file-static in psp_hle_kernel_memory.cpp.
    If FindGlobalVariables fails, falls back to
    EvaluateExpression (requires being stopped in the correct
    translation unit scope).
    """
    try:
        target = debugger.GetSelectedTarget()
        process = target.GetProcess()
        thread = process.GetSelectedThread()
        frame = thread.GetSelectedFrame()

        if not frame.IsValid():
            result.SetError(
                "No valid frame. Is the process stopped?"
            )
            return

        # Try to read dlmalloc state
        dlm_pos = _read_global_u32(
            target, frame, "g_dlmalloc_pos"
        )
        dlm_end = _read_global_u32(
            target, frame, "g_dlmalloc_end"
        )
        dlm_count = _read_global_u32(
            target, frame, "g_dlmalloc_count"
        )
        dlm_ready = _read_global_bool(
            target, frame, "g_dlmalloc_arena_ready"
        )
        dlm_base = _read_global_u32(
            target, frame, "g_dlmalloc_base"
        )

        # Partition allocator state
        heap_pos = _read_global_u32(
            target, frame, "g_heap_pos"
        )
        heap_end_val = _read_global_u32(
            target, frame, "g_heap_end"
        )

        if dlm_pos is None and heap_pos is None:
            result.AppendMessage(
                "Cannot access heap globals. "
                "Try breaking inside "
                "psp_hle_kernel_memory.cpp scope."
            )
            return

        # Display dlmalloc arena
        result.AppendMessage("dlmalloc Arena:")
        if dlm_pos is not None:
            ready_str = "true" if dlm_ready else "false"
            result.AppendMessage(
                f"  ready:       {ready_str}"
            )

            if dlm_base is not None:
                result.AppendMessage(
                    f"  base:        0x{dlm_base:08X}"
                )
            else:
                result.AppendMessage(
                    "  base:        (unavailable)"
                )

            if dlm_end is not None and dlm_end > 0:
                base_for_used = (
                    dlm_base
                    if dlm_base is not None and dlm_base > 0
                    else 0x09000000
                )
                used = (
                    dlm_pos - base_for_used
                    if dlm_pos >= base_for_used
                    else 0
                )
                remaining = (
                    dlm_end - dlm_pos
                    if dlm_end > dlm_pos
                    else 0
                )
                result.AppendMessage(
                    f"  position:    0x{dlm_pos:08X}"
                )
                result.AppendMessage(
                    f"  end:         0x{dlm_end:08X}"
                )
                result.AppendMessage(
                    f"  used:        {used:,} bytes "
                    f"({used // (1024 * 1024)} MB)"
                )
                result.AppendMessage(
                    f"  remaining:   {remaining:,} bytes "
                    f"({remaining // (1024 * 1024)} MB)"
                )
            else:
                result.AppendMessage(
                    f"  position:    0x{dlm_pos:08X}"
                )

            if dlm_count is not None:
                result.AppendMessage(
                    f"  alloc_count: {dlm_count}"
                )

            # Display top-N allocation sizes
            top_sizes = _read_top_sizes(
                target, process, frame
            )
            _display_top_sizes(top_sizes, result)
        else:
            result.AppendMessage(
                "  (cannot read dlmalloc state)"
            )

        result.AppendMessage("")
        result.AppendMessage("Partition Allocator:")
        if heap_pos is not None:
            if heap_end_val is None:
                heap_end_val = 0x0C000000
            remaining = (
                heap_end_val - heap_pos
                if heap_end_val > heap_pos
                else 0
            )
            result.AppendMessage(
                f"  heap_pos:    0x{heap_pos:08X}"
            )
            result.AppendMessage(
                f"  heap_end:    0x{heap_end_val:08X}"
            )
            result.AppendMessage(
                f"  remaining:   {remaining} bytes"
            )
        else:
            result.AppendMessage(
                "  (cannot read partition allocator state)"
            )
    except Exception as e:
        result.SetError(str(e))


def _read_top_sizes(target, process, frame):
    """Read the g_dlmalloc_top_sizes[8] array.

    Returns a list of up to 8 uint32 values, or empty list
    on failure.
    """
    sizes = []

    # Try FindGlobalVariables for array access
    gvars = target.FindGlobalVariables(
        "g_dlmalloc_top_sizes", 1
    )
    if gvars.GetSize() > 0:
        arr_val = gvars.GetValueAtIndex(0)
        if arr_val.IsValid():
            count = arr_val.GetNumChildren()
            for i in range(min(count, 8)):
                child = arr_val.GetChildAtIndex(i)
                if child.IsValid():
                    sizes.append(
                        child.GetValueAsUnsigned(0)
                    )
            if sizes:
                return sizes

    # Fallback: EvaluateExpression for each element
    for i in range(8):
        val = frame.EvaluateExpression(
            f"g_dlmalloc_top_sizes[{i}]"
        )
        if val.IsValid() and not val.GetError().Fail():
            sizes.append(val.GetValueAsUnsigned(0))
        else:
            break
    if sizes:
        return sizes

    # Last resort: raw memory read
    gvars = target.FindGlobalVariables(
        "g_dlmalloc_top_sizes", 1
    )
    if gvars.GetSize() > 0:
        arr_val = gvars.GetValueAtIndex(0)
        addr = arr_val.GetLoadAddress()
        if addr != lldb.LLDB_INVALID_ADDRESS:
            error = lldb.SBError()
            data = process.ReadMemory(addr, 32, error)
            if error.Success() and len(data) == 32:
                return list(
                    struct.unpack_from("<8I", data, 0)
                )

    return []


def _format_size_human(size):
    """Format a byte size with human-readable suffix."""
    if size >= 1024 * 1024:
        return f"{size / (1024 * 1024):.0f} MB"
    if size >= 1024:
        return f"{size / 1024:.0f} KB"
    return f"{size} B"


def _display_top_sizes(sizes, result):
    """Display top-N allocation sizes."""
    non_zero = [s for s in sizes if s > 0]
    result.AppendMessage("")
    result.AppendMessage("  Top allocations (by size):")
    if not non_zero:
        result.AppendMessage(
            "    (no allocations recorded)"
        )
        return

    for idx, size in enumerate(non_zero):
        human = _format_size_human(size)
        result.AppendMessage(
            f"    {idx + 1}. {size:,} bytes ({human})"
        )


def _read_global_u32(target, frame, name):
    """Read a uint32 global variable by name.

    Tries FindGlobalVariables first, falls back to
    EvaluateExpression.
    """
    gvars = target.FindGlobalVariables(name, 1)
    if gvars.GetSize() > 0:
        val = gvars.GetValueAtIndex(0)
        if val.IsValid():
            return val.GetValueAsUnsigned(0)

    # Fallback: EvaluateExpression
    val = frame.EvaluateExpression(name)
    if val.IsValid() and not val.GetError().Fail():
        return val.GetValueAsUnsigned(0)

    return None


def _read_global_bool(target, frame, name):
    """Read a bool global variable by name."""
    gvars = target.FindGlobalVariables(name, 1)
    if gvars.GetSize() > 0:
        val = gvars.GetValueAtIndex(0)
        if val.IsValid():
            return bool(val.GetValueAsUnsigned(0))

    val = frame.EvaluateExpression(name)
    if val.IsValid() and not val.GetError().Fail():
        return bool(val.GetValueAsUnsigned(0))

    return False


# ---- Registration ----


def __lldb_init_module(debugger, internal_dict):
    """Register all PSP formatters and custom commands.

    Called automatically by lldb when this script is loaded
    via 'command script import'.
    """
    # Type summary for PspThread
    debugger.HandleCommand(
        "type summary add PspThread "
        "-F psp_lldb.psp_thread_summary "
        "-w psprecomp"
    )

    # Custom commands
    debugger.HandleCommand(
        "command script add "
        "-f psp_lldb.psp_current_thread_command "
        "psp_thread"
    )
    debugger.HandleCommand(
        "command script add "
        "-f psp_lldb.psp_threads_command "
        "psp_threads"
    )
    debugger.HandleCommand(
        "command script add "
        "-f psp_lldb.psp_module_command "
        "psp_module"
    )
    debugger.HandleCommand(
        "command script add "
        "-f psp_lldb.psp_sema_command "
        "psp_sema"
    )
    debugger.HandleCommand(
        "command script add "
        "-f psp_lldb.psp_heap_command "
        "psp_heap"
    )

    # Enable the formatter category
    debugger.HandleCommand("type category enable psprecomp")

    print(
        "[psprecomp] lldb formatters loaded: "
        "psp_thread, psp_threads, psp_module, "
        "psp_sema, psp_heap"
    )
