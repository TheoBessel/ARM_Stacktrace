#############  Binutils  #############
.PHONY += readelf size

readelf:
	$(READELF) -a $(TARGET) > $(TARGET:.elf=.readelf)

size:
	$(SIZE) $(TARGET) > $(TARGET:.elf=.size)
######################################

###############  Debug  ##############
.PHONY += gdb debug

gdb: clean build
	@echo "[ =========================================================== ]"
	@echo "|                       Starting QEMU ...                     |"
	@make debug &
	@echo "[ =========================================================== ]"
	@echo "|                       Starting GDB ...                      |"
	@echo "| ----------------------------------------------------------- |"
	@$(GDB) -q --eval-command="target remote:1234" $(TARGET)
	@echo "| ----------------------------------------------------------- |"
	@echo "|                        GDB exited !                         |"
	@echo "[ =========================================================== ]"

debug:
	@$(EMU) -S -s -machine mps2-an500 -cpu cortex-m7 -m 16M -kernel $(TARGET) -nographic -serial mon:stdio
######################################
