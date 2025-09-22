#include "OpenGL.hpp"

#include "../../window/ToolkitWindow.hpp"
#include "../../core/renderPlatforms/Egl.hpp"
#include "../../Macros.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../element/Element.hpp"
#include "Shaders.hpp"
#include "GLTexture.hpp"

#include <hyprutils/memory/Casts.hpp>

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

    prog                          = createProgram(TEXVERTSRC, TEXFRAGSRCRGBA);
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

    prog                                 = createProgram(QUADVERTSRC, FRAGBORDER);
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
}

COpenGLRenderer::~COpenGLRenderer() {
    ;
}

CBox COpenGLRenderer::logicalToGL(const CBox& box) {
    auto b = box.copy();
    b.scale(m_scale).transform(Hyprutils::Math::HYPRUTILS_TRANSFORM_FLIPPED_180, m_currentViewport.x, m_currentViewport.y);
    return b;
}

void COpenGLRenderer::beginRendering(SP<IToolkitWindow> window) {
    m_projection      = Mat3x3::outputProjection(window->pixelSize(), HYPRUTILS_TRANSFORM_NORMAL);
    m_currentViewport = window->pixelSize();
    m_scale           = window->scale();
    m_window          = window;
    m_damage          = window->m_damageRing.getBufferDamage(DAMAGE_RING_PREVIOUS_LEN);

    if (m_damage.empty())
        return;

    glViewport(0, 0, window->pixelSize().x, window->pixelSize().y);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    window->m_rootElement->impl->breadthfirst([](SP<IElement> el) {
        if (!el->impl->failedPositioning)
            el->paint();
    });

    glDisable(GL_BLEND);
}

void COpenGLRenderer::endRendering() {
    m_window->m_damageRing.rotate();
    m_window.reset();
    m_damage.clear();
}

void COpenGLRenderer::scissor(const pixman_box32_t* box) {
    scissor(CBox{
        sc<double>(box->x1),
        sc<double>(box->y1),
        sc<double>(box->x2 - box->x1),
        sc<double>(box->y2 - box->y1),
    });
}

void COpenGLRenderer::scissor(const CBox& box) {
    // only call glScissor if the box has changed
    static CBox m_lastScissorBox = {};

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

void COpenGLRenderer::renderRectangle(const SRectangleRenderData& data) {
    const auto ROUNDEDBOX = logicalToGL(data.box).round();
    Mat3x3     matrix     = m_projMatrix.projectBox(ROUNDEDBOX, HYPRUTILS_TRANSFORM_NORMAL, data.box.rot);
    Mat3x3     glMatrix   = m_projection.copy().multiply(matrix);

    if (m_damage.copy().intersect(ROUNDEDBOX).empty())
        return;

    glUseProgram(m_rectShader.program);

    glUniformMatrix3fv(m_rectShader.proj, 1, GL_TRUE, glMatrix.getMatrix().data());

    // premultiply the color as well as we don't work with straight alpha
    const auto COL = data.color;
    glUniform4f(m_rectShader.color, COL.r * COL.a, COL.g * COL.a, COL.b * COL.a, COL.a);

    const auto TOPLEFT  = Vector2D(ROUNDEDBOX.x, ROUNDEDBOX.y);
    const auto FULLSIZE = Vector2D(ROUNDEDBOX.width, ROUNDEDBOX.height);

    // Rounded corners
    glUniform2f(m_rectShader.topLeft, (float)TOPLEFT.x, (float)TOPLEFT.y);
    glUniform2f(m_rectShader.fullSize, (float)FULLSIZE.x, (float)FULLSIZE.y);
    glUniform1f(m_rectShader.radius, data.rounding);

    glVertexAttribPointer(m_rectShader.posAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);

    glEnableVertexAttribArray(m_rectShader.posAttrib);

    m_damage.forEachRect([this](const auto& RECT) {
        scissor(&RECT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    });

    glDisableVertexAttribArray(m_rectShader.posAttrib);
}

SP<IRendererTexture> COpenGLRenderer::uploadTexture(const STextureData& data) {
    const auto TEX = m_glTextures.emplace_back(makeShared<CGLTexture>(data.resource));
    return TEX;
}

void COpenGLRenderer::renderTexture(const STextureRenderData& data) {
    const auto ROUNDEDBOX = logicalToGL(data.box);
    Mat3x3     matrix     = m_projMatrix.projectBox(ROUNDEDBOX, Hyprutils::Math::HYPRUTILS_TRANSFORM_FLIPPED_180, data.box.rot);
    Mat3x3     glMatrix   = m_projection.copy().multiply(matrix);

    if (m_damage.copy().intersect(ROUNDEDBOX).empty())
        return;

    CShader* shader = &m_texShader;

    RASSERT(data.texture->type() == IRendererTexture::TEXTURE_GL, "OpenGL renderer: passed a non-gl texture");

    SP<CGLTexture> tex = reinterpretPointerCast<CGLTexture>(data.texture);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(tex->m_target, tex->m_texID);

    glUseProgram(shader->program);

    glUniformMatrix3fv(shader->proj, 1, GL_TRUE, glMatrix.getMatrix().data());
    glUniform1i(shader->tex, 0);
    glUniform1f(shader->alpha, data.a);
    const auto TOPLEFT  = Vector2D(ROUNDEDBOX.x, ROUNDEDBOX.y);
    const auto FULLSIZE = Vector2D(ROUNDEDBOX.width, ROUNDEDBOX.height);

    // Rounded corners
    glUniform2f(shader->topLeft, TOPLEFT.x, TOPLEFT.y);
    glUniform2f(shader->fullSize, FULLSIZE.x, FULLSIZE.y);
    glUniform1f(shader->radius, data.rounding);

    glUniform1i(shader->discardOpaque, 0);
    glUniform1i(shader->discardAlpha, 0);
    glUniform1i(shader->applyTint, 0);

    glVertexAttribPointer(shader->posAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);
    glVertexAttribPointer(shader->texAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);

    glEnableVertexAttribArray(shader->posAttrib);
    glEnableVertexAttribArray(shader->texAttrib);

    m_damage.forEachRect([this](const auto& RECT) {
        scissor(&RECT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    });

    glDisableVertexAttribArray(shader->posAttrib);
    glDisableVertexAttribArray(shader->texAttrib);

    glBindTexture(tex->m_target, 0);
}

void COpenGLRenderer::renderBorder(const SBorderRenderData& data) {
    const auto ROUNDEDBOX = logicalToGL(data.box).round();
    Mat3x3     matrix     = m_projMatrix.projectBox(ROUNDEDBOX, HYPRUTILS_TRANSFORM_NORMAL, data.box.rot);
    Mat3x3     glMatrix   = m_projection.copy().multiply(matrix);

    glUseProgram(m_borderShader.program);

    glUniformMatrix3fv(m_borderShader.proj, 1, GL_TRUE, glMatrix.getMatrix().data());

    const auto           OKLAB = data.color.asOkLab();
    std::array<float, 4> grad  = {sc<float>(OKLAB.l), sc<float>(OKLAB.a), sc<float>(OKLAB.b), sc<float>(data.color.a)};

    glUniform4fv(m_borderShader.gradient, grad.size() / 4, (float*)grad.data());
    glUniform1i(m_borderShader.gradientLength, grad.size() / 4);
    glUniform1f(m_borderShader.angle, (int)(0.F / (M_PI / 180.0)) % 360 * (M_PI / 180.0));
    glUniform1f(m_borderShader.alpha, 1.F);
    glUniform1i(m_borderShader.gradient2Length, 0);

    const auto TOPLEFT  = Vector2D(ROUNDEDBOX.x, ROUNDEDBOX.y);
    const auto FULLSIZE = Vector2D(ROUNDEDBOX.width, ROUNDEDBOX.height);

    glUniform2f(m_borderShader.topLeft, (float)TOPLEFT.x, (float)TOPLEFT.y);
    glUniform2f(m_borderShader.fullSize, (float)FULLSIZE.x, (float)FULLSIZE.y);
    glUniform2f(m_borderShader.fullSizeUntransformed, (float)data.box.width, (float)data.box.height);
    glUniform1f(m_borderShader.radius, data.rounding);
    glUniform1f(m_borderShader.radiusOuter, data.rounding);
    glUniform1f(m_borderShader.thick, data.thick);

    glVertexAttribPointer(m_borderShader.posAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);
    glVertexAttribPointer(m_borderShader.texAttrib, 2, GL_FLOAT, GL_FALSE, 0, fullVerts);

    glEnableVertexAttribArray(m_borderShader.posAttrib);
    glEnableVertexAttribArray(m_borderShader.texAttrib);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(m_borderShader.posAttrib);
    glDisableVertexAttribArray(m_borderShader.texAttrib);
}
