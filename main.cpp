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
    float r = 2;
    float fps = 30;
    float n = 100;
    float D = 1;
    float m = 1;
    float t = 1;
    float K = 10000;
    float w = 500;
    for (size_t i = 1; i < argc; ++i) {
        auto const get_var = [&] () -> float& {
            switch (argv[i][0]) {
            case 'r': return r;
            case 'f': return fps;
            case 'd': return D;
            case 'm': return m;
            case 't': return t;
            case 'k': return K;
            case 'w': return w;
            case 'n': return n;
            default:  return n;
            };
        };
        get_var() = std::atof(argv[i][1] ? argv[i] + 1 : argv[++i]);
    }
    const float a2 = r * r;
    std::vector<point> x(n);
    std::random_device rand;
    std::mt19937 g(rand());
    std::generate(x.begin(), x.end(), [&](){
        return point {
            w - 2 * w * std::generate_canonical<float, 5>(g),
            w - 2 * w * std::generate_canonical<float, 5>(g),
            0,
            0,
        };
    });
    std::vector<point> p = x;
    std::vector<point> x0 = x;
    std::vector<point> f(n);
    std::vector<float> e;
    e.reserve(K);
    std::cout.precision(4);
    histogram v(50);
    histogram rdf(100);
    auto const lj_0 = lj(1.0f / 0.8f);
    Graphics graphics(w, fps);
    for (int k = 0; graphics.valid() && k < K; ++k) {
        std::fill(v.begin(), v.end(), 0);
        std::fill(rdf.begin(), rdf.end(), 0);
        float E = 0;
        float S = 0;
        point P { 0, 0, 0, 0 };
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
                auto const ff = -12 * jj / r2 * r;
                F = F + ff;
                f[j] = f[j] - ff;
                hist(rdf, 0, 2 * w, std::sqrt(r2));
            }
            const auto delta = (x[i] - p[i]) % w;
            P = P + m / t * delta;
            const auto speed = std::sqrt(mag2(delta)) / t;
            hist(v, 0, w / 8, speed);
            E += speed * speed * m / 2;
            S += mag2(x[i] - x0[i]);
            p[i] = x[i] + delta + t * t / m * F;
            p[i] = p[i] % w;
            graphics.points.push_back({
                x[i][0], x[i][1],
                std::max(delta[0], w / 100.0f), std::max(delta[1], w / 100.0f),
            });
        }
        if (!graphics.valid()) {
            break;
        }
        std::swap(x, p);
        S /= x.size();
        auto const draw = graphics.step(v, rdf, P, E, S);
        using namespace std::literals::chrono_literals;
        std::fill(f.begin(), f.end(), point { });
        e.push_back(E);
        if (draw) {
            std::cout
                << std::setw(Align) << k
                << ") E: " << std::setw(Align) << E
                << " P: " << std::setw(Align) << std::sqrt(mag2(P))
                << " MSD: " << std::setw(Align) << S
                << std::endl
            ;
        }
    }
    if (graphics.valid()) {
        write(std::ofstream("velocity.bin", std::ios::binary), v);
        write(std::ofstream("energy.bin", std::ios::binary), e);
    }
    std::cout << std::endl;
    return 0;
}
