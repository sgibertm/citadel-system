# OS - Project - Makefile for The Citadel System project
# Code by Sara Gibert & Oriol Pros
# Group: 4

CC     = gcc
CFLAGS = -Wall -Wextra -pthread

# ── Object files ──────────────────────────────────────────────────────────────
OBJS = maester.o \
       utils.o \
       detectCommand.o \
       startComm.o \
       tradeFunc.o \
       pledgeComm.o \
       envoyComm.o \
       listComm.o \
       localConfig.o \
       networkConfig.o \
	   envoyConfig.o \
       frame.o \
       threadHandler.o

# ── Default target ─────────────────────────────────────────────────────────────
all: Maester

# ── Final binary ───────────────────────────────────────────────────────────────
Maester: $(OBJS)
	$(CC) $(OBJS) -o Maester $(CFLAGS)

# ── Root-level sources ─────────────────────────────────────────────────────────
maester.o: maester.c utils.h CONFIG/localConfig.h CONFIG/networkConfig.h CONFIG/envoyConfig.h
	$(CC) -c maester.c -o maester.o $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -c utils.c -o utils.o $(CFLAGS)

# ── COMMANDS/ ──────────────────────────────────────────────────────────────────
detectCommand.o: COMMANDS/detectCommand.c COMMANDS/detectCommand.h utils.h
	$(CC) -c COMMANDS/detectCommand.c -o detectCommand.o $(CFLAGS)

startComm.o: COMMANDS/START/startComm.c COMMANDS/START/startComm.h utils.h
	$(CC) -c COMMANDS/START/startComm.c -o startComm.o $(CFLAGS)

tradeFunc.o: COMMANDS/START/tradeFunc.c COMMANDS/START/tradeFunc.h utils.h
	$(CC) -c COMMANDS/START/tradeFunc.c -o tradeFunc.o $(CFLAGS)

pledgeComm.o: COMMANDS/PLEDGE/pledgeComm.c COMMANDS/PLEDGE/pledgeComm.h utils.h
	$(CC) -c COMMANDS/PLEDGE/pledgeComm.c -o pledgeComm.o $(CFLAGS)

envoyComm.o: COMMANDS/ENVOY/envoyComm.c COMMANDS/ENVOY/envoyComm.h utils.h
	$(CC) -c COMMANDS/ENVOY/envoyComm.c -o envoyComm.o $(CFLAGS)

listComm.o: COMMANDS/LIST/listComm.c COMMANDS/LIST/listComm.h utils.h
	$(CC) -c COMMANDS/LIST/listComm.c -o listComm.o $(CFLAGS)

# ── CONFIG/ ──────────────────────────────────────────────────────────────────
localConfig.o: CONFIG/localConfig.c CONFIG/localConfig.h utils.h
	$(CC) -c CONFIG/localConfig.c -o localConfig.o $(CFLAGS)

networkConfig.o: CONFIG/networkConfig.c CONFIG/networkConfig.h SOCKET/frame.h
	$(CC) -c CONFIG/networkConfig.c -o networkConfig.o $(CFLAGS)

envoyConfig.o: CONFIG/envoyConfig.c CONFIG/envoyConfig.h utils.h
	$(CC) -c CONFIG/envoyConfig.c -o envoyConfig.o $(CFLAGS)

# ── SOCKET/ ───────────────────────────────────────────────────────────
frame.o: SOCKET/frame.c SOCKET/frame.h
	$(CC) -c SOCKET/frame.c -o frame.o $(CFLAGS)

threadHandler.o: SOCKET/threadHandler.c SOCKET/threadHandler.h
	$(CC) -c SOCKET/threadHandler.c -o threadHandler.o $(CFLAGS)

# ── Housekeeping ───────────────────────────────────────────────────────────────
clean:
	rm -f $(OBJS) Maester