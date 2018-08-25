/**************************************************/
/*  Invictus 2018						          */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include "typedefs.h"
#include "utils.h"
#include "uci.h"
#include "search.h"
#include "attacks.h"
#include "eval.h"

const std::string uci_t::name = "Invictus";
const std::string uci_t::author = "Edsel Apostol";
const std::string uci_t::year = "2018";
const std::string uci_t::version = "0.001";

void uci_t::info() {
	LogAndPrintOutput() << name << " " << version;
	LogAndPrintOutput() << "Copyright (C) " << year << " " << author;
	LogAndPrintOutput() << "Use UCI commands\n";
}

uci_t::uci_t() {
	std::cout.setf(std::ios::unitbuf);
	Attacks::initArr();
	PositionData::initArr();
	EvalPar::initArr();
	Search::initArr();
	//EvalPar::displayPST();
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
	else LogAndPrintError() << "Invalid cmd: " << cmd;

	return true;
}

void uci_t::quit() {
	engine.stopthreads();
	engine.waitForThreads();
	LogInfo() << "Engine quitting";
}

void uci_t::displayID() {
	LogAndPrintOutput() << "id name " << name << " " << version;
	LogAndPrintOutput() << "id author " << author;
	engine.printUCIoptions();
	LogAndPrintOutput() << "uciok";
}

void uci_t::stop() {
	LogInfo() << "Aborting search: stop";
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
	while (param != "") {
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
		else { LogAndPrintError() << "Wrong go command param: " << param; return; }
		param = "";
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
		LogAndPrintError() << "Invalid position!";
		return;
	}
	engine.origpos.setPosition(fen);
	for (bool found = true; stream >> token && found;) {
		movelist_t ml;
		undo_t undo;
		found = false;
		engine.origpos.genLegal(ml);
		for (auto& a : token) a = tolower(a);
		for (int x = 0; x < ml.size; ++x) {
			if (ml.mv(x).to_str() == token) {
				found = true;
				engine.origpos.doMove(undo, ml.mv(x));
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

void uci_t::perftbench(iss& stream) {
	int depth;
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
	movelist_t ml;
	engine.origpos.genLegal(ml);
	for (int x = 0; x < ml.size; ++x)
		PrintOutput() << " " << ml.mv(x).to_str();
}

void uci_t::displaypos() {
	PrintOutput() << engine.origpos.to_str();
}