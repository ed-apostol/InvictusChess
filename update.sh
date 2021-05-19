#!/bin/bash
git clone https://github.com/ed-apostol/InvictusChess.git
cd InvictusChess
read -p "Download done, press enter to continue"
cmake .
make
EXE=$PWD/invictus
