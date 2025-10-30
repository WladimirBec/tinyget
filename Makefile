CC     ?= clang
CFLAGS ?= -march=native -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L

BLD_DIR := $(abspath ./bld)

SRCS  := url.c http.c
OBJS  := $(patsubst %.c,$(BLD_DIR)/%.o,$(SRCS))

DEBUG ?= 0
ifeq ($(DEBUG), 0)
CFLAGS += -O3 -flto -static -pipe
else
CFLAGS += -O0 -g -mno-avx -mno-avx512f -march=x86-64
endif

example: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) example.c -o $(BLD_DIR)/$@

$(BLD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

compile_commands.json:
	@echo "[" > ./compile_commands.json
	@for obj in $(OBJS); do \
		src="$${obj##$(BLD_DIR)/}"; \
		src="$${src%%.o}.c"; \
		echo "  {" >> ./compile_commands.json; \
		echo "    \"directory\": \"$(abspath .)\"," >> ./compile_commands.json; \
		echo "    \"file\": \"$$src\"," >> ./compile_commands.json; \
		echo "    \"output\": \"$$obj\"," >> ./compile_commands.json; \
		echo "    \"command\": \"$(CC) $(CFLAGS) -c $$src -o $$obj\"" >> ./compile_commands.json; \
		echo "  }," >> ./compile_commands.json; \
	done 
	@echo "]" >> ./compile_commands.json

.PHONY: compile_commands.json
