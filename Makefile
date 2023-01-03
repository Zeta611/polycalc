TARGET_EXEC := poly

BUILD_DIR := ./build
SRC_DIRS := ./src

SRCS := $(shell find $(SRC_DIRS) -name *.c -or -name *.y -or -name *.l)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CC := gcc
CFLAGS := -O3 -Wall -Wextra -Wpedantic -std=c17 -Wno-implicit-function-declaration
CPPFLAGS := $(INC_FLAGS) -MMD -MP
LDFLAGS := -ly -ll -lm

YACC := bison
YFLAGS := -d

LEX := flex
LFLAGS :=

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.y.o: %.tab.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.l.o: %.yy.c
	mkdir -p $(dir $@)
	$(YACC) $(YFLAGS) $(SRC_DIRS)/poly.y -o $(BUILD_DIR)/src/poly.tab.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.tab.c: %.y
	mkdir -p $(dir $@)
	$(YACC) $(YFLAGS) $< -o $@

$(BUILD_DIR)/%.yy.c: %.l
	mkdir -p $(dir $@)
	$(LEX) $(LFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

-include $(DEPS)
