#pragma once
#include "common.h"

std::string make_challenge(uint32_t seed, int cols, int rows, bool wrap,
                           int speed, int preset);
bool parse_challenge(const std::string &s, uint32_t &seed, int &cols, int &rows,
                     bool &wrap, int &speed, int &preset);
