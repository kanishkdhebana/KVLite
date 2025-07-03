# KV-Lite: In-Memory Key-Value Store

KV-Lite is a lightweight, single-threaded, TCP-based in-memory key-value store written in modern C++. It supports basic Redis-like commands (`SET`, `GET`, `DEL`, `CLEAR`) over a custom binary protocol, built to deepen understanding of low-level network programming, memory management, and data structures.

---

## Highlights

- **Custom Hash Table**: Implements a basic separate-chaining hash table from scratch.
- **Non-Blocking I/O**: Uses `poll()` for concurrent client handling without threads.
- **Custom Binary Protocol**: Lightweight protocol with explicit framing for commands and responses.
- **Minimal Dependencies**: No external libraries required.

---

## Current Features

- **SET**, **GET**, **DEL**, **CLEAR** commands.
- Handles multiple client connections via a single-threaded loop.
- All data is stored in-memory (volatile).
- Well-documented request/response protocol.

---

## Build

The project has no external dependencies and can be compiled with a standard C++ compiler like `g++`.

```bash
# Makefile
make

# Compile the client
g++ -o client client.cpp
```
---

## Usage

### Start the Server

The server listens on port `1234` by default.

```bash
./server
```

### Use the Client

Interact with the server from another terminal.

#### Set a Value

```bash
./client set mykey "hello world"
# Output:
#   status: NX
#   data: (nil)
```

#### Get a Value

```bash
./client get mykey
# Output:
#   status: OK
#   data: hello world
```

#### Delete a Value

```bash
./client del mykey
# Output:
#   status: OK
#   data: (nil)
```

#### Get a Non-Existent Key

```bash
./client get mykey
# Output:
#   status: OK
#   data: (nil)
```

#### List All Keys

```bash
./client keys
# Output:
#   status: OK
#   data: [ "key1", "key2", ... ]
```

---

## Network Protocol

Communication is done over a simple binary protocol. All integers are 4-byte unsigned integers in **network byte order (big-endian)**.

### Client Request Format

```
[Total Payload Length] [Number of Arguments] [Arg1 Length] [Arg1] [Arg2 Length] [Arg2] ...
```

- **Total Payload Length**: The byte length of the entire message that follows (i.e., from `Number of Arguments` to the end).
- **Number of Arguments**: The count of string arguments in the command.
- **ArgN Length**: The byte length of the Nth argument string.
- **ArgN**: The raw bytes of the Nth argument string.

### Server Response Format

```
[Total Payload Length] [Tag] [Response Data]
```

- **Total Payload Length**: The byte length of the message that follows (i.e., Tag + Response Data).
- **Status Code**: A status code indicating the outcome of the operation.
- **Tag**: Indicates the type of response:
  - 0: NIL (not found)
  - 1: ERROR
  - 2: STRING
  - 3: INT
  - 4: DOUBLE
  - 5: ARRAY
- **Response Data**: The value associated with a key, an array of keys, or error information.

---
