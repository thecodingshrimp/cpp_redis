# CPP Redis

This is a hobby project of coding Redis fundamentals in C++. I use it to gain more experience in C++.

It uses multithreading for serving client requests and snapshot its current state to/from disk by using a background process through `fork()`.

## Installation
`cmake --build build`

## Execution
`./build/database [port]`

Default port is 3000.

## Connection
You can connect via TCP.

## Supported Types and Commands

### Hashmap
The current version supports multiple types inside a hashmap. I might extend the types if there is something interesting to learn from it.

#### List commands
- LADD <key> <value>
- LGET <key> <idx>
- LDEL <key> <idx>

#### Native commands
- SET <key> <value>
- GET <key>
- DEL <key>

#### Hashmap commands
- HSET <key> <field> <value>
- HGET <key> <field>
- HDEL <key> <field>

### Snapshotting
- SAVE <filename> <filetype>
- LOAD <filename> <filetype>

