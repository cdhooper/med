SRCS    := main.c cmdline.c cmds.c readline.c sfile.c mem_access.c
OBJS    := $(SRCS:%.c=%.o)
CFLAGS  := -DVERSION=\"1.0\" -DBUILD_DATE=\"$(DATE)\" -DBUILD_TIME=\"$(TIME)\"
CFLAGS  += -g -Wall -Werror
#CFLAGS += -g
QUIET   := @
CC	:= cc

med: $(OBJS)
	@echo "Creating $@"
	$(QUIET)$(CC) -o $@ $^

main.o: cmdline.h
cmdline.o: cmds.h readline.h
cmds.o: cmds.h sfile.h
readline.o: readline.h cmds.h
sfile.o: cmds.h sfile.h
mem_access.o: mem_access.h cmdline.h

$(OBJS): Makefile cmdline.h

.c.o:
	@echo "Creating $@"
	$(QUIET)$(CC) -o $@ -c $(filter %.c,$^) $(CFLAGS)

clean:
	rm -f $(OBJS)
