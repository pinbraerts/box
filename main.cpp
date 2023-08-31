#include <algorithm>
#include <array>
#include <clocale>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <cstdint>
#include <memory>
#include <numeric>
#include <random>
#include <thread>
#include <vector>
#include <fstream>
#include <chrono>

#include "graphics.hpp"

constexpr size_t Align = 10;

template<class T>
void write(std::ostream&& stream, const std::vector<T>& value) {
    stream.write(reinterpret_cast<const char*>(value.data()), value.size() * sizeof(value.front()));
}

template<class T>
void write(std::ostream& stream, const T& value) {
    stream.write(reinterpret_cast<const char*>(std::addressof(value)), sizeof(value));
}

auto lj(float ar) {
    auto const c = ar * ar * ar;
    return c * c - c;
}

int main(int argc, const char** argv) {
    unsigned count = 0;
    const unsigned n = argc > ++count ? std::atoi(argv[count]) : 100;
    const float    A = argc > ++count ? std::atof(argv[count]) : 2.0f;
    const float    D = argc > ++count ? std::atof(argv[count]) : 1.0f;
    const float    w = argc > ++count ? std::atof(argv[count]) : 500.0f;
    const float    t = argc > ++count ? std::atof(argv[count]) : 1.0f;
    const float    m = argc > ++count ? std::atof(argv[count]) : 1.0f;
    const int frames = argc > ++count ? std::atoi(argv[count]) : 10000;
    const float a2 = A * A;
    std::vector<point> x(n);
    std::random_device r;
    std::mt19937 g(r());
    std::generate(x.begin(), x.end(), [&](){
        return point {
            w * std::generate_canonical<float, 5>(g),
            w * std::generate_canonical<float, 5>(g),
            0,
            0,
        };
    });
    std::vector<point> p = x;
    std::vector<point> f(n);
    std::vector<point> e;
    e.reserve(frames);
    std::cout.precision(4);
    histogram v(100);
    auto const lj_0 = lj(1.0f / 0.8f);
    Graphics graphics(w);
    graphics.points.reserve(x.size());
    for (int k = 0; graphics.valid() && k < frames; ++k) {
        std::fill(v.begin(), v.end(), 0);
        float E = 0;
        float P = 0;
        graphics.points.resize(0);
        for (std::size_t i = 0; graphics.valid() && i < x.size(); ++i) {
            auto const x1 = x[i];
            auto F = f[i];
            for (std::size_t j = i + 1; j < x.size(); ++j) {
                auto const r = (x1 - x[j]) % w;
                auto const r2 = mag2(r);
                if (r2 == 0 || r2 < 8 * a2) {
                    continue;
                }
                auto const jj = D * (lj(a2 / r2) - lj_0);
                P += jj;
                auto const ff = -12 * jj / r2 * r;
                F = F + ff;
                f[j] = f[j] - ff;
            }
            const auto delta = (x[i] - p[i]) % w;
            const auto speed = std::sqrt(mag2(delta)) / t;
            hist(v, 0, w, speed);
            E += speed * speed * m / 2;
            p[i] = x[i] + delta + t * t / m * F;
            p[i] = p[i] % w;
            graphics.points.push_back({
                x[i][0], x[i][1],
                F[0], F[1],
            });
        }
        if (!graphics.valid()) {
            break;
        }
        graphics.velocity = v;
        graphics.step();
        using namespace std::literals::chrono_literals;
        std::fill(f.begin(), f.end(), point { });
        e.push_back(point { E, P, E + P, E - P });
        std::swap(x, p);
        if (k % 100 == 0) {
            std::cout
                << std::setw(Align) << k
                << ") K: " << std::setw(Align) << E
                << " P: " << std::setw(Align) << P
                << " E: " << std::setw(Align) << (E + P)
                << " L: " << std::setw(Align) << (E - P)
                << '\r'
            ;
        }
        std::this_thread::sleep_for(16ms);
    }
    if (graphics.valid()) {
        write(std::ofstream("velocity.bin", std::ios::binary), v);
        write(std::ofstream("energy.bin", std::ios::binary), e);
    }
    std::cout << std::endl;
    return 0;
}
