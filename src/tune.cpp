/**************************************************/
/*  Invictus 2021                                 */
/*  Edsel Apostol                                 */
/*  ed_apostol@yahoo.com                          */
/**************************************************/

#include <vector>
#include <chrono>
#include <string>
#include <algorithm>
#include <random>

#include "utils.h"
#include "log.h"
#include "eval.h"
#include "params.h"

#define N(v) (#v)

#ifdef TUNE
namespace Tuner {
    using Ldouble = long double;

    Ldouble K = 1.37996;
    const int num_threads = 4;

    const int TuneAll = 1;
    const int TuneMaterial = 0;
    const int TuneActivity = 0;
    const int TunePawnStructure = 0;
    const int TuneThreats = 0;
    const int TuneKingSafety = 0;
    const int TunePassedPawns = 0;
    const int TunePhase = 0;
    const int TuneNew = 0;

    struct PositionResults {
        position_t* p;
        float result;
    };

    struct TunerParam {
        TunerParam(basic_score_t& v, basic_score_t l, basic_score_t u, std::string n) : val(&v), lower(l), upper(u), name(n) {}
        basic_score_t *val, lower, upper;
        std::string name;
        operator basic_score_t&() { return *val; }
        operator const basic_score_t&()const { return *val; }
        void operator=(const basic_score_t& value) { *val = std::min(std::max(lower, value), upper); }
    };

    Ldouble Sigmoid(Ldouble score) {
        return 1.0 / (1.0 + std::pow(10.0, -K * score / 400.0));
    }

    void RunOnThread(std::vector<PositionResults>& data, std::vector<Ldouble>& errors, size_t batchsize, int t, int numthreads) {
        for (size_t k = t; k < batchsize; k += numthreads) {
            eval_t eval;
            basic_score_t scr = eval.score(*data[k].p) * (data[k].p->side == WHITE ? +1.0 : -1.0);
            errors[k] = std::pow(data[k].result - Sigmoid(scr), 2);
        }
    }

    Ldouble Error(std::vector<PositionResults>& data, size_t batchsize) {
        std::vector<Ldouble> errors(batchsize, 0);
        std::thread threads[num_threads];
        for (int t = 0; t < num_threads; ++t) {
            threads[t] = std::thread(RunOnThread, ref(data), ref(errors), batchsize, t, num_threads);
        }
        for (int t = 0; t < num_threads; ++t) {
            threads[t].join();
        }
        // Neumaier Sum
        Ldouble sum = 0.0, c = 0.0;
        for (auto e : errors) {
            Ldouble t = sum + e;
            if (fabs(sum) >= fabs(e)) c += (sum - t) + e;
            else c += (e - t) + sum;
            sum = t;
        }
        return (sum + c) / batchsize;
    }

    void Randomize(std::vector<PositionResults>& data) {
        std::shuffle(data.begin(), data.end(), std::default_random_engine(Utils::getTime()));
    }

    void FindBestK(std::vector<PositionResults>& data) {
        Ldouble min = -10, max = 10, delta = 1, best = 1, error = 100;
        for (int precision = 0; precision < 10; ++precision) {
            PrintOutput() << "\nFindBestK: min:" << min << " max:" << max << " delta:" << delta;
            while (min < max) {
                K = min;
                Ldouble e = Error(data, data.size());
                if (e < error) {
                    error = e, best = K;
                    PrintOutput() << "  new best K = " << K << ", E = " << error;
                }
                min += delta;
            }
            min = best - delta;
            max = best + delta;
            delta /= 10;
        }
        K = best;
    }

    void CalculateGradients(std::vector<Ldouble>& gradients, std::vector<TunerParam>& params, std::vector<PositionResults>& data, size_t batchsize, Ldouble baseError) {
        int k = 0;
        for (auto par : params) {
            const basic_score_t oldvalue = par;
            par = oldvalue + 1;
            if (par != oldvalue) { // reached upper value
                Ldouble ep1 = Error(data, batchsize);
                gradients[k] = ep1 - baseError;
            }
            else {
                par = oldvalue - 1;
                Ldouble em1 = Error(data, batchsize);
                gradients[k] = -(em1 - baseError);
            }
            par = oldvalue;
            //PrintOutput() << "gradient: " << gradients[k];
            ++k;
        }
    }

    void PrintParams(std::ofstream& outfile, std::vector<TunerParam>& params, Ldouble start, Ldouble end, int epoch) {
        std::ostringstream  out;
        out << "\n";
        for (auto par : params) {
            out << par.name << "\t\t" << par << "\n";
        }
        out << "\nEpoch = " << epoch << " start error = " << start << " new error = " << end;
        out << " improved by = " << (start - end) * 10e6 << "\n";
        PrintOutput() << out.str();
        outfile << out.str();
        outfile.flush();
    }

    void AdamSGD(std::vector<TunerParam>& params, std::vector<PositionResults>& data, const size_t batchsize) {
        std::ofstream str("tuned.txt");
        std::vector<Ldouble> gradients(params.size(), 0.0);
        std::vector<Ldouble> M(params.size(), 0.0);
        std::vector<Ldouble> V(params.size(), 0.0);
        const Ldouble Alpha = 0.3;
        const Ldouble Beta1 = 0.9;
        const Ldouble Beta2 = 0.999;
        const Ldouble Epsilon = 1.0e-8;

        Ldouble best = Error(data, data.size());

        for (int epoch = 1; epoch <= 100000; ++epoch) {
            PrintOutput() << "\n\nEpoch = " << epoch;
            Randomize(data);

            const Ldouble baseError = Error(data, batchsize);

            PrintOutput() << "Computing gradients...";
            CalculateGradients(gradients, params, data, batchsize, baseError);

            int k = 0;
            for (auto par : params) {
                Ldouble grad = gradients[k];
                M[k] = Beta1 * M[k] + (1.0 - Beta1) * grad;
                V[k] = Beta2 * V[k] + (1.0 - Beta2) * grad * grad;
                Ldouble learnrate = Alpha * (1.0 - pow(Beta2, epoch)) / (1.0 - pow(Beta1, epoch));
                Ldouble delta = learnrate * M[k] / (sqrt(V[k]) + Epsilon);
                const basic_score_t oldpar = par;
                par = par - delta;
                if (par != oldpar) PrintOutput() << par.name << "\t\t" << oldpar << " --> " << par;
                ++k;
            }

            Ldouble currError = Error(data, batchsize);
            PrintOutput() << "Base error = " << baseError << " Current error = " << currError << " diff = " << (baseError - currError) * 10e6;

            if (epoch % 10 == 0) {
                Ldouble completeError = Error(data, data.size());
                PrintParams(str, params, best, completeError, epoch);
                if (completeError == best) break;
                else best = completeError;
            }
        }
    }

    void LocalSearch(std::vector<TunerParam>& params, std::vector<PositionResults>& data) {
        std::ofstream str("tuned.txt");
        const std::vector<Ldouble> delta = { 1, -1 };
        std::vector<Ldouble> last_delta(params.size(), 1.0);

        Ldouble currError = Error(data, data.size());
        PrintOutput() << "Base Error: " << currError;

        int epoch = 0;
        while (true) {
            ++epoch;
            PrintOutput() << "\n\nEpoch = " << epoch << " start error = " << currError;
            Ldouble baseError = currError;
            Randomize(data);
            int k = -1;
            for (auto par : params) {
                ++k;
                const basic_score_t oldpar = par;
                par = oldpar + last_delta[k];
                if (oldpar != par) {
                    Ldouble error = Error(data, data.size());
                    if (error < currError) {
                        currError = error;
                        PrintOutput() << par.name << " " << oldpar << " --> " << par;
                        continue;
                    }
                }
                par = oldpar + last_delta[k] * -1.0;
                if (oldpar != par) {
                    Ldouble error = Error(data, data.size());
                    if (error < currError) {
                        currError = error;
                        last_delta[k] = last_delta[k] * -1.0;
                        PrintOutput() << par.name << " " << oldpar << " --> " << par;
                        continue;
                    }
                }
                par = oldpar;
                last_delta[k] = last_delta[k] * -1.0;
            }
            PrintParams(str, params, baseError, currError, epoch);
            if (baseError == currError) break;
        }
    }

    float getResult(const std::string& s) {
        if (s == "White") return 1.0;
        if (s == "Black") return 0.0;
        if (s == "Draw") return 0.5;
        PrintOutput() << "Bad position result " << s;
        return 0.5;
    }

    void Tune(const std::string& filename) {
        std::vector<PositionResults> data;
        PrintOutput() << "Tuning with file = " << filename;

        std::ifstream file = std::ifstream(filename);
        if (!file) {
            PrintOutput() << "File not found = " << filename;
            return;
        }
        std::string line;
        while (getline(file, line)) {
            data.push_back({ new position_t(line), getResult(line.substr(line.find("|") + 1)) });
        }
        file.close();
        PrintOutput() << "Data size : " << data.size();

        size_t batchSize = data.size() / 10;
        //size_t batchSize = data.size() / 100;
        //size_t batchSize = 16384;
        //size_t batchSize = 1;

        using namespace EvalParam;
        std::vector<TunerParam> input;

        if (TuneAll || TuneMaterial) {
            //input.push_back({MaterialValues[PAWN].m, 0, 2000, N(PawnVal.m"}); // peg to 100
            input.push_back({ MaterialValues[PAWN].e, 0, 2000, N(PawnVal.e) });
            input.push_back({ MaterialValues[KNIGHT].m, 0, 2000, N(KnightVal.m) });
            input.push_back({ MaterialValues[KNIGHT].e, 0, 2000, N(KnightVal.e) });
            input.push_back({ MaterialValues[BISHOP].m, 0, 2000, N(BishopVal.m) });
            input.push_back({ MaterialValues[BISHOP].e, 0, 2000, N(BishopVal.e) });
            input.push_back({ MaterialValues[ROOK].m, 0, 2000, N(RookVal.m) });
            input.push_back({ MaterialValues[ROOK].e, 0, 2000, N(RookVal.e) });
            input.push_back({ MaterialValues[QUEEN].m, 0, 2000, N(QueenVal.m) });
            input.push_back({ MaterialValues[QUEEN].e, 0, 2000, N(QueenVal.e) });
            input.push_back({ BishopPair.m, 0, 100, N(BishopPair.m) });
            input.push_back({ BishopPair.e, 0, 100, N(BishopPair.e) });
        }
        if (TuneAll || TunePawnStructure) {
            input.push_back({ PawnConnected.m, 0, 100, N(PawnConnected.m) });
            input.push_back({ PawnConnected.e, 0, 100, N(PawnConnected.e) });
            input.push_back({ PawnDoubled.m, 0, 100, N(PawnDoubled.m) });
            input.push_back({ PawnDoubled.e, 0, 100, N(PawnDoubled.e) });
            input.push_back({ PawnIsolated.m, 0, 100, N(PawnIsolated.m) });
            input.push_back({ PawnIsolated.e, 0, 100, N(PawnIsolated.e) });
            input.push_back({ PawnBackward.m, 0, 100, N(PawnBackward.m) });
            input.push_back({ PawnBackward.e, 0, 100, N(PawnBackward.e) });
            input.push_back({ PawnIsolatedOpen.m, 0, 100, N(PawnIsolatedOpen.m) });
            input.push_back({ PawnIsolatedOpen.e, 0, 100, N(PawnIsolatedOpen.e) });
            input.push_back({ PawnBackwardOpen.m, 0, 100, N(PawnBackwardOpen.m) });
            input.push_back({ PawnBackwardOpen.e, 0, 100, N(PawnBackwardOpen.e) });
        }
        if (TuneAll || TunePassedPawns) {
            for (int rank = 1; rank <= 6; ++rank) {
                input.push_back({ PasserDistOwn[rank].m, 0, 1000, "PasserDistOwn[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserDistOwn[rank].e, 0, 1000, "PasserDistOwn[" + std::to_string(rank) + "].e" });
                input.push_back({ PasserDistEnemy[rank].m, 0, 1000, "PasserDistEnemy[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserDistEnemy[rank].e, 0, 1000, "PasserDistEnemy[" + std::to_string(rank) + "].e" });
                input.push_back({ PasserBonus[rank].m, 0, 1000, "PasserBonus[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserBonus[rank].e, 0, 1000, "PasserBonus[" + std::to_string(rank) + "].e" });
                input.push_back({ PasserNotBlocked[rank].m, 0, 1000, "PasserNotBlocked[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserNotBlocked[rank].e, 0, 1000, "PasserNotBlocked[" + std::to_string(rank) + "].e" });
                input.push_back({ PasserSafePush[rank].m, 0, 1000, "PasserSafePush[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserSafePush[rank].e, 0, 1000, "PasserSafePush[" + std::to_string(rank) + "].e" });
                input.push_back({ PasserSafeProm[rank].m, 0, 1000, "PasserSafeProm[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserSafeProm[rank].e, 0, 1000, "PasserSafeProm[" + std::to_string(rank) + "].e" });
            }
        }
        if (TuneAll || TuneActivity) {
            for (int cnt = 0; cnt < 9; ++cnt) {
                input.push_back({ KnightMob[cnt].m, -300, 300, "KnightMob[" + std::to_string(cnt) + "].m" });
                input.push_back({ KnightMob[cnt].e, -300, 300, "KnightMob[" + std::to_string(cnt) + "].e" });
            }
            for (int cnt = 0; cnt < 14; ++cnt) {
                input.push_back({ BishopMob[cnt].m, -300, 300, "BishopMob[" + std::to_string(cnt) + "].m" });
                input.push_back({ BishopMob[cnt].e, -300, 300, "BishopMob[" + std::to_string(cnt) + "].e" });
            }
            for (int cnt = 0; cnt < 15; ++cnt) {
                input.push_back({ RookMob[cnt].m, -300, 300, "RookMob[" + std::to_string(cnt) + "].m" });
                input.push_back({ RookMob[cnt].e, -300, 300, "RookMob[" + std::to_string(cnt) + "].e" });
            }
            for (int cnt = 0; cnt < 28; ++cnt) {
                input.push_back({ QueenMob[cnt].m, -300, 300, "QueenMob[" + std::to_string(cnt) + "].m" });
                input.push_back({ QueenMob[cnt].e, -300, 300, "QueenMob[" + std::to_string(cnt) + "].e" });
            }
            input.push_back({ RookOn7th.m, 0, 100, N(RookOn7th.m) });
            input.push_back({ RookOn7th.e, 0, 100, N(RookOn7th.e) });
            input.push_back({ RookOnSemiOpenFile.m, 0, 100, N(RookOnSemiOpenFile.m) });
            input.push_back({ RookOnSemiOpenFile.e, 0, 100, N(RookOnSemiOpenFile.e) });
            input.push_back({ RookOnOpenFile.m, 0, 100, N(RookOnOpenFile.m) });
            input.push_back({ RookOnOpenFile.e, 0, 100, N(RookOnOpenFile.e) });
            input.push_back({ OutpostBonus.m, 0, 100, N(OutpostBonus.m) });
            input.push_back({ OutpostBonus.e, 0, 100, N(OutpostBonus.e) });
            input.push_back({ BishopPawns.m, 0, 100, N(BishopPawns.m) });
            input.push_back({ BishopPawns.e, 0, 100, N(BishopPawns.e) });
            input.push_back({ Tempo, 0, 100, N(Tempo) });
        }
        if (TuneAll || TuneThreats) {
            input.push_back({ PawnPush.m, 0, 100, N(PawnPush.m) });
            input.push_back({ PawnPush.e, 0, 100, N(PawnPush.e) });
            input.push_back({ WeakPawns.m, 0, 100, N(WeakPawns.m) });
            input.push_back({ WeakPawns.e, 0, 100, N(WeakPawns.e) });
            input.push_back({ PawnsxMinors.m, 0, 100, N(PawnsxMinors.m) });
            input.push_back({ PawnsxMinors.e, 0, 100, N(PawnsxMinors.e) });
            input.push_back({ MinorsxMinors.m, 0, 100, N(MinorsxMinors.m) });
            input.push_back({ MinorsxMinors.e, 0, 100, N(MinorsxMinors.e) });
            input.push_back({ MajorsxWeakMinors.m, 0, 100, N(MajorsxWeakMinors.m) });
            input.push_back({ MajorsxWeakMinors.e, 0, 100, N(MajorsxWeakMinors.e) });
            input.push_back({ PawnsMinorsxMajors.m, 0, 100, N(PawnsMinorsxMajors.m) });
            input.push_back({ PawnsMinorsxMajors.e, 0, 100, N(PawnsMinorsxMajors.e) });
            input.push_back({ AllxQueens.m, 0, 100, N(AllxQueens.m) });
            input.push_back({ AllxQueens.e, 0, 100, N(AllxQueens.e) });
            input.push_back({ KingxMinors.m, 0, 100, N(KingxMinors.m) });
            input.push_back({ KingxMinors.e, 0, 100, N(KingxMinors.e) });
            input.push_back({ KingxRooks.m, 0, 100, N(KingxRooks.m) });
            input.push_back({ KingxRooks.e, 0, 100, N(KingxRooks.e) });
        }
        if (TuneAll || TuneKingSafety) {
            input.push_back({ KnightAtk, 0, 200, N(KnightAtk) });
            input.push_back({ BishopAtk, 0, 200, N(BishopAtk) });
            input.push_back({ RookAtk, 0, 200, N(RookAtk) });
            input.push_back({ QueenAtk, 0, 200, N(QueenAtk) });
            input.push_back({ KingZoneAttacks, 0, 200, N(KingZoneAttacks) });
            input.push_back({ WeakSquares, 0, 200, N(WeakSquares) });
            input.push_back({ EnemyPawns, 0, 200, N(EnemyPawns) });
            input.push_back({ QueenSafeCheckValue, 0, 200, N(QueenSafeCheckValue) });
            input.push_back({ RookSafeCheckValue, 0, 200, N(RookSafeCheckValue) });
            input.push_back({ BishopSafeCheckValue, 0, 200, N(BishopSafeCheckValue) });
            input.push_back({ KnightSafeCheckValue, 0, 200, N(KnightSafeCheckValue) });
            input.push_back({ KingShelter1, 0, 100, N(KingShelter1) });
            input.push_back({ KingShelter2, 0, 100, N(KingShelter2) });
            input.push_back({ KingStorm1, 0, 100, N(KingStorm1) });
            input.push_back({ KingStorm2, 0, 100, N(KingStorm2) });
        }
        if (TuneAll || TunePhase) {
            input.push_back({ KnightPhase, 0, 100, N(KnightPhase) });
            input.push_back({ BishopPhase, 0, 100, N(BishopPhase) });
            input.push_back({ RookPhase, 0, 100, N(RookPhase) });
            input.push_back({ QueenPhase, 0, 100, N(QueenPhase) });
        }
        if (TuneAll || TuneNew) {
            input.push_back({ CenterSquares.m, 0, 100, N(CenterSquares.m) });
            input.push_back({ CenterSquares.e, 0, 100, N(CenterSquares.e) });
            input.push_back({ PieceSpace.m, 0, 100, N(PieceSpace.m) });
            input.push_back({ PieceSpace.e, 0, 100, N(PieceSpace.e) });
            input.push_back({ EmptySpace.m, 0, 100, N(EmptySpace.m) });
            input.push_back({ EmptySpace.e, 0, 100, N(EmptySpace.e) });
        }
        //FindBestK(data);
        PrintOutput() << "Best K " << K;

        PrintOutput() << "\nInitial values:";
        for (auto par : input) PrintOutput() << par.name << "\t\t\t" << par;

        //AdamSGD(input, data, batchSize);
        LocalSearch(input, data);

        PrintOutput() << "\nTuned values:";
        for (auto par : input) PrintOutput() << par.name << "\t\t\t" << par;

        for (auto &d : data) delete d.p;
    }
}
#endif