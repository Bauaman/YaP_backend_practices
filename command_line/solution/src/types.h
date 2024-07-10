#pragma once

#include <functional>

#include "tagged.h"

namespace model {

using Dimention = int;
using Coord = Dimention;

struct Point {
    Coord x, y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    bool operator<(const Point& other) const {
        if (x == other.x) {
            return y < other.y;
        }
        if (y == other.y) {
            return x < other.x;
        }
        return (x < other.x && y < other.y);
    }
};

struct PointHash {
    std::size_t operator()(const Point& p) const {
        std::size_t h1 = std::hash<Coord>{}(p.x);
        std::size_t h2 = std::hash<Coord>{}(p.y);
        return h1 ^ (h2 << 1);
    }
};

struct Size {
    Dimention width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimention dx, dy;
};

template <typename T>
struct ParamPair {
    ParamPair() = default;

    ParamPair(const T& x, const T& y) :
        x_(x),
        y_(y) {}

    T x_;
    T y_;
};

template <typename T>
ParamPair<T> operator*(const ParamPair<T> pp_val, const T& scalar) {
    return {pp_val.x_ * scalar, pp_val.y_ * scalar};
}

template <typename T>
ParamPair<T> operator+(const ParamPair<T> pp_left, const ParamPair<T> pp_right) {
    return {pp_left.x_ + pp_right.x_, pp_left.y_ + pp_right.y_};
}

using ParamPairInt = ParamPair<int>;
using ParamPairDouble = ParamPair<double>;

struct RoadArea {
    ParamPairDouble left_bottom;
    ParamPairDouble right_top;
};

}