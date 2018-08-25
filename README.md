# Invictus Chess

Invictus is a state of the art UCI compliant chess engine. It uses magic bitboards with pext optimizations, PVS search on top of alpha-beta and iterative aspiration window search, null move pruning and internal iterative deepening, etc. It also has support for SMP using a modified ABDADA algorithm that should scale well with large number of threads/processors.

Invictus is influenced by other excellent open source projects especially Stokfish and Ethereal.
