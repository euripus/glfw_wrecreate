#ifndef WINDOW_H
#define WINDOW_H

#include <string>

// Include GLEW
#include <GL/glew.h>
// Include GLFW
#include <GLFW/glfw3.h>
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Window
{
    // window state
    bool                m_is_fullscreen;
    GLFWvidmode const * mp_base_video_mode;
    GLFWwindow *        mp_glfw_win;
    glm::ivec2          m_size;
    std::string         m_title;
    // scene state
    glm::mat4 m_MV;
    GLuint    m_vertexbuffer;
    GLuint    m_uvbuffer;
    GLuint    m_texture;

public:
    Window(int width, int height, const char * title);
    ~Window();

    Window(const Window &) = delete;
    Window & operator=(const Window &) = delete;

    bool isFullscreen() const { return m_is_fullscreen; }

    void create();
    void initScene();
    void fullscreen(bool is_fullscreen);
    void run();
};

#endif   // WINDOW_H
