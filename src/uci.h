/**************************************************/
/*  Invictus 2019                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#pragma once
#include <sstream>
#include <vector>
#include <unordered_map>
#include "typedefs.h"
#include "utils.h"
#include "engine.h"
#include "log.h"

using iss = std::istringstream;
class uci_t {
public:
    uci_t();
    ~uci_t();
    void info();
    void run();

private:
    bool input(iss& stream);
    void stop();
    void ponderhit();
    void isready();
    void gocmd(iss& stream);
    void positioncmd(iss& stream);
    void setoption(iss& stream);
    void newgame();
    void displayID();
    void quit();
    void tune();
    void perftbench(iss& stream);
    void perft(iss& stream);
    void eval();
    void moves();
    void displaypos();
    void speedup(iss& stream);

    static const std::string name;
    static const std::string author;
    static const std::string year;
    static const std::string version;

    engine_t engine;
};
