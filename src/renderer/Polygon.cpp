#include "Polygon.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

CPolygon::CPolygon(std::vector<Hyprutils::Math::Vector2D> points) : m_points(points) {
    ;
}

CPolygon CPolygon::checkmark() {
    return CPolygon{std::vector<Vector2D>{
        {0.12F, 0.55F},
        {0.25F, 0.39F},
        {0.4F, 0.82F},
        {0.4F, 0.57F},
        {0.9F, 0.32F},
        {0.78F, 0.17F},
    }};
}

CPolygon CPolygon::rangle() {
    return CPolygon{std::vector<Vector2D>{
        {0.25F, 0.15F},
        {0.35F, 0.05F},
        {0.6F, 0.5F},
        {0.8F, 0.5F},
        {0.25F, 0.85F},
        {0.35F, 0.95F},
    }};
}

CPolygon CPolygon::langle() {
    auto p = rangle();
    for (auto& v : p.m_points) {
        v.x = 1.F - v.x;
    }
    return p;
}
