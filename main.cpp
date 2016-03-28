#include <iostream>
#include <time.h>
#include <cmath>

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <stb_image.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/vector_angle.hpp>

#include "src/BezierCurve.hpp"

#ifndef DEBUG
#define DEBUG 0
#endif

using namespace std;
using namespace glm;

// Font buffers
extern const unsigned char DroidSans_ttf[];
extern const unsigned int DroidSans_ttf_len;

// Shader utils
int check_link_error(GLuint program);
int check_compile_error(GLuint shader, const char ** sourceBuffer);
GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize);
GLuint compile_shader_from_file(GLenum shaderType, const char * fileName);

// OpenGL utils
bool checkError(const char* title);

struct Camera
{
    float radius;
    float theta;
    float phi;
    vec3 o;
    vec3 eye;
    vec3 up;
};

void camera_compute(Camera & c);
void camera_defaults(Camera & c);
void camera_zoom(Camera & c, float factor);
void camera_turn(Camera & c, float phi, float theta);
void camera_pan(Camera & c, float x, float y);
float angleBetween(vec3 a, vec3 b, vec3 ref);

struct GUIStates
{
    bool panLock;
    bool turnLock;
    bool zoomLock;
    int lockPositionX;
    int lockPositionY;
    int camera;
    double time;
    bool playing;
    static const float MOUSE_PAN_SPEED;
    static const float MOUSE_ZOOM_SPEED;
    static const float MOUSE_TURN_SPEED;
};

const float GUIStates::MOUSE_PAN_SPEED = 0.001f;
const float GUIStates::MOUSE_ZOOM_SPEED = 0.05f;
const float GUIStates::MOUSE_TURN_SPEED = 0.005f;
void init_gui_states(GUIStates & guiStates);

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(void)
{
    /******************
     * Global Variables
     *****************/

    int width, height;
    float currentTime;

    int grid_size = 500;

    int directionalLightCount = 1;
    float directionalLightIntensity = 1.f;
    vec3 directionalLightColor = vec3(1.f, 0.f, 0.f);
    vec4 directionalLightDir = vec4(-1.0, -1.0, 0.0, 0.0);

    vec3 sphereColor(1.f);

    // Shading Grid
    vec3 colorNear(0.91f, 0.41f, 0.19f);
    vec3 colorFar(0.56f, 0.06f, 0.32f);
    float brightness(100.f);
    float attenuation(7.f);

    // Fx
    int sampleCount = 5;
    float gamma = 1.2f;


    /****************************
     * Window Initialization GLFW
     ***************************/

    GLFWwindow* window;
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
    {
        cerr << "Failed to initialize GLFW" << endl;
        exit(EXIT_FAILURE);
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

#if DEBUG
    width = 1280;
    height = 720;
    window = glfwCreateWindow(width, height, "OpenGL project", NULL, NULL);
#else
    width = mode->width;
    height = mode->height;
    window = glfwCreateWindow(width, height, "OpenGL project", monitor, NULL);
#endif

    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    /*********************
     * GLEW Initialization
     *********************/

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        cerr << "Error: \n" << glewGetErrorString(err) << endl;
        exit(EXIT_FAILURE);
    }

    /***********************
     *  ImGui Initialization
     **********************/

    ImGui_ImplGlfwGL3_Init(window, true);
    float clearColor[]{0.f, 0.f, 0.f, 1.f};


    /***********************
     * Camera Initialization
     **********************/

    Camera camera;
    camera_defaults(camera);
    GUIStates guiStates;
    init_gui_states(guiStates);

    float focusPlane = 15.0;
    float nearPlane = 1.0;
    float farPlane = 50.0;

    // Camera Path
    unsigned int timer = unsigned(time(NULL));
    BezierCurve bSmoother;
    vector<vec3> cameraPath;

    cameraPath.push_back(vec3(-100, 100, -100));
    cameraPath.push_back(vec3(-150, 0, -150));
    cameraPath.push_back(vec3(0, 0, 0));

    for(int i = 0; i < bSmoother.getFactorialMax() - cameraPath.size() - 1; ++i)
    {
        vec3 newPos;
        srand(i * timer);
        newPos.x = (rand() % grid_size) - (grid_size / 2);
        srand(i+1 * timer);
        newPos.y = (rand() % 16)+1;
        srand(i+2 * timer);
        newPos.z = (rand() % grid_size) - (grid_size / 2);

        unsigned int j = 3;
        while (i && int(distance(newPos, cameraPath[i - 1])) < 200)
        {
            srand((i + j + 0) * timer);
            newPos.x = (rand() % grid_size) - (grid_size / 2);
            srand((i + j + 2) * timer);
            newPos.z = (rand() % grid_size) - (grid_size / 2);

            ++j;
        }

        cameraPath.push_back(newPos);
    }
    cameraPath.push_back(cameraPath.front());

    int smoothValue = 50;
    vector<vec3> cameraSmoothPath = bSmoother.Bezier3D(cameraPath, bSmoother.getFactorialMax() * smoothValue);

    /**************
     * Framebuffers
     *************/

    GLuint gbufferFbo;

    GLuint gbufferTextures[3];
    glGenTextures(3, gbufferTextures);

    GLuint gbufferDrawBuffers[2];


    // Create color texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create normal texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create depth texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    // Create Framebuffer Object
    glGenFramebuffers(1, &gbufferFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);
    // Initialize DrawBuffers
    gbufferDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    gbufferDrawBuffers[1] = GL_COLOR_ATTACHMENT1;
    glDrawBuffers(2, gbufferDrawBuffers);

    // Attach textures to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, gbufferTextures[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 , GL_TEXTURE_2D, gbufferTextures[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbufferTextures[2], 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        cerr << "Error on building framebuffer" << endl;
        exit( EXIT_FAILURE );
    }


    /*****************
     * FX Framebuffers
     ****************/

    // Create Fx Framebuffer Object
    GLuint fxFbo;
    GLuint fxDrawBuffers[1];
    glGenFramebuffers(1, &fxFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fxFbo);
    fxDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, fxDrawBuffers);

    // Create Fx textures
    const int FX_TEXTURE_COUNT = 4;
    GLuint fxTextures[FX_TEXTURE_COUNT];
    glGenTextures(FX_TEXTURE_COUNT, fxTextures);
    for (int i = 0; i < FX_TEXTURE_COUNT; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, fxTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Attach first fx texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[0], 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        cerr << "Error on building FX framebuffer" << endl;
        exit( EXIT_FAILURE );
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    checkError("Framebuffers");


    /*********
     * SHADERS
     ********/

    GLuint vertShaderCubeGrid = compile_shader_from_file(GL_VERTEX_SHADER, "shaders/cube_grid.vert");
    GLuint fragShaderCubeGrid = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/cube_grid.frag");
    GLuint programCubeGrid = glCreateProgram();
    glAttachShader(programCubeGrid, vertShaderCubeGrid);
    glAttachShader(programCubeGrid, fragShaderCubeGrid);
    glLinkProgram(programCubeGrid);

    if (check_link_error(programCubeGrid) < 0)
        exit(1);

    GLuint vertShaderSphere = compile_shader_from_file(GL_VERTEX_SHADER, "shaders/sphere.vert");
    GLuint fragShaderSphere = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/sphere.frag");
    GLuint programSphere = glCreateProgram();
    glAttachShader(programSphere, vertShaderSphere);
    glAttachShader(programSphere, fragShaderSphere);
    glLinkProgram(programSphere);

    if (check_link_error(programSphere) < 0)
        exit(1);

    GLuint vertShaderBlit = compile_shader_from_file(GL_VERTEX_SHADER, "shaders/blit.vert");
    GLuint fragShaderBlit = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/blit.frag");
    GLuint programBlit = glCreateProgram();
    glAttachShader(programBlit, vertShaderBlit);
    glAttachShader(programBlit, fragShaderBlit);
    glLinkProgram(programBlit);

    if (check_link_error(programBlit) < 0)
        exit(1);

    // Try to load and compile directionallight shaders
    GLuint fragShaderDirLight = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/dirLight.frag");
    GLuint programDirLight = glCreateProgram();
    glAttachShader(programDirLight, vertShaderBlit);
    glAttachShader(programDirLight, fragShaderDirLight);
    glLinkProgram(programDirLight);
    if (check_link_error(programDirLight) < 0)
        exit(1);

    // Try to load and compile blur shaders
    GLuint fragShaderBlur = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/blur.frag");
    GLuint programBlur = glCreateProgram();
    glAttachShader(programBlur, vertShaderBlit);
    glAttachShader(programBlur, fragShaderBlur);
    glLinkProgram(programBlur);
    if (check_link_error(programBlur) < 0)
        exit(1);

    GLint blurTextureLocation = glGetUniformLocation(programBlur, "Texture");
    glProgramUniform1i(programBlur, blurTextureLocation, 0);
    GLint blurSampleCountLocation = glGetUniformLocation(programBlur, "SampleCount");
    GLint blurDirectionLocation = glGetUniformLocation(programBlur, "Direction");

    // Try to load and compile coc shaders
    GLuint fragShaderCoC = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/coc.frag");
    GLuint programCoC = glCreateProgram();
    glAttachShader(programCoC, vertShaderBlit);
    glAttachShader(programCoC, fragShaderCoC);
    glLinkProgram(programCoC);
    if (check_link_error(programCoC) < 0)
        exit(1);

    GLint cocTextureLocation = glGetUniformLocation(programCoC, "Texture");
    glProgramUniform1i(programCoC, cocTextureLocation, 0);
    GLint cocScreenToViewCountLocation = glGetUniformLocation(programCoC, "ScreenToView");
    GLint cocFocusnLocation = glGetUniformLocation(programCoC, "Focus");

    // Try to load and compile dof shaders
    GLuint fragShaderDoF = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/dof.frag");
    GLuint programDoF = glCreateProgram();
    glAttachShader(programDoF, vertShaderBlit);
    glAttachShader(programDoF, fragShaderDoF);
    glLinkProgram(programDoF);
    if (check_link_error(programDoF) < 0)
        exit(1);

    GLint dofColorLocation = glGetUniformLocation(programDoF, "Color");
    glProgramUniform1i(programDoF, dofColorLocation, 0);
    GLint dofCoCLocation = glGetUniformLocation(programDoF, "CoC");
    glProgramUniform1i(programDoF, dofCoCLocation, 1);
    GLint dofBlurLocation = glGetUniformLocation(programDoF, "Blur");
    glProgramUniform1i(programDoF, dofBlurLocation, 2);

    // Try to load and compile gamma shaders
    GLuint fragShaderGamma = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/gamma.frag");
    GLuint gammaProgramObject = glCreateProgram();
    glAttachShader(gammaProgramObject, vertShaderBlit);
    glAttachShader(gammaProgramObject, fragShaderGamma);
    glLinkProgram(gammaProgramObject);
    if (check_link_error(gammaProgramObject) < 0)
        exit(1);
    GLint gammaTextureLocation = glGetUniformLocation(gammaProgramObject, "Texture");
    glProgramUniform1i(gammaProgramObject, gammaTextureLocation, 0);
    GLint gammaGammaLocation = glGetUniformLocation(gammaProgramObject, "Gamma");

    // Try to load and compile glitches shaders
    GLuint fragShaderGlitch = compile_shader_from_file(GL_FRAGMENT_SHADER, "shaders/glitch.frag");
    GLuint glitchProgramObject = glCreateProgram();
    glAttachShader(glitchProgramObject, vertShaderBlit);
    glAttachShader(glitchProgramObject, fragShaderGlitch);
    glLinkProgram(glitchProgramObject);
    if (check_link_error(glitchProgramObject) < 0)
        exit(1);
    GLint glitchTextureLocation = glGetUniformLocation(glitchProgramObject, "Texture");
    glProgramUniform1i(glitchProgramObject, glitchTextureLocation, 0);
    GLint glitchTimeLocation = glGetUniformLocation(glitchProgramObject, "time");

    /**********
     * UNIFORMS
     *********/

    GLint mvpLocation = glGetUniformLocation(programCubeGrid, "MVP");
    GLint mvLocation = glGetUniformLocation(programCubeGrid, "MV");
    GLint grid_sizeLocation = glGetUniformLocation(programCubeGrid, "grid_size");
    GLint timeLocation = glGetUniformLocation(programCubeGrid, "time");
    GLint colorNearLocation = glGetUniformLocation(programCubeGrid, "colorNear");
    GLint colorFarLocation = glGetUniformLocation(programCubeGrid, "colorFar");
    GLint brightnessLocation = glGetUniformLocation(programCubeGrid, "brightness");
    GLint attenuationLocation = glGetUniformLocation(programCubeGrid, "attenuation");
    GLint cameraPosLocation = glGetUniformLocation(programCubeGrid, "camPos");
    GLint lightDirLocation = glGetUniformLocation(programCubeGrid, "lightDir");
    glProgramUniform3fv(programCubeGrid, lightDirLocation, 1, value_ptr(directionalLightDir));

    GLint blitTextureLocation = glGetUniformLocation(programBlit, "Texture");
    glProgramUniform1i(programBlit, blitTextureLocation, 0);

    GLint directionallightColorLocation = glGetUniformLocation(programDirLight, "ColorBuffer");
    GLint directionallightNormalLocation = glGetUniformLocation(programDirLight, "NormalBuffer");
    GLint directionallightDepthLocation = glGetUniformLocation(programDirLight, "DepthBuffer");
    GLint mvLightLocation = glGetUniformLocation(programDirLight, "MV");
    GLint directionallightLightLocation = glGetUniformBlockIndex(programDirLight, "light");
    GLint directionalInverseProjectionLocation = glGetUniformLocation(programDirLight, "InverseProjection");
    glProgramUniform1i(programDirLight, directionallightColorLocation, 0);
    glProgramUniform1i(programDirLight, directionallightNormalLocation, 1);
    glProgramUniform1i(programDirLight, directionallightDepthLocation, 2);

    GLint mvpSphereLocation = glGetUniformLocation(programSphere, "MVP");
    GLint mvSphereLocation = glGetUniformLocation(programSphere, "MV");
    GLint colorSphereLocation = glGetUniformLocation(programSphere, "Color");
    GLint position_scriptedLocation = glGetUniformLocation(programSphere, "position_scripted");

    if (!checkError("Uniforms"))
        exit(1);


    /*****************
     * Uniform Buffers
     ****************/

    // Update and bind uniform buffer object
    GLuint ubo[1];
    glGenBuffers(1, ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
    GLint uboSize = 0;
    glGetActiveUniformBlockiv(programDirLight, (GLuint) directionallightLightLocation, GL_UNIFORM_BLOCK_DATA_SIZE, &uboSize);

    uboSize = 512;
    glBufferData(GL_UNIFORM_BUFFER, uboSize, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    /***************
     * Blit geometry
     **************/

    // Load geometry
    int quad_triangleCount = 2;
    int quad_triangleList[] = {0, 1, 2, 2, 1, 3};
    float quad_vertices[] =  {-1.f, -1.f, 1.0, -1.f, -1.f, 1.0, 1.0, 1.0};

    // Vertex Array Object
    GLuint quad_vao;
    glGenVertexArrays(1, &quad_vao);

    // Vertex Buffer Objects
    GLuint quad_vbo[2];
    glGenBuffers(2, quad_vbo);

    // Quad
    glBindVertexArray(quad_vao);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_triangleList), quad_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);


    /******
     * Cube
     *****/

    // Load geometry
    int cube_triangleCount = 12;
    int cube_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float cube_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float cube_vertices[] = {-0.5f, 0.f, 0.5, 0.5, 0.f, 0.5, -0.5f, 1.f, 0.5, 0.5, 1.f, 0.5, -0.5f, 1.f, 0.5, 0.5, 1.f, 0.5, -0.5f, 1.f, -0.5f, 0.5, 1.f, -0.5f, -0.5f, 1.f, -0.5f, 0.5, 1.f, -0.5f, -0.5f, 0.f, -0.5f, 0.5, 0.f, -0.5f, -0.5f, 0.f, -0.5f, 0.5, 0.f, -0.5f, -0.5f, 0.f, 0.5, 0.5, 0.f, 0.5, 0.5, 0.f, 0.5, 0.5, 0.f, -0.5f, 0.5, 1.f, 0.5, 0.5, 1.f, 0.5, 0.5, 1.f, -0.5f, -0.5f, 0.f, -0.5f, -0.5f, 0.f, 0.5, -0.5f, 1.f, -0.5f, -0.5f, 1.f, -0.5f, -0.5f, 0.f, 0.5, -0.5f, 1.f, 0.5 };
    float cube_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, };

    // Vertex Array Object
    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Vertex Buffer Objects
    GLuint vbo[4];
    glGenBuffers(4, vbo);

    // Cube
    glBindVertexArray(vao);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangleList), cube_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_STATIC_DRAW);


    /********
     * Sphere
     *******/

    vector<GLuint> sphere_indices;
    vector<GLfloat> sphere_vertices;
    vector<GLfloat> sphere_normals;

    // Construction
    int i, j, indicator{0}, lats{40}, longs{40};

    for(i = 0; i <= lats; i++) {
        double lat0 = pi<double>() * (-0.5 + (double) (i - 1) / lats);
        double z0  = sin(lat0);
        double zr0 =  cos(lat0);

        double lat1 = pi<double>() * (-0.5 + (double) i / lats);
        double z1 = sin(lat1);
        double zr1 = cos(lat1);

        for(j = 0; j <= longs; j++) {
            double lng = 2 * glm::pi<double>() * (double) (j - 1) / longs;
            double x = cos(lng);
            double y = sin(lng);

            sphere_vertices.push_back((float &&) (x * zr0));
            sphere_vertices.push_back((float &&) (y * zr0));
            sphere_vertices.push_back((const float &) z0);
            sphere_indices.push_back((const unsigned int &) indicator);

            vec3 normal = normalize(vec3(x * zr0, y * zr0, z0));
            sphere_normals.push_back(normal.x);
            sphere_normals.push_back(normal.y);
            sphere_normals.push_back(normal.z);

            indicator++;

            sphere_vertices.push_back((float &&) (x * zr1));
            sphere_vertices.push_back((float &&) (y * zr1));
            sphere_vertices.push_back((const float &) z1);
            sphere_indices.push_back((const unsigned int &) indicator);

            normal = normalize(vec3(x * zr1, y * zr1, z1));
            sphere_normals.push_back(normal.x);
            sphere_normals.push_back(normal.y);
            sphere_normals.push_back(normal.z);

            indicator++;
        }
        sphere_indices.push_back(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    }

    // Vertex Array Object
    GLuint sphere_vao;
    glGenVertexArrays(1, &sphere_vao);

    // Vertex Buffer Objects
    GLuint sphere_vbo[4];
    glGenBuffers(4, sphere_vbo);

    glBindVertexArray(sphere_vao);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sphere_vertices.size() * sizeof(GLfloat), &sphere_vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray (0);

    glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo[1]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sphere_normals.size()* sizeof(GLfloat), &sphere_normals[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_vbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere_indices.size() * sizeof(GLuint), &sphere_indices[0], GL_STATIC_DRAW);

    int numsToDraw = sphere_indices.size();


    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    while (!glfwWindowShouldClose(window))
    {
        ImGui_ImplGlfwGL3_NewFrame();

        currentTime = glfwGetTime();

        /**********
        * Viewport
        *********/

        glViewport(0, 0, width, height);

        // Mouse states
        int leftButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT );
        int rightButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT );
        int middleButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_MIDDLE );

        guiStates.turnLock = leftButton == GLFW_PRESS;
        guiStates.zoomLock = rightButton == GLFW_PRESS;
        guiStates.panLock = middleButton == GLFW_PRESS;

        // Camera movements
        int altPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
        if (!altPressed && (leftButton == GLFW_PRESS || rightButton == GLFW_PRESS || middleButton == GLFW_PRESS))
        {
            double x; double y;
            glfwGetCursorPos(window, &x, &y);
            guiStates.lockPositionX = (int) x;
            guiStates.lockPositionY = (int) y;
        }
        if (altPressed == GLFW_PRESS)
        {
            double mousex; double mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            int diffLockPositionX = (int) (mousex - guiStates.lockPositionX);
            int diffLockPositionY = (int) (mousey - guiStates.lockPositionY);
            if (guiStates.zoomLock)
            {
                float zoomDir = 0.0;
                if (diffLockPositionX > 0)
                    zoomDir = -1.f;
                else if (diffLockPositionX < 0 )
                    zoomDir = 1.f;
                camera_zoom(camera, zoomDir * GUIStates::MOUSE_ZOOM_SPEED);
            }
            else if (guiStates.turnLock)
            {
                camera_turn(camera, diffLockPositionY * GUIStates::MOUSE_TURN_SPEED,
                            diffLockPositionX * GUIStates::MOUSE_TURN_SPEED);

            }
            else if (guiStates.panLock)
            {
                camera_pan(camera, diffLockPositionX * GUIStates::MOUSE_PAN_SPEED,
                           diffLockPositionY * GUIStates::MOUSE_PAN_SPEED);
            }
            guiStates.lockPositionX = (int) mousex;
            guiStates.lockPositionY = (int) mousey;
        }

        // Default states
        glEnable(GL_DEPTH_TEST);

        // Clear the front buffer
        glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get camera matrices
        mat4 projection = perspective(45.0f, (float) width / (float) height, 0.1f, 10000.f);

#if DEBUG
        mat4 worldToView = lookAt(camera.eye, camera.o, camera.up);
#else
        int indexEye = int(currentTime * 40) % (cameraSmoothPath.size());
        int indexO = int((currentTime + 1.0 ) * 40) % (cameraSmoothPath.size());
        int indexPrev = int((currentTime - 0.1 ) * 40) % (cameraSmoothPath.size());

        vec3 cameraBezierEye = cameraSmoothPath[indexEye];
        camera.eye = cameraBezierEye;
        vec3 cameraBezierO = cameraSmoothPath[(indexO + indexEye) / 2];
        camera.o = cameraSmoothPath[indexO];
        vec3 cameraPrevious = cameraSmoothPath[indexPrev];


        float cameraAngle = (float) (
                -angleBetween(cameraBezierO - cameraBezierEye, cameraBezierEye - cameraPrevious, vec3(1.f, 0.f, 0.f)) / M_PI);

        camera.up = vec3(0, abs(cos(cameraAngle)), sin(cameraAngle));

        mat4 worldToView = lookAt(cameraBezierEye, cameraBezierO, camera.up);
#endif
        mat4 objectToWorld;
        mat4 mv = worldToView * objectToWorld;
        mat4 mvp = projection * mv;
        mat4 inverseProjection = inverse(projection);


        // Bind gbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);
        // Clear the gbuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Select shader
        glUseProgram(programCubeGrid);

        // Upload uniforms
        glProgramUniformMatrix4fv(programCubeGrid, mvLocation, 1, 0, value_ptr(mv));
        glProgramUniformMatrix4fv(programCubeGrid, mvpLocation, 1, 0, value_ptr(mvp));
        glProgramUniform1i(programCubeGrid, grid_sizeLocation, grid_size);
        glProgramUniform1f(programCubeGrid, timeLocation, currentTime);
        glProgramUniform3fv(programCubeGrid, colorNearLocation, 1, value_ptr(colorNear));
        glProgramUniform3fv(programCubeGrid, colorFarLocation, 1, value_ptr(colorFar));
        glProgramUniform1f(programCubeGrid, brightnessLocation, brightness);
        glProgramUniform1f(programCubeGrid, attenuationLocation, attenuation);
        glProgramUniform3fv(programCubeGrid, cameraPosLocation, 1, value_ptr(camera.eye));

        glProgramUniformMatrix4fv(programDirLight, directionalInverseProjectionLocation, 1, 0, value_ptr(inverseProjection));
        glProgramUniformMatrix4fv(programDirLight, mvLightLocation, 1, 0, value_ptr(mv));

        glProgramUniformMatrix4fv(programSphere, mvSphereLocation, 1, 0, value_ptr(mv));
        glProgramUniformMatrix4fv(programSphere, mvpSphereLocation, 1, 0, value_ptr(mvp));
        glProgramUniform3fv(programSphere, colorSphereLocation, 1, value_ptr(sphereColor));
        glProgramUniform3fv(programSphere, position_scriptedLocation, 1, value_ptr(camera.o));

        // Render vaos

        // Cubes
        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, grid_size * grid_size);

        // Sphere

        glUseProgram(programSphere);

        glBindVertexArray(sphere_vao);
        glEnable(GL_PRIMITIVE_RESTART);
        glPrimitiveRestartIndex(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_vbo[1]);
        glDrawElements(GL_QUAD_STRIP, numsToDraw, GL_UNSIGNED_INT, NULL);
        glDisable(GL_PRIMITIVE_RESTART);

        glBindFramebuffer(GL_FRAMEBUFFER, fxFbo);
        // Attach first fx texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[0], 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        // Select textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);

        glBindVertexArray(quad_vao);


        /**************
         * Light Render
         *************/

        // Render directional lights
        glUseProgram(programDirLight);
        struct DirectionalLight
        {
            vec3 direction;
            int padding;
            vec3 color;
            float intensity;
        };
        for (int i = 0; i < directionalLightCount; ++i)
        {
            glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
            DirectionalLight d = {
                    vec3(directionalLightDir), 0,
                    directionalLightColor,
                    directionalLightIntensity
            };
            DirectionalLight * directionalLightBuffer = (DirectionalLight *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, uboSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            *directionalLightBuffer = d;
            glUnmapBuffer(GL_UNIFORM_BUFFER);
            glBindBufferBase(GL_UNIFORM_BUFFER, (GLuint) directionallightLightLocation, ubo[0]);
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        }

        glUseProgram(programBlit);
        glActiveTexture(GL_TEXTURE0);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        glDisable(GL_BLEND);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);


        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[1], 0);
        glClear(GL_COLOR_BUFFER_BIT);

        // vertical blur
        glUseProgram(programBlur);
        glProgramUniform1i(programBlur, blurSampleCountLocation, sampleCount);
        glProgramUniform2i(programBlur, blurDirectionLocation, 0, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[0]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);


        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[2], 0);
        glClear(GL_COLOR_BUFFER_BIT);

        // horizontal blur
        glProgramUniform2i(programBlur, blurDirectionLocation, 1, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[1]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);


        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[1], 0);
        glClear(GL_COLOR_BUFFER_BIT);

        // CoC compute
        glUseProgram(programCoC);
        glProgramUniform3f(programCoC, cocFocusnLocation, focusPlane, nearPlane, farPlane);
        glProgramUniformMatrix4fv(programCoC, cocScreenToViewCountLocation, 1, 0, glm::value_ptr(inverseProjection));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);


        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[3], 0);
        glClear(GL_COLOR_BUFFER_BIT);

        // dof compute
        glUseProgram(programDoF);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[0]); // Color
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, fxTextures[1]); // CoC
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, fxTextures[2]); // Blur
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[1], 0);
        glClear(GL_COLOR_BUFFER_BIT);

        // Gamma
        glUseProgram(gammaProgramObject);
        glProgramUniform1f(gammaProgramObject, gammaGammaLocation, gamma);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[3]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Glitches
        glUseProgram(glitchProgramObject);
        glProgramUniform1f(glitchProgramObject, glitchTimeLocation, currentTime);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[1]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);



#if DEBUG
        /**************
         * Blit Screens
         *************/

        glUseProgram(programBlit);
        glActiveTexture(GL_TEXTURE0);
        glViewport( 0, 0, width/3, height/4);

        // Bind quad VAO
        glBindVertexArray(quad_vao);
        // Bind gbuffer color texture
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
        // Draw quad
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        // Viewport
        glViewport( width/3, 0, width/3, height/4  );
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
        // Draw quad
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        // Viewport
        glViewport( width/3 * 2, 0, width/3, height/4  );
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
        // Draw quad
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);


        ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Settings");
        ImGui::ColorEdit3("clear color", clearColor);
        ImGui::Text("Grid Size");
        ImGui::RadioButton("10", &grid_size, 10); ImGui::SameLine();
        ImGui::RadioButton("100", &grid_size, 100); ImGui::SameLine();
        ImGui::RadioButton("1000", &grid_size, 1000);
        ImGui::Text("Grid Shader");
        ImGui::ColorEdit3("colorNear", value_ptr(colorNear));
        ImGui::ColorEdit3("colorFar", value_ptr(colorFar));
        ImGui::SliderFloat("Brightness", &brightness, 0.f, 100.f);
        ImGui::SliderFloat("Attenuation", &attenuation, 0.f, 20.f);
        ImGui::Text("Directional light");
        ImGui::ColorEdit3("Dir Light color", value_ptr(directionalLightColor));
        ImGui::SliderFloat3("Dir light dir", value_ptr(directionalLightDir), -1.f, 1.f);
        ImGui::SliderFloat("Dir Light Intensity", &directionalLightIntensity, 0.f, 5.f);
        ImGui::Text("FX");
        ImGui::SliderFloat("Gamma", &gamma, 0.01f, 3.0f);
        ImGui::SliderFloat("Focus plane", &focusPlane, 1.f, 100.f);
        ImGui::SliderFloat("Near plane", &nearPlane, 1.f, 100.f);
        ImGui::SliderFloat("Far plane", &farPlane, 1.f, 100.f);
        ImGui::DragInt("Sample Count", &sampleCount, .1f, 0, 100);

        ImGui::ColorEdit3("colorSphere", value_ptr(sphereColor));
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

#endif
        ImGui::Render();

        // Check for errors
        checkError("End loop");

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplGlfwGL3_Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


// No windows implementation of strsep
char * strsep_custom(char **stringp, const char *delim)
{
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;
    if ((s = *stringp) == NULL)
        return (NULL);
    for (tok = s; ; ) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    return 0;
}

int check_compile_error(GLuint shader, const char ** sourceBuffer)
{
    // Get error log size and print it eventually
    int logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        char *token, *string;
        string = strdup(sourceBuffer[0]);
        int lc = 0;
        while ((token = strsep_custom(&string, "\n")) != NULL) {
            printf("%3d : %s\n", lc, token);
            ++lc;
        }
        fprintf(stderr, "Compile : %s", log);
        delete[] log;
    }
    // If an error happend quit
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
        return -1;
    return 0;
}

int check_link_error(GLuint program)
{
    // Get link error log size and print it eventually
    int logLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetProgramInfoLog(program, logLength, &logLength, log);
        fprintf(stderr, "Link : %s \n", log);
        delete[] log;
    }
    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
        return -1;
    return 0;
}


GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize)
{
    GLuint shaderObject = glCreateShader(shaderType);
    const char * sc[1] = { sourceBuffer };
    glShaderSource(shaderObject,
                   1,
                   sc,
                   NULL);
    glCompileShader(shaderObject);
    check_compile_error(shaderObject, sc);
    return shaderObject;
}

GLuint compile_shader_from_file(GLenum shaderType, const char * path)
{
    FILE * shaderFileDesc = fopen( path, "rb" );
    if (!shaderFileDesc)
        return 0;
    fseek ( shaderFileDesc , 0 , SEEK_END );
    long fileSize = ftell ( shaderFileDesc );
    rewind ( shaderFileDesc );
    char * buffer = new char[fileSize + 1];
    fread( buffer, 1, fileSize, shaderFileDesc );
    buffer[fileSize] = '\0';
    GLuint shaderObject = compile_shader(shaderType, buffer, fileSize );
    delete[] buffer;
    return shaderObject;
}


bool checkError(const char* title)
{
    int error;
    if((error = glGetError()) != GL_NO_ERROR)
    {
        std::string errorString;
        switch(error)
        {
            case GL_INVALID_ENUM:
                errorString = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                errorString = "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                errorString = "GL_INVALID_OPERATION";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                errorString = "GL_OUT_OF_MEMORY";
                break;
            default:
                errorString = "UNKNOWN";
                break;
        }
        fprintf(stdout, "OpenGL Error(%s): %s\n", errorString.c_str(), title);
    }
    return error == GL_NO_ERROR;
}

void camera_compute(Camera & c)
{
    c.eye.x = cos(c.theta) * sin(c.phi) * c.radius + c.o.x;
    c.eye.y = cos(c.phi) * c.radius + c.o.y ;
    c.eye.z = sin(c.theta) * sin(c.phi) * c.radius + c.o.z;
    c.up = glm::vec3(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
}

void camera_defaults(Camera & c)
{
    c.phi = (float) (3.14 / 2.f);
    c.theta = (float) (3.14 / 2.f);
    c.radius = 10.f;
    camera_compute(c);
}

void camera_zoom(Camera & c, float factor)
{
    c.radius += factor * c.radius ;
    if (c.radius < 0.1)
    {
        c.radius = 10.f;
        c.o = c.eye + glm::normalize(c.o - c.eye) * c.radius;
    }
    camera_compute(c);
}

void camera_turn(Camera & c, float phi, float theta)
{
    c.theta += 1.f * theta;
    c.phi   -= 1.f * phi;
    if (c.phi >= (2 * M_PI) - 0.1 )
        c.phi = 0.00001;
    else if (c.phi <= 0 )
        c.phi = (float) (2 * M_PI - 0.1);
    camera_compute(c);
}

void camera_pan(Camera & c, float x, float y)
{
    glm::vec3 up(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
    glm::vec3 fwd = glm::normalize(c.o - c.eye);
    glm::vec3 side = glm::normalize(glm::cross(fwd, up));
    c.up = glm::normalize(glm::cross(side, fwd));
    c.o[0] += up[0] * y * c.radius * 2;
    c.o[1] += up[1] * y * c.radius * 2;
    c.o[2] += up[2] * y * c.radius * 2;
    c.o[0] -= side[0] * x * c.radius * 2;
    c.o[1] -= side[1] * x * c.radius * 2;
    c.o[2] -= side[2] * x * c.radius * 2;
    camera_compute(c);
}

void init_gui_states(GUIStates & guiStates)
{
    guiStates.panLock = false;
    guiStates.turnLock = false;
    guiStates.zoomLock = false;
    guiStates.lockPositionX = 0;
    guiStates.lockPositionY = 0;
    guiStates.camera = 0;
    guiStates.time = 0.0;
    guiStates.playing = false;
}

float angleBetween(vec3 a, vec3 b, vec3 ref)
{
    /*
    float dot = a.x * b.x + a.y * b.y + a.z * b.z;
    float lenSq1 = a.x * a.x + a.y * a.y + a.z * a.z;
    float lenSq2 = b.x * b.x + b.y * b.y + b.z * b.z;
    return acos(dot/sqrt(lenSq1 * lenSq2));
     */

    float angle = acos(dot(normalize(a), normalize(b)));
    vec3 crossProduct = cross(a, b);
    if (float(dot(ref, crossProduct)) < 0.0) { // Or > 0
        angle = -angle;
    }

    return angle;
}