###############  Help  ###############
.PHONY += help

help:
	@echo "[ =========================================================== ]"
	@echo "|  Usage: make <target>                                       |"
	@echo "[ =========================================================== ]"
	@echo "|  Available targets:                                         |"
	@echo "|    make          Alias for 'make all'.                      |"
	@echo "|    make all      Alias for 'make build'.                    |"
	@echo "| ----------------------------------------------------------- |"
	@echo "|    make help     Show this help message.                    |"
	@echo "| ----------------------------------------------------------- |"
	@echo "|    make build    Build the project.                         |"
	@echo "|    make clean    Clean the project.                         |"
	@echo "| ----------------------------------------------------------- |"
	@echo "|    make gdb      Start gdb on port 1234.                    |"
	@echo "|    make debug    Start QEMU with gdb started on port 1234.  |"
	@echo "[ =========================================================== ]"
	@echo "|  Copyright (c) Theo Bessel - contact[at]theobessel.fr       |"
	@echo "[ =========================================================== ]"
######################################
