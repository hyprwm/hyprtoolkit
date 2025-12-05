#ifdef HYPRTOOLKIT_LUA_ENABLED

#include <sol/sol.hpp>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/core/Output.hpp>
#include <hyprtoolkit/system/Icons.hpp>

#include "../helpers/SmartPtrAdapter.hpp"
#include "../helpers/CallbackAdapter.hpp"

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;

namespace Hyprtoolkit::Lua {

void registerTimer(sol::state& lua) {
    lua.new_usertype<CTimer>("CTimer",
        sol::no_constructor,
        "cancel", &CTimer::cancel,
        "passed", &CTimer::passed,
        "canForceUpdate", &CTimer::canForceUpdate,
        "leftMs", &CTimer::leftMs,
        "cancelled", &CTimer::cancelled,
        "updateTimeout", [](CTimer* self, double timeoutMs) {
            self->updateTimeout(std::chrono::milliseconds(static_cast<int64_t>(timeoutMs)));
        }
    );
}

void registerOutput(sol::state& lua) {
    lua.new_usertype<IOutput>("IOutput",
        sol::no_constructor,
        "handle", &IOutput::handle,
        "port", &IOutput::port,
        "desc", &IOutput::desc,
        "fps", &IOutput::fps
        // Note: m_events.removed signal would need special handling
    );
}

void registerSystemIcons(sol::state& lua) {
    lua.new_usertype<ISystemIconDescription>("ISystemIconDescription",
        sol::no_constructor,
        "exists", &ISystemIconDescription::exists,
        "scalable", &ISystemIconDescription::scalable
    );

    lua.new_usertype<ISystemIconFactory>("ISystemIconFactory",
        sol::no_constructor,
        "lookupIcon", &ISystemIconFactory::lookupIcon
    );
}

void registerBackend(sol::state& lua) {
    // Backend creation data
    lua.new_usertype<IBackend::SBackendCreationData>("BackendCreationData",
        sol::constructors<IBackend::SBackendCreationData()>()
    );

    lua.new_usertype<IBackend>("IBackend",
        sol::no_constructor,

        // Static factory methods
        "create", &IBackend::create,
        "createWithData", &IBackend::createWithData,

        // Instance methods
        "destroy", &IBackend::destroy,
        "enterLoop", &IBackend::enterLoop,
        "getPalette", &IBackend::getPalette,
        "systemIcons", &IBackend::systemIcons,
        "getOutputs", &IBackend::getOutputs,

        // Timer with Lua callback (timeout in milliseconds)
        "addTimer", [](CSharedPointer<IBackend> self, double timeoutMs, sol::function callback) {
            auto duration = std::chrono::milliseconds(static_cast<int64_t>(timeoutMs));
            return self->addTimer(
                duration,
                [callback](CAtomicSharedPointer<CTimer> timer, void*) {
                    sol::protected_function_result result = callback(timer);
                    if (!result.valid()) {
                        sol::error err = result;
                        fprintf(stderr, "[Lua] Timer callback error: %s\n", err.what());
                    }
                },
                nullptr,
                false
            );
        },

        // Idle with Lua callback
        "addIdle", [](CSharedPointer<IBackend> self, sol::function callback) {
            self->addIdle([callback]() {
                sol::protected_function_result result = callback();
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Idle callback error: %s\n", err.what());
                }
            });
        },

        // File descriptor callbacks
        "addFd", [](CSharedPointer<IBackend> self, int fd, sol::function callback) {
            self->addFd(fd, [callback]() {
                sol::protected_function_result result = callback();
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Fd callback error: %s\n", err.what());
                }
            });
        },
        "removeFd", &IBackend::removeFd

        // Note: Events (outputAdded) would need signal binding infrastructure
    );
}

// Main registration function for all core types
void registerCore(sol::state& lua) {
    registerTimer(lua);
    registerOutput(lua);
    registerSystemIcons(lua);
    registerBackend(lua);
}

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
