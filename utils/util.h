#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <random>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <climits>
#include <regex.h>
#include <map>
#include <iomanip>
#include <chrono>
#include "config.h"
using namespace std;

#define TIMECOUNT(t, f, ...)                                                  \
    {                                                                         \
        auto t1 = std::chrono::system_clock::now();                           \
        f(__VA_ARGS__);                                                       \
        auto t2 = std::chrono::system_clock::now();                           \
        double t_gap =                                                        \
            static_cast<double>(                                              \
                std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1) \
                    .count())                                                 \
            / 1e9;                                                            \
        t += t_gap;                                                           \
    }

int char_cmp_map(const char *a, const char *b, int alen, int blen) {
    // 1 : a > b
    // const size_t min_len = (alen < blen) ? alen : blen;
    int cmp_len = min(alen, blen);
    // int idx = *matchp;
    for (int idx = 0; idx < cmp_len; ++idx) {
        int cmp = a[idx] - b[idx];
        if (cmp != 0)
            return cmp;
    }
    /* Contents are equal up to the smallest length. */
    return (alen - blen);
}

struct mapcmp {
    bool operator()(const char *lhs, const char *rhs) const {
        int len_l = strlen(lhs);
        int len_r = strlen(rhs);
        return char_cmp_map(lhs, rhs, len_l, len_r) < 0;
    }
};

string convert_to_16_bytes(string key) {
    if (key.length() < 16)
        return key + string(16 - key.length(), '0');
    return key;
}

string convert_to_n_bytes(string key, int n) {
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

void generate_random_string(vector<string> &values, int num_keys, int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    for (int i = 0; i < num_keys; i++) {
        std::string tmp_s;
        tmp_s.reserve(len);

        for (int j = 0; j < len; ++j) {
            tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        
        values.push_back(tmp_s);
    }  
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

int read_dataset_char(vector<char *> &values, string filename, int max_keylen, int max_number = -1) {
    cout << "----- Processing file ----- " << filename << "max length" << max_keylen << endl;
    std::ifstream in(filename);
    int max_length = 0;
    int counter = 0;
    if (max_number < 0)
        max_number = INT_MAX;
    if (max_keylen == 0)
        max_keylen = INT_MAX;
    while (counter < max_number) {
        string str;
        std::getline(in, str);
        int len = str.size();
        int real_len = min(len, max_keylen);
        char *cptr = new char[real_len + 1];
        // strcpy(cptr, str.data());
        strncpy(cptr, str.data(), real_len);
        cptr[real_len] = '\0';

        if (real_len > 0) {
            values.push_back(cptr);
            if (real_len > max_length)
                max_length = real_len;
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

map<char *, int, mapcmp> convert_to_freq_map_char(vector<char *> values, vector<char *> values_warmup) {
    map<char *, int, mapcmp> freq_map;
    for (auto element : values) {
        freq_map[element]++;
    }
    for (auto element : values_warmup) {
        freq_map[element]++;
    }
    return freq_map;
}

vector<char *> get_map_keys(map<char *, int, mapcmp> map) {
    vector<char *> keys;
    for (auto &element : map) {
        keys.push_back(element.first);
        cout << element.first << ",";
    }
    cout << endl;
    return keys;
}

int count_range(vector<char *> &sorted, int idx, int num) {
    char *last = sorted[idx + num];
    int last_l = strlen(last);
    int count = num;
    while (idx + count < sorted.size()) {
        if (char_cmp_map(last, sorted[idx + count], last_l, strlen(sorted[idx + count])))
            break;
        count++;
    }
    return count;
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

string generate_output_path(char *dataset, int number, int iter, int thread_num, int keylen) {
    // thread=0 for single thread, thread=1 for single thread with lock
    string res;
    char temp[64];
    if (dataset != nullptr) {
        int status;
        int cflags = REG_EXTENDED | REG_NEWLINE;
        regmatch_t pmatch;
        const size_t nmatch = 1;
        regex_t reg;
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

        sprintf(temp, "../output/%s_%d_i%d_t%d_ns%d_kl%d", out, number, iter, thread_num, MAX_SIZE_IN_BYTES, keylen);
    }
    else {
        sprintf(temp, "../output/default_%d_i%d_t%d_ns%d_kl%d", number, iter, thread_num, MAX_SIZE_IN_BYTES, keylen);
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