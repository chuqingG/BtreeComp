#pragma once

#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include <random>
#include <memory>

// A very simple random number generator.  Not especially good at
// generating truly random bits, but good enough for our needs in this
// package.
class Random {
 private:
  enum : uint32_t {
    M = 2147483647L  // 2^31-1
  };
  enum : uint64_t {
    A = 16807  // bits 14, 8, 7, 5, 2, 1, 0
  };

  uint32_t seed_;

  static uint32_t GoodSeed(uint32_t s) { return (s & M) != 0 ? (s & M) : 1; }

 public:
  // This is the largest value that can be returned from Next()
  enum : uint32_t { kMaxNext = M };

  explicit Random(uint32_t s) : seed_(GoodSeed(s)) {}

  void Reset(uint32_t s) { seed_ = GoodSeed(s); }

  uint32_t Next() {
    // We are computing
    //       seed_ = (seed_ * A) % M,    where M = 2^31-1
    //
    // seed_ must not be zero or M, or else all subsequent computed values
    // will be zero or M respectively.  For all other values, seed_ will end
    // up cycling through every number in [1,M-1]
    uint64_t product = seed_ * A;

    // Compute (product % M) using the fact that ((x << 31) % M) == x.
    seed_ = static_cast<uint32_t>((product >> 31) + (product & M));
    // The first reduction may overflow by 1 bit, so we may need to
    // repeat.  mod == M is not possible; using > allows the faster
    // sign-bit-based test.
    if (seed_ > M) {
      seed_ -= M;
    }
    return seed_;
  }

  // Returns a uniformly distributed value in the range [0..n-1]
  // REQUIRES: n > 0
  uint32_t Uniform(int n) { return Next() % n; }

  // Randomly returns true ~"1/n" of the time, and false otherwise.
  // REQUIRES: n > 0
  bool OneIn(int n) { return Uniform(n) == 0; }

  // "Optional" one-in-n, where 0 or negative always returns false
  // (may or may not consume a random value)
  bool OneInOpt(int n) { return n > 0 && OneIn(n); }

  // Returns random bool that is true for the given percentage of
  // calls on average. Zero or less is always false and 100 or more
  // is always true (may or may not consume a random value)
  bool PercentTrue(int percentage) {
    return static_cast<int>(Uniform(100)) < percentage;
  }

  // Skewed: pick "base" uniformly from range [0,max_log] and then
  // return "base" random bits.  The effect is to pick a number in the
  // range [0,2^max_log-1] with exponential bias towards smaller numbers.
  uint32_t Skewed(int max_log) {
    return Uniform(1 << Uniform(max_log + 1));
  }

  // Returns a random string of length "len"
  std::string RandomString(int len);

  // Generates a random string of len bytes using human-readable characters
  std::string HumanReadableString(int len);

  // Returns a Random instance for use by the current thread without
  // additional locking
  static Random* GetTLSInstance();
};

// A good 64-bit random number generator based on std::mt19937_64
class Random64 {
 private:
  std::mt19937_64 generator_;

 public:
  explicit Random64(uint64_t s) : generator_(s) { }

  // Generates the next random number
  uint64_t Next() { return generator_(); }

  // Returns a uniformly distributed value in the range [0..n-1]
  // REQUIRES: n > 0
  uint64_t Uniform(uint64_t n) {
    return std::uniform_int_distribution<uint64_t>(0, n - 1)(generator_);
  }

  // Randomly returns true ~"1/n" of the time, and false otherwise.
  // REQUIRES: n > 0
  bool OneIn(uint64_t n) { return Uniform(n) == 0; }

  // Skewed: pick "base" uniformly from range [0,max_log] and then
  // return "base" random bits.  The effect is to pick a number in the
  // range [0,2^max_log-1] with exponential bias towards smaller numbers.
  uint64_t Skewed(int max_log) {
    return Uniform(uint64_t(1) << Uniform(max_log + 1));
  }
};

// Helper for quickly generating random data.
class RandomGenerator {
 private:
  std::string data_;
  int pos_;

 public:
  RandomGenerator() {
    // We use a limited amount of data over and over again and ensure
    // that it is larger than the compression window (32KB), and also
    // large enough to serve all typical value sizes we want to write.
    Random rnd(301);
    std::string piece;
    while (data_.size() < 1048576) {
      // Add a short fragment that is as compressible as specified
      // by FLAGS_compression_ratio.
      RandomString(&rnd, 100, &piece);
      data_.append(piece);
    }
    pos_ = 0;
  }
};

void RandomString(Random* rnd, int len, std::string* dst) {
  dst->resize(len);
  for (int i = 0; i < len; i++) {
    (*dst)[i] = static_cast<char>(' ' + rnd->Uniform(95));  // ' ' .. '~'
  }
}

void AllocateKey(std::unique_ptr<const char[]>* key_guard, int key_size) {
    // allocate a space for key_guard
    char* data = new char[key_size];
    const char* const_data = data;
    key_guard->reset(const_data);
  }

// void Validation_Write(int key_size) {
//     Random64 rand(123);
//     Status s;
//     std::unique_ptr<const char[]> key_guard;
//     AllocateKey(&key_guard, key_size+1);
//     for (int i = 0; i < 1000; i++) {
//       GenerateKeyFromInt(i, &key);
//       key.Reset(key.data(), key.size()-1);
//       char to_be_append = 'v';// add an extra char to make key different from write bench.
//       assert(key.size() == FLAGS_key_size);
//       key.append(&to_be_append, 1);

//       s = db_->Put(write_options_, key, key);
//       validation_keys.push_back(key.ToString());
//     }
//     printf("validation write finished\n");
//   }