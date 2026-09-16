#include "OpenGL.hpp"

#include "../../window/ToolkitWindow.hpp"
#include "../../Macros.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../core/BackendContext.hpp"
#include "../../element/Element.hpp"
#include "../sync/SyncTimeline.hpp"
#include "./shaders/Shaders.hpp"
#include "GLTexture.hpp"
#include "Renderbuffer.hpp"
#include "Sync.hpp"

#include <cmath>
#include <fstream>
#include <sys/stat.h>
#include <hyprutils/memory/Casts.hpp>
#include <hyprutils/string/String.hpp>

#include <xf86drm.h>

#include <algorithm>
#include <cstring>

using namespace Hyprtoolkit;
using namespace Hyprutils::OS;
using namespace Hyprutils::String;

inline const float fullVerts[] = {
    1, 0, // top right
    0, 0, // top left
    1, 1, // bottom right
    0, 1, // bottom left
};

static enum Hyprtoolkit::eLogLevel eglLogToLevel(EGLint type) {
    switch (type) {
        case EGL_DEBUG_MSG_CRITICAL_KHR: return HT_LOG_CRITICAL;
        case EGL_DEBUG_MSG_ERROR_KHR: return HT_LOG_ERROR;
        case EGL_DEBUG_MSG_WARN_KHR: return HT_LOG_WARNING;
        case EGL_DEBUG_MSG_INFO_KHR: return HT_LOG_DEBUG;
        default: return HT_LOG_DEBUG;
    }
}

static const char* eglErrorToString(EGLint error) {
    switch (error) {
        case EGL_SUCCESS: return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
        case EGL_BAD_DEVICE_EXT: return "EGL_BAD_DEVICE_EXT";
        case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
    }
    return "Unknown";
}

static void eglLog(EGLenum error, const char* command, EGLint type, EGLLabelKHR thread, EGLLabelKHR obj, const char* msg) {
    g_logger->log(eglLogToLevel(type), "[EGL] Command {} errored out with {} (0x{}): {}", command, eglErrorToString(error), error, msg);
}

static GLuint compileShader(const GLuint& type, std::string src) {
    auto shader = glCreateShader(type);

    auto shaderSource = src.c_str();

    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

    RASSERT(ok != GL_FALSE, "compileShader() failed! GL_COMPILE_STATUS not OK!");

    return shader;
}

static GLuint createProgram(const std::string& vert, const std::string& frag) {
    auto vertCompiled = compileShader(GL_VERTEX_SHADER, vert);

    RASSERT(vertCompiled, "Compiling shader failed. VERTEX NULL! Shader source:\n\n{}", vert);

    auto fragCompiled = compileShader(GL_FRAGMENT_SHADER, frag);

    RASSERT(fragCompiled, "Compiling shader failed. FRAGMENT NULL! Shader source:\n\n{}", frag);

    auto prog = glCreateProgram();
    glAttachShader(prog, vertCompiled);
    glAttachShader(prog, fragCompiled);
    glLinkProgram(prog);

    glDetachShader(prog, vertCompiled);
    glDetachShader(prog, fragCompiled);
    glDeleteShader(vertCompiled);
    glDeleteShader(fragCompiled);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);

    RASSERT(ok != GL_FALSE, "createProgram() failed! GL_LINK_STATUS not OK!");

    return prog;
}

// Non-fatal variants of compileShader()/createProgram() for untrusted, caller-supplied
// transition shaders (hyprpaper config/IPC). On failure they log nothing themselves but
// fill `err` (including the GLSL info log) and delete every GL object created so far.
static bool tryCompileShader(const GLuint& type, const std::string& src, GLuint& outShader, std::string& err) {
    auto shader = glCreateShader(type);

    auto shaderSource = src.c_str();

    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

    if (ok == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(sc<size_t>(logLen > 0 ? logLen : 1), '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        err = std::format("shader compilation failed: {}", log);
        glDeleteShader(shader);
        return false;
    }

    outShader = shader;
    return true;
}

static bool tryCreateProgram(const std::string& vert, const std::string& frag, GLuint& outProg, std::string& err) {
    GLuint vertCompiled = 0;
    if (!tryCompileShader(GL_VERTEX_SHADER, vert, vertCompiled, err))
        return false;

    GLuint fragCompiled = 0;
    if (!tryCompileShader(GL_FRAGMENT_SHADER, frag, fragCompiled, err)) {
        glDeleteShader(vertCompiled);
        return false;
    }

    auto prog = glCreateProgram();
    glAttachShader(prog, vertCompiled);
    glAttachShader(prog, fragCompiled);
    glLinkProgram(prog);

    glDetachShader(prog, vertCompiled);
    glDetachShader(prog, fragCompiled);
    glDeleteShader(vertCompiled);
    glDeleteShader(fragCompiled);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);

    if (ok == GL_FALSE) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(sc<size_t>(logLen > 0 ? logLen : 1), '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, log.data());
        err = std::format("program link failed: {}", log);
        glDeleteProgram(prog);
        return false;
    }

    outProg = prog;
    return true;
}

static void glMessageCallbackA(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    if (type != GL_DEBUG_TYPE_ERROR)
        return;
    g_logger->log(Hyprtoolkit::HT_LOG_DEBUG, "[gl] {}", message);
}

static inline void loadGLProc(void* pProc, const char* name) {
    void* proc = rc<void*>(eglGetProcAddress(name));
    if (proc == nullptr) {
        g_logger->log(Hyprtoolkit::HT_LOG_CRITICAL, "[GL] eglGetProcAddress({}) failed", name);
        abort();
    }
    *sc<void**>(pProc) = proc;
}

static int openRenderNode(int drmFd) {
    auto renderName = drmGetRenderDeviceNameFromFd(drmFd);
    if (!renderName) {
        // This can happen on split render/display platforms, fallback to
        // primary node
        renderName = drmGetPrimaryDeviceNameFromFd(drmFd);
        if (!renderName) {
            g_logger->log(Hyprtoolkit::HT_LOG_ERROR, "drmGetPrimaryDeviceNameFromFd failed");
            return -1;
        }
        g_logger->log(HT_LOG_DEBUG, "DRM dev {} has no render node, falling back to primary", renderName);

        drmVersion* render_version = drmGetVersion(drmFd);
        if (render_version && render_version->name) {
            g_logger->log(HT_LOG_DEBUG, "DRM dev versionName", render_version->name);
            if (strcmp(render_version->name, "evdi") == 0) {
                free(renderName);
                renderName = strdup("/dev/dri/card0");
            }
            drmFreeVersion(render_version);
        }
    }

    g_logger->log(HT_LOG_DEBUG, "openRenderNode got drm device {}", renderName);

    int renderFD = open(renderName, O_RDWR | O_CLOEXEC);
    if (renderFD < 0)
        g_logger->log(HT_LOG_ERROR, "openRenderNode failed to open drm device {}", renderName);

    free(renderName);
    return renderFD;
}

void COpenGLRenderer::initEGL(bool gbm) {
    std::vector<EGLint> attrs;
    if (m_exts.KHR_display_reference) {
        attrs.push_back(EGL_TRACK_REFERENCES_KHR);
        attrs.push_back(EGL_TRUE);
    }

    attrs.push_back(EGL_NONE);

    m_eglDisplay = m_proc.eglGetPlatformDisplayEXT(gbm ? EGL_PLATFORM_GBM_KHR : EGL_PLATFORM_DEVICE_EXT, gbm ? m_gbmDevice : m_eglDevice, attrs.data());
    if (m_eglDisplay == EGL_NO_DISPLAY)
        RASSERT(false, "EGL: failed to create a platform display");

    attrs.clear();

    EGLint major, minor;
    if (eglInitialize(m_eglDisplay, &major, &minor) == EGL_FALSE)
        RASSERT(false, "EGL: failed to initialize a platform display");

    const std::string EGLEXTENSIONS = eglQueryString(m_eglDisplay, EGL_EXTENSIONS);

    m_exts.IMG_context_priority               = EGLEXTENSIONS.contains("IMG_context_priority");
    m_exts.EXT_create_context_robustness      = EGLEXTENSIONS.contains("EXT_create_context_robustness");
    m_exts.EXT_image_dma_buf_import           = EGLEXTENSIONS.contains("EXT_image_dma_buf_import");
    m_exts.EXT_image_dma_buf_import_modifiers = EGLEXTENSIONS.contains("EXT_image_dma_buf_import_modifiers");
    m_exts.EGL_ANDROID_native_fence_sync_ext  = EGLEXTENSIONS.contains("EGL_ANDROID_native_fence_sync");

    if (m_exts.IMG_context_priority) {
        g_logger->log(HT_LOG_DEBUG, "EGL: IMG_context_priority supported, requesting high");
        attrs.push_back(EGL_CONTEXT_PRIORITY_LEVEL_IMG);
        attrs.push_back(EGL_CONTEXT_PRIORITY_HIGH_IMG);
    }

    if (m_exts.EXT_create_context_robustness) {
        g_logger->log(HT_LOG_DEBUG, "EGL: EXT_create_context_robustness supported, requesting lose on reset");
        attrs.push_back(EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT);
        attrs.push_back(EGL_LOSE_CONTEXT_ON_RESET_EXT);
    }

    auto attrsNoVer = attrs;

    attrs.push_back(EGL_CONTEXT_MAJOR_VERSION);
    attrs.push_back(3);
    attrs.push_back(EGL_CONTEXT_MINOR_VERSION);
    attrs.push_back(0);
    attrs.push_back(EGL_NONE);

    m_eglContext = eglCreateContext(m_eglDisplay, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, attrs.data());
    if (m_eglContext == EGL_NO_CONTEXT) {
        RASSERT(false, "EGL: failed to create a context");
    }

    if (m_exts.IMG_context_priority) {
        EGLint priority = EGL_CONTEXT_PRIORITY_MEDIUM_IMG;
        eglQueryContext(m_eglDisplay, m_eglContext, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &priority);
        if (priority != EGL_CONTEXT_PRIORITY_HIGH_IMG)
            g_logger->log(HT_LOG_ERROR, "EGL: Failed to obtain a high priority context");
        else
            g_logger->log(HT_LOG_DEBUG, "EGL: Got a high priority context");
    }

    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext);
}

EGLImageKHR COpenGLRenderer::createEGLImage(const Aquamarine::SDMABUFAttrs& attrs) {
    std::array<uint32_t, 50> attribs;
    size_t                   idx = 0;

    attribs[idx++] = EGL_WIDTH;
    attribs[idx++] = attrs.size.x;
    attribs[idx++] = EGL_HEIGHT;
    attribs[idx++] = attrs.size.y;
    attribs[idx++] = EGL_LINUX_DRM_FOURCC_EXT;
    attribs[idx++] = attrs.format;

    struct {
        EGLint fd;
        EGLint offset;
        EGLint pitch;
        EGLint modlo;
        EGLint modhi;
    } attrNames[4] = {
        {EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE0_OFFSET_EXT, EGL_DMA_BUF_PLANE0_PITCH_EXT, EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT},
        {EGL_DMA_BUF_PLANE1_FD_EXT, EGL_DMA_BUF_PLANE1_OFFSET_EXT, EGL_DMA_BUF_PLANE1_PITCH_EXT, EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT},
        {EGL_DMA_BUF_PLANE2_FD_EXT, EGL_DMA_BUF_PLANE2_OFFSET_EXT, EGL_DMA_BUF_PLANE2_PITCH_EXT, EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT},
        {EGL_DMA_BUF_PLANE3_FD_EXT, EGL_DMA_BUF_PLANE3_OFFSET_EXT, EGL_DMA_BUF_PLANE3_PITCH_EXT, EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT}};

    for (int i = 0; i < attrs.planes; ++i) {
        attribs[idx++] = attrNames[i].fd;
        attribs[idx++] = attrs.fds[i];
        attribs[idx++] = attrNames[i].offset;
        attribs[idx++] = attrs.offsets[i];
        attribs[idx++] = attrNames[i].pitch;
        attribs[idx++] = attrs.strides[i];

        if (m_hasModifiers && attrs.modifier != DRM_FORMAT_MOD_INVALID) {
            attribs[idx++] = attrNames[i].modlo;
            attribs[idx++] = sc<uint32_t>(attrs.modifier & 0xFFFFFFFF);
            attribs[idx++] = attrNames[i].modhi;
            attribs[idx++] = sc<uint32_t>(attrs.modifier >> 32);
        }
    }

    attribs[idx++] = EGL_IMAGE_PRESERVED_KHR;
    attribs[idx++] = EGL_TRUE;
    attribs[idx++] = EGL_NONE;

    RASSERT(idx <= attribs.size(), "createEglImage: attribs array out of bounds.");

    EGLImageKHR image = m_proc.eglCreateImageKHR(m_eglDisplay, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, rc<int*>(attribs.data()));
    if (image == EGL_NO_IMAGE_KHR) {
        g_logger->log(HT_LOG_ERROR, "EGL: EGLCreateImageKHR failed: {}", eglGetError());
        return EGL_NO_IMAGE_KHR;
    }

    return image;
}

static bool drmDeviceHasName(const drmDevice* device, const std::string& name) {
    for (size_t i = 0; i < DRM_NODE_MAX; i++) {
        if (!(device->available_nodes & (1 << i)))
            continue;

        if (device->nodes[i] == name)
            return true;
    }
    return false;
}

EGLDeviceEXT COpenGLRenderer::eglDeviceFromDRMFD(int drmFD) {
    EGLint nDevices = 0;
    if (!m_proc.eglQueryDevicesEXT(0, nullptr, &nDevices)) {
        g_logger->log(HT_LOG_ERROR, "eglDeviceFromDRMFD: eglQueryDevicesEXT failed");
        return EGL_NO_DEVICE_EXT;
    }

    if (nDevices <= 0) {
        g_logger->log(HT_LOG_ERROR, "eglDeviceFromDRMFD: no devices");
        return EGL_NO_DEVICE_EXT;
    }

    std::vector<EGLDeviceEXT> devices;
    devices.resize(nDevices);

    if (!m_proc.eglQueryDevicesEXT(nDevices, devices.data(), &nDevices)) {
        g_logger->log(HT_LOG_ERROR, "eglDeviceFromDRMFD: eglQueryDevicesEXT failed (2)");
        return EGL_NO_DEVICE_EXT;
    }

    drmDevice* drmDev = nullptr;
    if (int ret = drmGetDevice(drmFD, &drmDev); ret < 0) {
        g_logger->log(HT_LOG_ERROR, "eglDeviceFromDRMFD: drmGetDevice failed");
        return EGL_NO_DEVICE_EXT;
    }

    for (auto const& d : devices) {
        auto devName = m_proc.eglQueryDeviceStringEXT(d, EGL_DRM_DEVICE_FILE_EXT);
        if (!devName)
            continue;

        if (drmDeviceHasName(drmDev, devName)) {
            g_logger->log(HT_LOG_DEBUG, "eglDeviceFromDRMFD: Using device {}", devName);
            drmFreeDevice(&drmDev);
            return d;
        }
    }

    drmFreeDevice(&drmDev);
    g_logger->log(HT_LOG_DEBUG, "eglDeviceFromDRMFD: No drm devices found");
    return EGL_NO_DEVICE_EXT;
}

void COpenGLRenderer::makeEGLCurrent() {
    if (m_borrowedContext) {
        RASSERT(eglGetCurrentContext() == m_eglContext, "Embedded GL renderer used without its borrowed context current");
        return;
    }

    if (eglGetCurrentContext() != m_eglContext)
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext);
}

void COpenGLRenderer::unsetEGL() {
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

static std::string loadShader(const std::string& filename) {
    if (SHADERS.contains(filename))
        return SHADERS.at(filename);
    throw std::runtime_error(std::format("Couldn't load shader {}", filename));
}

static void loadShaderInclude(const std::string& filename, std::map<std::string, std::string>& includes) {
    includes.insert({filename, loadShader(filename)});
}

static void processShaderIncludes(std::string& source, const std::map<std::string, std::string>& includes) {
    for (auto it = includes.begin(); it != includes.end(); ++it) {
        replaceInString(source, "#include \"" + it->first + "\"", it->second);
    }
}

static std::string processShader(const std::string& filename, const std::map<std::string, std::string>& includes) {
    auto source = loadShader(filename);
    processShaderIncludes(source, includes);
    return source;
}

void COpenGLRenderer::initGLResources() {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);

    auto* const EXTENSIONS = rc<const char*>(glGetString(GL_EXTENSIONS));
    RASSERT(EXTENSIONS, "Couldn't retrieve openGL extensions!");

    std::map<std::string, std::string> includes;
    loadShaderInclude("rounding.glsl", includes);
    loadShaderInclude("CM.glsl", includes);

    const auto VERTSRC        = processShader("tex300.vert", includes);
    const auto FRAGBORDER1    = processShader("border.frag", includes);
    const auto QUADFRAGSRC    = processShader("quad.frag", includes);
    const auto TEXFRAGSRCRGBA = processShader("rgba.frag", includes);

    GLuint     prog            = createProgram(VERTSRC, QUADFRAGSRC);
    m_rectShader.program       = prog;
    m_rectShader.proj          = glGetUniformLocation(prog, "proj");
    m_rectShader.color         = glGetUniformLocation(prog, "color");
    m_rectShader.posAttrib     = glGetAttribLocation(prog, "pos");
    m_rectShader.topLeft       = glGetUniformLocation(prog, "topLeft");
    m_rectShader.fullSize      = glGetUniformLocation(prog, "fullSize");
    m_rectShader.radius        = glGetUniformLocation(prog, "radius");
    m_rectShader.roundingPower = glGetUniformLocation(prog, "roundingPower");

    prog                          = createProgram(VERTSRC, TEXFRAGSRCRGBA);
    m_texShader.program           = prog;
    m_texShader.proj              = glGetUniformLocation(prog, "proj");
    m_texShader.tex               = glGetUniformLocation(prog, "tex");
    m_texShader.alphaMatte        = glGetUniformLocation(prog, "texMatte");
    m_texShader.alpha             = glGetUniformLocation(prog, "alpha");
    m_texShader.texAttrib         = glGetAttribLocation(prog, "texcoord");
    m_texShader.matteTexAttrib    = glGetAttribLocation(prog, "texcoordMatte");
    m_texShader.posAttrib         = glGetAttribLocation(prog, "pos");
    m_texShader.discardOpaque     = glGetUniformLocation(prog, "discardOpaque");
    m_texShader.discardAlpha      = glGetUniformLocation(prog, "discardAlpha");
    m_texShader.discardAlphaValue = glGetUniformLocation(prog, "discardAlphaValue");
    m_texShader.topLeft           = glGetUniformLocation(prog, "topLeft");
    m_texShader.fullSize          = glGetUniformLocation(prog, "fullSize");
    m_texShader.radius            = glGetUniformLocation(prog, "radius");
    m_texShader.applyTint         = glGetUniformLocation(prog, "applyTint");
    m_texShader.tint              = glGetUniformLocation(prog, "tint");
    m_texShader.useAlphaMatte     = glGetUniformLocation(prog, "useAlphaMatte");
    m_texShader.roundingPower     = glGetUniformLocation(prog, "roundingPower");

    prog                                 = createProgram(VERTSRC, FRAGBORDER1);
    m_borderShader.program               = prog;
    m_borderShader.proj                  = glGetUniformLocation(prog, "proj");
    m_borderShader.thick                 = glGetUniformLocation(prog, "thick");
    m_borderShader.posAttrib             = glGetAttribLocation(prog, "pos");
    m_borderShader.texAttrib             = glGetAttribLocation(prog, "texcoord");
    m_borderShader.topLeft               = glGetUniformLocation(prog, "topLeft");
    m_borderShader.bottomRight           = glGetUniformLocation(prog, "bottomRight");
    m_borderShader.fullSize              = glGetUniformLocation(prog, "fullSize");
    m_borderShader.fullSizeUntransformed = glGetUniformLocation(prog, "fullSizeUntransformed");
    m_borderShader.radius                = glGetUniformLocation(prog, "radius");
    m_borderShader.radiusOuter           = glGetUniformLocation(prog, "radiusOuter");
    m_borderShader.gradient              = glGetUniformLocation(prog, "gradient");
    m_borderShader.gradientLength        = glGetUniformLocation(prog, "gradientLength");
    m_borderShader.angle                 = glGetUniformLocation(prog, "angle");
    m_borderShader.gradient2             = glGetUniformLocation(prog, "gradient2");
    m_borderShader.gradient2Length       = glGetUniformLocation(prog, "gradient2Length");
    m_borderShader.angle2                = glGetUniformLocation(prog, "angle2");
    m_borderShader.gradientLerp          = glGetUniformLocation(prog, "gradientLerp");
    m_borderShader.alpha                 = glGetUniformLocation(prog, "alpha");
    m_borderShader.roundingPower         = glGetUniformLocation(prog, "roundingPower");

    const auto TRANSFRAGSRC = processShader("transition.frag", includes);
    prog                                 = createProgram(VERTSRC, TRANSFRAGSRC);
    m_transitionShader.program           = prog;
    m_transitionShader.proj              = glGetUniformLocation(prog, "proj");
    m_transitionShader.tex               = glGetUniformLocation(prog, "tex1");
    m_transitionShader.tex2              = glGetUniformLocation(prog, "tex2");
    m_transitionShader.progress          = glGetUniformLocation(prog, "progress");
    m_transitionShader.alpha             = glGetUniformLocation(prog, "alpha");
    m_transitionShader.posAttrib         = glGetAttribLocation(prog, "pos");
    m_transitionShader.texAttrib         = glGetAttribLocation(prog, "texcoord");
    m_transitionShader.topLeft           = glGetUniformLocation(prog, "topLeft");
    m_transitionShader.fullSize          = glGetUniformLocation(prog, "fullSize");
    m_transitionShader.radius            = glGetUniformLocation(prog, "radius");
    m_transitionShader.uDuration         = glGetUniformLocation(prog, "u_duration");
    m_transitionShader.randomPixel       = glGetUniformLocation(prog, "randomPixel");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    m_polyRenderFb = makeShared<CFramebuffer>();
}

COpenGLRenderer::COpenGLRenderer() : m_borrowedContext(true) {
    m_eglDisplay = eglGetCurrentDisplay();
    m_eglContext = eglGetCurrentContext();

    RASSERT(m_eglDisplay != EGL_NO_DISPLAY && m_eglContext != EGL_NO_CONTEXT, "Embedded GL renderer requires a current EGL GLES context");

    initGLResources();
}

COpenGLRenderer::COpenGLRenderer(int drmFD) : m_drmFD(drmFD) {
    const std::string EGLEXTENSIONS = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

    g_logger->log(HT_LOG_DEBUG, "Supported EGL global extensions: ({}) {}", std::ranges::count(EGLEXTENSIONS, ' '), EGLEXTENSIONS);

    m_exts.KHR_display_reference = EGLEXTENSIONS.contains("KHR_display_reference");

    loadGLProc(&m_proc.glEGLImageTargetRenderbufferStorageOES, "glEGLImageTargetRenderbufferStorageOES");
    loadGLProc(&m_proc.eglCreateImageKHR, "eglCreateImageKHR");
    loadGLProc(&m_proc.eglDestroyImageKHR, "eglDestroyImageKHR");
    loadGLProc(&m_proc.eglQueryDmaBufFormatsEXT, "eglQueryDmaBufFormatsEXT");
    loadGLProc(&m_proc.eglQueryDmaBufModifiersEXT, "eglQueryDmaBufModifiersEXT");
    loadGLProc(&m_proc.glEGLImageTargetTexture2DOES, "glEGLImageTargetTexture2DOES");
    loadGLProc(&m_proc.eglDebugMessageControlKHR, "eglDebugMessageControlKHR");
    loadGLProc(&m_proc.eglGetPlatformDisplayEXT, "eglGetPlatformDisplayEXT");
    loadGLProc(&m_proc.eglCreateSyncKHR, "eglCreateSyncKHR");
    loadGLProc(&m_proc.eglDestroySyncKHR, "eglDestroySyncKHR");
    loadGLProc(&m_proc.eglDupNativeFenceFDANDROID, "eglDupNativeFenceFDANDROID");
    loadGLProc(&m_proc.eglWaitSyncKHR, "eglWaitSyncKHR");

    RASSERT(m_proc.eglCreateSyncKHR, "Display driver doesn't support eglCreateSyncKHR");
    RASSERT(m_proc.eglDupNativeFenceFDANDROID, "Display driver doesn't support eglDupNativeFenceFDANDROID");
    RASSERT(m_proc.eglWaitSyncKHR, "Display driver doesn't support eglWaitSyncKHR");

    if (EGLEXTENSIONS.contains("EGL_EXT_device_base") || EGLEXTENSIONS.contains("EGL_EXT_device_enumeration"))
        loadGLProc(&m_proc.eglQueryDevicesEXT, "eglQueryDevicesEXT");

    if (EGLEXTENSIONS.contains("EGL_EXT_device_base") || EGLEXTENSIONS.contains("EGL_EXT_device_query")) {
        loadGLProc(&m_proc.eglQueryDeviceStringEXT, "eglQueryDeviceStringEXT");
        loadGLProc(&m_proc.eglQueryDisplayAttribEXT, "eglQueryDisplayAttribEXT");
    }

    if (EGLEXTENSIONS.contains("EGL_KHR_debug")) {
        loadGLProc(&m_proc.eglDebugMessageControlKHR, "eglDebugMessageControlKHR");
        static const EGLAttrib debugAttrs[] = {
            EGL_DEBUG_MSG_CRITICAL_KHR, EGL_TRUE, EGL_DEBUG_MSG_ERROR_KHR, EGL_TRUE, EGL_DEBUG_MSG_WARN_KHR, EGL_TRUE, EGL_DEBUG_MSG_INFO_KHR, EGL_TRUE, EGL_NONE,
        };
        m_proc.eglDebugMessageControlKHR(::eglLog, debugAttrs);
    }

    RASSERT(eglBindAPI(EGL_OPENGL_ES_API) != EGL_FALSE, "Couldn't bind to EGL's opengl ES API. This means your gpu driver f'd up. This is not a hyprland issue.");

    bool success = false;
    if (EGLEXTENSIONS.contains("EXT_platform_device") || !m_proc.eglQueryDevicesEXT || !m_proc.eglQueryDeviceStringEXT) {
        m_eglDevice = eglDeviceFromDRMFD(drmFD);

        if (m_eglDevice != EGL_NO_DEVICE_EXT) {
            success = true;
            initEGL(false);
        }
    }

    if (!success) {
        g_logger->log(HT_LOG_WARNING, "EGL: EXT_platform_device or EGL_EXT_device_query not supported, using gbm");
        if (EGLEXTENSIONS.contains("KHR_platform_gbm")) {
            success = true;
            m_gbmFD = CFileDescriptor{openRenderNode(drmFD)};
            if (!m_gbmFD.isValid())
                RASSERT(false, "Couldn't open a gbm fd");

            m_gbmDevice = gbm_create_device(m_gbmFD.get());
            if (!m_gbmDevice)
                RASSERT(false, "Couldn't open a gbm device");

            initEGL(true);
        }
    }

    RASSERT(success, "EGL does not support KHR_platform_gbm or EXT_platform_device, this is an issue with your gpu driver.");

    makeEGLCurrent();

#if defined(__linux__)
    auto syncObjSupport = [](auto fd) {
        if (fd < 0)
            return false;

        uint64_t cap = 0;
        int      ret = drmGetCap(fd, DRM_CAP_SYNCOBJ_TIMELINE, &cap);
        return ret == 0 && cap != 0;
    };

    m_syncobjSupported = syncObjSupport(m_drmFD);

    g_logger->log(HT_LOG_DEBUG, "DRM syncobj timeline support: {}", m_syncobjSupported ? "yes" : "no");
#else
    g_logger->log(HT_LOG_DEBUG, "DRM syncobj timeline support: no (not linux)");
#endif

#ifdef HYPRTOOLKIT_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glMessageCallbackA, nullptr);
#endif

    initGLResources();

    RASSERT(eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT), "Couldn't unset current EGL!");
}

COpenGLRenderer::~COpenGLRenderer() {
    makeEGLCurrent();
    if (m_borrowedContext)
        saveGLState();
    runQueuedGL();

    m_currentRBO.reset();
    m_rbos.clear();
    for (const auto& texture : m_textures) {
        if (texture)
            texture->releaseFromRenderer();
    }
    m_textures.clear();
    m_polyRenderFb.reset();
    m_fitFrameCache.clear(); // release cached fit-resolved transition frames while the context is current
    m_customShaders.clear();
    m_rectShader.destroy();
    m_texShader.destroy();
    m_borderShader.destroy();
    m_transitionShader.destroy();

    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);

    if (m_borrowedContext) {
        restoreGLState();
        return;
    }

    glFinish();
    unsetEGL();

    if (m_eglDisplay && m_eglContext != EGL_NO_CONTEXT)
        eglDestroyContext(m_eglDisplay, m_eglContext);

    if (m_eglDisplay)
        eglTerminate(m_eglDisplay);

    eglReleaseThread();

    if (m_gbmDevice)
        gbm_device_destroy(m_gbmDevice);
}

bool COpenGLRenderer::explicitSyncSupported() {
    return !m_borrowedContext && !Env::envEnabled("HT_NO_EXPLICIT_SYNC") && m_syncobjSupported && m_exts.EGL_ANDROID_native_fence_sync_ext;
}

int COpenGLRenderer::getMaxTextureSize() {
    return m_maxTextureSize;
}

CBox COpenGLRenderer::logicalToGL(const CBox& box, bool transform) {
    auto b = box.copy();
    b.scale(m_scale).round();
    if (transform)
        b.transform(Hyprutils::Math::HYPRUTILS_TRANSFORM_FLIPPED_180, m_currentViewport.x, m_currentViewport.y);
    return b;
}

CBox COpenGLRenderer::presentationBox(const CBox& box) const {
    return m_currentElement ? m_currentElement->impl->presentationBox(box) : box;
}

float COpenGLRenderer::presentationOpacity() const {
    return m_currentElement ? m_currentElement->impl->effectiveOpacity() : 1.F;
}

SP<CRenderbuffer> COpenGLRenderer::getRBO(SP<Aquamarine::IBuffer> buf) {
    for (const auto& r : m_rbos) {
        if (r->m_hlBuffer == buf)
            return r;
    }

    auto rbo = m_rbos.emplace_back(makeShared<CRenderbuffer>(*this, buf, buf->dmabuf().format));

    RASSERT(rbo->good(), "GL: Couldn't make a rbo for a render");

    return rbo;
}

void COpenGLRenderer::onRenderbufferDestroy(CRenderbuffer* p) {
    std::erase_if(m_rbos, [p](const auto& rbo) { return !rbo || rbo.get() == p; });
}

void COpenGLRenderer::waitOnSync() {
    auto timeline = exportSync(m_currentRBO->m_hlBuffer.lock());

    if (!timeline)
        return;

    timeline->check(timeline->m_releasePoint, DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT);
}

void COpenGLRenderer::beginRendering(SP<IToolkitWindow> window, SP<Aquamarine::IBuffer> buf) {
    RASSERT(buf, "GL: null buffer passed to rendering");

    makeEGLCurrent();

    m_currentRBO = getRBO(buf);

    m_currentRBO->bind();
    m_targetFB = m_currentRBO->getFB()->getFBID();
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    prepareRenderState();

    m_projection      = Mat3x3::outputProjection(window->pixelSize(), HYPRUTILS_TRANSFORM_FLIPPED_180);
    m_currentViewport = window->pixelSize();
    m_scale           = window->scale();
    m_window          = window;
    m_damage          = window->m_damageRing.getBufferDamage(DAMAGE_RING_PREVIOUS_LEN);
}

void COpenGLRenderer::saveGLState() {
    glGetIntegerv(GL_CURRENT_PROGRAM, &m_savedState.program);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_savedState.drawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_savedState.readFramebuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_savedState.vertexArray);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &m_savedState.arrayBuffer);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &m_savedState.pixelUnpackBuffer);
    glGetIntegerv(GL_VIEWPORT, m_savedState.viewport);
    glGetIntegerv(GL_SCISSOR_BOX, m_savedState.scissorBox);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, m_savedState.clearColor);
    glGetIntegerv(GL_BLEND_SRC_RGB, &m_savedState.blendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &m_savedState.blendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &m_savedState.blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &m_savedState.blendDstAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &m_savedState.blendEquationRGB);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &m_savedState.blendEquationAlpha);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &m_savedState.activeTexture);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &m_savedState.unpackAlignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &m_savedState.unpackRowLength);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &m_savedState.unpackSkipPixels);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &m_savedState.unpackSkipRows);
    glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &m_savedState.unpackImageHeight);
    glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &m_savedState.unpackSkipImages);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_savedState.texture2D);
    m_savedState.blend               = glIsEnabled(GL_BLEND);
    m_savedState.scissor             = glIsEnabled(GL_SCISSOR_TEST);
    m_savedState.depth               = glIsEnabled(GL_DEPTH_TEST);
    m_savedState.stencil             = glIsEnabled(GL_STENCIL_TEST);
    m_savedState.cull                = glIsEnabled(GL_CULL_FACE);
    m_savedState.rasterizerDiscard   = glIsEnabled(GL_RASTERIZER_DISCARD);
    m_savedState.sampleCoverage      = glIsEnabled(GL_SAMPLE_COVERAGE);
    m_savedState.sampleAlphaCoverage = glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glGetBooleanv(GL_COLOR_WRITEMASK, m_savedState.colorMask);

    if (m_savedState.activeTexture != GL_TEXTURE0) {
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_savedState.texture2DUnit0);
        glActiveTexture(m_savedState.activeTexture);
    } else
        m_savedState.texture2DUnit0 = m_savedState.texture2D;

    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_SAMPLER_BINDING, &m_savedState.samplerUnit0);
    glActiveTexture(m_savedState.activeTexture);
}

bool COpenGLRenderer::contextCurrent() {
    return eglGetCurrentContext() == m_eglContext;
}

void COpenGLRenderer::enqueueGL(std::function<void()>&& callback) {
    std::lock_guard<std::mutex> lock(m_queuedGLMutex);
    m_queuedGL.emplace_back(std::move(callback));
}

void COpenGLRenderer::runQueuedGL() {
    std::vector<std::function<void()>> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_queuedGLMutex);
        callbacks.swap(m_queuedGL);
    }

    for (const auto& callback : callbacks)
        callback();
}

void COpenGLRenderer::prepareRenderState() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_RASTERIZER_DISCARD);
    glDisable(GL_SAMPLE_COVERAGE);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBindSampler(0, 0);
}

void COpenGLRenderer::uploadVertices(const float* data, size_t size) {
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STREAM_DRAW);
}

void COpenGLRenderer::registerTexture(const SP<CGLTexture>& texture) {
    m_textures.emplace_back(texture);
}

void COpenGLRenderer::restoreGLState() {
    glUseProgram(m_savedState.program);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_savedState.drawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_savedState.readFramebuffer);
    glBindVertexArray(m_savedState.vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_savedState.arrayBuffer);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_savedState.pixelUnpackBuffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT, m_savedState.unpackAlignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_savedState.unpackRowLength);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, m_savedState.unpackSkipPixels);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, m_savedState.unpackSkipRows);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, m_savedState.unpackImageHeight);
    glPixelStorei(GL_UNPACK_SKIP_IMAGES, m_savedState.unpackSkipImages);
    glViewport(m_savedState.viewport[0], m_savedState.viewport[1], m_savedState.viewport[2], m_savedState.viewport[3]);
    glScissor(m_savedState.scissorBox[0], m_savedState.scissorBox[1], m_savedState.scissorBox[2], m_savedState.scissorBox[3]);
    glClearColor(m_savedState.clearColor[0], m_savedState.clearColor[1], m_savedState.clearColor[2], m_savedState.clearColor[3]);
    glBlendFuncSeparate(m_savedState.blendSrcRGB, m_savedState.blendDstRGB, m_savedState.blendSrcAlpha, m_savedState.blendDstAlpha);
    glBlendEquationSeparate(m_savedState.blendEquationRGB, m_savedState.blendEquationAlpha);
    glColorMask(m_savedState.colorMask[0], m_savedState.colorMask[1], m_savedState.colorMask[2], m_savedState.colorMask[3]);

    if (m_savedState.blend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    if (m_savedState.scissor)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);

    if (m_savedState.depth)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (m_savedState.stencil)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);

    if (m_savedState.cull)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    if (m_savedState.rasterizerDiscard)
        glEnable(GL_RASTERIZER_DISCARD);
    else
        glDisable(GL_RASTERIZER_DISCARD);

    if (m_savedState.sampleCoverage)
        glEnable(GL_SAMPLE_COVERAGE);
    else
        glDisable(GL_SAMPLE_COVERAGE);

    if (m_savedState.sampleAlphaCoverage)
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    else
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_savedState.texture2DUnit0);
    glBindSampler(0, m_savedState.samplerUnit0);
    glActiveTexture(m_savedState.activeTexture);
    glBindTexture(GL_TEXTURE_2D, m_savedState.texture2D);
}

void COpenGLRenderer::beginRenderingExternal(SP<IToolkitWindow> window, uint32_t bufferAge) {
    RASSERT(m_borrowedContext, "External framebuffer rendering requires a borrowed GL renderer");
    makeEGLCurrent();
    saveGLState();

    m_targetFB = m_savedState.drawFramebuffer;
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    runQueuedGL();
    prepareRenderState();

    m_projection      = Mat3x3::outputProjection(window->pixelSize(), HYPRUTILS_TRANSFORM_FLIPPED_180);
    m_currentViewport = window->pixelSize();
    m_scale           = window->scale();
    m_window          = window;
    m_damage          = window->m_damageRing.getBufferDamage(bufferAge);
    m_lastScissorBox  = {};
}

void COpenGLRenderer::render(bool ignoreSync) {
    if (m_damage.empty())
        return;

    if (!ignoreSync && explicitSyncSupported())
        waitOnSync();

    glViewport(0, 0, m_window->pixelSize().x, m_window->pixelSize().y);

    m_damage.forEachRect([this](const auto& RECT) {
        scissor(&RECT);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
    });

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    m_alreadyRendered.clear();
    m_currentElement.reset();

    renderBreadthfirst(m_window->m_rootElement);

    m_alreadyRendered.clear();
    m_currentElement.reset();

    glDisable(GL_BLEND);
}

void COpenGLRenderer::renderBreadthfirst(SP<IElement> e) {
    static const auto DEBUG_LAYOUT = Env::envEnabled("HT_DEBUG_LAYOUT");
    CHyprColor        DEBUG_COLOR  = {Hyprgraphics::CColor::SHSL{.h = 0.0F, .s = 0.7F, .l = 0.5F}, 0.8F};

    e->impl->breadthfirst([this, &DEBUG_COLOR](SP<IElement> el) {
        if (el->impl->failedPositioning)
            return;

        if (std::ranges::find(m_alreadyRendered, el) != m_alreadyRendered.end())
            return;

        m_currentElement = el;
        el->paint();
        el->impl->hasBeenPresented = true;

        m_alreadyRendered.emplace_back(el);

        if (DEBUG_LAYOUT) {
            auto BOX = el->impl->position.copy();
            if (BOX.w == 0)
                BOX.w = 1;
            if (BOX.h == 0)
                BOX.h = 1;

            renderBorder(SBorderRenderData{
                .box      = BOX,
                .gradient = CGradientValueData{DEBUG_COLOR},
                .thick    = 1,
            });

            auto hsl = DEBUG_COLOR.asHSL();
            hsl.h += 0.05F;
            if (hsl.h > 1.F)
                hsl.h -= 1.F;
            DEBUG_COLOR = CHyprColor{hsl, 0.8F};
        }

        if (el->impl->clipChildren) {
            // clip children: push a clip box and render all children now, then pop box
            m_clipBoxes.emplace_back(logicalToGL(el->impl->presentationBox(el->impl->position), false));

            renderBreadthfirst(el);

            m_clipBoxes.pop_back();
        }

        if (el->impl->grouped) {
            // grouped: render all children as one
            renderBreadthfirst(el);
        }
    });
}

void COpenGLRenderer::endRendering() {
    if (m_currentRBO) {
        m_currentRBO->unbind();
        m_currentRBO.reset();

        // FIXME: explicit sync for nvidia!!!!
        glFlush();
        glBindVertexArray(0);
    }

    m_window->m_damageRing.rotate();
    m_window.reset();
    m_damage.clear();

    if (m_borrowedContext)
        restoreGLState();
}

void COpenGLRenderer::scissor(const pixman_box32_t* box) {
    if (!box) {
        scissor(CBox{});
        return;
    }

    scissor(CBox{
        sc<double>(box->x1),
        sc<double>(box->y1),
        sc<double>(box->x2 - box->x1),
        sc<double>(box->y2 - box->y1),
    });
}

void COpenGLRenderer::scissor(const CBox& box) {
    if (box.empty()) {
        glDisable(GL_SCISSOR_TEST);
        return;
    }

    if (box != m_lastScissorBox) {
        glScissor(box.x, box.y, box.width, box.height);
        m_lastScissorBox = box;
    }

    glEnable(GL_SCISSOR_TEST);
}

CRegion COpenGLRenderer::damageWithClip() {
    auto dmg = m_damage.copy();

    for (const auto& cb : m_clipBoxes) {
        dmg.intersect(cb);
    }

    return dmg;
}

void COpenGLRenderer::renderRectangle(const SRectangleRenderData& data) {
    const auto PRESENTED     = presentationBox(data.box);
    const auto ROUNDEDBOX    = logicalToGL(PRESENTED);
    const auto UNTRANSFORMED = logicalToGL(PRESENTED, false);
    Mat3x3     matrix        = m_projMatrix.projectBox(ROUNDEDBOX, HYPRUTILS_TRANSFORM_FLIPPED_180, PRESENTED.rot);
    Mat3x3     glMatrix      = m_projection.copy().multiply(matrix);

    const auto DAMAGE = damageWithClip();

    if (DAMAGE.copy().intersect(UNTRANSFORMED).empty())
        return;

    glUseProgram(m_rectShader.program);

    glUniformMatrix3fv(m_rectShader.proj, 1, GL_TRUE, glMatrix.getMatrix().data());

    // premultiply the color as well as we don't work with straight alpha
    auto COL = data.color;
    COL.a *= presentationOpacity();
    glUniform4f(m_rectShader.color, COL.r * COL.a, COL.g * COL.a, COL.b * COL.a, COL.a);

    const auto TOPLEFT  = Vector2D(UNTRANSFORMED.x, UNTRANSFORMED.y);
    const auto FULLSIZE = Vector2D(UNTRANSFORMED.width, UNTRANSFORMED.height);

    // Rounded corners
    glUniform2f(m_rectShader.topLeft, sc<float>(TOPLEFT.x), sc<float>(TOPLEFT.y));
    glUniform2f(m_rectShader.fullSize, sc<float>(FULLSIZE.x), sc<float>(FULLSIZE.y));
    glUniform1f(m_rectShader.radius, data.rounding * m_scale);
    glUniform1f(m_rectShader.roundingPower, 2);

    uploadVertices(fullVerts, sizeof(fullVerts));
    glVertexAttribPointer(m_rectShader.posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(m_rectShader.posAttrib);

    DAMAGE.forEachRect([this](const auto& RECT) {
        scissor(&RECT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    });

    glDisableVertexAttribArray(m_rectShader.posAttrib);
}

SP<IRendererTexture> COpenGLRenderer::uploadTexture(const STextureData& data) {
    const auto TEX = makeShared<CGLTexture>();
    registerTexture(TEX);
    TEX->m_fitMode = data.fitMode;
    TEX->attachAsync(TEX, data.resource);
    return TEX;
}

static CBox containImage(const CBox& requested, const Vector2D& imageSize) {
    const auto SOURCE_ASPECT_RATIO = requested.w / requested.h;
    const auto IMAGE_ASPECT_RATIO  = imageSize.x / imageSize.y;

    if (SOURCE_ASPECT_RATIO > IMAGE_ASPECT_RATIO) {
        // box is wider than the target
        const auto HEIGHT = requested.h;
        const auto WIDTH  = (requested.h / imageSize.y) * imageSize.x;
        return CBox{
            requested.x + ((requested.w - WIDTH) / 2.F), //
            requested.y,                                 //
            WIDTH,                                       //
            HEIGHT,                                      //
        };
    }

    // box is taller than the target
    const auto WIDTH  = requested.w;
    const auto HEIGHT = (requested.w / imageSize.x) * imageSize.y;
    return CBox{
        requested.x,                                  //
        requested.y + ((requested.h - HEIGHT) / 2.F), //
        WIDTH,                                        //
        HEIGHT,                                       //
    };
}

static std::array<float, 8> coverImage(const CBox& requested, const Vector2D& imageSize) {
    const float SOURCE_ASPECT_RATIO = requested.w / requested.h;
    const float IMAGE_ASPECT_RATIO  = imageSize.x / imageSize.y;

    CBox        texbox{};

    if (SOURCE_ASPECT_RATIO > IMAGE_ASPECT_RATIO) {
        // box is wider than the target
        const float WIDTH  = requested.w;
        const float HEIGHT = (requested.w / imageSize.x) * imageSize.y;

        texbox = {
            requested.x,
            requested.y - ((HEIGHT - requested.h) / 2.F),
            WIDTH,
            HEIGHT,
        };
    } else {
        // box is taller than the target
        const float HEIGHT = requested.h;
        const float WIDTH  = (requested.h / imageSize.y) * imageSize.x;

        texbox = {
            requested.x - ((WIDTH - requested.w) / 2.F),
            requested.y,
            WIDTH,
            HEIGHT,
        };
    }

    // where does requested sit inside the enlarged texbox, in [0,1]?
    const float          TOP    = std::abs(requested.y - texbox.y) / texbox.h;
    const float          LEFT   = std::abs(requested.x - texbox.x) / texbox.w;
    const float          BOTTOM = TOP + (requested.h / texbox.h);
    const float          RIGHT  = LEFT + (requested.w / texbox.w);

    std::array<float, 8> verts = {
        sc<float>(RIGHT), sc<float>(TOP),    // top right
        sc<float>(LEFT),  sc<float>(TOP),    // top left
        sc<float>(RIGHT), sc<float>(BOTTOM), // bottom right
        sc<float>(LEFT),  sc<float>(BOTTOM), // bottom left
    };

    return verts;
}

static std::array<float, 8> tileImage(const CBox& requested, const Vector2D& imageSize) {

    const auto           IMAGE_AS_PERCENT = imageSize / requested.size();
    const auto           INVERSE_RATIOS   = Vector2D{1.F, 1.F} / IMAGE_AS_PERCENT;

    std::array<float, 8> verts = {
        sc<float>(INVERSE_RATIOS.x),
        sc<float>(0), // top right
        sc<float>(0),
        sc<float>(0), // top left
        sc<float>(INVERSE_RATIOS.x),
        sc<float>(INVERSE_RATIOS.y), // bottom right
        sc<float>(0),
        sc<float>(INVERSE_RATIOS.y), // bottom left
    };

    return verts;
}

void COpenGLRenderer::renderTexture(const STextureRenderData& data) {
    RASSERT(data.texture->type() == IRendererTexture::TEXTURE_GL, "OpenGL renderer: passed a non-gl texture");

    SP<CGLTexture> tex = reinterpretPointerCast<CGLTexture>(data.texture);

    // A fit-applied texture is already a fit-resolved full-screen frame (e.g. a transition
    // start or capture): sample it with the identity mapping (STRETCH), never re-fit it.
    // Otherwise a per-call fit mode overrides the texture's stored mode.
    const eImageFitMode fit = tex->m_fitApplied ? IMAGE_FIT_MODE_STRETCH : data.fitMode.value_or(data.texture->fitMode());

    const auto     SOURCE_LAYOUT = fit == IMAGE_FIT_MODE_CONTAIN ? containImage(data.box, tex->m_size) : data.box;
    const auto     SOURCE_BOX    = presentationBox(SOURCE_LAYOUT);
    const auto     ROUNDEDBOX    = logicalToGL(SOURCE_BOX);
    const auto     UNTRANSFORMED = logicalToGL(SOURCE_BOX, false);
    Mat3x3         matrix        = m_projMatrix.projectBox(ROUNDEDBOX, Hyprutils::Math::HYPRUTILS_TRANSFORM_FLIPPED_180, SOURCE_BOX.rot);
    Mat3x3         glMatrix      = m_projection.copy().multiply(matrix);

    const auto     DAMAGE = damageWithClip();

    if (DAMAGE.copy().intersect(UNTRANSFORMED).empty())
        return;

    CShader* shader = &m_texShader;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(tex->m_target, tex->m_texID);

    glUseProgram(shader->program);

    glUniformMatrix3fv(shader->proj, 1, GL_TRUE, glMatrix.getMatrix().data());
    glUniform1i(shader->tex, 0);
    glUniform1f(shader->alpha, data.a * presentationOpacity());
    const auto TOPLEFT  = Vector2D(UNTRANSFORMED.x, UNTRANSFORMED.y);
    const auto FULLSIZE = Vector2D(UNTRANSFORMED.width, UNTRANSFORMED.height);

    // Rounded corners
    glUniform2f(shader->topLeft, TOPLEFT.x, TOPLEFT.y);
    glUniform2f(shader->fullSize, FULLSIZE.x, FULLSIZE.y);
    glUniform1f(shader->radius, data.rounding * m_scale);
    glUniform1f(shader->roundingPower, 2);

    glUniform1i(shader->discardOpaque, 0);
    glUniform1i(shader->discardAlpha, 0);
    if (data.tint) {
        glUniform1i(shader->applyTint, data.tintGrayscaleOnly ? 2 : 1);
        glUniform3f(shader->tint, data.tint->r, data.tint->g, data.tint->b);
    } else
        glUniform1i(shader->applyTint, 0);

    std::array<float, 8> texVerts = {
        fullVerts[0], fullVerts[1], fullVerts[2], fullVerts[3], fullVerts[4], fullVerts[5], fullVerts[6], fullVerts[7],
    };
    if (fit == IMAGE_FIT_MODE_COVER)
        texVerts = coverImage(data.box, tex->m_size);
    else if (fit == IMAGE_FIT_MODE_TILE) {
        texVerts = tileImage(data.box, tex->m_size);
        glTexParameteri(tex->m_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(tex->m_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    std::array<float, 16> vertices;
    std::ranges::copy(fullVerts, vertices.begin());
    std::ranges::copy(texVerts, vertices.begin() + 8);
    uploadVertices(vertices.data(), sizeof(vertices));
    glVertexAttribPointer(shader->posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribPointer(shader->texAttrib, 2, GL_FLOAT, GL_FALSE, 0, rc<const void*>(sizeof(fullVerts)));

    glEnableVertexAttribArray(shader->posAttrib);
    glEnableVertexAttribArray(shader->texAttrib);

    DAMAGE.forEachRect([this](const auto& RECT) {
        scissor(&RECT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    });

    glDisableVertexAttribArray(shader->posAttrib);
    glDisableVertexAttribArray(shader->texAttrib);

    glBindTexture(tex->m_target, 0);
}

size_t COpenGLRenderer::ensureTransitionShader(const std::string& source) {
    if (source.empty())
        return 0;

    // May be invoked from the IPC handler outside the render loop.
    makeEGLCurrent();

    const auto KEY = std::hash<std::string>{}(source);

    if (m_customShaders.contains(KEY))
        return KEY;

    // Caller-supplied shader: compile non-fatally. On any failure, log (including the
    // GLSL info log) and fall back to the default transition shader.
    GLuint      prog = 0;
    std::string err;
    if (!tryCreateProgram(loadShader("tex300.vert"), source, prog, err)) {
        g_logger->log(HT_LOG_ERROR, "ensureTransitionShader: failed to compile shader program: {}", err);
        return 0;
    }

    const GLint posAttrib   = glGetAttribLocation(prog, "pos");
    const GLint texAttrib   = glGetAttribLocation(prog, "texcoord");
    const GLint proj        = glGetUniformLocation(prog, "proj");
    const GLint tex         = glGetUniformLocation(prog, "tex1");
    const GLint tex2        = glGetUniformLocation(prog, "tex2");
    const GLint progress    = glGetUniformLocation(prog, "progress");
    const GLint alpha       = glGetUniformLocation(prog, "alpha");
    const GLint topLeft     = glGetUniformLocation(prog, "topLeft");
    const GLint fullSize    = glGetUniformLocation(prog, "fullSize");
    const GLint radius      = glGetUniformLocation(prog, "radius");
    const GLint uDuration   = glGetUniformLocation(prog, "u_duration");
    const GLint randomPixel = glGetUniformLocation(prog, "randomPixel");

    // Mandatory interface validation: attribs `pos`/`texcoord` (a -1 here makes
    // glVertexAttribPointer/glEnableVertexAttribArray raise GL_INVALID_VALUE and break
    // the draw) and uniforms `proj`/`tex1`/`tex2`/`progress` (a program missing these
    // renders garbage; glUniform with -1 is a silent no-op, so validate explicitly).
    // `pos`/`proj` come from the fixed vertex stage and are always active, so their
    // check is redundant-but-harmless. Optional uniforms (`alpha`, `topLeft`,
    // `fullSize`, `radius`, `randomPixel`, `u_duration`) may be -1; glUniform on them
    // is a defined no-op.
    if (posAttrib < 0 || texAttrib < 0 || proj < 0 || tex < 0 || tex2 < 0 || progress < 0) {
        glDeleteProgram(prog);
        g_logger->log(HT_LOG_ERROR,
                      "ensureTransitionShader: shader is missing required interface (pos={}, texcoord={}, proj={}, tex1={}, tex2={}, progress={}); "
                      "the custom fragment must declare `uniform sampler2D tex1, tex2; uniform float progress;` and consume v_texcoord",
                      posAttrib, texAttrib, proj, tex, tex2, progress);
        return 0;
    }

    // Build directly into the cache so the program outlives this scope.
    auto& entry    = m_customShaders[KEY];
    auto& sh       = entry.shader;
    sh.program     = prog;
    sh.posAttrib   = posAttrib;
    sh.texAttrib   = texAttrib;
    sh.proj        = proj;
    sh.tex         = tex;
    sh.tex2        = tex2;
    sh.progress    = progress;
    sh.alpha       = alpha;
    sh.topLeft     = topLeft;
    sh.fullSize    = fullSize;
    sh.radius      = radius;
    sh.uDuration   = uDuration;
    sh.randomPixel = randomPixel;

    return KEY;
}

void COpenGLRenderer::renderTransition(const STransitionRenderData& data) {
    RASSERT(data.startTexture->type() == IRendererTexture::TEXTURE_GL, "OpenGL renderer: passed a non-gl start texture");
    RASSERT(data.endTexture->type() == IRendererTexture::TEXTURE_GL, "OpenGL renderer: passed a non-gl end texture");

    SP<CGLTexture> startTex = reinterpretPointerCast<CGLTexture>(data.startTexture);
    SP<CGLTexture> endTex   = reinterpretPointerCast<CGLTexture>(data.endTexture);

    const auto     ROUNDEDBOX    = logicalToGL(data.box);
    const auto     UNTRANSFORMED = logicalToGL(data.box, false);
    Mat3x3         matrix        = m_projMatrix.projectBox(ROUNDEDBOX, HYPRUTILS_TRANSFORM_FLIPPED_180, data.box.rot);
    Mat3x3         glMatrix      = m_projection.copy().multiply(matrix);

    const auto     DAMAGE = damageWithClip();

    if (DAMAGE.copy().intersect(UNTRANSFORMED).empty())
        return;

    // Pre-render both frames to fit-resolved offscreen FBOs (cached). The transition
    // shaders are pure crossfades, so all fit handling lives here.
    SP<CGLTexture> startFitTex = reinterpretPointerCast<CGLTexture>(renderFitFrame(startTex, data.box, data.fitMode));
    SP<CGLTexture> endFitTex   = reinterpretPointerCast<CGLTexture>(renderFitFrame(endTex, data.box, data.fitMode));
    if (!startFitTex || !endFitTex)
        return;

    CShader* activeShader = &m_transitionShader;
    if (data.shaderKey) {
        // Shader was loaded by ensureTransitionShader() at transition start; just look it up.
        auto it = m_customShaders.find(data.shaderKey);
        if (it != m_customShaders.end())
            activeShader = &it->second.shader;
    }

    glUseProgram(activeShader->program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(startFitTex->m_target, startFitTex->m_texID);
    glUniform1i(activeShader->tex, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(endFitTex->m_target, endFitTex->m_texID);
    glUniform1i(activeShader->tex2, 1);

    glUniformMatrix3fv(activeShader->proj, 1, GL_TRUE, glMatrix.getMatrix().data());
    glUniform1f(activeShader->progress, data.progress);
    glUniform1f(activeShader->alpha, data.a);

    const auto TOPLEFT  = Vector2D(UNTRANSFORMED.x, UNTRANSFORMED.y);
    const auto FULLSIZE = Vector2D(UNTRANSFORMED.width, UNTRANSFORMED.height);
    glUniform2f(activeShader->topLeft, TOPLEFT.x, TOPLEFT.y);
    glUniform2f(activeShader->fullSize, FULLSIZE.x, FULLSIZE.y);
    glUniform1f(activeShader->radius, data.rounding * m_scale);
    glUniform2f(activeShader->randomPixel, data.randomPixel.x, data.randomPixel.y);
    if (activeShader->uDuration >= 0)
        glUniform1f(activeShader->uDuration, data.duration);

    std::array<float, 16> vertices;
    std::ranges::copy(fullVerts, vertices.begin());
    std::ranges::copy(fullVerts, vertices.begin() + 8);
    uploadVertices(vertices.data(), sizeof(vertices));
    glVertexAttribPointer(activeShader->posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribPointer(activeShader->texAttrib, 2, GL_FLOAT, GL_FALSE, 0, rc<const void*>(sizeof(fullVerts)));

    glEnableVertexAttribArray(activeShader->posAttrib);
    glEnableVertexAttribArray(activeShader->texAttrib);

    DAMAGE.forEachRect([this](const auto& RECT) {
        scissor(&RECT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    });

    glDisableVertexAttribArray(activeShader->posAttrib);
    glDisableVertexAttribArray(activeShader->texAttrib);

    // Unbind textures
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(endFitTex->m_target, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(startFitTex->m_target, 0);
}

SP<IRendererTexture> COpenGLRenderer::captureTransitionState(const STransitionRenderData& data) {
    if (!data.startTexture || !data.endTexture)
        return nullptr;

    RASSERT(data.startTexture->type() == IRendererTexture::TEXTURE_GL, "OpenGL renderer: passed a non-gl start texture");
    RASSERT(data.endTexture->type() == IRendererTexture::TEXTURE_GL, "OpenGL renderer: passed a non-gl end texture");

    // Capture may be called from the IPC handler, outside the render loop.
    makeEGLCurrent();

    SP<CGLTexture> startTex = reinterpretPointerCast<CGLTexture>(data.startTexture);
    SP<CGLTexture> endTex   = reinterpretPointerCast<CGLTexture>(data.endTexture);

    // Pre-render both source frames (fit-resolved) so the captured blend is correct
    // for every fit mode; the transition shader is a pure crossfade.
    SP<CGLTexture> startFitTex = reinterpretPointerCast<CGLTexture>(renderFitFrame(startTex, data.box, data.fitMode));
    SP<CGLTexture> endFitTex   = reinterpretPointerCast<CGLTexture>(renderFitFrame(endTex, data.box, data.fitMode));
    if (!startFitTex || !endFitTex)
        return nullptr;

    int w = static_cast<int>(data.box.width * m_scale);
    int h = static_cast<int>(data.box.height * m_scale);
    if (w <= 0 || h <= 0)
        return nullptr;

    // Save GL state before alloc(): alloc() rebinds GL_FRAMEBUFFER and leaves the
    // binding at 0, so prevFb must be captured beforehand to restore it afterwards.
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    GLint prevFb = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFb);
    GLboolean prevScissor = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prevBlend   = glIsEnabled(GL_BLEND);
    GLint     prevScissorBox[4];
    glGetIntegerv(GL_SCISSOR_BOX, prevScissorBox);

    auto captureFb = makeShared<CFramebuffer>();
    if (!captureFb->alloc(w, h)) {
        g_logger->log(HT_LOG_ERROR, "captureTransitionState: failed to allocate framebuffer");
        glBindFramebuffer(GL_FRAMEBUFFER, prevFb);
        return nullptr;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, captureFb->getFBID());
    glViewport(0, 0, w, h);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    // This FBO is the *result* of the blend (not a per-image pre-render), so an
    // opaque black clear is correct — the full-box draw overwrites it.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Use the cached custom shader if any (it was loaded by a prior renderTransition).
    CShader* activeShader = &m_transitionShader;
    if (data.shaderKey) {
        auto it = m_customShaders.find(data.shaderKey);
        if (it != m_customShaders.end())
            activeShader = &it->second.shader;
        else
            g_logger->log(HT_LOG_ERROR, "captureTransitionState: custom shader not in cache: {}", data.shaderKey);
    }

    glUseProgram(activeShader->program);

    // Project a box covering the full FBO into 0..1 vertices.
    CBox captureBox{0, 0, (float)w, (float)h};
    Mat3x3 captureProj = Mat3x3::outputProjection(Vector2D{w, h}, HYPRUTILS_TRANSFORM_FLIPPED_180);
    Mat3x3 captureMat  = captureProj.projectBox(captureBox, HYPRUTILS_TRANSFORM_FLIPPED_180, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(startFitTex->m_target, startFitTex->m_texID);
    glUniform1i(activeShader->tex, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(endFitTex->m_target, endFitTex->m_texID);
    glUniform1i(activeShader->tex2, 1);

    glUniformMatrix3fv(activeShader->proj, 1, GL_TRUE, captureMat.getMatrix().data());
    glUniform1f(activeShader->progress, data.progress);
    glUniform1f(activeShader->alpha, data.a);

    const auto TOPLEFT  = Vector2D(0, 0);
    const auto FULLSIZE = Vector2D(w, h);
    glUniform2f(activeShader->topLeft, TOPLEFT.x, TOPLEFT.y);
    glUniform2f(activeShader->fullSize, FULLSIZE.x, FULLSIZE.y);
    glUniform1f(activeShader->radius, data.rounding * m_scale);
    glUniform2f(activeShader->randomPixel, data.randomPixel.x, data.randomPixel.y);
    if (activeShader->uDuration >= 0)
        glUniform1f(activeShader->uDuration, data.duration);

    GLfloat fullVerts[] = { 0, 0, 1, 0, 0, 1, 1, 1 };
    std::array<float, 16> vertices;
    std::ranges::copy(fullVerts, vertices.begin());
    std::ranges::copy(fullVerts, vertices.begin() + 8);
    uploadVertices(vertices.data(), sizeof(vertices));
    glVertexAttribPointer(activeShader->posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribPointer(activeShader->texAttrib, 2, GL_FLOAT, GL_FALSE, 0, rc<const void*>(sizeof(fullVerts)));

    glEnableVertexAttribArray(activeShader->posAttrib);
    glEnableVertexAttribArray(activeShader->texAttrib);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(activeShader->posAttrib);
    glDisableVertexAttribArray(activeShader->texAttrib);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(endFitTex->m_target, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(startFitTex->m_target, 0);

    glUseProgram(0);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    if (prevScissor) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(prevScissorBox[0], prevScissorBox[1], prevScissorBox[2], prevScissorBox[3]);
    } else
        glDisable(GL_SCISSOR_TEST);
    if (prevBlend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    SP<CGLTexture> capturedTex = captureFb->getTexture();
    if (!capturedTex) {
        // NOTE: dead branch — getTexture() is non-null after a successful alloc(); kept for consistency.
        g_logger->log(HT_LOG_ERROR, "captureTransitionState: failed to get texture from framebuffer");
        captureFb.reset(); // releases the FBO (binds 0) before restoring the caller's binding
        glBindFramebuffer(GL_FRAMEBUFFER, prevFb);
        return nullptr;
    }

    // The captured blend is already fit-resolved, so mark it to prevent a second fit pass.
    capturedTex->m_fitApplied = true;

    // Release the FBO before restoring the caller's framebuffer binding: ~CFramebuffer
    // -> release() issues glBindFramebuffer(0) and would clobber the restored binding.
    // The EGL context is current here (makeEGLCurrent() above), so the GL calls in
    // release() are safe.
    captureFb.reset();
    glBindFramebuffer(GL_FRAMEBUFFER, prevFb); // restore the caller's render target AFTER the FBO release

    return capturedTex;
}

SP<IRendererTexture> COpenGLRenderer::renderFitFrame(const SP<IRendererTexture>& src, const CBox& box, eImageFitMode fitMode) {
    RASSERT(src->type() == IRendererTexture::TEXTURE_GL, "OpenGL renderer: passed a non-gl texture to renderFitFrame");

    SP<CGLTexture> tex = reinterpretPointerCast<CGLTexture>(src);

    // Already a fit-resolved frame (the output of an earlier pre-render or
    // capture). Return it unchanged so it is never fit-transformed twice.
    if (tex->m_fitApplied)
        return src;

    // May run from the IPC handler, outside the render loop.
    makeEGLCurrent();

    const int w = static_cast<int>(box.width * m_scale);
    const int h = static_cast<int>(box.height * m_scale);
    if (w <= 0 || h <= 0)
        return nullptr;

    // Re-render only when the source identity, its size, the target box, or the
    // fit mode changes; otherwise reuse the cached fit-resolved frame.
    const SFitFrameKey key{tex->m_texID, tex->m_size, w, h, static_cast<int>(fitMode)};
    auto               cached = m_fitFrameCache.find(key);
    if (cached != m_fitFrameCache.end() && cached->second)
        return cached->second;

    // Save the GL state up front: captureFb->alloc() rebinds GL_FRAMEBUFFER and leaves
    // the binding at 0, so prevFb must be captured beforehand in order to restore the
    // caller's framebuffer (the surface) after the pre-render.
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    GLint prevFb = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFb);
    GLboolean prevScissor = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prevBlend   = glIsEnabled(GL_BLEND);
    GLint     prevActiveTex = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTex);

    auto captureFb = makeShared<CFramebuffer>();
    if (!captureFb->alloc(w, h)) {
        g_logger->log(HT_LOG_ERROR, "renderFitFrame: failed to allocate framebuffer");
        glBindFramebuffer(GL_FRAMEBUFFER, prevFb);
        return nullptr;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, captureFb->getFBID());
    glViewport(0, 0, w, h);
    glDisable(GL_SCISSOR_TEST);

    // CONTAIN letterbox must stay transparent to match renderTexture's
    // left-untouched clear. (This is a per-image pre-render, NOT the blend
    // result, so it differs from captureTransitionState's opaque clear.)
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Premultiplied-alpha texture draw, mirroring renderTexture's environment.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    CShader* shader = &m_texShader;
    glUseProgram(shader->program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(tex->m_target, tex->m_texID);
    glUniform1i(shader->tex, 0);

    // Project the (possibly contain-shrunk) box into the full FBO. The box is in
    // FBO-pixel units so it matches outputProjection's {w,h} size.
    const CBox srcBox = fitMode == IMAGE_FIT_MODE_CONTAIN
        ? containImage(CBox{0, 0, (float)w, (float)h}, tex->m_size)
        : CBox{0, 0, (float)w, (float)h};
    Mat3x3 outProj = Mat3x3::outputProjection(Vector2D{w, h}, HYPRUTILS_TRANSFORM_FLIPPED_180);
    Mat3x3 drawMat = outProj.projectBox(srcBox, HYPRUTILS_TRANSFORM_FLIPPED_180, 0);
    glUniformMatrix3fv(shader->proj, 1, GL_TRUE, drawMat.getMatrix().data());
    glUniform1f(shader->alpha, 1.F);

    // Plain textured quad: no rounding, no discard, no tint (same as renderTexture).
    glUniform2f(shader->topLeft, 0.0f, 0.0f);
    glUniform2f(shader->fullSize, (float)w, (float)h);
    glUniform1f(shader->radius, 0.F);
    glUniform1f(shader->roundingPower, 2.F);
    glUniform1i(shader->discardOpaque, 0);
    glUniform1i(shader->discardAlpha, 0);
    glUniform1i(shader->applyTint, 0);

    // Per-mode geometry / texcoords / wrap — identical rules to renderTexture.
    // coverImage/tileImage only need the box aspect ratio, so the logical box is fine.
    std::array<float, 8> texVerts = {
        fullVerts[0], fullVerts[1], fullVerts[2], fullVerts[3], fullVerts[4], fullVerts[5], fullVerts[6], fullVerts[7],
    };
    if (fitMode == IMAGE_FIT_MODE_COVER)
        texVerts = coverImage(box, tex->m_size);
    else if (fitMode == IMAGE_FIT_MODE_TILE) {
        texVerts = tileImage(box, tex->m_size);
        glTexParameteri(tex->m_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(tex->m_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    std::array<float, 16> fitVertices;
    std::ranges::copy(fullVerts, fitVertices.begin());
    std::ranges::copy(texVerts, fitVertices.begin() + 8);
    uploadVertices(fitVertices.data(), sizeof(fitVertices));
    glVertexAttribPointer(shader->posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribPointer(shader->texAttrib, 2, GL_FLOAT, GL_FALSE, 0, rc<const void*>(sizeof(fullVerts)));

    glEnableVertexAttribArray(shader->posAttrib);
    glEnableVertexAttribArray(shader->texAttrib);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(shader->posAttrib);
    glDisableVertexAttribArray(shader->texAttrib);

    // Restore TILE wrap so it never leaks into the crossfade or later draws.
    if (fitMode == IMAGE_FIT_MODE_TILE) {
        glTexParameteri(tex->m_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(tex->m_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glBindTexture(tex->m_target, 0);
    glUseProgram(0);

    // Restore GL state (framebuffer binding is restored last, after the FBO release below).
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    if (prevScissor)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
    if (prevBlend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    glActiveTexture(prevActiveTex);

    SP<CGLTexture> capturedTex = captureFb->getTexture();
    if (!capturedTex) {
        // NOTE: dead branch — getTexture() is non-null after a successful alloc(); kept for consistency.
        g_logger->log(HT_LOG_ERROR, "renderFitFrame: failed to get texture from framebuffer");
        captureFb.reset(); // releases the FBO (binds 0) before restoring the caller's binding
        glBindFramebuffer(GL_FRAMEBUFFER, prevFb);
        return nullptr;
    }

    // Mark the fit-resolved texture so it is never fit-transformed again.
    capturedTex->m_fitApplied = true;

    m_fitFrameCache[key] = capturedTex;

    // Release the FBO before restoring the caller's framebuffer binding: ~CFramebuffer
    // -> release() issues glBindFramebuffer(0) and would clobber the restored binding.
    // The EGL context is current here (makeEGLCurrent() above), so the GL calls in
    // release() are safe.
    captureFb.reset();
    glBindFramebuffer(GL_FRAMEBUFFER, prevFb); // restore the caller's render target AFTER the FBO release

    return capturedTex;
}

void COpenGLRenderer::clearTransitionCache() {
    m_fitFrameCache.clear();
}

void COpenGLRenderer::renderBorder(const SBorderRenderData& data) {
    const auto PRESENTED     = presentationBox(data.box);
    const auto ROUNDEDBOX    = logicalToGL(PRESENTED);
    const auto UNTRANSFORMED = logicalToGL(PRESENTED, false);
    Mat3x3     matrix        = m_projMatrix.projectBox(ROUNDEDBOX, HYPRUTILS_TRANSFORM_FLIPPED_180, PRESENTED.rot);
    Mat3x3     glMatrix      = m_projection.copy().multiply(matrix);

    const auto DAMAGE = damageWithClip();

    if (DAMAGE.copy().intersect(UNTRANSFORMED).empty())
        return;

    glUseProgram(m_borderShader.program);

    glUniformMatrix3fv(m_borderShader.proj, 1, GL_TRUE, glMatrix.getMatrix().data());

    const int numStops = std::clamp(sc<int>(data.gradient.m_vColors.size()), 0, 10);
    if (numStops > 0) {
        std::array<float, 40> gradData{};
        for (int i = 0; i < numStops; ++i) {
            const auto OKL      = data.gradient.m_vColors[i].asOkLab();
            gradData[i * 4 + 0] = sc<float>(OKL.l);
            gradData[i * 4 + 1] = sc<float>(OKL.a);
            gradData[i * 4 + 2] = sc<float>(OKL.b);
            gradData[i * 4 + 3] = sc<float>(data.gradient.m_vColors[i].a);
        }
        glUniform4fv(m_borderShader.gradient, numStops, gradData.data());
    }
    glUniform1i(m_borderShader.gradientLength, numStops);
    glUniform1f(m_borderShader.angle, data.gradient.m_fAngle);
    glUniform1f(m_borderShader.alpha, presentationOpacity());
    glUniform1i(m_borderShader.gradient2Length, 0);

    const auto TOPLEFT  = Vector2D(UNTRANSFORMED.x, UNTRANSFORMED.y);
    const auto FULLSIZE = Vector2D(UNTRANSFORMED.width, UNTRANSFORMED.height);

    glUniform2f(m_borderShader.topLeft, sc<float>(TOPLEFT.x), sc<float>(TOPLEFT.y));
    glUniform2f(m_borderShader.fullSize, sc<float>(FULLSIZE.x), sc<float>(FULLSIZE.y));
    glUniform2f(m_borderShader.fullSizeUntransformed, sc<float>(UNTRANSFORMED.width), sc<float>(UNTRANSFORMED.height));
    glUniform1f(m_borderShader.radius, data.rounding * m_scale);
    glUniform1f(m_borderShader.radiusOuter, data.rounding * m_scale);
    glUniform1f(m_borderShader.roundingPower, 2);
    glUniform1f(m_borderShader.thick, data.thick * m_scale);

    uploadVertices(fullVerts, sizeof(fullVerts));
    glVertexAttribPointer(m_borderShader.posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribPointer(m_borderShader.texAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(m_borderShader.posAttrib);
    glEnableVertexAttribArray(m_borderShader.texAttrib);

    DAMAGE.forEachRect([this](const auto& RECT) {
        scissor(&RECT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    });

    glDisableVertexAttribArray(m_borderShader.posAttrib);
    glDisableVertexAttribArray(m_borderShader.texAttrib);
}

void COpenGLRenderer::renderPolygon(const SPolygonRenderData& data) {
    const auto PRESENTED     = presentationBox(data.box);
    const auto ROUNDEDBOX    = logicalToGL(PRESENTED);
    const auto UNTRANSFORMED = logicalToGL(PRESENTED, false);

    const auto DAMAGE = damageWithClip();

    if (DAMAGE.copy().intersect(UNTRANSFORMED).empty())
        return;

    // We always do 4X MSAA on polygons, otherwise pixel galore

    Vector2D FB_SIZE = ROUNDEDBOX.size() * 2.F;

    Mat3x3   matrix = m_projMatrix.projectBox(CBox{{}, FB_SIZE}, HYPRUTILS_TRANSFORM_NORMAL, 0);

    auto     proj     = Mat3x3::outputProjection(FB_SIZE, HYPRUTILS_TRANSFORM_NORMAL);
    Mat3x3   glMatrix = proj.copy().multiply(matrix);

    m_polyRenderFb->alloc(FB_SIZE.x, FB_SIZE.y);
    m_polyRenderFb->bind();

    glViewport(0, 0, FB_SIZE.x, FB_SIZE.y);

    scissor(nullptr);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_rectShader.program);

    glUniformMatrix3fv(m_rectShader.proj, 1, GL_TRUE, glMatrix.getMatrix().data());

    // premultiply the color as well as we don't work with straight alpha
    const auto COL = data.color;
    glUniform4f(m_rectShader.color, COL.r * COL.a, COL.g * COL.a, COL.b * COL.a, COL.a);

    glUniform1f(m_rectShader.radius, 0);

    std::vector<float> verts;
    verts.resize(data.poly.m_points.size() * 2);
    for (size_t i = 0; i < data.poly.m_points.size(); i++) {
        verts[i * 2]       = data.poly.m_points[i].x;
        verts[1 + (i * 2)] = data.poly.m_points[i].y;
    }

    uploadVertices(verts.data(), verts.size() * sizeof(float));
    glVertexAttribPointer(m_rectShader.posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(m_rectShader.posAttrib);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, verts.size() / 2);

    glDisableVertexAttribArray(m_rectShader.posAttrib);

    // bind back to our fbo and render
    auto tex = m_polyRenderFb->getTexture();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_targetFB);

    glViewport(0, 0, m_currentViewport.x, m_currentViewport.y);

    renderTexture(STextureRenderData{
        .box     = data.box,
        .texture = tex,
    });
}

void COpenGLRenderer::renderLine(const SLineRenderData& data) {
    const auto PRESENTED     = presentationBox(data.box);
    const auto ROUNDEDBOX    = logicalToGL(PRESENTED);
    const auto UNTRANSFORMED = logicalToGL(PRESENTED, false);

    const auto DAMAGE = damageWithClip();

    if (DAMAGE.copy().intersect(UNTRANSFORMED).empty())
        return;

    if (data.points.size() <= 1)
        return;

    // generate a polygon. perpendicular thickness has to be computed in pixel
    // space, otherwise diagonal lines in non-square boxes get the right
    // magnitude but the wrong angle (the ndc perpendicular is not perpendicular
    // in pixels when the aspect ratio is not 1:1).

    std::vector<Vector2D> polyPoints;
    polyPoints.resize((data.points.size() * 4) - 2);

    for (size_t i = 0; i < data.points.size(); ++i) {
        Vector2D dir;

        if (i == data.points.size() - 1) {
            // last point: copy dir from last
            dir = (data.points.at(i) - data.points.at(i - 1));
        } else
            dir = (data.points.at(i + 1) - data.points.at(i));

        // unit perpendicular in pixel space, then mapped back to ndc
        const auto dirPx     = dir * ROUNDEDBOX.size();
        const auto dirUnitPx = dirPx / dirPx.size();
        const auto perpPx    = Vector2D{-dirUnitPx.y, dirUnitPx.x};

        const auto V1              = perpPx * data.thick / ROUNDEDBOX.size();
        const auto V2              = -V1;
        polyPoints.at(i * 4)       = data.points.at(i) + V1;
        polyPoints.at(1 + (i * 4)) = data.points.at(i) + V2;

        // then add the same for the next point, if there is one
        if (i + 1 < data.points.size()) {
            polyPoints.at(2 + (i * 4)) = data.points.at(i + 1) + V1;
            polyPoints.at(3 + (i * 4)) = data.points.at(i + 1) + V2;
        }
    }

    renderPolygon(SPolygonRenderData{
        .box   = data.box,
        .color = data.color,
        .poly  = CPolygon{polyPoints},
    });
}

SP<CSyncTimeline> COpenGLRenderer::exportSync(SP<Aquamarine::IBuffer> buf) {
    if (!buf)
        return nullptr;

    auto rbo = getRBO(buf);

    if (!rbo)
        return nullptr;

    if (!rbo->m_syncTimeline)
        rbo->m_syncTimeline = CSyncTimeline::create(m_drmFD);

    return rbo->m_syncTimeline;
}

void COpenGLRenderer::signalRenderPoint(SP<CSyncTimeline> timeline) {
    auto sync = CEGLSync::create();

    if (sync && sync->isValid()) {
        g_backendServices->doOnReadable(sync->takeFd(), [ap = timeline->m_acquirePoint, tl = WP<CSyncTimeline>{timeline}]() {
            if (!tl)
                return;

            tl->signal(ap);
        });
    }
}
