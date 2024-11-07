target remote:1234

# Interface with source, regs and cmd windows
define debug_layout
  layout src
  layout regs
  focus cmd
end

# Quit QEMU
define qemu_quit
    mon gdbserver none
end

# Debug the SaveRegisters function
define debug_save_registers
    # On SaveRegisters call (in Handler)
    break fdir.c:99
    continue
    p/x *debug_info->registers
    p/x debug_info->cfsr
    p/x debug_info->hfsr
end

# Debug the frame loop
define debug_frame
    # On UnwindNextFrame call (in UnwindStack)
    break fdir.c:114
    continue
    p/x *debug_info->call_stack
end

debug_layout