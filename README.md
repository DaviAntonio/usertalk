# Usertalk

A simple `epoll()`-based chat room application. Client programs are able to
talk to other clients in the same chat room using a server program.

## Requirements

- GCC 10+
- GNU/Linux Operating System (`epoll()` is a Linux-exclusive system call)
- GNU Make

## Compiling
Run `make` in the project's directory. Running `make clean` will clean the build
artefacts (object files and the `server` and `client` binaries)

## Running

Run the `server` binary first. Please provide the IPV4 address and the port as
arguments. The `client` binary must be launched with the same arguments:

```bash
./server 127.0.0.3 8000
./client 127.0.0.3 8000
```

### Commands

The client applications may issue these commands to the server:

- `\HELP` shows the available commands
- `\SETNICK new_nickname` sets up a new nickname
- `\NEWROOM limit name` creates a new chat room with a user limit and a nickname
- `\JOINROOM room_id` switches the client to the room with the inserted identifier
- `\INFO` shows information concerning the current user and the current chat rooom

## Known bugs

The client and rooms queues may get tangled up. The server will terminate on
these rare ocasions.

The server does not frees all the memory it should.

Code needs better organisation.