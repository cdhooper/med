NOW	:= $(shell date)
DATE    := $(shell date -d '$(NOW)' '+%Y-%m-%d')
TIME    := $(shell date -d '$(NOW)' '+%H:%M:%S')

OBJDIR	:= objs
SRCS    := main.c cmdline.c cmds.c readline.c sfile.c mem_access.c version.c
OBJS    := $(SRCS:%.c=$(OBJDIR)/%.o)
CFLAGS  := -DBUILD_DATE=\"$(DATE)\" -DBUILD_TIME=\"$(TIME)\"
CFLAGS  += -O2 -g -Wall -Werror -Wpedantic -Wundef
#CFLAGS += -g
QUIET   := @
#QUIET   :=
CC	:= cc

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
$(OBJDIR)/cmds.o: cmds.h sfile.h
$(OBJDIR)/readline.o: readline.h cmds.h
$(OBJDIR)/sfile.o: cmds.h sfile.h
$(OBJDIR)/mem_access.o: mem_access.h cmdline.h
$(OBJDIR)/version.o: version.h $(filter-out $(OBJDIR)/version.o, $(OBJS))

$(OBJS): Makefile cmdline.h | $(OBJDIR)
	@echo "Creating $@"
	$(QUIET)$(CC) -o $@ -c $(filter %.c,$^) $(CFLAGS)

$(OBJDIR):
	mkdir -p $@

clean:
	rm -f $(OBJS)
