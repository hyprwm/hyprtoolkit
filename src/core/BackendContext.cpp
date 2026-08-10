#include "BackendContext.hpp"

#include <hyprtoolkit/core/Output.hpp>

#include <atomic>

using namespace Hyprtoolkit;

SBackendLifetime::SBackendLifetime() :
    generation([] {
        static std::atomic<uint64_t> next = 1;
        return next.fetch_add(1, std::memory_order_relaxed);
    }()) {
    ;
}

class CNullClipboard final : public IClipboard {
  public:
    void setText(const std::string& text) override {
        ;
    }

    std::string getText() override {
        return {};
    }
};

class CNullTextInput final : public ITextInput {
  public:
    void activate(EmbeddedSurfaceID surface, const Hyprutils::Math::CBox& cursorBox, const std::string& surroundingText, size_t cursor) override {
        ;
    }

    void deactivate(EmbeddedSurfaceID surface) override {
        ;
    }
};

class CNullCursor final : public ICursor {
  public:
    void setShape(EmbeddedSurfaceID surface, ePointerShape shape) override {
        ;
    }
};

class CEmptyOutputProvider final : public IOutputProvider {
  public:
    std::vector<SP<IOutput>> outputs() override {
        return {};
    }
};

class CUnsupportedSessionLockProvider final : public ISessionLockProvider {
  public:
    std::expected<SP<ISessionLockState>, eSessionLockError> acquire() override {
        return std::unexpected(LOCK_ERROR_PLATFORM_UNINITIALIZED);
    }
};

SBackendServices Hyprtoolkit::makeDefaultBackendServices() {
    return SBackendServices{
        .clipboard   = makeShared<CNullClipboard>(),
        .textInput   = makeShared<CNullTextInput>(),
        .cursor      = makeShared<CNullCursor>(),
        .outputs     = makeShared<CEmptyOutputProvider>(),
        .sessionLock = makeShared<CUnsupportedSessionLockProvider>(),
    };
}
