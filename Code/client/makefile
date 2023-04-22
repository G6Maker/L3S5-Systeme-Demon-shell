semaphore_dir = ../semaphore/
CC = gcc
CFLAGS = -std=c18 \
  -Wall -Wconversion -Werror -Wextra -Wfatal-errors -Wpedantic -Wwrite-strings \
  -DxNATIMPHIGH \
  -D_POSIX_C_SOURCE=200809L\
  -D_XOPEN_SOURCE=800 \
  -DxDINPUT -DxDOUTPUT \
  -pthread -g \
  -I$(semaphore_dir) \
  -DRECTERM
LDFLAGS = -pthread -lrt
vpath %.c $(semaphore_dir)
vpath %.h $(semaphore_dir)
objects = sema.o client.o
executable = client

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

tar:
	$(MAKE) clean
	tar -zcf "$(CURDIR)_$(shell date +'%d.%m.%y-%Hh%M').tar.gz" *.c makefile

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

semaphore.o: sema.c sema.h
client.o: client.c sema.h
