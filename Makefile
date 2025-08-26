CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -D_POSIX_C_SOURCE=200809L

SRCDIR = src
OBJDIR = obj
BINDIR = bin
LISTDIR = z_listing

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

EXECUTABLE = $(BINDIR)/gemini

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(OBJDIR) $(BINDIR) $(LISTDIR)

run: all
	./$(EXECUTABLE) examples/test.gemini

list_source:
	@mkdir -p $(LISTDIR)
	@echo "Creating source code listing..."
	@> $(LISTDIR)/listing.txt
	@find . -type d \( -name .git -o -name bin -o -name obj -o -name .vscode -o -name $(LISTDIR) \) -prune -o \
	-type f \( -name "*.c" -o -name "*.h" -o -name "Makefile" -o -name "*.gemini" \) -print | while read -r file; do \
		echo "=================================================================" >> $(LISTDIR)/listing.txt; \
		echo "FILE: $$file" >> $(LISTDIR)/listing.txt; \
		echo "=================================================================" >> $(LISTDIR)/listing.txt; \
		cat "$$file" >> $(LISTDIR)/listing.txt; \
		echo "" >> $(LISTDIR)/listing.txt; \
	done
	@echo "Source code listing created at $(LISTDIR)/listing.txt"

.PHONY: all clean run list_source