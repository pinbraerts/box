#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>
#include <fstream>

using histogram = std::vector<unsigned>;
using point = std::array<float, 3>;
using data = std::vector<float>;

float box(float p, float w) {
    if (p < -w) {
        return p - std::ceil(p / w) * w;
    }
    else if (p >= w) {
        return p - std::floor(p / w) * w;
    }
    else {
        return p;
    }
}

point operator%(point x, float w) {
    return {
        box(x[0], w),
        box(x[1], w),
        box(x[2], w),
    };
}

point operator+(point x, point y) {
    return {
        x[0] + y[0],
        x[1] + y[1],
        x[2] + y[2],
    };
}

point operator-(point x, point y) {
    return {
        x[0] - y[0],
        x[1] - y[1],
        x[2] - y[2],
    };
}

point operator*(float y, point x) {
    return {
        x[0] * y,
        x[1] * y,
        x[2] * y,
    };
}

float mag2(point x) {
    return x[0] * x[0] + x[1] * x[1] + x[2] * x[2];
}

auto lj(point x, point y, float w, float a2) {
    auto const r = (x - y) % w;
    auto const r2 = mag2(r);
    if (r2 == 0 || r2 < 9 * a2) {
        return point { };
    }
    auto const b = a2 / r2;
    auto const c = b * b * b;
    return -12 * (c * c - c) / r2 * r;
}

bool hist(histogram& hist, float start, float end, float value) {
    auto const bin = static_cast<std::size_t>((value - start) * hist.size() / (end - start));
    if (value < start || bin >= hist.size()) {
        return false;
    }
    hist[bin] += 1;
    return true;
}

void draw(histogram const& h) {
    std::ofstream output("hist.bin", std::ios::binary);
    output.write(reinterpret_cast<const char*>(h.data()), h.size() * sizeof(h[0]));
}

int main(int argc, const char** argv) {
    unsigned count = 0;
    const unsigned n = argc > ++count ? std::atoi(argv[count]) : 100;
    const float    A = argc > ++count ? std::atof(argv[count]) : 25.0f;
    const float    D = argc > ++count ? std::atof(argv[count]) : 100.0f;
    const float    w = argc > ++count ? std::atof(argv[count]) : 1000.0f;
    const float    t = argc > ++count ? std::atof(argv[count]) : 1.0f;
    const float    m = argc > ++count ? std::atof(argv[count]) : 1.0f;
    const int frames = argc > ++count ? std::atoi(argv[count]) : 10000;
    const float a = A * A;
    std::vector<point> x(n);
    std::random_device r;
    std::mt19937 g(r());
    std::generate(x.begin(), x.end(), [&](){
        return point {
            w * std::generate_canonical<float, 5>(g) ,
            w * std::generate_canonical<float, 5>(g),
            w * std::generate_canonical<float, 5>(g),
        };
    });
    std::vector<point> p = x;
    std::vector<point> f(n);
    histogram v(100);
    for (int k = 0; k < frames; ++k) {
        for (std::size_t i = 0; i < x.size(); ++i) {
            auto const x1 = x[i];
            auto F = f[i];
            for (std::size_t j = i + 1; j < x.size(); ++j) {
                auto const ff = D * lj(x1, x[j], w,  a);
                F = F + ff;
                f[j] = f[j] + ff;
            }
            const auto delta = (x[i] - p[i]) % w;
            hist(v, 0, 10, std::sqrt(mag2(delta)) / t);
            p[i] = x[i] + delta + t * t / m * F;
            p[i] = p[i] % w;
        }
        std::swap(x, p);
        std::fill(f.begin(), f.end(), point { });
        std::cout << k << std::endl;
    }
    draw(v);
    return 0;
}
