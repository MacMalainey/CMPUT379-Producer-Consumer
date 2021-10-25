# Compiler options
CC     := gcc
CFLAGS := -Wall -Wno-unused-result
LFLAGS := -pthread

DEBUG_FLAGS := -g

# Path options
BUILD_DIR := build
SRC_DIR   := src

# Project specific options
EXECUTABLE := prodcon
SOURCES    := main.c \
			  tasks/taskman.c \
			  tasks/tands.c \
			  tasks/taskqueue.c

# Submission options
ARCHIVE   := prodcon.zip
ARTIFACTS := makefile
ARTIFACTS += $(SRC_DIR)

# Generated settings
OBJS = $(addprefix $(BUILD_DIR)/, $(SOURCES:.c=.o))

$(EXECUTABLE): compile link

$(OBJS): $(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

link: $(OBJS)
	$(CC) $(LFLAGS) -o $(EXECUTABLE) $(OBJS)

compile: $(OBJS)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: compile link;

clean:
	rm $(EXECUTABLE) $(ARCHIVE) $(BUILD_DIR)/ -R

compress:
	zip $(ARCHIVE) -r $(ARTIFACTS)
