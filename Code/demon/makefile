semaphore_dir = ../semaphore/
CC = gcc
CFLAGS = -std=c18 \
  -Wall -Wconversion -Werror -Wextra -Wfatal-errors -Wpedantic -Wwrite-strings \
  -DxNATIMPHIGH \
  -D_POSIX_C_SOURCE=200112L \
  -D_XOPEN_SOURCE=501 \
  -DxDINPUT -DxDOUTPUT \
  -pthread \
  -I$(semaphore_dir) -g \
  -DRECTERM
LDFLAGS = -pthread -lrt
vpath %.c $(semaphore_dir)
vpath %.h $(semaphore_dir)
objects = sema.o demon.o
executable = demon

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

tar:
	$(MAKE) clean
	tar -zcf "$(CURDIR)_$(shell date +'%d.%m.%y-%Hh%M').tar.gz" *.c makefile

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

semaphore.o: sema.c sema.h
demon.o: demon.c sema.h
