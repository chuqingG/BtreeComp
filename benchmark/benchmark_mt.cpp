#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <random>
#include <string>
#include <chrono>
#include <vector>
#include <map>
#include "benchmark_mt.h"
#include "../utils/config.h"
#include "../utils/util.h"

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <boost/atomic.hpp>
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
#ifdef TIMEDEBUG
    std::vector<std::map<BenchmarkTypes, double>> prefix_update_times;
    std::vector<std::map<BenchmarkTypes, double>> prefix_calc_times;
#endif
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
int thread_num = 0;

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

const std::vector<std::tuple<std::string, Benchmark_c *>> kIndexStructures{
    {"BPTree-std", new BPTreeStdBenchmark()},
    // {"BPTree-head", new BPTreeHeadCompBenchmark()},
    // {"BPTree-tail", new BPTreeTailCompBenchmark()},
    // {"BPTree-headtail", new BPTreeHeadTailCompBenchmark()},
    // {"BPTree-DB2", new BPTreeDB2Benchmark()},
    // {"BPTree-MyISAM", new BPTreeMyISAMBenchmark()},
    // {"BPTree-WT", new BPTreeWTBenchmark()},
    // {"BPTree-PkB", new BPTreePkBBenchmark()},
};

auto RunBenchmarkIteration(std::vector<char *> values, std::vector<char *> values_warmup) {
    // List of time spent for each benchmark
    std::vector<std::map<BenchmarkTypes, double>> structure_times(kIndexStructures.size());
    std::vector<TreeStatistics> structure_statistics(kIndexStructures.size());

#ifdef THREAD_DEBUG
    auto t_start = std::chrono::system_clock::now();
#endif

    // construct elements for range and backward if needed
    vector<char *> sorted_values;
    map<char *, int, mapcmp> values_freq_map;
    std::vector<int> minIndxs(100);

    std::vector<BenchmarkTypes> tofind{BenchmarkTypes::RANGE, BenchmarkTypes::BACKWARDSCAN};
    auto founded = std::find_if(benchmarks.begin(), benchmarks.end(), [&](BenchmarkTypes val) {
        return std::find(tofind.begin(), tofind.end(), val) != tofind.end();
    });

    if (founded != benchmarks.end()) {
        // Concatenate both values and values warmup for sorted list to perform range query
        // vector<const char *> sorted_values;
        sorted_values.insert(sorted_values.end(), values.begin(), values.end());
        sorted_values.insert(sorted_values.end(), values_warmup.begin(), values_warmup.end());
        sort(sorted_values.begin(), sorted_values.end());
        values_freq_map = convert_to_freq_map_char(values, values_warmup);
        // Initialize min indexes for range benchmark
        for (int i = 0; i < minIndxs.size(); i++) {
            minIndxs[i] = rand() % values_freq_map.size() / 2;
        }
    }

#ifdef THREAD_DEBUG
    auto time_gap = static_cast<double>(std::chrono::duration_cast<
                                            std::chrono::nanoseconds>(std::chrono::system_clock::now() - t_start)
                                            .count())
                    / 1e9;
    cout << "Finish sort and freq for range query: " << time_gap << " (s)" << endl;
#endif

    for (uint32_t i = 0; i < kIndexStructures.size(); ++i) {
        const auto &[name, structure] = kIndexStructures[i];
        structure->InitializeStructure(thread_num, column_num);
        double time_spent;
        std::chrono::system_clock::time_point t1;

#ifdef THREAD_DEBUG
        auto t_start = std::chrono::system_clock::now();
#endif

        // Insert values for warmup
        structure->Insert(values_warmup);

#ifdef THREAD_DEBUG
        auto time_gap = static_cast<double>(std::chrono::duration_cast<
                                                std::chrono::nanoseconds>(std::chrono::system_clock::now() - t_start)
                                                .count())
                        / 1e9;
        cout << "Finish Warmup: " << time_gap << " (s)" << endl;
#endif
        //msort(values.begin(), values.end());

        for (BenchmarkTypes benchmark : benchmarks) {
#ifdef TIMEDEBUG
            double prefix_update = 0.0;
            double prefix_calc = 0.0;
#endif
            
            switch (benchmark) {
            case BenchmarkTypes::INSERT:
                t1 = std::chrono::system_clock::now();
#ifndef TIMEDEBUG
                structure->Insert(values);
#else
                structure->Insert(values, &prefix_calc, &prefix_update);
#endif
                time_spent = static_cast<double>(std::chrono::duration_cast<
                                                     std::chrono::nanoseconds>(std::chrono::system_clock::now() - t1)
                                                     .count())
                             / 1e9;
                cout << name << "\t:"
                     << "Finish" << endl;
                break;
            case BenchmarkTypes::SEARCH:
                //boost::asio::thread_pool *pool = new boost::asio::thread_pool(thread_num);
                t1 = std::chrono::system_clock::now();
                // TODO(chuqing): more accurate correctness check, e.g. generate new search set
                if (structure->Search(values))
                    cout << name << "\t:"
                         << "No errors happen during search" << endl;
                else
                    cout << name << "\t:"
                         << "Errors happen during search" << endl;
                time_spent = static_cast<double>(std::chrono::duration_cast<
                                                     std::chrono::nanoseconds>(std::chrono::system_clock::now() - t1)
                                                     .count())
                             / 1e9;

                break;
            }
            structure_times[i].insert({benchmark, time_spent});
#ifdef THREAD_DEBUG
            std::ofstream myfile;
            if (write_to_file) {
                const double ops = key_numbers / time_spent / 1e6;
                string file_name = output_path + ".txt";
                myfile.open(file_name, fstream::out | ios::app);
                myfile << name << " (" << benchmarkStrMap.at(benchmark) << ")\t"
                       << FormatTime(time_spent, true) << FormatTime(ops, false) << endl;
            }
#endif
        }
        structure_statistics[i] = structure->CalcStatistics();
        structure->DeleteStructure();
    }
    BenchmarkResult results = {
        structure_times,
        structure_statistics,
#ifdef TIMEDEBUG
#endif
    };
    return results;
}

auto BenchmarkToString() {
    std::string s;
    std::for_each(benchmarks.begin(), benchmarks.end(),
                  [&](const BenchmarkTypes &benchmark) {  
                    std::string benchmark_str = benchmarkStrMap.at(benchmark);
                    s += s.empty() ? benchmark_str : "," + benchmark_str; });
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

void PerformanceBenchmarkResults(std::vector<map<BenchmarkTypes, vector<double>>> structure_benchmark_times) {
    for (auto benchmark : benchmarks) {
        std::cout << "=================================================================================================================" << std::endl;
        std::cout << "\t\t\t\t\tPERFORMANCE BENCHMARK RESULTS - " << benchmarkStrMap.at(benchmark) << std::endl;
        std::cout << "=================================================================================================================" << std::endl;
        std::cout << "Index Structure\t|      Min\t|      Max\t|      Avg\t|      Med\t| M Ops/s (Avg)\t| M Ops/s (Med)\t|" << std::endl;
        std::cout << "-----------------------------------------------------------------------------------------------------------------" << std::endl;

        // New file for every benchmark result
        std::ofstream myfile;
        if (write_to_file) {
            string file_name = output_path + ".txt";
            myfile.open(file_name, fstream::out | ios::app);
            myfile << "\n"
                   << benchmarkStrMap.at(benchmark) << " performance\n"
                   << "name\tmin\tmax\tavg\tmed\tmops_avg\tmops_med\n";
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
            const double med = times.size() % 2 == 1 ? times[times.size() / 2] : (times[times.size() / 2] + times[times.size() / 2 + 1]) / 2.0;

            // std::cout << name << "\t" << benchmarkStrMap.at(benchmark) << "\t";
            std::cout << name << "\t";

            const double avg_ops = key_numbers / avg / 1e6;
            const double med_ops = key_numbers / med / 1e6;

            std::cout << "|"
                      << FormatTime(min, true) << FormatTime(max, true)
                      << FormatTime(avg, true) << FormatTime(med, true)
                      << FormatTime(avg_ops, false) << FormatTime(med_ops, false)
                      << std::endl;

            if (write_to_file) {
                output_row << name << "\t" << min << "\t" << max << "\t"
                           << avg << "\t" << med << "\t"
                           << avg_ops << "\t" << med_ops << std::endl;
                myfile << output_row.str();
            }

            structure->DeleteStructure();
        }
        if (write_to_file) {
            myfile.close();
        }
    }
}

void TreeStatisticBenchmarkResults(std::vector<vector<TreeStatistics>> structure_benchmark_statistics) {
    std::cout << "=================================================================================================================" << std::endl;
    std::cout << "\t\t\t\t\tTREE STATISTICS BENCHMARK RESULTS" << std::endl;
    std::cout << "=================================================================================================================" << std::endl;
    std::cout << "Index Structure\t|      Height\t|      Avg Key Size\t|      Avg Prefix Size\t|      Avg Branching degree\t| Nodes\t| Non-leaf Nodes\t|" << std::endl;
    std::cout << "-----------------------------------------------------------------------------------------------------------------" << std::endl;

    std::ofstream myfile;
    if (write_to_file) {
        string file_name = output_path + ".txt";
        myfile.open(file_name, fstream::out | ios::app);
        myfile << "\nstatistics\n"
               << "name\theight\tkeysize\tprefixsize\tbranchdegree\tnodes\tnonleafnodes\n";
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
            sum_avg_prefix_size += statistic.totalPrefixSize / (double)statistic.numKeys;
            sum_avg_branch_degree += statistic.totalBranching / (double)statistic.nonLeafNodes;
        }
        const double avg_height = sum_height / statistics.size();
        const double avg_nodes = sum_nodes / statistics.size();
        const double avg_non_leaf_nodes = sum_non_leaf_nodes / statistics.size();
        const double avg_key_size = sum_avg_key_size / statistics.size();
        const double avg_prefix_size = sum_avg_prefix_size / statistics.size();
        const double avg_branch_degree = sum_avg_branch_degree / statistics.size();

        std::cout << name << "\t";

        std::cout << "|"
                  << FormatStatistic(avg_height) << FormatStatistic(avg_key_size)
                  << FormatStatistic(avg_prefix_size) << FormatStatistic(avg_branch_degree)
                  << FormatStatistic(avg_nodes) << FormatStatistic(avg_non_leaf_nodes)
                  << std::endl;
        if (write_to_file) {
            output_row << name << "\t" << avg_height << "\t" << avg_key_size << "\t"
                       << avg_prefix_size << "\t" << avg_branch_degree << "\t"
                       << avg_nodes << "\t" << avg_non_leaf_nodes << std::endl;
            myfile << output_row.str();
        }

        structure->DeleteStructure();
    }
    if (write_to_file) {
        myfile << "====================================================\n";
        myfile.close();
    }
}

void RunBenchmark() {
    cout << "Starting [" << BenchmarkToString() << "]: size=" << key_numbers
         << ", iter=" << iterations << ", thread=" << thread_num << endl;

    std::vector<map<BenchmarkTypes, vector<double>>> structure_benchmark_times(kIndexStructures.size());
    std::vector<vector<TreeStatistics>> structure_benchmark_statistics(kIndexStructures.size());

    const auto t1 = std::chrono::system_clock::now();

    std::vector<char *> values;
    std::vector<char *> values_warmup;
    if (dataset == DatasetTypes::RANDOM) {
        column_num = 1;
#ifndef MEMDEBUG
        generate_random_number(values, key_numbers);
        generate_random_number(values_warmup, 1'000); // TODO(chuqing): now set a small value for test
#endif
    }
    else {
        cout << "Dataset file name " << dataset_file_name << endl;
        std::vector<char *> allvalues;
        int max_data_length;
        if (key_numbers) {
            // cutoff on the whole dataset
            // TODO: add keylen here
            max_data_length = read_dataset_char(allvalues, dataset_file_name, 0, key_numbers);
        }
        else {
            max_data_length = read_dataset_char(allvalues, dataset_file_name, 0);
            key_numbers = allvalues.size();
        }
#ifdef THREAD_DEBUG
        cout << "Finish reading: " << key_numbers << " items in total, "
             << "max length: " << max_data_length << endl;
#endif

        // auto split_point = std::next(allvalues.begin(), std::size_t(std::round(allvalues.size() * (1 - warmup_split_ratio))));
        // std::partition_copy(allvalues.begin(), allvalues.end(), std::back_inserter(values), std::back_inserter(values_warmup),
        // [&](char* elem){ return &elem < &(*split_point); });
        // std::partition_copy(allvalues.begin(), allvalues.end(), values, values_warmup,[&](char* elem){ return &elem < &(*split_point); });
        int warm_num = std::round(allvalues.size() * warmup_split_ratio);
        for (int i = 0; i < warm_num; i++) {
            values_warmup.push_back(allvalues[i]);
        }
        for (int i = warm_num; i < allvalues.size(); i++) {
            values.push_back(allvalues[i]);
        }
        // count columns
        // column_num = count_column(values[0]);
        // free allvalues
        values.shrink_to_fit();
        values_warmup.shrink_to_fit();
        allvalues.clear();
        vector<char *>(allvalues).swap(allvalues);
    }

#ifndef MYDEBUG
    // Run Benchmarks
    for (int i = 0; i < iterations; ++i) {
        std::cout << '\r' << "Running iteration " << (i + 1) << "/" << iterations << "...\n"
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
    const auto time = static_cast<double>(std::chrono::duration_cast<
                                              std::chrono::seconds>(after_bench - t1)
                                              .count())
                      / 60;

    std::cout << "\nFinish [" << BenchmarkToString() << "]: size=" << key_numbers
              << ", iter=" << iterations << std::fixed << ", " << std::setprecision(1) << time << " minutes."
              << std::endl;

    PerformanceBenchmarkResults(structure_benchmark_times);
    TreeStatisticBenchmarkResults(structure_benchmark_statistics);
#endif
}

int main(int argc, char *argv[]) {
    char *benchmark_arg = GetCmdArg(argv, argv + argc, "-b");
    char *size_arg = GetCmdArg(argv, argv + argc, "-n");
    char *iterations_arg = GetCmdArg(argv, argv + argc, "-i");
    char *datasets_arg = GetCmdArg(argv, argv + argc, "-d");
    char *output_file_arg = GetCmdArg(argv, argv + argc, "-o");
    char *thread_arg = GetCmdArg(argv, argv + argc, "-t");

    if (benchmark_arg == nullptr || (datasets_arg == nullptr && size_arg == nullptr)) {
        perror("invalid args");
    }

    const string benchmark_str{benchmark_arg};
    benchmarks = ArgToBenchmark(benchmark_str);

    if (size_arg != nullptr) {
        const string size_str{size_arg};
        key_numbers = std::stoul(size_str);
    }

    if (iterations_arg != nullptr) {
        const string iterations_str{iterations_arg};
        iterations = std::stoul(iterations_str);
    }
    else {
        iterations = 5; // default
    }

    if (thread_arg != nullptr) {
        const string thread_str{thread_arg};
        thread_num = std::stoul(thread_str);
        
    }
    else {
        thread_num = 16; // default
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
            output_path = generate_output_path(datasets_arg, key_numbers, iterations, thread_num, 0); // 0 is place holder
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