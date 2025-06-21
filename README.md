# KV-Lite: In-Memory Key-Value Store

![Status](https://img.shields.io/badge/status-work_in_progress-yellow)

KV-Lite is a lightweight, single-threaded, TCP-based in-memory key-value store written in C++. It is built as a learning exercise in C++ network programming and is not intended for production use.

---

## Features

- **In-Memory Storage:** All data is stored in memory and will be lost when the server stops.
- **Non-Blocking I/O:** Uses `poll()` to handle multiple client connections without threads.
- **Simple Binary Protocol:** A custom, straightforward binary protocol for client-server communication.
- **Core Commands:** Implements basic key-value operations:
  - `set <key> <value>`
  - `get <key>`
  - `del <key>`
  - `clear` (deletes all keys)

---

## Project Status

This project is a **work-in-progress**. The primary goal is educational. Key areas for future development include:

- Data persistence to disk.
- Improved error handling and reporting.
- Concurrency and multi-threading.
- Support for more complex data types.

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
#   status: 0 (0=OK, 1=ERR, 2=NX)
```

#### Get a Value

```bash
./client get mykey
# Output:
#   status: 0 (0=OK, 1=ERR, 2=NX)
#   data: hello world
```

#### Delete a Value

```bash
./client del mykey
# Output:
#   status: 0 (0=OK, 1=ERR, 2=NX)
```

#### Get a Non-Existent Key

```bash
./client get mykey
# Output:
#   status: 2 (0=OK, 1=ERR, 2=NX)
```

#### Clear the Entire Store

```bash
./client clear
# Output:
#   status: 0 (0=OK, 1=ERR, 2=NX)
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
[Total Payload Length] [Status Code] [Response Data]
```

- **Total Payload Length**: The byte length of the message that follows (i.e., `Status Code + Response Data`).
- **Status Code**: A status code indicating the outcome of the operation.
  - `0`: OK
  - `1`: Error
  - `2`: Not Found (NX)
- **Response Data**: The value associated with a key (for a `get` command) or empty otherwise.

---
