cc = cc
flags = -Wall -Wextra -Wpedantic -Wshadow -Wconversion

src = src
obj = obj

sources = $(shell find $(src) -name "*.c")
objects = $(patsubst $(src)/%,$(obj)/%,$(sources:.c=.o))

target = warden

version = 0.1.4
commit = $(shell git rev-parse --short HEAD 2> /dev/null || echo unknown)

flags += -Dversion=\"$(version)\"
flags += -Dcommit=\"$(commit)\"

$(obj)/%.o: $(src)/%.c
	@mkdir -p $(dir $@)
	@echo "compiling $<..."
	@$(cc) $(flags) -c $< -o $@

all:
	@echo "available build options for warden"
	@echo "make clean      clean compiled assets"
	@echo "make develop    address sanitized"
	@echo "make release    performance optimized"

develop: $(objects)
	@echo "linking $(target) $(commit)..."
	@$(cc) $(flags) -o $(target) $(objects) -lsqlite3 -O0 -fsanitize=address

release: $(objects)
	@echo "linking $(target) $(commit)..."
	@$(cc) $(flags) -o $(target) $(objects) -lsqlite3 -O3 -march=native

clean:
	@echo "cleaning up..."
	@rm -rf $(obj) $(target)
