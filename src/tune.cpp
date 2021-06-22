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
#include <iomanip>

#include "utils.h"
#include "log.h"
#include "eval.h"
#include "params.h"

#define N(v) (#v)

#ifdef TUNE
namespace Tuner {
    using Ldouble = double;
    using namespace EvalParam;

    Ldouble K = 1.0;
    const int NumThreads = 31;

    const int TuneAll = 1;
    const int TuneMaterial = 1;
    const int TuneMobility = 0;
    const int Imbalance = 1;
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
        TunerParam(basic_score_t& v, std::string n) : val(&v), name(n) {}
        basic_score_t *val;
        std::string name;
        operator basic_score_t&() { return *val; }
        operator const basic_score_t&()const { return *val; }
        void operator=(const basic_score_t& value) { *val = value; }
    };

    Ldouble Sigmoid(Ldouble score) {
        return 1.0 / (1.0 + std::pow(10.0, -K * score / 400.0));
    }

    Ldouble Error(std::vector<PositionResults>& data, size_t batchsize) {
        static uint64_t counter = 0;
        static uint64_t lasttime = Utils::getTime();
        std::vector<Ldouble> errors(batchsize, 0);
        std::thread threads[NumThreads];

        auto threaded = [&](size_t start, size_t end) {
            eval_t eval;
            for (size_t k = start; k < end; k += NumThreads) {
                basic_score_t scr = eval.score(*data[k].p) * (data[k].p->side == WHITE ? +1.0 : -1.0);
                errors[k] = std::pow(data[k].result - Sigmoid(scr), 2);
            }
        };
        for (int t = 0; t < NumThreads; ++t) {
            threads[t] = std::thread(threaded, t, batchsize);
        }
        for (int t = 0; t < NumThreads; ++t) {
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
        counter += batchsize;
        uint64_t timenow = Utils::getTime();
        if (timenow - lasttime > 10000) {
            PrintOutput() << "\n" << counter / (timenow - lasttime) << " knps using " << NumThreads << " threads\n";
            counter = 0;
            lasttime = timenow;
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

    void PrintWeights(std::ostringstream& out) {
        if (TuneAll || TuneMaterial) {
            out << "score_t MaterialValues[7] = { "; for (auto a : MaterialValues) out << a.to_str() + ", "; out << "};\n\n";
        }
        if (TuneAll || Imbalance) {
            out << "score_t ImbalanceInternal[6][6] = {\n";
            for (int pt1 = 0; pt1 <= QUEEN; ++pt1) {
                out << "{ ";
                for (int pt2 = 0; pt2 <= pt1; ++pt2) {
                    out << ImbalanceInternal[pt1][pt2].to_str() + ", ";
                }
                out << "},\n";
            }
            out << "};\n\n";
            out << "score_t ImbalanceExternal[6][6] = {\n";
            for (int pt1 = 0; pt1 <= QUEEN; ++pt1) {
                out << "{ ";
                for (int pt2 = 0; pt2 < pt1; ++pt2) {
                    out << ImbalanceExternal[pt1][pt2].to_str() + ", ";
                }
                out << "},\n";
            }
            out << "};\n\n";
        }
        if (TuneAll || TuneMobility) {
            out << "score_t KnightMob[9] = { "; for (auto a : KnightMob) out << a.to_str() + ", "; out << "};\n";
            out << "score_t BishopMob[14] = { "; for (auto a : BishopMob) out << a.to_str() + ", "; out << "};\n";
            out << "score_t RookMob[15] = { "; for (auto a : RookMob) out << a.to_str() + ", "; out << "};\n";
            out << "score_t QueenMob[28] = { "; for (auto a : QueenMob) out << a.to_str() + ", "; out << "};\n\n";
        }
        if (TuneAll || TunePassedPawns) {
            out << "score_t PasserDistOwn[8] = { "; for (auto a : PasserDistOwn) out << a.to_str() + ", "; out << "};\n";
            out << "score_t PasserDistEnemy[8] = { "; for (auto a : PasserDistEnemy) out << a.to_str() + ", "; out << "};\n";
            out << "score_t PasserBonus[8] = { "; for (auto a : PasserBonus) out << a.to_str() + ", "; out << "};\n";
            out << "score_t PasserNotBlocked[8] = { "; for (auto a : PasserNotBlocked) out << a.to_str() + ", "; out << "};\n";
            out << "score_t PasserSafePush[8] = { "; for (auto a : PasserSafePush) out << a.to_str() + ", "; out << "};\n";
            out << "score_t PasserSafeProm[8] = { "; for (auto a : PasserSafeProm) out << a.to_str() + ", "; out << "};\n\n";
        }
        if (TuneAll || TunePawnStructure) {
            out << "score_t PawnConnected = " + PawnConnected.to_str() + ";\n";
            out << "score_t PawnDoubled = " + PawnDoubled.to_str() + ";\n";
            out << "score_t PawnIsolated = " + PawnIsolated.to_str() + ";\n";
            out << "score_t PawnBackward = " + PawnBackward.to_str() + ";\n";
            out << "score_t PawnIsolatedOpen = " + PawnIsolatedOpen.to_str() + ";\n";
            out << "score_t PawnBackwardOpen = " + PawnBackwardOpen.to_str() + ";\n\n";
        }
        if (TuneAll || TuneActivity) {
            out << "score_t RookOn7th = " + RookOn7th.to_str() + ";\n";
            out << "score_t RookOnSemiOpenFile = " + RookOnSemiOpenFile.to_str() + ";\n";
            out << "score_t RookOnOpenFile = " + RookOnOpenFile.to_str() + ";\n";
            out << "score_t KnightOutpost = " + KnightOutpost.to_str() + ";\n";
            out << "score_t KnightXOutpost = " + KnightXOutpost.to_str() + ";\n";
            out << "score_t BishopOutpost = " + BishopOutpost.to_str() + ";\n";
            out << "score_t BishopPawns = " + BishopPawns.to_str() + ";\n";
            out << "score_t BishopCenterControl = " + BishopCenterControl.to_str() + ";\n";
            out << "score_t PieceSpace = " + PieceSpace.to_str() + ";\n";
            out << "score_t EmptySpace = " + EmptySpace.to_str() + ";\n\n";
        }
        if (TuneAll || TuneThreats) {
            out << "score_t PawnPush = " + PawnPush.to_str() + ";\n";
            out << "score_t WeakPawns = " + WeakPawns.to_str() + ";\n";
            out << "score_t PawnsxMinors = " + PawnsxMinors.to_str() + ";\n";
            out << "score_t MinorsxMinors = " + MinorsxMinors.to_str() + ";\n";
            out << "score_t MajorsxWeakMinors = " + MajorsxWeakMinors.to_str() + ";\n";
            out << "score_t PawnsMinorsxMajors = " + PawnsMinorsxMajors.to_str() + ";\n";
            out << "score_t AllxQueens = " + AllxQueens.to_str() + ";\n";
            out << "score_t KingxMinors = " + KingxMinors.to_str() + ";\n";
            out << "score_t KingxRooks = " + KingxRooks.to_str() + ";\n\n";
        }
        if (TuneAll || TuneKingSafety) {
            out << "basic_score_t KnightAtk = " << (int)std::round(KnightAtk) << ";\n";
            out << "basic_score_t BishopAtk = " << (int)std::round(BishopAtk) << ";\n";
            out << "basic_score_t RookAtk = " << (int)std::round(RookAtk) << ";\n";
            out << "basic_score_t QueenAtk = " << (int)std::round(QueenAtk) << ";\n";
            out << "basic_score_t KingZoneAttacks = " << (int)std::round(KingZoneAttacks) << ";\n";
            out << "basic_score_t WeakSquares = " << (int)std::round(WeakSquares) << ";\n";
            out << "basic_score_t EnemyPawns = " << (int)std::round(EnemyPawns) << ";\n";
            out << "basic_score_t QueenSafeCheckValue = " << (int)std::round(QueenSafeCheckValue) << ";\n";
            out << "basic_score_t RookSafeCheckValue = " << (int)std::round(RookSafeCheckValue) << ";\n";
            out << "basic_score_t BishopSafeCheckValue = " << (int)std::round(BishopSafeCheckValue) << ";\n";
            out << "basic_score_t KnightSafeCheckValue = " << (int)std::round(KnightSafeCheckValue) << ";\n";
            out << "basic_score_t KingShelter1 = " << (int)std::round(KingShelter1) << ";\n";
            out << "basic_score_t KingShelter2 = " << (int)std::round(KingShelter2) << ";\n";
            out << "basic_score_t KingShelterF1 = " << (int)std::round(KingShelterF1) << ";\n";
            out << "basic_score_t KingShelterF2 = " << (int)std::round(KingShelterF2) << ";\n";
            out << "basic_score_t KingStorm1 = " << (int)std::round(KingStorm1) << ";\n";
            out << "basic_score_t KingStorm2 = " << (int)std::round(KingStorm2) << ";\n\n";
        }
        if (TuneAll || TunePhase) {
            out << "basic_score_t KnightPhase = " << (int)std::round(KnightPhase) << ";\n";
            out << "basic_score_t BishopPhase = " << (int)std::round(BishopPhase) << ";\n";
            out << "basic_score_t RookPhase = " << (int)std::round(RookPhase) << ";\n";
            out << "basic_score_t QueenPhase = " << (int)std::round(QueenPhase) << ";\n";
            out << "basic_score_t Tempo = " << (int)std::round(Tempo) << ";\n\n";
        }
    }

    void PrintParams(std::ofstream& outfile, std::vector<TunerParam>& params, Ldouble start, Ldouble end, int epoch) {
        std::ostringstream  out;
        out << "\n";
        PrintWeights(out);
        out << "Epoch = " << epoch << " start error = " << start << " new error = " << end;
        out << " improved by = " << (start - end) * 10e6 << "\n\n";
        PrintOutput() << out.str();
        outfile << out.str();
        outfile.flush();
    }

    void CalculateGradients(std::vector<Ldouble>& gradients, std::vector<TunerParam>& params,
        std::vector<PositionResults>& data, size_t batchsize, Ldouble last) {
        int k = 0;
        for (auto& par : params) {
            const basic_score_t oldvalue = par;
            par = oldvalue + 2.0;
            Ldouble ep1 = Error(data, batchsize);
            gradients[k] = (ep1 - last) / 2.0;
            par = oldvalue;
            ++k;
        }
    }

    void AdamDemonOptimizer(std::vector<TunerParam>& params, std::vector<PositionResults>& data, const size_t batchsize) {
        std::ofstream str("tuned.txt");
        std::vector<Ldouble> G(params.size(), 0.0);
        std::vector<Ldouble> M(params.size(), 0.0);
        std::vector<Ldouble> V(params.size(), 0.0);
        const Ldouble Alpha_ = 0.3;
        const Ldouble Beta1_ = 0.9;
        const Ldouble Beta2 = 0.999;
        const Ldouble Epsilon = 1.0e-8;
        const int MaxEpoch = 50000;

        FindBestK(data);
        PrintOutput() << "Best K " << K;

        for (int epoch = 1; epoch <= MaxEpoch; ++epoch) {
            PrintOutput() << "\n\nEpoch = " << epoch;
            Randomize(data);

            Ldouble baseError = Error(data, batchsize);
            PrintOutput() << "Base Error: " << baseError;

            PrintOutput() << "Computing gradients...";
            CalculateGradients(G, params, data, batchsize, baseError);

            Ldouble decay = 1.0 - Ldouble(epoch) / Ldouble(MaxEpoch);
            Ldouble Beta1 = Beta1_ * decay / ((1.0 - Beta1_) + Beta1_ * decay);
            Ldouble Alpha = Alpha_ * decay;

            std::thread threads[NumThreads];
            auto threaded = [&](size_t start, size_t end) {
                for (size_t k = start; k < end; k += NumThreads) {
                    Ldouble g = G[k];
                    M[k] = Beta1 * M[k] + (1.0 - Beta1) * g;
                    V[k] = Beta2 * V[k] + (1.0 - Beta2) * g * g;
                    Ldouble mhat = M[k] / (1.0 - pow(Beta1, epoch));
                    Ldouble vhat = V[k] / (1.0 - pow(Beta2, epoch));
                    Ldouble delta = Alpha * mhat / (sqrt(vhat) + Epsilon);
                    //const basic_score_t oldpar = params[k];
                    params[k] = params[k] - delta;
                    //if (par != oldpar) PrintOutput() << par.name << "\t\t" << oldpar << " --> " << par;
                }
            };

            for (int t = 0; t < NumThreads; ++t) {
                threads[t] = std::thread(threaded, t, params.size());
            }
            for (int t = 0; t < NumThreads; ++t) {
                threads[t].join();
            }

            Ldouble currError = Error(data, batchsize);
            PrintOutput() << "Base error = " << baseError << " Current error = " << currError << " diff = " << (baseError - currError) * 10e6;
            if (epoch % 10 == 0) PrintParams(str, params, baseError, currError, epoch);
        }
    }

    void LocalSearch(std::vector<TunerParam>& params, std::vector<PositionResults>& data) {
        std::ofstream str("tuned.txt");
        for (int i = 0; i < 2; ++i) {
            std::vector<Ldouble> direction(params.size(), 1.0);
            std::vector<Ldouble> stepsize(params.size(), 1.0);
            std::vector<int> stability(params.size(), 0);

            FindBestK(data);
            PrintOutput() << "Best K " << K;

            Ldouble currError = Error(data, data.size());
            PrintOutput() << "Base Error: " << currError;

            int epoch = 0;
            while (true) {
                ++epoch;
                PrintOutput() << "\n\nEpoch = " << epoch << " start error = " << currError;
                Ldouble baseError = currError;
                int k = -1;
                for (auto& par : params) {
                    ++k;
                    if (stability[k] >= 10) continue; // didn't change value in 5 epochs
                    for (int t = 0; t < 2; ++t) {
                        basic_score_t oldpar = par;
                        par = oldpar + (direction[k] * stepsize[k]);
                        Ldouble error = Error(data, data.size());
                        if (error < currError) {
                            currError = error;
                            stepsize[k] *= 2.0;
                            stability[k] = 0;
                            PrintOutput() << par.name << " " << oldpar << " --> " << par;
                            continue;
                        }
                        par = oldpar;
                        direction[k] *= -1.0;
                        stepsize[k] = 1.0;
                        ++stability[k];
                    }
                }
                PrintParams(str, params, baseError, currError, epoch);
                if (baseError == currError) break;
            }
        }
    }

    void DeclareParams(std::vector<TunerParam>& input) {
        if (TuneAll || TuneMaterial) {
            //input.push_back({MaterialValues[PAWN].m, N(PawnVal.m"}); // peg to 100
            input.push_back({ MaterialValues[PAWN].e, N(PawnVal.e) });
            input.push_back({ MaterialValues[KNIGHT].m, N(KnightVal.m) });
            input.push_back({ MaterialValues[KNIGHT].e, N(KnightVal.e) });
            input.push_back({ MaterialValues[BISHOP].m, N(BishopVal.m) });
            input.push_back({ MaterialValues[BISHOP].e, N(BishopVal.e) });
            input.push_back({ MaterialValues[ROOK].m, N(RookVal.m) });
            input.push_back({ MaterialValues[ROOK].e, N(RookVal.e) });
            input.push_back({ MaterialValues[QUEEN].m, N(QueenVal.m) });
            input.push_back({ MaterialValues[QUEEN].e, N(QueenVal.e) });
        }
        if (TuneAll || Imbalance) {
            for (int pt1 = 0; pt1 <= QUEEN; ++pt1) {
                for (int pt2 = 0; pt2 < pt1; ++pt2) {
                    input.push_back({ ImbalanceInternal[pt1][pt2].m, "Ours[" + std::to_string(pt1) + "][" + std::to_string(pt2) + "].m" });
                    input.push_back({ ImbalanceInternal[pt1][pt2].e, "Ours[" + std::to_string(pt1) + "][" + std::to_string(pt2) + "].e" });
                    input.push_back({ ImbalanceExternal[pt1][pt2].m, "Theirs[" + std::to_string(pt1) + "][" + std::to_string(pt2) + "].m" });
                    input.push_back({ ImbalanceExternal[pt1][pt2].e, "Theirs[" + std::to_string(pt1) + "][" + std::to_string(pt2) + "].e" });
                }
                input.push_back({ ImbalanceInternal[pt1][pt1].m, "Ours[" + std::to_string(pt1) + "][" + std::to_string(pt1) + "].m" });
                input.push_back({ ImbalanceInternal[pt1][pt1].e, "Ours[" + std::to_string(pt1) + "][" + std::to_string(pt1) + "].e" });
            }
        }

        if (TuneAll || TuneMobility) {
            for (int cnt = 0; cnt < 9; ++cnt) {
                input.push_back({ KnightMob[cnt].m, "KnightMob[" + std::to_string(cnt) + "].m" });
                input.push_back({ KnightMob[cnt].e, "KnightMob[" + std::to_string(cnt) + "].e" });
            }
            for (int cnt = 0; cnt < 14; ++cnt) {
                input.push_back({ BishopMob[cnt].m, "BishopMob[" + std::to_string(cnt) + "].m" });
                input.push_back({ BishopMob[cnt].e, "BishopMob[" + std::to_string(cnt) + "].e" });
            }
            for (int cnt = 0; cnt < 15; ++cnt) {
                input.push_back({ RookMob[cnt].m, "RookMob[" + std::to_string(cnt) + "].m" });
                input.push_back({ RookMob[cnt].e, "RookMob[" + std::to_string(cnt) + "].e" });
            }
            for (int cnt = 0; cnt < 28; ++cnt) {
                input.push_back({ QueenMob[cnt].m, "QueenMob[" + std::to_string(cnt) + "].m" });
                input.push_back({ QueenMob[cnt].e, "QueenMob[" + std::to_string(cnt) + "].e" });
            }
        }
        if (TuneAll || TunePassedPawns) {
            for (int rank = 1; rank <= 6; ++rank) {
                input.push_back({ PasserDistOwn[rank].m, "PasserDistOwn[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserDistOwn[rank].e, "PasserDistOwn[" + std::to_string(rank) + "].e" });
                input.push_back({ PasserDistEnemy[rank].m, "PasserDistEnemy[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserDistEnemy[rank].e, "PasserDistEnemy[" + std::to_string(rank) + "].e" });
                input.push_back({ PasserBonus[rank].m, "PasserBonus[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserBonus[rank].e, "PasserBonus[" + std::to_string(rank) + "].e" });
                input.push_back({ PasserNotBlocked[rank].m, "PasserNotBlocked[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserNotBlocked[rank].e, "PasserNotBlocked[" + std::to_string(rank) + "].e" });
                input.push_back({ PasserSafePush[rank].m, "PasserSafePush[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserSafePush[rank].e, "PasserSafePush[" + std::to_string(rank) + "].e" });
                input.push_back({ PasserSafeProm[rank].m, "PasserSafeProm[" + std::to_string(rank) + "].m" });
                input.push_back({ PasserSafeProm[rank].e, "PasserSafeProm[" + std::to_string(rank) + "].e" });
            }
        }
        if (TuneAll || TunePawnStructure) {
            input.push_back({ PawnConnected.m, N(PawnConnected.m) });
            input.push_back({ PawnConnected.e, N(PawnConnected.e) });
            input.push_back({ PawnDoubled.m, N(PawnDoubled.m) });
            input.push_back({ PawnDoubled.e, N(PawnDoubled.e) });
            input.push_back({ PawnIsolated.m, N(PawnIsolated.m) });
            input.push_back({ PawnIsolated.e, N(PawnIsolated.e) });
            input.push_back({ PawnBackward.m, N(PawnBackward.m) });
            input.push_back({ PawnBackward.e, N(PawnBackward.e) });
            input.push_back({ PawnIsolatedOpen.m, N(PawnIsolatedOpen.m) });
            input.push_back({ PawnIsolatedOpen.e, N(PawnIsolatedOpen.e) });
            input.push_back({ PawnBackwardOpen.m, N(PawnBackwardOpen.m) });
            input.push_back({ PawnBackwardOpen.e, N(PawnBackwardOpen.e) });
        }
        if (TuneAll || TuneActivity) {
            input.push_back({ RookOn7th.m, N(RookOn7th.m) });
            input.push_back({ RookOn7th.e, N(RookOn7th.e) });
            input.push_back({ RookOnSemiOpenFile.m, N(RookOnSemiOpenFile.m) });
            input.push_back({ RookOnSemiOpenFile.e, N(RookOnSemiOpenFile.e) });
            input.push_back({ RookOnOpenFile.m, N(RookOnOpenFile.m) });
            input.push_back({ RookOnOpenFile.e, N(RookOnOpenFile.e) });
            input.push_back({ KnightOutpost.m, N(KnightOutpost.m) });
            input.push_back({ KnightOutpost.e, N(KnightOutpost.e) });
            input.push_back({ KnightXOutpost.m, N(KnightXOutpost.m) });
            input.push_back({ KnightXOutpost.e, N(KnightXOutpost.e) });
            input.push_back({ BishopOutpost.m, N(BishopOutpost.m) });
            input.push_back({ BishopOutpost.e, N(BishopOutpost.e) });
            input.push_back({ BishopPawns.m, N(BishopPawns.m) });
            input.push_back({ BishopPawns.e, N(BishopPawns.e) });
            input.push_back({ BishopCenterControl.m, N(BishopCenterControl.m) });
            input.push_back({ BishopCenterControl.e, N(BishopCenterControl.e) });
            input.push_back({ PieceSpace.m, N(PieceSpace.m) });
            input.push_back({ PieceSpace.e, N(PieceSpace.e) });
            input.push_back({ EmptySpace.m, N(EmptySpace.m) });
            input.push_back({ EmptySpace.e, N(EmptySpace.e) });
        }
        if (TuneAll || TuneThreats) {
            input.push_back({ PawnPush.m, N(PawnPush.m) });
            input.push_back({ PawnPush.e, N(PawnPush.e) });
            input.push_back({ WeakPawns.m, N(WeakPawns.m) });
            input.push_back({ WeakPawns.e, N(WeakPawns.e) });
            input.push_back({ PawnsxMinors.m, N(PawnsxMinors.m) });
            input.push_back({ PawnsxMinors.e, N(PawnsxMinors.e) });
            input.push_back({ MinorsxMinors.m, N(MinorsxMinors.m) });
            input.push_back({ MinorsxMinors.e, N(MinorsxMinors.e) });
            input.push_back({ MajorsxWeakMinors.m, N(MajorsxWeakMinors.m) });
            input.push_back({ MajorsxWeakMinors.e, N(MajorsxWeakMinors.e) });
            input.push_back({ PawnsMinorsxMajors.m, N(PawnsMinorsxMajors.m) });
            input.push_back({ PawnsMinorsxMajors.e, N(PawnsMinorsxMajors.e) });
            input.push_back({ AllxQueens.m, N(AllxQueens.m) });
            input.push_back({ AllxQueens.e, N(AllxQueens.e) });
            input.push_back({ KingxMinors.m, N(KingxMinors.m) });
            input.push_back({ KingxMinors.e, N(KingxMinors.e) });
            input.push_back({ KingxRooks.m, N(KingxRooks.m) });
            input.push_back({ KingxRooks.e, N(KingxRooks.e) });
        }
        if (TuneAll || TuneKingSafety) {
            input.push_back({ KnightAtk, N(KnightAtk) });
            input.push_back({ BishopAtk, N(BishopAtk) });
            input.push_back({ RookAtk, N(RookAtk) });
            input.push_back({ QueenAtk, N(QueenAtk) });
            input.push_back({ KingZoneAttacks, N(KingZoneAttacks) });
            input.push_back({ WeakSquares, N(WeakSquares) });
            input.push_back({ EnemyPawns, N(EnemyPawns) });
            input.push_back({ QueenSafeCheckValue, N(QueenSafeCheckValue) });
            input.push_back({ RookSafeCheckValue, N(RookSafeCheckValue) });
            input.push_back({ BishopSafeCheckValue, N(BishopSafeCheckValue) });
            input.push_back({ KnightSafeCheckValue, N(KnightSafeCheckValue) });
            input.push_back({ KingShelter1, N(KingShelter1) });
            input.push_back({ KingShelter2, N(KingShelter2) });
            input.push_back({ KingShelterF1, N(KingShelterF1) });
            input.push_back({ KingShelterF2, N(KingShelterF2) });
            input.push_back({ KingStorm1, N(KingStorm1) });
            input.push_back({ KingStorm2, N(KingStorm2) });
        }
        if (TuneAll || TunePhase) {
            input.push_back({ KnightPhase, N(KnightPhase) });
            input.push_back({ BishopPhase, N(BishopPhase) });
            input.push_back({ RookPhase, N(RookPhase) });
            input.push_back({ QueenPhase, N(QueenPhase) });
            input.push_back({ Tempo, N(Tempo) });
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
        std::vector<TunerParam> input;

        PrintOutput() << "Tuning with file = " << filename;
        std::ifstream file = std::ifstream(filename);
        if (!file) {
            PrintOutput() << "File not found = " << filename;
            return;
        }
        std::string line;
        while (getline(file, line)) {
            data.push_back({ new position_t(line), getResult(line.substr(line.find("|") + 1)) });
            if (data.size() % 100000 == 0) PrintOutput() << "Processed: " << data.size() << " entries!";
        }
        file.close();
        PrintOutput() << "Data size : " << data.size();

        //size_t batchSize = data.size();
        //size_t batchSize = data.size() / 10;
        //size_t batchSize = data.size() / 100;
        size_t batchSize = 16384;
        //size_t batchSize = 8192;
        //size_t batchSize = 1024;
        //size_t batchSize = 1;

        DeclareParams(input);

        //AdamDemonOptimizer(input, data, batchSize);
        LocalSearch(input, data);

        PrintOutput() << "\nTuning finished: see tuned.txt file!!!\n";

        for (auto &d : data) delete d.p;
    }
}
#endif