# Invictus Chess

Invictus is a state of the art UCI compliant chess engine. 

Features:
* -magic bitboards with pext optimizations
* -PVS search on top of alpha-beta and iterative aspiration window search
* -null move pruning
* -internal iterative deepening
* -SMP using a modified ABDADA algorithm that should scale well with large number of threads/processors
* -Texel tuning with Stochastic Gradient Descent using Adam optimizer
* -Texel tuning with local search

Invictus is influenced by other excellent open source projects especially Stocksfish, Ethereal, and Defenchess. Special thanks to Minic from which I learned the Texel tuning code.
