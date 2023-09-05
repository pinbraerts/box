#pragma once

#include "point.hpp"
#include <chrono>
#include <deque>

struct ID2D1Factory;
struct ID2D1HwndRenderTarget;
struct ID2D1LinearGradientBrush;
struct ID2D1GradientStopCollection;
struct ID2D1StrokeStyle;
struct ID2D1SolidColorBrush;
struct ID2D1Brush;

class Graphics {

private:
    void* window = nullptr;
    ID2D1Factory* factory = nullptr;
    ID2D1HwndRenderTarget* target = nullptr;
    ID2D1SolidColorBrush* blue = nullptr;
    ID2D1SolidColorBrush* red = nullptr;
    ID2D1SolidColorBrush* green = nullptr;
    ID2D1GradientStopCollection* stops = nullptr;

    std::chrono::steady_clock::time_point last_frame;
    std::chrono::milliseconds frame_time;
    std::chrono::milliseconds sleep_time;
    bool wait;
    unsigned clear;
    float max_energy = 0;
    std::deque<float> potential;
    std::deque<float> kinetic;
    std::deque<float> energy;
    std::deque<float> const& MSD;
    std::deque<histogram> velocities;
    histogram velocity;
    histogram maxwell;
    histogram rdf;
    point impulse;
    float w;
    float t;
    float a;
    float b;
    unsigned steps = 0;

    template<class T>
    inline bool plot(T const& data, float x, float y, float w, float h, float scale, ID2D1Brush* brush);

public:
    std::vector<point> points;
    bool create();
    void release();
    bool resize(unsigned width, unsigned height);
    bool render();

    bool valid();
    float step(
        histogram const& velocity_, 
        histogram const& rdf_,
        float energy_,
        float lj_,
        float a_,
        float b_
    );
    float invalidate();

    Graphics(float width_, float fps_, float t_, unsigned clear_, std::deque<float> const& MSD_);
    ~Graphics();
};
