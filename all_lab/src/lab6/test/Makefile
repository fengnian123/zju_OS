C_SRC = $(sort $(wildcard *.c))
obj   = $(patsubst %.c,%.o,$(C_SRC))
all:$(obj)
%.o:%.c
	${GCC}  ${CFLAG} -c $<

clean:
	$(shell rm *.o 2>/dev/null)