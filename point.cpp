#include "point.hpp"

float box(float p, float w) {
    if (p < -w / 2) {
        return p - std::floor(p / w) * w;
    }
    else if (p >= w / 2) {
        return p - std::ceil(p / w) * w;
    }
    else {
        return p;
    }
}

point operator%(point x, float w) {
    return {
        box(x[0], w * 2),
        box(x[1], w * 2),
        box(x[2], w * 2),
        box(x[3], w * 2),
    };
}

point operator+(point x, point y) {
    return {
        x[0] + y[0],
        x[1] + y[1],
        x[2] + y[2],
        x[3] + y[3],
    };
}

point operator-(point x, point y) {
    return {
        x[0] - y[0],
        x[1] - y[1],
        x[2] - y[2],
        x[3] - y[3],
    };
}

point operator*(float y, point x) {
    return {
        x[0] * y,
        x[1] * y,
        x[2] * y,
        x[3] * y,
    };
}

float mag2(point x) {
    return x[0] * x[0] + x[1] * x[1] + x[2] * x[2];
}

bool hist(histogram& hist, float start, float end, float value) {
    auto const bin = static_cast<std::size_t>((value - start) * hist.size() / (end - start));
    if (value < start || bin >= hist.size()) {
        return false;
    }
    hist[bin] += 1;
    return true;
}

