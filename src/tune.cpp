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

double K = 1.0;
const int num_threads = 8;

struct PositionResults {
    position_t* p;
    double result;
};

template < typename T >
struct TunerParam {
public:
    TunerParam(T & val, const T& lower, const T& upper, const std::string & name)
        : val(&val), lower(lower), upper(upper), name(name) {}
    T *val, lower, upper;
    std::string name;
    operator T&() { return *val; }
    operator const T&()const { return *val; }
    void operator=(const T& value) { *val = std::min(std::max(lower, value), upper); }
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const TunerParam<T>& p) { const T& t = p; os << t; return os; }

void Sigmoid(std::vector<PositionResults> &data, std::vector<double>& errors, size_t batchsize, int t, int numthreads) {
    for (size_t k = t; k < batchsize; k += numthreads) {
        eval_t eval;
        double scr = eval.score(*data[k].p) * (data[k].p->side == WHITE ? +1.0 : -1.0);
        double s = 1.0 / (1.0 + std::pow(10.0, -K * scr / 400.0));
        errors[t] += std::pow(data[k].result - s, 2);
    }
}

double Error(std::vector<PositionResults> &data, size_t batchsize) {
    std::vector<double> errors(num_threads, 0);
    std::thread threads[num_threads];
    for (int t = 0; t < num_threads; ++t) {
        threads[t] = std::thread(Sigmoid, std::ref(data), std::ref(errors), batchsize, t, num_threads);
    }
    for (int t = 0; t < num_threads; ++t) {
        threads[t].join();
    }
    double sum = 0;
    for (auto e : errors) sum += e;
    return sum / batchsize;
}

void Randomize(std::vector<PositionResults>& data) {
    std::shuffle(data.begin(), data.end(), std::default_random_engine(Utils::getTime()));
}

void FindBestK(std::vector<PositionResults>& data) {
    double min = 0.5, max = 1.5, delta = 0.01, best = 1;
    double error = 100;
    while (min < max) {
        PrintOutput() << "\nFindBestK: " << min;
        K = min;
        double e = Error(data, data.size());
        if (e < error) {
            error = e, best = K;
            PrintOutput() << "  new best K = " << K << ", E = " << error;
        }
        min += delta;
    }
    K = best;
}

std::vector<double> CalculateGradient(std::vector<TunerParam<int16_t>>& params, std::vector<PositionResults> &data, size_t batchsize) {
    std::vector<double> gradients;
    const int16_t dx = 1;
    for (auto par : params) {
        const int16_t oldvalue = par;
        par = oldvalue + dx;
        double Ep1 = Error(data, batchsize);
        par = oldvalue - dx;
        double Em1 = Error(data, batchsize);
        par = oldvalue;
        double grad = (Ep1 - Em1) / (2 * dx);
        gradients.push_back(grad);
    }
    return gradients;
}

std::vector<TunerParam<int16_t>> GradientDescent(const std::vector<TunerParam<int16_t>>& origParam, std::vector<PositionResults> &data, const size_t batchsize) {
    std::ofstream str("tuned.txt");
    std::vector<TunerParam<int16_t>> bestParam = origParam;
    std::vector<TunerParam<int16_t>> currentParam = bestParam;
    std::vector<int16_t> previousUpdate(bestParam.size(), 0);

    double bestError = Error(data, data.size());
    PrintOutput() << "Base Error: " << bestError;

    for (int it = 0; it < 100000; ++it) {
        PrintOutput() << "\nComputing gradient: " << it;
        Randomize(data);
        std::vector<double> gradients = CalculateGradient(currentParam, data, batchsize);

        double maxgradient = -100;
        for (auto grad : gradients) maxgradient = std::max(maxgradient, std::fabs(grad));
        PrintOutput() << "gmax " << maxgradient;
        //if (maxgradient < 0.0000001) break;

        double learningRate = 2.0 / maxgradient; // 2 is the max step, TODO: experiment with other values
        double alpha = 0.1;

        PrintOutput() << "Applying gradients, learning rate = " << learningRate << ", alpha " << alpha;
        for (size_t k = 0; k < currentParam.size(); ++k) {
            const int16_t oldValue = currentParam[k];
            const int16_t currentUpdate = int16_t(((1.0 - alpha) * learningRate * gradients[k]) + (alpha * previousUpdate[k]));
            currentParam[k] = int16_t(oldValue - currentUpdate);
            previousUpdate[k] = currentUpdate;
        }

        double currError = Error(data, data.size());
        PrintOutput() << "Current error: " << currError << " Best error: " << bestError;

        if (currError < bestError) {
            bestError = currError;
            bestParam = currentParam;
            str << it << " " << bestError << std::endl;
            for (auto param : currentParam) {
                str << param.name << " " << param << std::endl;
                PrintOutput() << param.name << " " << param;
            }
            str << std::endl;
        }
        else PrintOutput() << "Didn't improve on best!";
    }
    return bestParam;
}

double getResult(const std::string & s) {
    if (s == "\"1-0\";") return 1.0;
    if (s == "\"0-1\";") return 0.0;
    if (s == "\"1/2-1/2\";") return 0.5;
    PrintOutput() << "Bad position result " << s;
    return 0.5;
}

void Tune(const std::string& filename) {
    std::vector<PositionResults> data;
    PrintOutput() << "Running tuning with file " << filename;

    std::ifstream file = std::ifstream(filename);
    if (!file) {
        PrintOutput() << "File not found: " << filename;
        return;
    }
    std::string line;
    while (getline(file, line)) {
        data.push_back({ new position_t(line), getResult(line.substr(line.find("c9") + 3)) });
    }
    PrintOutput() << "Data size : " << data.size();

    //size_t batchSize = data.size();
    //size_t batchSize = data.size() / 100;
    size_t batchSize = 16384;
    //size_t batchSize = 1;

    std::vector<TunerParam<int16_t>> input;

    //input.push_back(TunerParam<int16_t>(EvalParam::mat_values[PAWN].m, 0, 2000, "PawnVal Mid")); // peg to 100
    input.push_back(TunerParam<int16_t>(EvalParam::mat_values[PAWN].e, 0, 2000, "PawnValEnd"));
    input.push_back(TunerParam<int16_t>(EvalParam::mat_values[KNIGHT].m, 0, 2000, "KnightVal Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::mat_values[KNIGHT].e, 0, 2000, "KnightVal End"));
    input.push_back(TunerParam<int16_t>(EvalParam::mat_values[BISHOP].m, 0, 2000, "BishopVal Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::mat_values[BISHOP].e, 0, 2000, "BishopVal End"));
    input.push_back(TunerParam<int16_t>(EvalParam::mat_values[ROOK].m, 0, 2000, "RookVal Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::mat_values[ROOK].e, 0, 2000, "RookVal End"));
    input.push_back(TunerParam<int16_t>(EvalParam::mat_values[QUEEN].m, 0, 2000, "QueenVal Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::mat_values[QUEEN].e, 0, 2000, "QueenVal End"));

    input.push_back(TunerParam<int16_t>(EvalParam::PawnConnected.m, 0, 100, "PawnConnected Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PawnConnected.e, 0, 100, "PawnConnected End"));
    input.push_back(TunerParam<int16_t>(EvalParam::PawnDoubled.m, 0, 100, "PawnDoubled Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PawnDoubled.e, 0, 100, "PawnDoubled End"));
    input.push_back(TunerParam<int16_t>(EvalParam::PawnIsolated.m, 0, 100, "PawnIsolated Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PawnIsolated.e, 0, 100, "PawnIsolated End"));
    input.push_back(TunerParam<int16_t>(EvalParam::PawnBackward.m, 0, 100, "PawnBackward Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PawnBackward.e, 0, 100, "PawnBackward End"));

    input.push_back(TunerParam<int16_t>(EvalParam::PasserBonusMin.m, 0, 100, "PasserBonusMin Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserBonusMin.e, 0, 100, "PasserBonusMin End"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserBonusMax.m, 0, 200, "PasserBonusMax Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserBonusMax.e, 0, 200, "PasserBonusMax End"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserDistOwn.m, 0, 100, "PasserDistOwn Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserDistOwn.e, 0, 100, "PasserDistOwn End"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserDistEnemy.m, 0, 100, "PasserDistEnemy Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserDistEnemy.e, 0, 100, "PasserDistEnemy End"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserNotBlocked.m, 0, 100, "PasserNotBlocked Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserNotBlocked.e, 0, 100, "PasserNotBlocked End"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserSafePush.m, 0, 100, "PasserSafePush Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PasserSafePush.e, 0, 100, "PasserSafePush End"));

    input.push_back(TunerParam<int16_t>(EvalParam::KnightMob.m, 0, 100, "KnightMob Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::KnightMob.e, 0, 100, "KnightMob End"));
    input.push_back(TunerParam<int16_t>(EvalParam::BishopMob.m, 0, 100, "BishopMob Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::BishopMob.e, 0, 100, "BishopMob End"));
    input.push_back(TunerParam<int16_t>(EvalParam::RookMob.m, 0, 100, "RookMob Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::RookMob.e, 0, 100, "RookMob End"));
    input.push_back(TunerParam<int16_t>(EvalParam::QueenMob.m, 0, 100, "QueenMob Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::QueenMob.e, 0, 100, "QueenMob End"));

    input.push_back(TunerParam<int16_t>(EvalParam::KnightAtk, 0, 100, "KnightAtk"));
    input.push_back(TunerParam<int16_t>(EvalParam::BishopAtk, 0, 100, "BishopAtk"));
    input.push_back(TunerParam<int16_t>(EvalParam::RookAtk, 0, 100, "RookAtk"));
    input.push_back(TunerParam<int16_t>(EvalParam::QueenAtk, 0, 100, "QueenAtk"));

    input.push_back(TunerParam<int16_t>(EvalParam::NumKZoneAttacks.m, 0, 100, "NumKZoneAttacks Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::NumKZoneAttacks.e, 0, 100, "NumKZoneAttacks End"));
    input.push_back(TunerParam<int16_t>(EvalParam::ShelterBonus.m, 0, 100, "ShelterBonus Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::ShelterBonus.e, 0, 100, "ShelterBonus End"));

    input.push_back(TunerParam<int16_t>(EvalParam::PawnsxMinors.m, 0, 100, "PawnsxMinors Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PawnsxMinors.e, 0, 100, "PawnsxMinors End"));
    input.push_back(TunerParam<int16_t>(EvalParam::MinorsxMinors.m, 0, 100, "MinorsxMinors Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::MinorsxMinors.e, 0, 100, "MinorsxMinors End"));
    input.push_back(TunerParam<int16_t>(EvalParam::MajorsxWeakMinors.m, 0, 100, "MajorsxWeakMinors Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::MajorsxWeakMinors.e, 0, 100, "MajorsxWeakMinors End"));
    input.push_back(TunerParam<int16_t>(EvalParam::PawnsMinorsxMajors.m, 0, 100, "PawnsMinorsxMajors Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::PawnsMinorsxMajors.e, 0, 100, "PawnsMinorsxMajors End"));
    input.push_back(TunerParam<int16_t>(EvalParam::AllxQueens.m, 0, 100, "AllxQueens Mid"));
    input.push_back(TunerParam<int16_t>(EvalParam::AllxQueens.e, 0, 100, "AllxQueens End"));

    FindBestK(data);
    PrintOutput() << "Best K " << K;

    PrintOutput() << "Initial values:";
    for (auto par : input) PrintOutput() << par.name << " " << par;

    std::vector<TunerParam<int16_t>> tuned = GradientDescent(input, data, batchSize);

    PrintOutput() << "Tuned values:";
    for (auto par : tuned) PrintOutput() << par.name << " " << par;

    for (auto &d : data) delete d.p;
}