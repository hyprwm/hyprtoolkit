#ifdef HYPRTOOLKIT_LUA_ENABLED

#include "bindings/lua/LuaBindings.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <script.lua>" << std::endl;
        return 1;
    }

    auto luaState = Hyprtoolkit::Lua::createLuaState();

    auto result = luaState->doFile(argv[1]);
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "Lua error: " << err.what() << std::endl;
        return 1;
    }

    return 0;
}

#else

#include <iostream>

int main() {
    std::cerr << "Lua support not enabled. Rebuild with -DHYPRTOOLKIT_WITH_LUA=ON" << std::endl;
    return 1;
}

#endif
