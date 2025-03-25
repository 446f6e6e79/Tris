# Multiplayer Terminal Tic Tac Toe

## Project Overview
This project is a **Multiplayer Terminal Tic Tac Toe** game developed in **C**. It leverages **Unix system calls** to handle synchronization between players. The game also includes an option to play against a bot for single-player mode. 

It was developed according to the given requirements for the Sistemi Operativi exam for

## Requirements
- A Unix-based operating system.
- GCC compiler for building the project.

## Installation
1. Clone the repository:
    ```bash
    git clone https://github.com/446f6e6e79/Tris
    cd Tris
    ```
2. Compile the program:
    ```
    make
    ```
    The makeFile, contained in the Tris directory will perform the compilation process.

## Usage
- To start the game in multiplayer mode, open a first terminal and start the server
    ```bash
    bin/TrisServer <timeout> <SimboloPlayer1> <SimboloPlayer2>
    ```
- Start other two terminals, to run each client
    ```bash
    bin/TrisClient <nomeUtente>
    ```
- To play againts the automated bot, run a single client, with the command:
    ```bash
        bin/TrisClient <nomeUtente> \*
    ```

## How It Works
- **Multiplayer Mode**: Two players take turns, with synchronization managed using Unix system calls.
- **Bot Mode**: The bot chooses a random free location, as required.

## File Structure
- `TrisClient.c`: Contains the logic for the clients.
- `TrisServer.c`: Contains the logic for the server.
- `semaphore.c`: Implements the general operations for the semaphores.
- `Documents/Elaborato_SO.pdf`: Little documentation for the project.

## Authors
Developed by Davide and Andrea.  
