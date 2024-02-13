# P2P File Transfer System README

This project aims to build a simple peer-to-peer (P2P) file transfer application. The system comprises a node acting as the source, distributing chunks of a file among other receivers on the network. Specific optimizations are implemented to ensure scalability with an increasing number of receivers and file sizes.

## Introduction

The project implements a file transfer system enabling a single file transfer between a sender and multiple receivers. The sender initiates the process by distributing file chunks to each receiver, who then share chunks amongst themselves to reconstruct the original file.

## Assumptions

1. The network operates within a Local Area Network (LAN) environment.
2. Each device on the network has knowledge of the IP addresses of all other devices.
3. The total number of receivers is known at the start of the file sharing process.
4. No receiver failures occur during the file sharing process.
5. Each receiver receives a distinct block from the sender; no receiver receives a block already obtained by another receiver.
6. Files shared are substantially larger than 4KB.

## Architectural Overview

Implemented in C, the system consists of three files: one for the sender, one for receivers, and a library file containing shared functions.

1. **Sender File (Sender.c)**:
   - The sender file is called first to initiate the file transfer process.
   - Arguments passed to the sender file include the file to be transferred and the number of receivers.
   - Within the sender file, the main function orchestrates the process of splitting the file into chunks and distributing them to the receivers.

2. **Receiver File (Receiver.c)**:
   - The receiver file is called after the sender to facilitate the reception and exchange of file chunks.
   - Arguments passed to the receiver file include the number of receivers. If there are five receivers, the receiver file is called on each of the receivers.
   - Each receiver already has the address of the sender beforehand.
   - Within the receiver file, the main function handles the connection to the sender and other receivers, as well as the reception and exchange of file chunks.

3. **Library File (Functions.c)**:
   - The library file contains shared functions utilized by both the sender and receiver files.
   - This file may not be directly called; instead, it is included in both the sender and receiver files.
   - Functions within this file may include chunking the file, establishing connections, sending and receiving chunks, and handling threading operations.


