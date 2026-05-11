CC      = gcc
CFLAGS  = -Wall -Wextra -pthread
LDFLAGS = -pthread

# race se compila sin optimizacion para que la condicion de carrera
# sea observable (con -O2 el compilador puede meter la suma a un registro)
CFLAGS_RACE = -Wall -Wextra -O0 -pthread

TARGETS = escaner race

.PHONY: all clean

all: $(TARGETS)

escaner: src/escaner.c
	$(CC) $(CFLAGS) -O2 -o $@ $< $(LDFLAGS)

race: src/race.c
	$(CC) $(CFLAGS_RACE) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)
