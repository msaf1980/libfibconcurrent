# libconcurrent

Concurrent data structuras

## Introduction

*Tested on Mac OSX 10.13 (clang 9.0 under Travis-CI only) and Linux 4.4 (gcc 4.8 and clang 5.0), 4.15 (gcc 5.4 and clang 7.0) and 5.3 (gcc 7.5 and clang 7.0) kernels. 

Function `size_to_power_of_2` round size to power of 2

The `mpmc_ring_queue` module implements a lock-free, non-blocking MPMC bounded queue (based on fixed size ring buffer).  Its characteristics are:
* Fixed size (size specified at queue creation time). Must be > 1 and a power of 2.
* Non-blocking (enqueuing to a full queue returns immediately with error; dequeuing from an empty queue returns NULL).
* Multi-Producer, Multi Consumer (MPMC).
  Producers and consumers operate independently, but the operations are not
  lock-free, i.e., a enqueue may have to wait for a pending dequeue operation to finish and vice versa.
* No dynamic memory allocates or frees during enqueue and dequeue operations.  Messages are stored as void pointers; _null pointers can be stored, but mpmc_ring_queue_dequeue return NULL when dequeued NULL or queue is epmty.

## Release Notes

## TODO
