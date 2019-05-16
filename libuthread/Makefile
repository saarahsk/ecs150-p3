# Target library
lib := libuthread.a
specificDelete := sem.o tps.o
objs := thread.o queue.o $(specificDelete)

#General gcc options
CC := gcc
CFLAGS := -Wall -Werror
CFLAGS += -g

ifneq ($(V), 1)
Q = @
endif

all: $(lib)

#dependencies
deps := $(patsubst %.o, %.d, $(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@: .o = .d)

$(lib): $(objs)
	@echo "AR $@"
	$(Q)ar rcs $@ $^

%.o: %.c
	@echo "CC $@"
	$(Q) $(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

clean:
	@echo "clean"
	$(Q) rm -f $(lib) $(specificDelete) $(deps)
