.PHONY = all

all: clean build

include gen/config.mk
include gen/build.mk	# Modifies .PHONY
include gen/debug.mk	# Modifies .PHONY
include gen/help.mk		# Modifies .PHONY
