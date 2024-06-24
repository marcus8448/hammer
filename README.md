# hammer

A minimax reversi AI for a Python reversi competition... but it's written in C.
Honestly, I'm surprised I was allowed to do this.

It uses a simple minimax algorithm and avoids interfacing with python as much as possible.
Moves are calculated using bit-shifted masks and the board is represented as two 64-bit integers
(one for the opponents pieces, the other for the player).
