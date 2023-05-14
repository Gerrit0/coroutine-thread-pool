CXX := g++
CXXFLAGS := -std=c++20 -Werror -I${PWD}/src \
        -Wall \
        -Wextra \
        -Wshadow \
        -Wnon-virtual-dtor \
        -Wold-style-cast \
        -Wcast-align \
        -Wunused \
        -Woverloaded-virtual \
        -Wpedantic \
        -Wconversion \
        -Wsign-conversion \
        -Wnull-dereference \
        -Wdouble-promotion \
        -Wformat=2 \
        -Wimplicit-fallthrough \
        -Wmisleading-indentation \
        -Wduplicated-cond \
        -Wduplicated-branches \
        -Wlogical-op \
        -Wuseless-cast

RELFLAGS := -O3
DBGFLAGS := -g
CCOBJFLAGS := $(CXXFLAGS) -c
LINK_FLAGS := -lboost_iostreams

BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src

TARGET_NAME := ws
TARGET := $(BIN_PATH)/$(TARGET_NAME)
TARGET_DBG := $(TARGET)_debug

# src files & obj files
SRC := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
OBJ_REL := $(addprefix $(OBJ_PATH)/release/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
DEP_REL := $(addprefix $(OBJ_PATH)/release/, $(addsuffix .depends, $(notdir $(basename $(SRC)))))
OBJ_DBG := $(addprefix $(OBJ_PATH)/debug/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
DEP_DBG := $(addprefix $(OBJ_PATH)/debug/, $(addsuffix .depends, $(notdir $(basename $(SRC)))))

debug: $(TARGET_DBG)
release: $(TARGET)

.PHONY: run
run: debug
	$(TARGET_DBG)

.PHONY: all
all: debug release

.PHONY: watch
watch:
	@while true; do \
		$(MAKE) --no-print-directory; \
		inotifywait -qre close_write ./src; \
		clear; \
	done

# binary
$(TARGET): $(OBJ_REL) | $(BIN_PATH)
	@$(CXX) $(CXXFLAGS) $(RELFLAGS) -o $@ $(OBJ_REL) $(LINK_FLAGS)
	@echo Built $(TARGET)

$(TARGET_DBG): $(OBJ_DBG) | $(BIN_PATH)
	@$(CXX) $(CXXFLAGS) $(DBGFLAGS) -o $@ $(OBJ_DBG) $(LINK_FLAGS)
	@echo Built $(TARGET_DBG)

# objs
$(OBJ_PATH)/release/%.o: $(SRC_PATH)/%.c* | $(OBJ_PATH)/release
	@$(CXX) $(CCOBJFLAGS) $(RELFLAGS) -o $@ $<

$(OBJ_PATH)/debug/%.o: $(SRC_PATH)/%.c* | $(OBJ_PATH)/debug
	@$(CXX) $(CCOBJFLAGS) $(DBGFLAGS) -o $@ $<

# depends
$(OBJ_PATH)/release/%.depends: $(SRC_PATH)/%.c* | $(OBJ_PATH)/release
	@$(CXX) -MM -MT $(basename $@).o -MF $@ $(CXXFLAGS) $(RELFLAGS) $<

$(OBJ_PATH)/debug/%.depends: $(SRC_PATH)/%.c* | $(OBJ_PATH)/debug
	@$(CXX) -MM -MT $(basename $@).o -MF $@ $(CXXFLAGS) $(DBGFLAGS) $<

# dirs
$(OBJ_PATH)/debug $(OBJ_PATH)/release $(BIN_PATH):
	@mkdir -p $@

.PHONY: clean
clean:
	@rm -rf $(BIN_PATH) $(OBJ_PATH)

-include $(DEP_REL) $(DEP_DBG)
