OBJDIR	:= objs
SRCS    := main.c cmdline.c cmds.c readline.c file_access.c mem_access.c \
           cpu.c db_disasm_x86-32.c db_disasm_x86-64.c version.c \
	   pci_access.c
CFLAGS  := -DBUILD_DATE=\"$(DATE)\" -DBUILD_TIME=\"$(TIME)\"
CFLAGS  += -O2 -g -Wall -Werror -Wpedantic -Wundef
#CFLAGS += -g
QUIET   := @
#QUIET   :=
CC	:= cc

OS   := $(shell uname -s)
NOW  := $(shell date +%s)
ifeq ($(OS),Darwin)
DATE := $(shell date -j -f %s $(NOW)  '+%Y-%m-%d')
TIME := $(shell date -j -f %s $(NOW)  '+%H:%M:%S')
OBJDIR := $(OBJDIR).mac
CFLAGS += -DOSX
else
DATE := $(shell date -d "@$(NOW)" '+%Y-%m-%d')
TIME := $(shell date -d "@$(NOW)" '+%H:%M:%S')
endif

# If verbose is specified with no other targets, then build everything
ifeq ($(MAKECMDGOALS),verbose)
verbose: all
endif
ifeq (,$(filter verbose timed, $(MAKECMDGOALS)))
QUIET   := @
else
QUIET   :=
VERBOSE := -v
endif

OBJS    := $(SRCS:%.c=$(OBJDIR)/%.o)

all: $(OBJDIR)/med

$(OBJDIR)/med: $(OBJS) | $(OBJDIR)
	@echo "Creating $@"
	$(QUIET)$(CC) -o $@ $^

define DEPEND_SRC
# The following line creates a rule for an object file to depend on a
# given source file.
$(patsubst %,$(OBJDIR)/%,$(filter-out $(OBJDIR)/%,$(basename $(1)).o)) $(filter $(OBJDIR)/%,$(basename $(1)).o): $(1)
endef
$(foreach SRCFILE,$(SRCS),$(eval $(call DEPEND_SRC,$(SRCFILE))))

$(OBJDIR)/main.o: cmdline.h
$(OBJDIR)/cmdline.o: cmds.h readline.h
$(OBJDIR)/cmds.o: cmds.h file_access.h pci_access.h
$(OBJDIR)/readline.o: readline.h cmds.h
$(OBJDIR)/file_access.o: cmds.h file_access.h
$(OBJDIR)/mem_access.o: mem_access.h cmdline.h
$(OBJDIR)/pci_access.o: cmds.h pci_access.h
$(OBJDIR)/version.o: version.h $(filter-out $(OBJDIR)/version.o, $(OBJS))

$(OBJS): Makefile cmdline.h | $(OBJDIR)
	@echo "Creating $@"
	$(QUIET)$(CC) -o $@ -c $(filter %.c,$^) $(CFLAGS)

$(OBJDIR):
	mkdir -p $@

clean:
	rm -f $(OBJS)
