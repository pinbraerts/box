#pragma once

#include <array>
#include <cmath>
#include <vector>

using point = std::array<float, 4>;
using histogram = std::vector<unsigned>;

bool hist(histogram& hist, float start, float end, float value);
float box(float p, float w);
float mag2(point x);
point operator*(float y, point x);
point operator-(point x, point y);
point operator+(point x, point y);
point operator%(point x, float w);

