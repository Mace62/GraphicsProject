// button.cpp
#include "button.hpp"
#include <vector>
#include <array>

#include "render_text.hpp"

Button::Button(float normalizedX, float normalizedY, float normalizedWidth, float normalizedHeight,
    const std::string & text, FONScontext * fontContext, int fontId, GLuint buttonShader)
    : normalizedX_(normalizedX), normalizedY_(normalizedY)
    , normalizedWidth_(normalizedWidth), normalizedHeight_(normalizedHeight)
    , text_(text), state_(State::NEUTRAL)
    , fontContext_(fontContext), fontId_(fontId)
    , shader_(buttonShader), borderThickness_(2.0f)
{
    initializeBuffers();
    updateScreenCoordinates(1, 1);
}

Button::~Button() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void Button::setOnClick(std::function<void()> callback) {
    onClick_ = callback;
}

void Button::setColors(const ButtonColors& colors) {
    colors_ = colors;
}

void Button::update(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    updateScreenCoordinates(windowWidth, windowHeight);

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    mouseY = windowHeight - mouseY;

    bool isInside = isPointInside(mouseX, mouseY);
    int buttonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    if (!isInside) state_ = State::NEUTRAL;
    else if (buttonState == GLFW_PRESS) state_ = State::PRESSED;
    else if (state_ == State::PRESSED && buttonState == GLFW_RELEASE) {
        if (onClick_) onClick_();
        state_ = State::HOVER;
    }
    else state_ = State::HOVER;

    updateVertexColors();
}

void Button::render(int screenWidth, int screenHeight) {
    glUseProgram(shader_);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUniform2f(0, screenWidth, screenHeight);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 12);

    if (fontContext_) {
        fonsSetFont(fontContext_, fontId_);
        fonsSetSize(fontContext_, screenHeight_ * 0.5f);
        fonsSetColor(fontContext_,
            glfonsRGBA(
                static_cast<unsigned char>(colors_.textColor[0] * 255),
                static_cast<unsigned char>(colors_.textColor[1] * 255),
                static_cast<unsigned char>(colors_.textColor[2] * 255),
                static_cast<unsigned char>(colors_.textColor[3] * 255)
            )
        );

        float bounds[4];
        fonsTextBounds(fontContext_, 0, 0, text_.c_str(), nullptr, bounds);
        float textWidth = bounds[2] - bounds[0];
        float textHeight = bounds[3] - bounds[1];

        float textX = screenX_ + (screenWidth_ - textWidth) * 0.5f;
        float textY = screenHeight - screenY_ - ((screenHeight_ - textHeight) * 0.5f);

        fonsDrawText(fontContext_, textX, textY, text_.c_str(), nullptr);
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Button::initializeBuffers() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
}

void Button::updateScreenCoordinates(int windowWidth, int windowHeight) {
    screenX_ = normalizedX_ * windowWidth;
    screenY_ = normalizedY_ * windowHeight;
    screenWidth_ = normalizedWidth_ * windowWidth;
    screenHeight_ = normalizedHeight_ * windowHeight;

    std::vector<Vertex> vertices(12);

    vertices[0].position[0] = screenX_;
    vertices[0].position[1] = screenY_;
    vertices[1].position[0] = screenX_ + screenWidth_;
    vertices[1].position[1] = screenY_;
    vertices[2].position[0] = screenX_ + screenWidth_;
    vertices[2].position[1] = screenY_ + screenHeight_;

    vertices[3].position[0] = screenX_;
    vertices[3].position[1] = screenY_;
    vertices[4].position[0] = screenX_ + screenWidth_;
    vertices[4].position[1] = screenY_ + screenHeight_;
    vertices[5].position[0] = screenX_;
    vertices[5].position[1] = screenY_ + screenHeight_;

    float bt = borderThickness_;
    vertices[6].position[0] = screenX_ + bt;
    vertices[6].position[1] = screenY_ + bt;
    vertices[7].position[0] = screenX_ + screenWidth_ - bt;
    vertices[7].position[1] = screenY_ + bt;
    vertices[8].position[0] = screenX_ + screenWidth_ - bt;
    vertices[8].position[1] = screenY_ + screenHeight_ - bt;

    vertices[9].position[0] = screenX_ + bt;
    vertices[9].position[1] = screenY_ + bt;
    vertices[10].position[0] = screenX_ + screenWidth_ - bt;
    vertices[10].position[1] = screenY_ + screenHeight_ - bt;
    vertices[11].position[0] = screenX_ + bt;
    vertices[11].position[1] = screenY_ + screenHeight_ - bt;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());

    updateVertexColors();
}

void Button::updateVertexColors() {
    const float* fillColor;
    switch (state_) {
    case State::HOVER: fillColor = colors_.hoverFill; break;
    case State::PRESSED: fillColor = colors_.pressedFill; break;
    default: fillColor = colors_.neutralFill;
    }

    std::vector<Vertex> vertices(12);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * 12, vertices.data());

    for (int i = 0; i < 6; i++) {
        std::copy(colors_.border, colors_.border + 4, vertices[i].color);
    }

    for (int i = 6; i < 12; i++) {
        std::copy(fillColor, fillColor + 4, vertices[i].color);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
}

bool Button::isPointInside(double px, double py) const {
    return px >= screenX_ && px <= screenX_ + screenWidth_ &&
        py >= screenY_ && py <= screenY_ + screenHeight_;
}