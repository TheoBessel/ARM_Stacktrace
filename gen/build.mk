###############  Build  ##############
.PHONY += build

SRCS = $(wildcard $(SRC_DIR)/*.c)
HEAD = $(wildcard $(SRC_DIR)/*.h)
OBJS = $(subst $(SRC_DIR)/,$(BUILD_DIR)/,$(SRCS:.c=.o))

print:
	@echo "[ =========================================================== ]"
	@echo "|                     Building objects ...                    |"
	@echo "|                                                             |"
	@printf "| %-60s|\n" "CC : $(CC)"
	@printf "| %-60s|\n" "AR : $(AR)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@echo "| ----------------------------------------------------------- |"
	@printf "| %-60s|\n" " $(subst $(SRC_DIR),src,$^)"
	@$(CC) $(CC_FLAGS) $^ -o $@

$(TARGET): print $(OBJS)
	@mkdir -p $(@D)
	@echo "[ =========================================================== ]"
	@echo "|                     Linking objects ...                     |"
	@$(CC) ${OBJS} $(LD_FLAGS) -o $@


build: $(TARGET)
	@echo "[ =========================================================== ]"
	@echo "|                     Build completed !                       |"
	@echo "[ =========================================================== ]"
######################################

###############  Clean  ##############
.PHONY += clean

clean:
	@echo "[ =========================================================== ]"
	@echo "|                     Cleaning targets ...                    |"
	@echo "[ =========================================================== ]"
	@rm -rf $(BUILD_DIR)
######################################
