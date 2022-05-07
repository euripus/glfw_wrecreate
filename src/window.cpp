#include "window.h"
#include "imagedata.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

// Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
static const GLfloat g_vertex_buffer_data[] = {
    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
    1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,
    -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,
    1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
    1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, 1.0f};

// Two UV coordinatesfor each vertex. They were created withe Blender.
static const GLfloat g_uv_buffer_data[] = {
    0.000059f, 1.0f - 0.000004f, 0.000103f, 1.0f - 0.336048f, 0.335973f, 1.0f - 0.335903f,
    1.000023f, 1.0f - 0.000013f, 0.667979f, 1.0f - 0.335851f, 0.999958f, 1.0f - 0.336064f,
    0.667979f, 1.0f - 0.335851f, 0.336024f, 1.0f - 0.671877f, 0.667969f, 1.0f - 0.671889f,
    1.000023f, 1.0f - 0.000013f, 0.668104f, 1.0f - 0.000013f, 0.667979f, 1.0f - 0.335851f,
    0.000059f, 1.0f - 0.000004f, 0.335973f, 1.0f - 0.335903f, 0.336098f, 1.0f - 0.000071f,
    0.667979f, 1.0f - 0.335851f, 0.335973f, 1.0f - 0.335903f, 0.336024f, 1.0f - 0.671877f,
    1.000004f, 1.0f - 0.671847f, 0.999958f, 1.0f - 0.336064f, 0.667979f, 1.0f - 0.335851f,
    0.668104f, 1.0f - 0.000013f, 0.335973f, 1.0f - 0.335903f, 0.667979f, 1.0f - 0.335851f,
    0.335973f, 1.0f - 0.335903f, 0.668104f, 1.0f - 0.000013f, 0.336098f, 1.0f - 0.000071f,
    0.000103f, 1.0f - 0.336048f, 0.000004f, 1.0f - 0.671870f, 0.336024f, 1.0f - 0.671877f,
    0.000103f, 1.0f - 0.336048f, 0.336024f, 1.0f - 0.671877f, 0.335973f, 1.0f - 0.335903f,
    0.667969f, 1.0f - 0.671889f, 1.000004f, 1.0f - 0.671847f, 0.667979f, 1.0f - 0.335851f};

Window::Window(int width, int height, const char * title) :
    m_is_fullscreen{false},
    mp_base_video_mode{nullptr},
    mp_glfw_win{nullptr},
    m_size{width, height},
    m_title{title},
    m_MV{1.0f},
    m_vertexbuffer{0},
    m_uvbuffer{0},
    m_texture{0}
{
    // Initialise GLFW
    if(!glfwInit())
    {
        throw std::runtime_error{"Failed to initialize GLFW"};
    }

    mp_base_video_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
}

Window::~Window()
{
    // Cleanup VBO and shader
    if(mp_glfw_win)
    {
        glDeleteBuffers(1, &m_vertexbuffer);
        glDeleteBuffers(1, &m_uvbuffer);
        glDeleteTextures(1, &m_texture);
    }

    // Close OpenGL window and terminate GLFW
    glfwTerminate();
}

void Window::create()
{
    int width{0}, height{0};

    GLFWmonitor * mon;
    if(m_is_fullscreen)
    {
        mon    = glfwGetPrimaryMonitor();
        width  = mp_base_video_mode->width;
        height = mp_base_video_mode->height;
    }
    else
    {
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        mon    = nullptr;
        width  = m_size.x;
        height = m_size.y;
    }

    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow * new_window{nullptr};
    if(mp_glfw_win != nullptr)
    {
        new_window = glfwCreateWindow(width, height, "", mon, mp_glfw_win);
        glfwDestroyWindow(mp_glfw_win);
    }
    else
    {
        new_window = glfwCreateWindow(width, height, "", mon, nullptr);
    }

    mp_glfw_win = new_window;
    if(mp_glfw_win == nullptr)
    {
        glfwTerminate();
        throw std::runtime_error{"Failed to create GLFW window"};
    }
    glfwMakeContextCurrent(mp_glfw_win);
    glfwSetWindowTitle(mp_glfw_win, m_title.c_str());
    glViewport(0, 0, width, height);

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(mp_glfw_win, GLFW_STICKY_KEYS, GL_TRUE);

    // Initialize GLEW
    if(glewInit() != GLEW_OK)
    {
        throw std::runtime_error{"Failed to initialize GLEW"};
    }

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    glEnable(GL_TEXTURE_2D);

    glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(Projection));
}

void Window::fullscreen(bool is_fullscreen)
{
    if(is_fullscreen == m_is_fullscreen)
        return;

    m_is_fullscreen = is_fullscreen;
    create();
}

void Window::initScene()
{
    // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(Projection));
    // Camera matrix
    glm::mat4 View = glm::lookAt(glm::vec3(4, 3, 3),   // Camera is at (4,3,3), in World Space
                                 glm::vec3(0, 0, 0),   // and looks at the origin
                                 glm::vec3(0, 1, 0)    // Head is up (set to 0,-1,0 to look upside-down)
    );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    // Our ModelViewProjection : multiplication of our 3 matrices
    m_MV = View * Model;   // Remember, matrix multiplication is the other way around
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Load the texture
    tex::ImageData tex_data;
    if(!tex::ReadTGA("uvtemplate.tga", tex_data))
        throw std::runtime_error{"Failed to load texture"};

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, tex_data.type == tex::ImageData::PixelType::pt_rgb ? 3 : 4,
                 static_cast<GLsizei>(tex_data.width), static_cast<GLsizei>(tex_data.height), 0,
                 tex_data.type == tex::ImageData::PixelType::pt_rgb ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE,
                 tex_data.data.get());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(1, &m_vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &m_uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);
}

void Window::run()
{
    do
    {
        if(glfwGetKey(mp_glfw_win, GLFW_KEY_F1) == GLFW_PRESS)
            fullscreen(!m_is_fullscreen);

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMultMatrixf(glm::value_ptr(m_MV));

        glBindTexture(GL_TEXTURE_2D, m_texture);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glBindBuffer(GL_ARRAY_BUFFER, m_uvbuffer);
        glTexCoordPointer(2, GL_FLOAT, 0, static_cast<char *>(nullptr));
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexbuffer);
        glVertexPointer(3, GL_FLOAT, 0, static_cast<char *>(nullptr));

        // Draw the triangles !
        glDrawArrays(GL_TRIANGLES, 0, 12 * 3);   // 12*3 indices starting at 0 -> 12 triangles

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindTexture(GL_TEXTURE_2D, 0);

        glPopMatrix();

        // Swap buffers
        glfwSwapBuffers(mp_glfw_win);
        glfwPollEvents();

    }   // Check if the ESC key was pressed or the window was closed
    while(glfwGetKey(mp_glfw_win, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(mp_glfw_win) == 0);
}
