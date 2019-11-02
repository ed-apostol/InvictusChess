/**************************************************/
/*  Invictus 2019                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "typedefs.h"
#include "utils.h"
#include "uci.h"
#include "search.h"
#include "attacks.h"
#include "eval.h"
#include "params.h"
#include "tune.h"

const std::string uci_t::name = "Invictus";
const std::string uci_t::author = "Edsel Apostol";
const std::string uci_t::year = "2019";
const std::string uci_t::version = "r275";

void uci_t::info() {
    LogAndPrintOutput() << name << " " << version;
    LogAndPrintOutput() << "Copyright (C) " << year << " " << author;
    LogAndPrintOutput() << "Use UCI commands\n";
}

uci_t::uci_t() {
    std::cout.setf(std::ios::unitbuf);
    Attacks::initArr();
    PositionData::initArr();
    EvalParam::initArr();
    Search::initArr();
    //EvalParam::displayPST();
    engine.origpos.setPosition(StartFEN);
}

void uci_t::run() {
    std::string line;
    while (getline(std::cin, line)) {
        LogInput() << line;
        iss ss(line);
        if (!input(ss)) break;
    }
}

bool uci_t::input(iss& stream) {
    std::string cmd;
    stream >> cmd;

    if (cmd == "isready") isready();
    else if (cmd == "quit") { quit(); return false; }
    else if (cmd == "stop") stop();
    else if (cmd == "ponderhit") ponderhit();
    else if (cmd == "go") gocmd(stream);
    else if (cmd == "position") positioncmd(stream);
    else if (cmd == "setoption") setoption(stream);
    else if (cmd == "ucinewgame") newgame();
    else if (cmd == "uci") displayID();
    else if (cmd == "perft") perftbench(stream);
    else if (cmd == "perft2") perft(stream);
    else if (cmd == "eval") eval();
    else if (cmd == "moves") moves();
    else if (cmd == "d") displaypos();
    else if (cmd == "speedup") speedup(stream);
    else if (cmd == "tune") tune();
    else LogAndPrintOutput() << "Invalid cmd: " << cmd;

    return true;
}

void uci_t::quit() {
    stop();
}

void uci_t::displayID() {
    LogAndPrintOutput() << "id name " << name << " " << version;
    LogAndPrintOutput() << "id author " << author;
    engine.printUCIoptions();
    LogAndPrintOutput() << "uciok";
}

void uci_t::stop() {
    engine.stopthreads();
    engine.waitForThreads();
}

void uci_t::ponderhit() {
    engine.ponderhit();
}

void uci_t::isready() {
    LogAndPrintOutput() << "readyok";
}

void uci_t::gocmd(iss& stream) {
    std::string param;
    uci_limits_t& limit = engine.limits;
    limit.init();

    stream >> param;
    while (!param.empty()) {
        if (param == "wtime") stream >> limit.wtime;
        else if (param == "btime") stream >> limit.btime;
        else if (param == "winc") stream >> limit.winc;
        else if (param == "binc") stream >> limit.binc;
        else if (param == "movestogo") stream >> limit.movestogo;
        else if (param == "ponder") limit.ponder = true;
        else if (param == "depth") stream >> limit.depth;
        else if (param == "movetime") stream >> limit.movetime;
        else if (param == "infinite") limit.infinite = true;
        else if (param == "nodes") stream >> limit.nodes;
        else if (param == "mate") stream >> limit.mate;
        else { LogAndPrintOutput() << "Wrong go command param: " << param; return; }
        param.clear();
        stream >> param;
    }
    engine.initSearch();
}

void uci_t::positioncmd(iss& stream) {
    std::string token, fen;
    stream >> token;
    if (token == "startpos") {
        fen = StartFEN;
        stream >> token;
    }
    else if (token == "fen") {
        while (stream >> token && token != "moves")
            fen += token + " ";
    }
    else {
        LogAndPrintOutput() << "Invalid position!";
        return;
    }
    engine.origpos.setPosition(fen);
    for (bool found = true; stream >> token && found;) {
        movelist_t<256> ml;
        undo_t undo;
        found = false;
        engine.origpos.genLegal(ml);
        for (auto& a : token) a = tolower(a);
        for (move_t m : ml) {
            if (m.to_str() == token) {
                found = true;
                engine.origpos.doMove(undo, m);
                break;
            }
        }
    }
}

void uci_t::setoption(iss& stream) {
    std::string token, name, value;
    stream >> token;
    while (stream >> token && token != "value") name += std::string(" ", !name.empty()) + token;
    while (stream >> token) value += std::string(" ", !value.empty()) + token;
    if (engine.options.count(name)) engine.options[name] = value;
    else LogAndPrintOutput() << "No such option: " << name;
}

void uci_t::newgame() {
    engine.newgame();
}

uci_t::~uci_t() {}

void uci_t::tune() {
#ifdef TUNE
    Tuner::Tune("lichess-quiet.txt");
    //Tuner::Tune("fish2.fens");
#endif
}

void uci_t::perftbench(iss& stream) {
    size_t depth;
    stream >> depth;

    std::ifstream infile("perft.epd");
    std::string line;
    uint64_t totaltime = 0;
    uint64_t totalnodes = 0;
    int cnt = 0;
    int correct = 0;
    while (std::getline(infile, line))
    {
        size_t split = 0;
        std::vector<uint64_t> n;
        while ((split = line.find(';', split + 1)) != std::string::npos) {
            std::string temp;
            uint64_t d;
            iss ss(line.substr(split));
            ss >> temp;
            ss >> d;
            n.push_back(d);
        }

        split = line.find(';');
        std::string fen = line.substr(0, split);
        PrintOutput() << "\n" << ++cnt << ": " << fen;
        engine[0]->pos.setPosition(fen.c_str());

        uint64_t oldtime = Utils::getTime();

        uint64_t nodes = engine[0]->perft(depth);
        //uint64_t nodes = engine[0]->perft2(depth);

        totalnodes += nodes;
        uint64_t timespent = Utils::getTime() - oldtime;
        totaltime += timespent;
        PrintOutput() << "nodes: " << nodes << " time: " << timespent << " ms";

        if (nodes == n[depth - 1]) ++correct;
        else PrintOutput() << "FAILED: " << fen << " correct nodes: " << n[depth - 1] << " current: " << nodes;
    }

    PrintOutput() << "\n" << correct << "/" << cnt << " NPS: " << (totalnodes * 1000 / (totaltime + 1)) << " totaltime: " << totaltime << " ms";
}

void uci_t::perft(iss& stream) {
    int depth;
    stream >> depth;
    engine[0]->pos = engine.origpos;
    uint64_t oldtime = Utils::getTime();
    uint64_t nodes = engine[0]->perft(depth);
    //uint64_t nodes = engine.perft2(depth);
    uint64_t timespent = Utils::getTime() - oldtime;
    PrintOutput() << "nodes: " << nodes << " time: " << timespent << " ms NPS: " << (nodes * 1000 / timespent);
}

void uci_t::eval() {
    eval_t eval;
    eval.score(engine.origpos);
}

void uci_t::moves() {
    movelist_t<256> ml;
    engine.origpos.genLegal(ml);
    for (move_t m : ml)
        PrintOutput() << " " << m.to_str();
}

void uci_t::displaypos() {
    PrintOutput() << engine.origpos.to_str();
}

void uci_t::speedup(iss& stream) {
    iss streamcmd;
    std::vector<std::string> fenPos = {
        "r3k2r/pbpnqp2/1p1ppn1p/6p1/2PP4/2PBPNB1/P4PPP/R2Q1RK1 w kq - 2 12",
        "2kr3r/pbpn1pq1/1p3n2/3p1R2/3P3p/2P2Q2/P1BN2PP/R3B2K w - - 4 22",
        "r2n1rk1/1pq2ppp/p2pbn2/8/P3Pp2/2PBB2P/2PNQ1P1/1R3RK1 w - - 0 17",
        "1r2r2k/1p4qp/p3bp2/4p2R/n3P3/2PB4/2PB1QPK/1R6 w - - 1 32",
        "1b3r1k/rb1q3p/pp2pppP/3n1n2/1P2N3/P2B1NPQ/1B3P2/2R1R1K1 b - - 1 32",
        "1r1r1qk1/pn1p2p1/1pp1npBp/8/2PB2QP/4R1P1/P4PK1/3R4 w - - 0 1",
        "3rr1k1/1b2nnpp/1p1q1p2/pP1p1P2/P1pP2P1/2N1P1QP/3N1RB1/2R3K1 w - - 0 1",
        "r1bqk1nr/ppp2pbp/2n1p1p1/7P/3Pp3/2N2N2/PPP2PP1/R1BQKB1R w KQkq - 0 7",
        "1r3rk1/3bb1pp/1qn1p3/3pP3/3P1N2/2Q2N2/2P3PP/R1BR3K w - - 0 1",
        "rn1q1rk1/2pbb3/pn2p3/1p1pPpp1/3P4/1PNBBN2/P1P1Q1PP/R4R1K w - - 0 1"
    };

    std::vector<int> threads;
    int depth;

    stream >> depth;
    for (int temp; stream >> temp; threads.push_back(temp));

    std::vector<double> timeSpeedupSum(threads.size(), 0.0);
    std::vector<double> nodesSpeedupSum(threads.size(), 0.0);

    for (size_t idxpos = 0; idxpos < fenPos.size(); ++idxpos) {
        LogAndPrintOutput() << "\n\nPos#" << idxpos + 1 << ": " << fenPos[idxpos];
        uint64_t nodes1 = 0;
        uint64_t spentTime1 = 0;
        for (size_t idxthread = 0; idxthread < threads.size(); ++idxthread) {
            streamcmd = iss("name Threads value " + std::to_string(threads[idxthread]));
            setoption(streamcmd);
            newgame();

            streamcmd = iss("fen " + fenPos[idxpos]);
            positioncmd(streamcmd);

            uint64_t startTime = Utils::getTime();

            streamcmd = iss("depth " + std::to_string(depth));
            gocmd(streamcmd);

            engine.waitForThreads();

            double timeSpeedUp;
            double nodesSpeedup;
            uint64_t spentTime = Utils::getTime() - startTime;
            uint64_t nodes = engine.nodesearched() / spentTime;

            if (0 == idxthread) {
                nodes1 = nodes;
                spentTime1 = spentTime;
                timeSpeedUp = (double)spentTime / 1000.0;
                timeSpeedupSum[idxthread] += timeSpeedUp;
                nodesSpeedup = (double)nodes;
                nodesSpeedupSum[idxthread] += nodesSpeedup;
                LogAndPrintOutput() << "\nPos#" << idxpos + 1 << " Threads: " << std::to_string(threads[idxthread]) << " time: " << std::to_string(timeSpeedUp)
                    << "s, " << std::to_string(nodes) << "knps\n";
            }
            else {
                timeSpeedUp = (double)spentTime1 / (double)spentTime;
                timeSpeedupSum[idxthread] += timeSpeedUp;
                nodesSpeedup = (double)nodes / (double)nodes1;
                nodesSpeedupSum[idxthread] += nodesSpeedup;
                LogAndPrintOutput() << "\nPos#" << idxpos + 1 << " Threads: " << std::to_string(threads[idxthread]) << " time: " << std::to_string(timeSpeedUp)
                    << " nodes: " << std::to_string(nodesSpeedup) << "\n";
            }
        }
    }
    LogAndPrintOutput() << "\n\n";
    LogAndPrintOutput() << "Threads: " << std::to_string(threads[0]) << " time: " << std::to_string(timeSpeedupSum[0] / fenPos.size()) << "s, "
        << std::to_string(nodesSpeedupSum[0] / fenPos.size()) << "knps";
    for (size_t idxthread = 1; idxthread < threads.size(); ++idxthread) {
        LogAndPrintOutput() << "Threads: " << std::to_string(threads[idxthread])
            << " time: " << std::to_string(timeSpeedupSum[idxthread] / fenPos.size()) << " nodes: " << std::to_string(nodesSpeedupSum[idxthread] / fenPos.size());
    }
    LogAndPrintOutput() << "\n\n";
}