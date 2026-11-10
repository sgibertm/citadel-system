# The Citadel System 🏰

A distributed system in **C** that simulates a network of independent realms forming alliances and trading with each other, with **no central authority**.

Pair project for the **Operating Systems** course, Bachelor in Artificial Intelligence & Data Science, La Salle – Universitat Ramon Llull (2025–2026).

![C](https://img.shields.io/badge/C-systems-555555) ![Linux](https://img.shields.io/badge/Linux-POSIX-yellow) ![Sockets](https://img.shields.io/badge/networking-TCP%20sockets-blue)

## What it does

Each realm runs its own **Maester** process, a node with its own inventory and routing table. Maesters talk to each other over TCP sockets to request alliances, list each other's products and trade. Messages that can't reach their destination directly are forwarded through intermediate realms (multi-hop routing).

## How it was built: 4 phases

| Phase | What we added | OS concepts |
| --- | --- | --- |
| 1 | The Maester process: reads its configuration and inventory and runs an interactive command line | Processes, file I/O |
| 2 | Communication between Maesters: alliances and messages across a multi-hop routing network | TCP sockets, routing, checksum validation |
| 3 | File transfer: sending alliance sigils and trade orders, with integrity checks | Framed binary protocol, MD5 verification |
| 4 | **Envoys**: worker processes that carry out missions so a Maester can handle several operations at once | Concurrency, threads, mutexes, IPC |

## Commands

```
LIST REALMS                       Show known realms and routes
LIST PRODUCTS [realm]             Show your inventory, or another realm's
PLEDGE <realm> <sigil>            Request an alliance, sending your sigil
PLEDGE RESPOND <realm> ACCEPT|REJECT
PLEDGE STATUS                     Show alliance requests
START TRADE <realm>               Open a trade session and build an order
ENVOY STATUS                      Show what each envoy is doing
EXIT
```

## My contribution

I programmed most of the system together with my partner.

## Authors

Sara Gibert · Oriol Pros

## Build and run (Linux)

```bash
make
./Maester REALMS/arryn/arryn.dat REALMS/arryn/Arryn_stock.db
```

Start several realms in different terminals (or machines) to see them communicate; each realm's `.dat` file sets its IP, port and routing table. `make clean` removes the build files.

## Project structure

```
maester.c              Entry point: one Maester (realm node)
COMMANDS/              Command parsing and LIST, PLEDGE, START TRADE, ENVOY commands
CONFIG/                Local, network and envoy configuration
SOCKET/                Frame protocol and socket thread handling
REALMS/                Sample realms: configuration (.dat), inventory (.db) and sigils
utils.c / utils.h      Shared helpers
report.pdf             Full technical report covering all four phases
```

The realm names come from the course assignment. The sigil images in `REALMS/` are simple placeholders.
