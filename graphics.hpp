#pragma once

#include "point.hpp"
#include <chrono>

struct ID2D1Factory;
struct ID2D1HwndRenderTarget;
struct ID2D1LinearGradientBrush;
struct ID2D1GradientStopCollection;
struct ID2D1StrokeStyle;

class Graphics {

private:
    void* window = nullptr;
    ID2D1Factory* factory = nullptr;
    ID2D1HwndRenderTarget* target = nullptr;
    ID2D1LinearGradientBrush* brush = nullptr;
    ID2D1GradientStopCollection* stops = nullptr;

    std::chrono::steady_clock::time_point last_frame;
    std::chrono::milliseconds frame_time;
    std::chrono::milliseconds sleep_time;
    bool wait;

public:
    bool create();
    void release();
    bool resize(unsigned width, unsigned height);
    bool render();

    bool valid();
    bool step();
    float invalidate();

    std::vector<point> points;
    histogram velocity;

    Graphics(unsigned width, float fps);
    ~Graphics();
};
