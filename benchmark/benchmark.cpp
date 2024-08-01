#include "benchmark.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

using namespace std;

enum class BenchmarkTypes {
    INSERT,
    SEARCH,
    RANGE,
    BACKWARDSCAN
};

enum class DatasetTypes {
    RANDOM,
    ELSE
}; // need to add further

// Benchmark will return both times and tree statistics
struct BenchmarkResult {
    std::vector<std::map<BenchmarkTypes, double>> structure_times;
    std::vector<TreeStatistics> structure_statistics;
};

// Define multiple benchmarks in single run
vector<BenchmarkTypes> benchmarks;
DatasetTypes dataset;
uint32_t size = 0;
uint32_t key_numbers = 0;
uint32_t iterations = 0;
double warmup_split_ratio = 0.2;
string dataset_file_name = "";
string output_path = "";
bool write_to_file = false;
int column_num = 0;
uint16_t max_keylen = 0;
int range_num = 100; // Default value

const std::map<BenchmarkTypes, std::string> benchmarkStrMap{
    {BenchmarkTypes::INSERT, "insert"},
    {BenchmarkTypes::SEARCH, "search"},
    {BenchmarkTypes::RANGE, "range"},
    {BenchmarkTypes::BACKWARDSCAN, "backward"}};

const std::map<std::string, BenchmarkTypes> strBenchmarksMap{
    {"insert", BenchmarkTypes::INSERT},
    {"search", BenchmarkTypes::SEARCH},
    {"range", BenchmarkTypes::RANGE},
    {"backward", BenchmarkTypes::BACKWARDSCAN}};

const std::vector<std::tuple<std::string, Benchmark *>> kIndexStructures{
    {"Btree-Std", new BPTreeStdBenchmark()},
    {"Btree-Head", new BPTreeHeadCompBenchmark()},
    {"Btree-Tail", new BPTreeTailCompBenchmark()},
    {"Btree-He+Tail", new BPTreeHeadTailCompBenchmark()},
    // {"Btree-WT", new BPTreeWTBenchmark()},
    // {"Btree-My", new BPTreeMyISAMBenchmark()},
    // {"Btree-PkB", new BPTreePkBBenchmark()},
    // {"Btree-DB2", new BPTreeDB2Benchmark()},
    //{"Other-ART", new ARTBenchmark()},
};

auto RunBenchmarkIteration(std::vector<char *> values,
                           std::vector<char *> values_warmup) {
    // List of time spent for each benchmark
    std::vector<std::map<BenchmarkTypes, double>> structure_times(
        kIndexStructures.size());
    std::vector<TreeStatistics> structure_statistics(kIndexStructures.size());

    vector<char *> sorted_values;
    std::vector<int> minIndxs(range_num);

    std::vector<BenchmarkTypes> tofind{BenchmarkTypes::RANGE,
                                       BenchmarkTypes::BACKWARDSCAN};
    auto founded = std::find_if(
        benchmarks.begin(), benchmarks.end(), [&](BenchmarkTypes val) { return std::find(tofind.begin(), tofind.end(), val) != tofind.end(); });

    if (founded != benchmarks.end()) {
        // Concatenate both values and values warmup for sorted list to perform

        sorted_values.insert(sorted_values.end(), values.begin(),
                             values.end());
        sorted_values.insert(sorted_values.end(), values_warmup.begin(),
                             values_warmup.end());

        std::sort(sorted_values.begin(), sorted_values.end(), [](const char *lhs, const char *rhs) {
            int llen = strlen(lhs);
            int rlen = strlen(rhs);
            return char_cmp_new(lhs, rhs, llen, rlen) < 0;
        });

        for (int i = 0; i < minIndxs.size(); i++) {
            minIndxs[i] = rand() % sorted_values.size() / 2;
        }
    }

    for (uint32_t i = 0; i < kIndexStructures.size(); ++i) {
        const auto &[name, structure] = kIndexStructures[i];
        structure->InitializeStructure();
        double time_spent;
        std::chrono::system_clock::time_point t1;

#ifdef SINGLE_DEBUG
        auto t_start = std::chrono::system_clock::now();
#endif

        // Insert values for warmup
        structure->Insert(values_warmup);
        #ifdef CHECK
            bool noerror_s = structure->Search(values_warmup);
             cout << name << "\t:" //check if insertions are correct by search through them
                         << "Warmup check, should be 0" << endl;
        #endif
#ifdef SINGLE_DEBUG
        auto time_gap = static_cast<double>(
                            std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::system_clock::now() - t_start)
                                .count())
                        / 1e9;
        cout << "Finish Warmup: " << time_gap << " (s)" << endl;
#endif

        for (BenchmarkTypes benchmark : benchmarks) {
            switch (benchmark) {
            case BenchmarkTypes::INSERT: {
                t1 = std::chrono::system_clock::now();
                structure->Insert(values);
                time_spent = static_cast<double>(
                                 std::chrono::duration_cast<std::chrono::nanoseconds>(
                                     std::chrono::system_clock::now() - t1)
                                     .count())
                             / 1e9;
#ifdef SINGLE_DEBUG
                cout << name << "\t:"
                     << "Finish" << endl;
#endif
                break;
            }
            case BenchmarkTypes::SEARCH: {
                t1 = std::chrono::system_clock::now();
                // TODO(chuqing): more accurate correctness check, e.g. generate new
                // search set
                bool noerror_s = structure->Search(values);
                time_spent = static_cast<double>(
                                 std::chrono::duration_cast<std::chrono::nanoseconds>(
                                     std::chrono::system_clock::now() - t1)
                                     .count())
                             / 1e9;
                if (noerror_s)
                    cout << name << "\t:"
                         << "No errors happen during search" << endl;
                else
                    cout << name << "\t:"
                         << "Errors happen during search" << endl;
                break;
            }
            case BenchmarkTypes::RANGE: {
                t1 = std::chrono::system_clock::now();
                bool noerror_sr = structure->SearchRange(sorted_values, minIndxs);
                time_spent = static_cast<double>(
                                 std::chrono::duration_cast<std::chrono::nanoseconds>(
                                     std::chrono::system_clock::now() - t1)
                                     .count())
                             / 1e9;
                if (noerror_sr)
                    cout << name << "\t:"
                         << "No errors happen during search range" << endl;
                else
                    cout << name << "\t:"
                         << "Errors happen during search range" << endl;
                break;
            }
            }
            structure_times[i].insert({benchmark, time_spent});
#ifdef VERBOSE_PRINT
            if (write_to_file) {
                std::ofstream myfile;
                const double ops = key_numbers / time_spent / 1e6;
                string file_name = output_path + ".txt";
                myfile.open(file_name, fstream::out | ios::app);
                myfile << name << " (" << benchmarkStrMap.at(benchmark) << ")\t"
                       << FormatTime(time_spent, true) << FormatTime(ops, false)
                       << endl;
                myfile.close();
            }
#endif
        }
        structure_statistics[i] = structure->CalcStatistics();
#ifdef VERBOSE_PRINT
        if (write_to_file) {
            std::ofstream myfile;
            auto ss = structure_statistics[i];
            string file_name = output_path + ".txt";
            myfile.open(file_name, fstream::out | ios::app);
            myfile << "name\theight\tkeysize\tprefix\tfanout\tnodes\tnonleaf\t\n";
            myfile << name << "\t" << ss.height << "   \t"
                   << ss.totalKeySize / (double)ss.numKeys << "\t"
                   << ss.totalPrefixSize / (double)ss.numKeys << "\t"
                   << ss.totalBranching / (double)ss.nonLeafNodes << "\t"
                   << ss.numNodes << "\t" << ss.nonLeafNodes << std::endl;
            myfile << "--------------------------------------------------" << endl;
            myfile.close();
        }
#endif
        structure->DeleteStructure();
    }
    BenchmarkResult results = {
        structure_times,
        structure_statistics,
    };
    return results;
}

auto BenchmarkToString() {
    std::string s;
    std::for_each(benchmarks.begin(), benchmarks.end(),
                  [&](const BenchmarkTypes &benchmark) {
                      std::string benchmark_str = benchmarkStrMap.at(benchmark);
                      s += s.empty() ? benchmark_str : "," + benchmark_str;
                  });
    return s;
}

// Convert list of comma separated strings to benchmarks list
auto ArgToBenchmark(std::string benchmark_str) {
    string delimiter = ",";
    size_t pos = 0;
    vector<BenchmarkTypes> tokens;
    while ((pos = benchmark_str.find(delimiter)) != std::string::npos) {
        string token = benchmark_str.substr(0, pos);
        auto it = strBenchmarksMap.find(token);
        if (it == strBenchmarksMap.end()) {
            perror("unknown benchmark type");
        }
        tokens.push_back(it->second);
        benchmark_str.erase(0, pos + delimiter.length());
    }
    auto it = strBenchmarksMap.find(benchmark_str);
    if (it == strBenchmarksMap.end()) {
        perror("unknown benchmark type");
    }
    tokens.push_back(it->second);
    return tokens;
}

void PerformanceBenchmarkResults(
    std::vector<map<BenchmarkTypes, vector<double>>>
        structure_benchmark_times) {
    std::ofstream myfile;
    if (write_to_file) {
        string file_name = output_path + ".txt";
        myfile.open(file_name, fstream::out | ios::app);
        myfile << "===========================START SUMMARY========================" << endl;
    }
    for (auto benchmark : benchmarks) {
        std::cout << "============================================================="
                     "===================================================="
                  << std::endl;
        if (benchmark == BenchmarkTypes::RANGE) {
            double r = 1 / RANGE_SCOPE;
            std::cout << "\t\t\t\t\tPERFORMANCE BENCHMARK RESULTS - "
                      << benchmarkStrMap.at(benchmark) << r << std::endl;
        }
        else
            std::cout << "\t\t\t\t\tPERFORMANCE BENCHMARK RESULTS - "
                      << benchmarkStrMap.at(benchmark) << std::endl;
        std::cout << "============================================================="
                     "===================================================="
                  << std::endl;
        if (benchmark == BenchmarkTypes::RANGE) {
            std::cout << "Index Structure\t|      Min\t|      Max\t|      Avg\t|      "
                         "Med\t| M Ops/s (Avg)\t| Ops/s (Med)\t|"
                      << std::endl;
        }
        else {
            std::cout << "Index Structure\t|      Min\t|      Max\t|      Avg\t|      "
                         "Med\t| M Ops/s (Avg)\t| M Ops/s (Med)\t|"
                      << std::endl;
        }
        std::cout << "-------------------------------------------------------------"
                     "----------------------------------------------------"
                  << std::endl;

        // New file for every benchmark result
        if (write_to_file) {
            if (benchmark == BenchmarkTypes::RANGE) {
                myfile << "\n"
                       << benchmarkStrMap.at(benchmark) << " performance: " << float(1 * RANGE_SCOPE) << "\n"
                       << "name\tmin\tmax\tavg\tmed\tmops_avg\tmops_med\n";
            }
            else {
                myfile << "\n"
                       << benchmarkStrMap.at(benchmark) << " performance\n"
                       << "name\tmin\tmax\tavg\tmed\tmops_avg\tmops_med\n";
            }
        }

        for (uint32_t i = 0; i < kIndexStructures.size(); ++i) {
            const auto &[name, structure] = kIndexStructures[i];
            std::stringstream output_row;
            auto times = structure_benchmark_times[i].at(benchmark);
            std::sort(times.begin(), times.end());

            const double min = times[0];
            const double max = times[times.size() - 1];
            double sum = 0.0;
            for (const auto &t : times)
                sum += t;
            const double avg = sum / times.size();
            const double med =
                times.size() % 2 == 1 ? times[times.size() / 2] : (times[times.size() / 2] + times[times.size() / 2 + 1]) / 2.0;

            std::cout << name << "\t";
            double avg_ops, med_ops;

            if (benchmark == BenchmarkTypes::RANGE) {
                avg_ops = range_num / avg;
                med_ops = range_num / avg;
            }
            else {
                avg_ops = key_numbers / avg / 1e6;
                med_ops = key_numbers / avg / 1e6;
            }

            std::cout << "|" << FormatTime(min, true) << FormatTime(max, true)
                      << FormatTime(avg, true) << FormatTime(med, true)
                      << FormatTime(avg_ops, false) << FormatTime(med_ops, false)
                      << std::endl;

            if (write_to_file) {
                output_row << name << "\t" << min << "\t" << max << "\t" << avg << "\t"
                           << med << "\t" << avg_ops << "\t" << med_ops << std::endl;
                myfile << output_row.str();
            }
            structure->DeleteStructure();
        }
    }
    if (write_to_file) {
        myfile.close();
    }
}

void TreeStatisticBenchmarkResults(
    std::vector<vector<TreeStatistics>> structure_benchmark_statistics) {
    std::cout << "==============================================================="
                 "=================================================="
              << std::endl;
    std::cout << "\t\t\t\t\tTREE STATISTICS BENCHMARK RESULTS" << std::endl;
    std::cout << "==============================================================="
                 "=================================================="
              << std::endl;
    std::cout << "Index Structure\t|      Height\t| Avg Key Size\t|"
                 "  Prefix Size\t|  Avg Fanout\t| Total Nodes\t| "
                 "Non-leaf #\t|"
              << std::endl;
    std::cout << "---------------------------------------------------------------"
                 "--------------------------------------------------"
              << std::endl;

    std::ofstream myfile;
    if (write_to_file) {
        string file_name = output_path + ".txt";
        myfile.open(file_name, fstream::out | ios::app);
        myfile
            << "\nname\theight\tkeysize\tprefixsize\tbranchdegree\tnodes\tnonleafnodes\n";
    }

    for (uint32_t i = 0; i < kIndexStructures.size(); ++i) {
        std::stringstream output_row;
        const auto &[name, structure] = kIndexStructures[i];
        auto statistics = structure_benchmark_statistics[i];
        long sum_height = 0;
        long sum_nodes = 0;
        long sum_non_leaf_nodes = 0;
        long sum_keys = 0;
        double sum_avg_key_size = 0;
        double sum_avg_prefix_size = 0;
        double sum_avg_branch_degree = 0;
        for (auto statistic : statistics) {
            sum_height += statistic.height;
            sum_nodes += statistic.numNodes;
            sum_non_leaf_nodes += statistic.nonLeafNodes;
            sum_keys += statistic.numKeys;
            sum_avg_key_size += statistic.totalKeySize / (double)statistic.numKeys;
            sum_avg_prefix_size +=
                statistic.totalPrefixSize / (double)statistic.numKeys;
            sum_avg_branch_degree +=
                statistic.totalBranching / (double)statistic.nonLeafNodes;
        }
        const double avg_height = sum_height / statistics.size();
        const double avg_nodes = sum_nodes / statistics.size();
        const double avg_non_leaf_nodes = sum_non_leaf_nodes / statistics.size();
        const double avg_key_size = sum_avg_key_size / statistics.size();
        const double avg_prefix_size = sum_avg_prefix_size / statistics.size();
        const double avg_branch_degree = sum_avg_branch_degree / statistics.size();

        std::cout << name << "\t";
        double sum_used_branch_degree = 0;
        if (name == "Other-ART") {
            for (auto statistic : statistics) {
                sum_used_branch_degree += statistic.numKeys / (double)statistic.nonLeafNodes;
            }
            std::cout << "|" << FormatStatistic(avg_height)
                      << FormatStatistic(avg_key_size)
                      << "   act: " << sum_used_branch_degree / statistics.size() << "\t|"
                      << FormatStatistic(avg_branch_degree)
                      << FormatStatistic(avg_nodes)
                      << FormatStatistic(avg_non_leaf_nodes) << std::endl;
        }
        else {
            std::cout << "|" << FormatStatistic(avg_height)
                      << FormatStatistic(avg_key_size)
                      << FormatStatistic(avg_prefix_size)
                      << FormatStatistic(avg_branch_degree)
                      << FormatStatistic(avg_nodes)
                      << FormatStatistic(avg_non_leaf_nodes) << std::endl;
        }
        if (write_to_file) {
            output_row << name << "\t" << avg_height << "\t" << avg_key_size << "\t"
                       << avg_prefix_size << "\t" << avg_branch_degree << "\t"
                       << avg_nodes << "\t" << avg_non_leaf_nodes << std::endl;
            myfile << output_row.str();
        }

        structure->DeleteStructure();
    }
    if (write_to_file) {
        myfile << "=========================END OF SUMMARY=====================" << endl;
        myfile.close();
    }
}

void RunBenchmark() {
    cout << "Starting [" << BenchmarkToString() << "]: size=" << key_numbers
         << ", iter=" << iterations << ", pagesize=" << MAX_SIZE_IN_BYTES << endl;

    std::vector<map<BenchmarkTypes, vector<double>>> structure_benchmark_times(
        kIndexStructures.size());
    std::vector<vector<TreeStatistics>> structure_benchmark_statistics(
        kIndexStructures.size());

    const auto t1 = std::chrono::system_clock::now();

    std::vector<char *> values;
    std::vector<char *> values_warmup;
    if (dataset == DatasetTypes::RANDOM) {
        column_num = 1;
#ifdef TOFIX
        generate_random_number(values, key_numbers);
        generate_random_number(
            values_warmup,
            100'000); // TODO(chuqing): now set a small value for test
#endif
    }
    else {
        cout << "Dataset file name " << dataset_file_name << endl;
        std::vector<char *> allvalues;
        int max_data_length;
        if (key_numbers) {
            // cutoff on the whole dataset
            max_data_length =
                read_dataset_char(allvalues, dataset_file_name, max_keylen, key_numbers);
        }
        else {
            max_data_length = read_dataset_char(allvalues, dataset_file_name, max_keylen);
            key_numbers = allvalues.size();
        }
#ifdef SINGLE_DEBUG
        cout << "Finish reading: " << key_numbers << " items in total, "
             << "max length: " << max_data_length << endl;
#endif
        int warm_num = std::round(allvalues.size() * warmup_split_ratio);
        for (int i = 0; i < warm_num; i++) {
            values_warmup.push_back(allvalues[i]);
        }
        for (int i = warm_num; i < allvalues.size(); i++) {
            values.push_back(allvalues[i]);
        }

        // Free extra space
        values.shrink_to_fit();
        values_warmup.shrink_to_fit();
        allvalues.clear();
        vector<char *>(allvalues).swap(allvalues);
    }

    // Run Benchmarks
    for (int i = 0; i < iterations; ++i) {
        std::cout << '\r' << "Running iteration " << (i + 1) << "/" << iterations
                  << "...\n"
                  << std::flush;
        BenchmarkResult results = RunBenchmarkIteration(values, values_warmup);
        // times is vector<std::map<BenchmarkTypes, double>> structure_times
        auto times = results.structure_times;
        // statistics is vector<TreeStatistics>
        auto statistics = results.structure_statistics;
        for (uint32_t j = 0; j < kIndexStructures.size(); ++j) {
            for (auto &it : times[j])
                structure_benchmark_times[j][it.first].push_back(it.second);
            structure_benchmark_statistics[j].push_back(statistics[j]);
        }
    }
    // dealloc values
    for (auto ptr : values)
        delete[] ptr;
    for (auto ptr : values_warmup)
        delete[] ptr;

    const auto after_bench = std::chrono::system_clock::now();
    const auto time =
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::seconds>(after_bench - t1)
                .count())
        / 60;

    std::cout << "\nFinish [" << BenchmarkToString() << "]: size=" << key_numbers
              << ", iter=" << iterations << std::fixed << ", "
              << std::setprecision(1) << time << " minutes." << std::endl;

    PerformanceBenchmarkResults(structure_benchmark_times);
    TreeStatisticBenchmarkResults(structure_benchmark_statistics);
}

int main(int argc, char *argv[]) {
    char *benchmark_arg = GetCmdArg(argv, argv + argc, "-b");
    char *size_arg = GetCmdArg(argv, argv + argc, "-n");
    char *iterations_arg = GetCmdArg(argv, argv + argc, "-i");
    char *datasets_arg = GetCmdArg(argv, argv + argc, "-d");
    char *output_file_arg = GetCmdArg(argv, argv + argc, "-o");
    char *keylen_arg = GetCmdArg(argv, argv + argc, "-l");
    char *range_arg = GetCmdArg(argv, argv + argc, "-r");

    if (benchmark_arg == nullptr || (datasets_arg == nullptr && size_arg == nullptr)) {
        perror("invalid args");
    }

    const string benchmark_str{benchmark_arg};
    benchmarks = ArgToBenchmark(benchmark_str);

    if (size_arg != nullptr) {
        const string size_str{size_arg};
        key_numbers = std::stoul(size_str);
    }

    if (keylen_arg != nullptr) {
        const string keylen_str{keylen_arg};
        max_keylen = std::stoul(keylen_str);
    }

    if (range_arg != nullptr) {
        const string range_str{range_arg};
        range_num = std::stoul(range_str);
    }

    if (iterations_arg != nullptr) {
        const string iterations_str{iterations_arg};
        iterations = std::stoul(iterations_str);
    }
    else {
        iterations = 5; // default
    }

    if (datasets_arg != nullptr) {
        const string datasets_str{datasets_arg};
        dataset_file_name = datasets_str;
        dataset = DatasetTypes::ELSE;
    }
    else {
        dataset = DatasetTypes::RANDOM; // default
        if (key_numbers == 0) {
            // in case
            key_numbers = DEFAULT_DATASET_SIZE;
        }
    }

    if (output_file_arg != nullptr) {
        const string output_path_str{output_file_arg};
        if (output_path_str == "auto") {
            output_path =
                generate_output_path(datasets_arg, key_numbers, iterations, 0, max_keylen);
        }
        else {
            output_path = output_path_str;
        }
        write_to_file = true;
        std::cout << "write to: " << output_path << std::endl;
    }

    RunBenchmark();
    return 0;
}