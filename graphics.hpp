#pragma once

#include "point.hpp"
#include <d2d1.h>

struct ID2D1Factory;
struct ID2D1HwndRenderTarget;
struct ID2D1LinearGradientBrush;
struct ID2D1GradietnStopCollection;
struct ID2D1StrokeStyle;

class Graphics {

private:
    void* window = nullptr;
    ID2D1Factory* factory = nullptr;
    ID2D1HwndRenderTarget* target = nullptr;
    ID2D1LinearGradientBrush* brush = nullptr;
    ID2D1GradientStopCollection* stops = nullptr;

public:
    bool create();
    void release();
    bool resize(unsigned width, unsigned height);
    bool render();

    bool valid();
    bool step();

    std::vector<point> points;
    histogram velocity;

    Graphics(unsigned width);
    ~Graphics();
};
