#include <hyprtoolkit/palette/Color.hpp>
#include "../Macros.hpp"
#include "../helpers/Memory.hpp"
#include <hyprutils/memory/Casts.hpp>

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

#define ALPHA(c) ((double)(((c) >> 24) & 0xff) / 255.0)
#define RED(c)   ((double)(((c) >> 16) & 0xff) / 255.0)
#define GREEN(c) ((double)(((c) >> 8) & 0xff) / 255.0)
#define BLUE(c)  ((double)(((c)) & 0xff) / 255.0)

CHyprColor::CHyprColor() {
    ;
}

CHyprColor::CHyprColor(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_) {
    okLab = Hyprgraphics::CColor(Hyprgraphics::CColor::SSRGB{.r = r, .g = g, .b = b}).asOkLab();
}

CHyprColor::CHyprColor(uint64_t hex) : r(RED(hex)), g(GREEN(hex)), b(BLUE(hex)), a(ALPHA(hex)) {
    okLab = Hyprgraphics::CColor(Hyprgraphics::CColor::SSRGB{.r = r, .g = g, .b = b}).asOkLab();
}

CHyprColor::CHyprColor(const Hyprgraphics::CColor& color, float a_) : a(a_) {
    const auto SRGB = color.asRgb();
    r               = SRGB.r;
    g               = SRGB.g;
    b               = SRGB.b;

    okLab = color.asOkLab();
}

bool CHyprColor::operator==(const CHyprColor& c2) const {
    return c2.r == r && c2.g == g && c2.b == b && c2.a == a;
}

CHyprColor CHyprColor::operator-(const CHyprColor& c2) const {
    RASSERT(false, "CHyprColor: - is a STUB");
    return {};
}

CHyprColor CHyprColor::operator+(const CHyprColor& c2) const {
    RASSERT(false, "CHyprColor: + is a STUB");
    return {};
}

CHyprColor CHyprColor::operator*(const float& c2) const {
    RASSERT(false, "CHyprColor: * is a STUB");
    return {};
}

uint32_t CHyprColor::getAsHex() const {
    return ((uint32_t)(a * 255.f) * 0x1000000) + ((uint32_t)(r * 255.f) * 0x10000) + ((uint32_t)(g * 255.f) * 0x100) + ((uint32_t)(b * 255.f) * 0x1);
}

Hyprgraphics::CColor::SSRGB CHyprColor::asRGB() const {
    return {.r = r, .g = g, .b = b};
}

Hyprgraphics::CColor::SOkLab CHyprColor::asOkLab() const {
    return okLab;
}

Hyprgraphics::CColor::SHSL CHyprColor::asHSL() const {
    return Hyprgraphics::CColor(okLab).asHSL();
}

CHyprColor CHyprColor::stripA() const {
    return {sc<float>(r), sc<float>(g), sc<float>(b), 1.F};
}

CHyprColor CHyprColor::brighten(float coeff) const {
    return {sc<float>(r * (1.F + coeff)), sc<float>(g * (1.F + coeff)), sc<float>(b * (1.F + coeff)), sc<float>(a)};
}

CHyprColor CHyprColor::darken(float coeff) const {
    return {sc<float>(r * (1.F - coeff)), sc<float>(g * (1.F - coeff)), sc<float>(b * (1.F - coeff)), sc<float>(a)};
}

CHyprColor CHyprColor::mix(const CHyprColor& with, float coeff) const {
    const auto C = std::clamp(coeff, 0.F, 1.F);

    const auto A = asOkLab();
    const auto B = with.asOkLab();

    return {
        Hyprgraphics::CColor::SOkLab{
            .l = (A.l * (1.F - C)) + (B.l * C),
            .a = (A.a * (1.F - C)) + (B.a * C),
            .b = (A.b * (1.F - C)) + (B.b * C),
        },
        sc<float>((a * (1.F - C)) + (with.a * C)),
    };
}
