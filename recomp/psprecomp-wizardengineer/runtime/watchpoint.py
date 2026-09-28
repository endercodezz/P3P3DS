"""lldb Python script to set a hardware watchpoint on PSP address 0x09012CD4."""
import lldb

def set_watchpoint(debugger, command, result, internal_dict):
    target = debugger.GetSelectedTarget()
    process = target.GetProcess()
    thread = process.GetSelectedThread()
    frame = thread.GetSelectedFrame()

    # Get rdram from the first argument of psp_init_data_sections
    rdram = frame.FindVariable("rdram").GetValueAsUnsigned()
    if rdram == 0:
        # Fallback: try register
        rdram = frame.FindRegister("x0").GetValueAsUnsigned()

    offset = 0x09012CD4 & 0x07FFFFFF  # = 0x01012CD4
    watch_addr = rdram + offset
    print(f"rdram = {hex(rdram)}")
    print(f"Watchpoint target: {hex(watch_addr)} (PSP 0x09012CD4)")

    err = lldb.SBError()
    wp = target.WatchAddress(watch_addr, 4, False, True, err)
    if err.Success():
        print(f"Watchpoint #{wp.GetID()} set successfully")
        # Only stop when the corrupt value is written
        wp.SetCondition(f"*(uint32_t*){watch_addr} == 0x0003796C")
        print("Condition: value == 0x0003796C")
    else:
        print(f"FAILED: {err}")

def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand('command script add -f watchpoint.set_watchpoint wp_setup')
    print("Loaded watchpoint.py — use 'wp_setup' after breaking at psp_init_data_sections")
