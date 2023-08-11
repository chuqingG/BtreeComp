#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <random>
#include <algorithm>
#include <fstream>
#include <climits>
#include <regex.h>
#include <map>
#include "config.h"
using namespace std;

string convert_to_16_bytes(string key) {
    if (key.length() < 16)
        return key + string(16 - key.length(), '0');
    return key;
}

void generate_random_number_unique(vector<string> &values, int num_keys) {
    int i;
    for (i = 0; i < 2'000'000'000; i++) { // close to rand_max
        int num = 1 + rand() % 2'000'000'000;
        string key = convert_to_16_bytes(to_string(num));
        values.push_back(key);
    }

    sort(values.begin(), values.end());
    values.erase(unique(values.begin(), values.end()), values.end());
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(values.begin(), values.end(), g);
    values.resize(num_keys);
}

void generate_random_number1(vector<string> &values, int num_keys) {
    int i;
    for (i = 0; i < num_keys; i++) {
        int num = 1 + rand() % 1'000'000'000; // close to rand_max
        string key = convert_to_16_bytes(to_string(num));
        values.push_back(key);
    }
    sort(values.begin(), values.end());
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(values.begin(), values.end(), g);
    values.resize(num_keys);
}

void generate_random_number(vector<string> &values, int num_keys) {
    // without unique restriction
    vector<int> origin(num_keys);
    for (int i = 0; i < num_keys; i++) {
        int num = 1 + rand() % 1'000'000'000; // close to rand_max
        origin.push_back(num);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(origin.begin(), origin.end(), g);
    cout << "finish shuffle" << endl;
    for (auto n : origin)
        values.push_back(convert_to_16_bytes(to_string(n)));
}

void read_dataset(vector<string> &values, string filename, int max_number = -1) {
    cout << "----- Processing file ----- " << filename << endl;
    std::ifstream in(filename);
    string str;
    int counter = 0;
    if (max_number < 0)
        max_number = INT_MAX;
    while (std::getline(in, str) && counter < max_number) {
        if (str.size() > 0)
            values.push_back(str);
        counter++;
    }
    if (!counter) {
        perror("Cannot read dataset");
        exit(1);
    }
    cout << "----FINISH READ---- : " << counter << " items in total" << endl;
    in.close();
}

int read_dataset_char(vector<char *> &values, string filename, int max_number = -1) {
    cout << "----- Processing file ----- " << filename << endl;
    std::ifstream in(filename);
    int max_length = 0;
    int counter = 0;
    if (max_number < 0)
        max_number = INT_MAX;
    while (counter < max_number) {
        string str;
        std::getline(in, str);
        int len = str.size();
        char *cptr = new char[len + 1];
        strcpy(cptr, str.data());

        if (len > 0) {
            values.push_back(cptr);
            if (len > max_length)
                max_length = len;
        }
        else {
            break;
        }
        counter++;
    }
    if (!counter) {
        perror("Cannot read dataset");
        exit(1);
    }
    cout << "----FINISH READ---- : " << counter << " items in total, max len = " << max_length << endl;
    in.close();
    return max_length;
}

map<string, int> convert_to_freq_map(vector<string> sorted_arr) {
    map<string, int> freq_map;
    for (auto &element : sorted_arr) {
        freq_map[string(element)]++;
    }
    return freq_map;
}

map<string, int> convert_to_freq_map_char(vector<char *> sorted_arr) {
    map<string, int> freq_map;
    for (auto &element : sorted_arr) {
        freq_map[string(element)]++;
    }
    return freq_map;
}

vector<string> get_map_keys(map<string, int> map) {
    vector<string> keys;
    for (auto &element : map) {
        keys.push_back(element.first);
    }
    return keys;
}

int get_map_values_in_range(map<string, int> map, string min, string max) {
    vector<string> keys;
    for (auto &element : map) {
        keys.push_back(element.first);
    }
    auto lower = map.lower_bound(min);
    auto upper = map.upper_bound(max);
    int sum = 0;
    for (auto it = lower; it != upper; ++it) {
        sum += it->second;
    }
    return sum;
}

inline char *GetCmdArg(char **begin, char **end, const std::string &arg) {
    char **itr = std::find(begin, end, arg);
    if (itr != end && ++itr != end)
        return *itr;
    return nullptr;
}

inline std::string FormatTime(const double n, const bool unit) {
    const int p = static_cast<int>(n);
    const int digits = p == 0 ? 1 : static_cast<int>(log10(p)) + 1;

    std::stringstream s;

    for (int i = 0; i < 7 - digits; ++i)
        s << " ";

    s << std::fixed << std::setprecision(4) << n << (unit ? "s" : "") << "\t|";

    return s.str();
}

inline std::string FormatStatistic(const double n) {
    const int p = static_cast<int>(n);
    const int digits = p == 0 ? 1 : static_cast<int>(log10(p)) + 1;

    std::stringstream s;

    for (int i = 0; i < 7 - digits; ++i)
        s << " ";

    s << std::fixed << std::setprecision(4) << n << "\t|";

    return s.str();
}

string generate_output_path(char *dataset, int number, int iter, int thread_num) {
    // thread=0 for single thread, thread=1 for single thread with lock
    string res;
    char temp[64];
    if (dataset != nullptr) {
        int status;
        int cflags = REG_EXTENDED | REG_NEWLINE;
        regmatch_t pmatch;
        const size_t nmatch = 1;
        regex_t reg, end_reg;
        const char *pattern = "[a-zA-Z0-9]+(_[a-zA-Z0-9]+)*.txt";
        regcomp(&reg, pattern, cflags);

        char out[50] = {0};
        status = regexec(&reg, dataset, nmatch, &pmatch, 0);
        if (status != REG_NOMATCH) {
            strncpy(out, dataset + pmatch.rm_so, pmatch.rm_eo - pmatch.rm_so);
            out[pmatch.rm_eo - pmatch.rm_so - 4] = '\0';
        }
        else {
            // in case, copy 20 chars from the name of dataset
            strncpy(out, dataset + strlen(dataset) - 24, 20);
            out[20] = '\0';
        }

        sprintf(temp, "../output/%s_%d_i%d_t%d_ns%d", out, number, iter, thread_num, MAX_SIZE_IN_BYTES);
    }
    else {
        sprintf(temp, "../output/default_%d_i%d_t%d_ns%d", number, iter, thread_num, MAX_SIZE_IN_BYTES);
    }
    res = temp;
    return res;
}

int count_column(string key) {
    int pos = 0;
    int count = 1;
    while ((pos = key.find('|', pos)) != std::string::npos) {
        // cout<<"position  "<<i<<" : "<<position<<endl;
        pos++;
        count++;
    }
    return count;
}

#ifdef __GNUC__ // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)

[[noreturn]] inline __attribute__((always_inline)) void __unreachable() {
    __builtin_unreachable();
}

#elif defined(_MSC_VER) // MSVC

[[noreturn]] __forceinline void __unreachable() {
    __assume(false);
}

#endif