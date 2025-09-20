#include "OpenGL.hpp"

#include <hyprtoolkit/window/Window.hpp>

#include "../../core/renderPlatforms/Egl.hpp"
#include "../../Macros.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../element/Element.hpp"
#include "Shaders.hpp"

using namespace Hyprtoolkit;

inline const float fullVerts[] = {
    1, 0, // top right
    0, 0, // top left
    1, 1, // bottom right
    0, 1, // bottom left
};

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

static void glMessageCallbackA(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    if (type != GL_DEBUG_TYPE_ERROR)
        return;
    g_logger->log(Hyprtoolkit::HT_LOG_DEBUG, "[gl] {}", (const char*)message);
}

COpenGLRenderer::COpenGLRenderer() {
    g_pEGL->makeCurrent(nullptr);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glMessageCallbackA, nullptr);

    GLuint prog            = createProgram(QUADVERTSRC, QUADFRAGSRC);
    m_rectShader.program   = prog;
    m_rectShader.proj      = glGetUniformLocation(prog, "proj");
    m_rectShader.color     = glGetUniformLocation(prog, "color");
    m_rectShader.posAttrib = glGetAttribLocation(prog, "pos");
    m_rectShader.topLeft   = glGetUniformLocation(prog, "topLeft");
    m_rectShader.fullSize  = glGetUniformLocation(prog, "fullSize");
    m_rectShader.radius    = glGetUniformLocation(prog, "radius");
}

COpenGLRenderer::~COpenGLRenderer() {
    ;
}

CBox COpenGLRenderer::logicalToGL(const CBox& box) {
    auto b = box.copy();
    b.scale(m_scale).transform(Hyprutils::Math::HYPRUTILS_TRANSFORM_FLIPPED_180, m_currentViewport.x, m_currentViewport.y);
    return b;
}

void COpenGLRenderer::bfHelper(std::vector<SP<IElement>> elements, const std::function<void(SP<IElement>)>& fn) {
    for (const auto& e : elements) {
        fn(e);
    }

    std::vector<SP<IElement>> els;
    for (const auto& e : elements) {
        for (const auto& c : e->m_elementData->children) {
            els.emplace_back(c);
        }
    }

    if (!els.empty())
        bfHelper(els, fn);
}

void COpenGLRenderer::breadthfirst(SP<IElement> element, const std::function<void(SP<IElement>)>& fn) {
    fn(element);

    std::vector<SP<IElement>> els = element->m_elementData->children;

    bfHelper(els, fn);
}

void COpenGLRenderer::beginRendering(SP<IWindow> window) {
    m_projection      = Mat3x3::outputProjection(window->pixelSize(), HYPRUTILS_TRANSFORM_NORMAL);
    m_currentViewport = window->pixelSize();
    m_scale           = window->scale();

    glViewport(0, 0, window->pixelSize().x, window->pixelSize().y);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    breadthfirst(window->m_rootElement, [](SP<IElement> el) {
        if (!el->m_elementData->failedPositioning)
            el->paint();
    });

    glDisable(GL_BLEND);
}

void COpenGLRenderer::renderRectangle(const SRectangleRenderData& data) {
    const auto ROUNDEDBOX = logicalToGL(data.box).round();
    Mat3x3     matrix     = m_projMatrix.projectBox(ROUNDEDBOX, HYPRUTILS_TRANSFORM_NORMAL, data.box.rot);
    Mat3x3     glMatrix   = m_projection.copy().multiply(matrix);

    glUseProgram(m_rectShader.program);

    glUniformMatrix3fv(m_rectShader.proj, 1, GL_TRUE, glMatrix.getMatrix().data());

    // premultiply the color as well as we don't work with straight alpha
    const auto COL = data.color.asRgb();
    glUniform4f(m_rectShader.color, COL.r * data.a, COL.g * data.a, COL.b * data.a, data.a);

    const auto TOPLEFT  = Vector2D(ROUNDEDBOX.x, ROUNDEDBOX.y);
    const auto FULLSIZE = Vector2D(ROUNDEDBOX.width, ROUNDEDBOX.height);

    // Rounded corners
    glUniform2f(m_rectShader.topLeft, (float)TOPLEFT.x, (float)TOPLEFT.y);
    glUniform2f(m_rectShader.fullSize, (float)FULLSIZE.x, (float)FULLSIZE.y);
    glUniform1f(m_rectShader.radius, data.rounding);

    glVertexAttribPointer(m_rectShader.posAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);

    glEnableVertexAttribArray(m_rectShader.posAttrib);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(m_rectShader.posAttrib);
}
