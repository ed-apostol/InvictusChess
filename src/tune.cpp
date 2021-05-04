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

#ifdef TUNE
namespace Tuner {
    using Ldouble = long double;

    Ldouble K = 1.37621;
    const int num_threads = 7;

    const int TuneAll = 0;
    const int TuneMaterial = 0;
    const int TuneActivity = 0;
    const int TunePawnStructure = 0;
    const int TuneThreats = 0;
    const int TuneKingSafety = 0;
    const int TunePassedPawns = 0;
    const int TunePhase = 1;

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

        Ldouble currError = Error(data, data.size());
        PrintOutput() << "Base Error: " << currError;

        int epoch = 0;
        while (true) {
            ++epoch;
            PrintOutput() << "\n\nEpoch = " << epoch << " start error = " << currError;
            Ldouble baseError = currError;
            Randomize(data);
            for (auto par : params) {
                bool improved = false;
                const basic_score_t oldpar = par;
                for (auto d : delta) {
                    if (improved) break;
                    par = oldpar + d;
                    if (oldpar == par) continue; // reached upper or lower
                    Ldouble error = Error(data, data.size());
                    if (error < currError) {
                        currError = error;
                        improved = true;
                        PrintOutput() << par.name << " " << oldpar << " --> " << par;
                    }
                }
                if (!improved) par = oldpar;
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
            //input.push_back({MaterialValues[PAWN].m, 0, 2000, "PawnVal.m"}); // peg to 100
            input.push_back({ MaterialValues[PAWN].e, 0, 2000, "PawnVal.e" });
            input.push_back({ MaterialValues[KNIGHT].m, 0, 2000, "KnightVal.m" });
            input.push_back({ MaterialValues[KNIGHT].e, 0, 2000, "KnightVal.e" });
            input.push_back({ MaterialValues[BISHOP].m, 0, 2000, "BishopVal.m" });
            input.push_back({ MaterialValues[BISHOP].e, 0, 2000, "BishopVal.e" });
            input.push_back({ MaterialValues[ROOK].m, 0, 2000, "RookVal.m" });
            input.push_back({ MaterialValues[ROOK].e, 0, 2000, "RookVal.e" });
            input.push_back({ MaterialValues[QUEEN].m, 0, 2000, "QueenVal.m" });
            input.push_back({ MaterialValues[QUEEN].e, 0, 2000, "QueenVal.e" });
            input.push_back({ BishopPair.m, 0, 100, "BishopPair.m" });
            input.push_back({ BishopPair.e, 0, 100, "BishopPair.e" });
        }
        if (TuneAll || TunePawnStructure) {
            input.push_back({ PawnConnected.m, 0, 100, "PawnConnected.m" });
            input.push_back({ PawnConnected.e, 0, 100, "PawnConnected.e" });
            input.push_back({ PawnDoubled.m, 0, 100, "PawnDoubled.m" });
            input.push_back({ PawnDoubled.e, 0, 100, "PawnDoubled.e" });
            input.push_back({ PawnIsolated.m, 0, 100, "PawnIsolated.m" });
            input.push_back({ PawnIsolated.e, 0, 100, "PawnIsolated.e" });
            input.push_back({ PawnBackward.m, 0, 100, "PawnBackward.m" });
            input.push_back({ PawnBackward.e, 0, 100, "PawnBackward.e" });
            input.push_back({ PawnIsolatedOpen.m, 0, 100, "PawnIsolatedOpen.m" });
            input.push_back({ PawnIsolatedOpen.e, 0, 100, "PawnIsolatedOpen.e" });
            input.push_back({ PawnBackwardOpen.m, 0, 100, "PawnBackwardOpen.m" });
            input.push_back({ PawnBackwardOpen.e, 0, 100, "PawnBackwardOpen.e" });
        }
        if (TuneAll || TunePassedPawns) {
            input.push_back({ PasserBonusMin.m, 0, 200, "PasserBonusMin.m" });
            input.push_back({ PasserBonusMin.e, 0, 200, "PasserBonusMin.e" });
            input.push_back({ PasserBonusMax.m, 0, 200, "PasserBonusMax.m" });
            input.push_back({ PasserBonusMax.e, 0, 200, "PasserBonusMax.e" });
            input.push_back({ PasserDistOwn.m, 0, 200, "PasserDistOwn.m" });
            input.push_back({ PasserDistOwn.e, 0, 200, "PasserDistOwn.e" });
            input.push_back({ PasserDistEnemy.m, 0, 200, "PasserDistEnemy.m" });
            input.push_back({ PasserDistEnemy.e, 0, 200, "PasserDistEnemy.e" });
            input.push_back({ PasserNotBlocked.m, 0, 200, "PasserNotBlocked.m" });
            input.push_back({ PasserNotBlocked.e, 0, 200, "PasserNotBlocked.e" });
            input.push_back({ PasserSafePush.m, 0, 200, "PasserSafePush.m" });
            input.push_back({ PasserSafePush.e, 0, 200, "PasserSafePush.e" });
            input.push_back({ PasserSafeProm.m, 0, 200, "PasserSafeProm.m" });
            input.push_back({ PasserSafeProm.e, 0, 200, "PasserSafeProm.e" });
        }
        if (TuneAll || TuneActivity) {
            input.push_back({ KnightMob.m, 0, 100, "KnightMob.m" });
            input.push_back({ KnightMob.e, 0, 100, "KnightMob.e" });
            input.push_back({ BishopMob.m, 0, 100, "BishopMob.m" });
            input.push_back({ BishopMob.e, 0, 100, "BishopMob.e" });
            input.push_back({ RookMob.m, 0, 100, "RookMob.m" });
            input.push_back({ RookMob.e, 0, 100, "RookMob.e" });
            input.push_back({ QueenMob.m, 0, 100, "QueenMob.m" });
            input.push_back({ QueenMob.e, 0, 100, "QueenMob.e" });
            input.push_back({ RookOn7th.m, 0, 100, "RookOn7th.m" });
            input.push_back({ RookOn7th.e, 0, 100, "RookOn7th.e" });
            input.push_back({ RookOnSemiOpenFile.m, 0, 100, "RookOnSemiOpenFile.m" });
            input.push_back({ RookOnSemiOpenFile.e, 0, 100, "RookOnSemiOpenFile.e" });
            input.push_back({ RookOnOpenFile.m, 0, 100, "RookOnOpenFile.m" });
            input.push_back({ RookOnOpenFile.e, 0, 100, "RookOnOpenFile.e" });
            input.push_back({ OutpostBonus.m, 0, 100, "OutpostBonus.m" });
            input.push_back({ OutpostBonus.e, 0, 100, "OutpostBonus.e" });
            input.push_back({ BishopPawns.m, 0, 100, "BishopPawns.m" });
            input.push_back({ BishopPawns.e, 0, 100, "BishopPawns.e" });
            input.push_back({ Tempo, 0, 100, "Tempo" });
        }
        if (TuneAll || TuneThreats) {
            input.push_back({ PawnPush.m, 0, 100, "PawnPush.m" });
            input.push_back({ PawnPush.e, 0, 100, "PawnPush.e" });
            input.push_back({ WeakPawns.m, 0, 100, "WeakPawns.m" });
            input.push_back({ WeakPawns.e, 0, 100, "WeakPawns.e" });
            input.push_back({ PawnsxMinors.m, 0, 100, "PawnsxMinors.m" });
            input.push_back({ PawnsxMinors.e, 0, 100, "PawnsxMinors.e" });
            input.push_back({ MinorsxMinors.m, 0, 100, "MinorsxMinors.m" });
            input.push_back({ MinorsxMinors.e, 0, 100, "MinorsxMinors.e" });
            input.push_back({ MajorsxWeakMinors.m, 0, 100, "MajorsxWeakMinors.m" });
            input.push_back({ MajorsxWeakMinors.e, 0, 100, "MajorsxWeakMinors.e" });
            input.push_back({ PawnsMinorsxMajors.m, 0, 100, "PawnsMinorsxMajors.m" });
            input.push_back({ PawnsMinorsxMajors.e, 0, 100, "PawnsMinorsxMajors.e" });
            input.push_back({ AllxQueens.m, 0, 100, "AllxQueens.m" });
            input.push_back({ AllxQueens.e, 0, 100, "AllxQueens.e" });
            input.push_back({ KingxMinors.m, 0, 100, "KingxMinors.m" });
            input.push_back({ KingxMinors.e, 0, 100, "KingxMinors.e" });
            input.push_back({ KingxRooks.m, 0, 100, "KingxRooks.m" });
            input.push_back({ KingxRooks.e, 0, 100, "KingxRooks.e" });
        }
        if (TuneAll || TuneKingSafety) {
            input.push_back({ KingShelter1, 0, 100, "KingShelter1" });
            input.push_back({ KingShelter2, 0, 100, "KingShelter2" });
            input.push_back({ KingStorm1, 0, 100, "KingStorm1" });
            input.push_back({ KingStorm2, 0, 100, "KingStorm2" });
            input.push_back({ KnightAtk, 0, 200, "KnightAtk" });
            input.push_back({ BishopAtk, 0, 200, "BishopAtk" });
            input.push_back({ RookAtk, 0, 200, "RookAtk" });
            input.push_back({ QueenAtk, 0, 200, "QueenAtk" });
            input.push_back({ AttackValue, 0, 200, "AttackValue" });
            input.push_back({ WeakSquares, 0, 200, "WeakSquares" });
            input.push_back({ NoEnemyQueens, 0, 200, "NoEnemyQueens" });
            input.push_back({ EnemyPawns, 0, 200, "EnemyPawns" });
            input.push_back({ QueenSafeCheckValue, 0, 200, "QueenSafeCheckValue" });
            input.push_back({ RookSafeCheckValue, 0, 200, "RookSafeCheckValue" });
            input.push_back({ BishopSafeCheckValue, 0, 200, "BishopSafeCheckValue" });
            input.push_back({ KnightSafeCheckValue, 0, 200, "KnightSafeCheckValue" });
        }
        if (TuneAll || TunePhase) {
            input.push_back({ KnightPhase, 0, 100, "KnightPhase" });
            input.push_back({ BishopPhase, 0, 100, "BishopPhase" });
            input.push_back({ RookPhase, 0, 100, "RookPhase" });
            input.push_back({ QueenPhase, 0, 100, "QueenPhase" });
        }
        FindBestK(data);
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