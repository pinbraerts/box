#include <algorithm>
#include <array>
#include <clocale>
#define _USE_MATH_DEFINES
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

constexpr size_t Align = 6;

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

template<class T>
std::pair<float, float> mnk(T const& y, size_t last) {
    float sx2 = 0, sx = 0, sy = 0, sxy = 0;
    for (size_t i = 0; i < y.size(); ++i) {
        sx2 += i * i;
        sx += i;
        sy += y[i];
        sxy += i * y[i];
    }
    auto const b = (y.size() * sxy - sx * sy) / (y.size() * sx2 - sx * sx);
    auto const a = (sy - b * sx) / y.size();
    return { a, b };
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
    float clear = 50;
    float vv = 0;
    float spl = 8;
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
            case 'c': return clear;
            case 'v': return vv;
            case 'x': return spl;
            default:  return n;
            };
        };
        get_var() = std::atof(argv[i][1] ? argv[i] + 1 : argv[++i]);
    }
    const float a2 = r * r;
    const float l = w * w / M_PI / r / r / n;
    std::vector<point> x(n);
    std::random_device rand;
    std::mt19937 g(rand());
    unsigned i = 0, j = 0, u = static_cast<unsigned>(std::sqrt(n));
    std::generate(x.begin(), x.end(), [&](){
        if (i >= u) {
            i = 0;
            ++j;
        }
        ++i;
        return point {
            w - 2 * w * i / u,
            w - 2 * w * j / u,
            0,
            0,
        };
    });
    std::vector<point> p = x;
    std::vector<point> x0 = x;
    std::vector<point> xf = x;
    std::vector<point> f(n);
    std::vector<float> e;
    std::deque<float> MSDt;
    e.reserve(K ? K : 100);
    std::cout.precision(4);
    histogram v(clear ? clear : 100);
    histogram rdf(clear ? clear : 100);
    auto const lj_0 = lj(1.0f / spl);
    Graphics graphics(w, fps, t, clear, MSDt);
    for (int k = 0; graphics.valid() && (K == 0 || k < K); ++k) {
        std::fill(v.begin(), v.end(), 0);
        std::fill(rdf.begin(), rdf.end(), 0);
        float E = 0;
        float MSD = 0;
        float LJ = 0;
        float V = 0;
        point P { 0, 0, 0, 0 };
        graphics.points.resize(0);
        for (std::size_t i = 0; i < x.size(); ++i) {
            auto const x1 = x[i];
            auto F = f[i];
            for (std::size_t j = i + 1; j < x.size(); ++j) {
                auto const rr = (x1 - x[j]) % w;
                auto const r2 = std::max(mag2(rr), w / 500);
                hist(rdf, 0, 2 * w, std::sqrt(r2));
                auto const jj = D * (lj(a2 / r2) - lj_0);
                LJ += jj;
                if (r2 > spl * a2) {
                    continue;
                }
                auto const ff = 12 * jj / r2 * rr;
                F = F + ff;
                // std::cout << i << ' ' << j << ' ' << mag2(ff) << std::endl;
                f[j] = f[j] - ff;
            }
            const auto delta = 
                k
                ? (x[i] - p[i]) % w
                : point {
                    vv / 2 - vv * std::generate_canonical<float, 5>(g),
                    vv / 2 - vv * std::generate_canonical<float, 5>(g),
                    0,
                    0,
                }
            ;
            xf[i] = xf[i] + delta;
            P = P + m / t * delta;
            const auto s2 = mag2(delta) / t / t;
            const auto speed = std::sqrt(s2);
            V += s2;
            hist(v, 0, w / t / 100, speed);
            E += s2 * m / 2;
            MSD += mag2(xf[i] - x0[i]);
            p[i] = x[i] + delta + t * t / m * F;
            p[i] = p[i] % w;
            graphics.points.push_back({
                x[i][0], x[i][1],
                std::max(-delta[0], w / 100.0f), delta[1]
            });
        }
        if (!graphics.valid()) {
            break;
        }
        std::swap(x, p);
        // MSD = std::sqrt(MSD);
        MSD /= x.size();
        V = std::sqrt(V);
        V /= x.size();
        for (size_t i = 1; i < rdf.size(); ++i) {
            rdf[i] /= i;
        }
        while (clear && MSDt.size() > clear) MSDt.pop_front();
        MSDt.push_back(MSD);
        float a, b;
        std::tie(a, b) = mnk(MSDt, k);
        auto const D = b / 2 / 2 / t;
        auto const fps = graphics.step(v, rdf, E, LJ, a, b);
        using namespace std::literals::chrono_literals;
        std::fill(f.begin(), f.end(), point { });
        e.push_back(E);
        if (fps) {
            std::cout
                << std::right << std::setw(Align) << k << std::left
                << ") K: "    << std::setw(Align) << E
                << " P: "     << std::setw(Align) << LJ
                << " E: "     << std::setw(Align) << (LJ + E)
                // << " V: "     << std::setw(Align) << V
                << " p: "     << std::setw(10) << (std::sqrt(mag2(P)) / x.size())
                // << " MSD: "   << std::setw(Align) << MSD
                // << " fps: "   << std::setw(Align) << fps
                << " l: "     << std::setw(Align) << l
                << " l2: "    << std::setw(Align) << (2 * D / V)
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
