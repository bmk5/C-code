# P2P File Transfer System README

This project aims to build a simple peer-to-peer (P2P) file transfer application. The system comprises a node acting as the source, distributing chunks of a file among other receivers on the network. Specific optimizations are implemented to ensure scalability with an increasing number of receivers and file sizes.

## Introduction

The project implements a file transfer system enabling a single file transfer between a sender and multiple receivers. The sender initiates the process by distributing file chunks to each receiver, who then share chunks amongst themselves to reconstruct the original file.

## Assumptions

1. The network operates within a Local Area Network (LAN) environment.
2. Each device on the network has knowledge of the IP addresses of all other devices.
3. The total number of receivers is known at the start of the file sharing process(in addresses.txt file).
4. No receiver failures occur during the file sharing process.
5. Each receiver receives a distinct block from the sender; no receiver receives a block already obtained by another receiver.
6. Files shared are substantially larger than 4KB.

## Architectural Overview

Implemented in C, the system consists of three files: one for the sender, one for receivers, and a library file containing shared functions. The sender splits the file into 4KB chunks and distributes them to receivers, recording the time taken for each receiver to receive the entire file. Receivers connect to the sender to receive chunks, then connect to each other for chunk exchange. Threading is utilized for concurrent operations.

This README provides an overview of the P2P file transfer system, detailing its design and assumptions.
