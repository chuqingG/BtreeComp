#pragma once
#include <string>
#include <iostream>

class Data {
public:
	Data(): data_(""), size_(0) {}

	Data(const std::string& s) : data_(s.data()), size_(s.size()) {}

    const char* data() const { return data_; }

    size_t size() const { return size_; }

private:
	const char* data_;
	size_t size_;
};