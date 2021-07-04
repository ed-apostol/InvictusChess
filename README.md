# Invictus Chess

Invictus is a state of the art UCI compliant chess engine. 

Features:
* -magic bitboards move generation with pext optimizations
* -PVS search on top of alpha-beta and iterative aspiration window search
* -null move pruning, and other search heuristics
* -one of the best multi-threading implementation out there, using a modified ABDADA algorithm that should scale well with large number of threads/processors
* -Texel tuning with Stochastic Gradient Descent using Adam optimizer
* -Texel tuning with local search
* -NUMA support

Invictus is influenced by other excellent open source projects especially Stocksfish, Ethereal, and Defenchess. Special thanks to Minic from which I learned the Texel tuning code.

To Do:
* -NNUE support
* -Syzygy support
* -Polyglot opening book support

Thanks Kan for the logo! 
Thanks TCEC for giving me motivation to work with the engine! 
Thanks Tom Kerrigan (of TSCP fame) for the ABDADA idea!

Ratings:

[CEGT 40/20](http://www.cegt.net/40_40%20Rating%20List/40_40%20SingleVersion/rangliste.html)
* Invictus r305 (December 05, 2019) - 2738
* Invictus r340 (May 19, 2021) - 2902
* Invictus r382 (June 07, 2021) - 2986

[CCRL 40/4](https://ccrl.chessdom.com/ccrl/404/rating_list_all.html)
* Invictus r228 (August 05, 2018) - 2255
* Invictus r305 (December 05, 2019) - 2922
* Invictus r340 (May 19, 2021) - 3031
* Invictus r382 (June 07, 2021) - 3108

[CCRL 40/40](http://ccrl.chessdom.com/ccrl/4040/rating_list_all.html)
* Invictus r228 (August 05, 2018) - 2292
* Invictus r305 (December 05, 2019) - 2878
* Invictus r382 (June 07, 2021) - 3099

If you would like to help with the development of Invictus chess engine, which is being tested in a Google Compute instance due to lack of hardware resource, please consider donating!

[![](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=Y79ZRSGB2KFSE)
