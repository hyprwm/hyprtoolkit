#include "Env.hpp"

#include <cstdlib>
#include <string_view>

using namespace Hyprtoolkit;
using namespace Hyprtoolkit::Env;

bool Hyprtoolkit::Env::envEnabled(const std::string& env) {
    auto ret = getenv(env.c_str());
    if (!ret)
        return false;

    const std::string_view sv = ret;

    return !sv.empty() && sv != "0";
}

bool Hyprtoolkit::Env::isTrace() {
    static bool TRACE = envEnabled("HT_TRACE");
    return TRACE;
}