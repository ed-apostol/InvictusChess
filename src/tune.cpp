/**************************************************/
/*  Invictus 2019                                 */
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

namespace Tuner {
    using Ldouble = long double;
    Ldouble K = 1.26088;
    const int num_threads = 7;

    struct PositionResults {
        position_t* p;
        Ldouble result;
    };

    struct TunerParam {
    public:
        TunerParam(int16_t& v, const int16_t& l, const int16_t& u, const std::string& n) : val(&v), lower(l), upper(u), name(n) {}
        int16_t *val, lower, upper;
        std::string name;
        operator int16_t&() { return *val; }
        operator const int16_t&()const { return *val; }
        void operator=(const int16_t& value) { *val = std::min(std::max(lower, value), upper); }
    };

    Ldouble Sigmoid(Ldouble score) {
        return 1.0 / (1.0 + std::pow(10.0, -K * score / 400.0));
    }

    void RunOnThread(std::vector<PositionResults>& data, std::vector<Ldouble>& errors, size_t batchsize, int t, int numthreads) {
        for (size_t k = t; k < batchsize; k += numthreads) {
            eval_t eval;
            Ldouble scr = eval.score(*data[k].p) * (data[k].p->side == WHITE ? +1.0 : -1.0);
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
        Ldouble min = -10, max = 10, delta = 1, best = 1;
        Ldouble error = 100;
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
            const int16_t oldvalue = par;
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
        for (auto param : params) {
            out << param.name << " " << param << "\n";
        }
        out << "\nEpoch = " << epoch << " start error = " << start << " new error = " << end;
        out << " improved by = " << (start - end) * 10e6 << "\n";
        PrintOutput() << out.str();
        outfile << out.str();
        outfile.flush();
    }

    void AdamSGD(std::vector<TunerParam>& params, std::vector<PositionResults>& data, const size_t batchsize) {
        std::ofstream str("tuned.txt");
        std::vector<Ldouble> gradients(params.size(), 0);
        std::vector<Ldouble> M(params.size(), 0);
        std::vector<Ldouble> V(params.size(), 0);
        const Ldouble Alpha = 0.015;
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

            PrintOutput() << "Applying gradients...";
            int k = 0;
            for (auto par : params) {
                Ldouble grad = gradients[k];
                M[k] = Beta1 * M[k] + (1.0 - Beta1) * grad;
                V[k] = Beta2 * V[k] + (1.0 - Beta2) * grad * grad;
                Ldouble M_t = M[k] / (1.0 - pow(Beta1, epoch));
                Ldouble V_t = V[k] / (1.0 - pow(Beta2, epoch));
                Ldouble delta = Alpha * M_t / (sqrt(V_t) + Epsilon);
                int16_t oldpar = par;
                par = par - delta;
                if (par != oldpar)
                    PrintOutput() << par.name << " " << oldpar << " --> " << par;
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
        const std::vector<int16_t> delta = { 1, -1 };

        Ldouble currError = Error(data, data.size());
        PrintOutput() << "Base Error: " << currError;

        int epoch = 1;
        while (true) {
            ++epoch;
            PrintOutput() << "\n\nEpoch = " << epoch << " start error = " << currError;
            Ldouble baseError = currError;
            Randomize(data);
            for (auto par : params) {
                bool improved = false;
                int16_t orig = par;
                for (auto d : delta) {
                    if (improved) break;
                    par = orig + d;
                    if (orig == par) continue; // reached upper or lower
                    Ldouble error = Error(data, data.size());
                    if (error < currError) {
                        currError = error;
                        improved = true;
                        PrintOutput() << par.name << " " << orig << " --> " << par;
                    }
                }
                if (!improved) par = orig;
            }
            PrintParams(str, params, baseError, currError, epoch);
            if (baseError == currError) break;
        }
    }

    Ldouble getResult(const std::string& s) {
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
        PrintOutput() << "Data size : " << data.size();

        size_t batchSize = data.size() / 10;
        //size_t batchSize = data.size() / 100;
        //size_t batchSize = 16384;
        //size_t batchSize = 1;

        using namespace EvalParam;

        std::vector<TunerParam> input;
        //input.push_back({MaterialValues[PAWN].m, 0, 2000, "PawnVal Mid"}); // peg to 100
        input.push_back({ MaterialValues[PAWN].e, 0, 2000, "PawnVal End" });
        input.push_back({ MaterialValues[KNIGHT].m, 0, 2000, "KnightVal Mid" });
        input.push_back({ MaterialValues[KNIGHT].e, 0, 2000, "KnightVal End" });
        input.push_back({ MaterialValues[BISHOP].m, 0, 2000, "BishopVal Mid" });
        input.push_back({ MaterialValues[BISHOP].e, 0, 2000, "BishopVal End" });
        input.push_back({ MaterialValues[ROOK].m, 0, 2000, "RookVal Mid" });
        input.push_back({ MaterialValues[ROOK].e, 0, 2000, "RookVal End" });
        input.push_back({ MaterialValues[QUEEN].m, 0, 2000, "QueenVal Mid" });
        input.push_back({ MaterialValues[QUEEN].e, 0, 2000, "QueenVal End" });

        input.push_back({ PawnConnected.m, 0, 100, "PawnConnected Mid" });
        input.push_back({ PawnConnected.e, 0, 100, "PawnConnected End" });
        input.push_back({ PawnDoubled.m, 0, 100, "PawnDoubled Mid" });
        input.push_back({ PawnDoubled.e, 0, 100, "PawnDoubled End" });
        input.push_back({ PawnIsolated.m, 0, 100, "PawnIsolated Mid" });
        input.push_back({ PawnIsolated.e, 0, 100, "PawnIsolated End" });
        input.push_back({ PawnBackward.m, 0, 100, "PawnBackward Mid" });
        input.push_back({ PawnBackward.e, 0, 100, "PawnBackward End" });

        input.push_back({ PasserBonusMin.m, 0, 100, "PasserBonusMin Mid" });
        input.push_back({ PasserBonusMin.e, 0, 100, "PasserBonusMin End" });
        input.push_back({ PasserBonusMax.m, 0, 200, "PasserBonusMax Mid" });
        input.push_back({ PasserBonusMax.e, 0, 200, "PasserBonusMax End" });
        input.push_back({ PasserDistOwn.m, 0, 100, "PasserDistOwn Mid" });
        input.push_back({ PasserDistOwn.e, 0, 100, "PasserDistOwn End" });
        input.push_back({ PasserDistEnemy.m, 0, 100, "PasserDistEnemy Mid" });
        input.push_back({ PasserDistEnemy.e, 0, 100, "PasserDistEnemy End" });
        input.push_back({ PasserNotBlocked.m, 0, 100, "PasserNotBlocked Mid" });
        input.push_back({ PasserNotBlocked.e, 0, 100, "PasserNotBlocked End" });
        input.push_back({ PasserSafePush.m, 0, 100, "PasserSafePush Mid" });
        input.push_back({ PasserSafePush.e, 0, 100, "PasserSafePush End" });

        input.push_back({ KnightMob.m, 0, 100, "KnightMob Mid" });
        input.push_back({ KnightMob.e, 0, 100, "KnightMob End" });
        input.push_back({ BishopMob.m, 0, 100, "BishopMob Mid" });
        input.push_back({ BishopMob.e, 0, 100, "BishopMob End" });
        input.push_back({ RookMob.m, 0, 100, "RookMob Mid" });
        input.push_back({ RookMob.e, 0, 100, "RookMob End" });
        input.push_back({ QueenMob.m, 0, 100, "QueenMob Mid" });
        input.push_back({ QueenMob.e, 0, 100, "QueenMob End" });

        input.push_back({ NumKZoneAttacks.m, 0, 100, "NumKZoneAttacks Mid" });
        input.push_back({ NumKZoneAttacks.e, 0, 100, "NumKZoneAttacks End" });
        input.push_back({ ShelterBonus.m, 0, 100, "ShelterBonus Mid" });
        input.push_back({ ShelterBonus.e, 0, 100, "ShelterBonus End" });

        input.push_back({ PawnsxMinors.m, 0, 100, "PawnsxMinors Mid" });
        input.push_back({ PawnsxMinors.e, 0, 100, "PawnsxMinors End" });
        input.push_back({ MinorsxMinors.m, 0, 100, "MinorsxMinors Mid" });
        input.push_back({ MinorsxMinors.e, 0, 100, "MinorsxMinors End" });
        input.push_back({ MajorsxWeakMinors.m, 0, 100, "MajorsxWeakMinors Mid" });
        input.push_back({ MajorsxWeakMinors.e, 0, 100, "MajorsxWeakMinors End" });
        input.push_back({ PawnsMinorsxMajors.m, 0, 100, "PawnsMinorsxMajors Mid" });
        input.push_back({ PawnsMinorsxMajors.e, 0, 100, "PawnsMinorsxMajors End" });
        input.push_back({ AllxQueens.m, 0, 100, "AllxQueens Mid" });
        input.push_back({ AllxQueens.e, 0, 100, "AllxQueens End" });

        input.push_back({ KnightAtk, 0, 100, "KnightAtk" });
        input.push_back({ BishopAtk, 0, 100, "BishopAtk" });
        input.push_back({ RookAtk, 0, 100, "RookAtk" });
        input.push_back({ QueenAtk, 0, 100, "QueenAtk" });

        //FindBestK(data);
        PrintOutput() << "Best K " << K;

        PrintOutput() << "\nInitial values:";
        for (auto par : input) PrintOutput() << par.name << " " << par;

        AdamSGD(input, data, batchSize);
        //LocalSearch(input, data);

        PrintOutput() << "\nTuned values:";
        for (auto par : input) PrintOutput() << par.name << " " << par;

        for (auto &d : data) delete d.p;

        if (file) file.close();
    }
}