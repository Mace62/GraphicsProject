// button.hpp
#ifndef UI_BUTTON_HPP
#define UI_BUTTON_HPP

#include <string>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "render_text.hpp"


class Button {
public:
    struct Vertex {
        float position[2];
        float color[4];
    };

    struct ButtonColors {
        float neutralFill[4] = { 0.7f, 0.7f, 0.7f, 0.6f };
        float hoverFill[4] = { 0.5f, 0.9f, 0.5f, 0.6f };
        float pressedFill[4] = { 0.3f, 0.3f, 0.3f, 0.6f };
        float border[4] = { 0.0f, 0.0f, 0.0f, 0.6f };
        float textColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    enum class State { NEUTRAL, HOVER, PRESSED };

    Button(float normalizedX, float normalizedY, float normalizedWidth, float normalizedHeight,
        const std::string& text, FONScontext* fontContext, int fontId, GLuint buttonShader);
    ~Button();

    void setOnClick(std::function<void()> callback);
    void setColors(const ButtonColors& colors);
    void update(GLFWwindow* window);
    void render(int screenWidth, int screenHeight);

private:
    void initializeBuffers();
    void updateScreenCoordinates(int windowWidth, int windowHeight);
    void updateVertexColors();
    bool isPointInside(double px, double py) const;

    float normalizedX_, normalizedY_;
    float normalizedWidth_, normalizedHeight_;
    float screenX_, screenY_;
    float screenWidth_, screenHeight_;
    std::string text_;
    State state_;
    ButtonColors colors_;
    FONScontext* fontContext_;
    int fontId_;
    GLuint shader_;
    float borderThickness_;
    GLuint vao_;
    GLuint vbo_;
    std::function<void()> onClick_;
};

#endif // UI_BUTTON_HPP