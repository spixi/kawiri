# kawiri
High-performance network service for detection of duplicated data

## Dependencies
[zlib](http://zlib.net)

## Compilation
`gcc kawiri.c -lz -o kawiri`

## Start
`kawiri [port\_number]`

## Stop
`killall -KILL kawiri`

## How to use
kawiri (= Chichewa for _twice_) is a high-performance network service for detection of duplicated data. When you send a TCP message to the kawiri server, it will return 0 when the message is received for the first time or 1 when the same message has already been received. You need 512 MiB memory to run kawiri.
