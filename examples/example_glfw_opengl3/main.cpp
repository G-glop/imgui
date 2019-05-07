// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>

// About OpenGL function loaders: modern OpenGL doesn't have a standard header file and requires individual function pointers to be loaded manually.
// Helper libraries are often used for this purpose! Here we are supporting a few common ones: gl3w, glew, glad.
// You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

GLFWwindow* glfw_window;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool init_window() {
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return false;

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    glfw_window = glfwCreateWindow(1280, 720, "Voxel tracing prototype", NULL, NULL);
    if (glfw_window == NULL)
        return false;
    glfwMakeContextCurrent(glfw_window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    ImFontConfig cfg;
    cfg.SizePixels = 15;
    io.Fonts->AddFontDefault(&cfg);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    return true;
}

bool start_frame() {
    if (glfwWindowShouldClose(glfw_window))
        return false;

    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    return true;
}

void render() {
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwMakeContextCurrent(glfw_window);
    glfwGetFramebufferSize(glfw_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwMakeContextCurrent(glfw_window);
    glfwSwapBuffers(glfw_window);
}

void shutdown() {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(glfw_window);
    glfwTerminate();
}

// User code:

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

const int size = 256;
int resolution = 1280;
float fov = 3.14 / 2;
bool arr[size][size] = { false };
float view_scale = 0;
int visited = 0;
int visited_unique = 0;

ImColor col_empty(102, 109, 122);
ImColor col_visited(119, 179, 216);

void draw_square(ImVec2 coords, ImU32 col) {
    auto& drw = *ImGui::GetWindowDrawList();
    coords = ImVec2((int)coords.x, (int)coords.y);
    drw.AddRectFilled(coords * view_scale, ImVec2{ coords.x + 1, coords.y + 1 } *view_scale, col);
}

void walk_ray(ImVec2 start, ImVec2 dir) {
    ImVec2 step(dir.x > 0 ? 1 : -1, dir.y > 0 ? 1 : -1);
    ImVec2 cur((int)start.x, (int)start.y);
    ImVec2 tMax = (cur + step - start) / dir;
    ImVec2 tDelta = ImVec2{ 1 / dir.x, 1 / dir.y } *step;

    while ((cur.x >= 0 && cur.x < size) && (cur.y >= 0 && cur.y < size)) {
        visited++;
        bool& voxel = arr[(int)cur.x][(int)cur.y];
        if (!voxel)
            visited_unique += 1;
        voxel = true;
        if (tMax.x < tMax.y) {
            tMax.x += tDelta.x;
            cur.x += step.x;
        }
        else {
            tMax.y += tDelta.y;
            cur.y += step.y;
        }
    }
}

int main(int, char**) {
    if (!init_window()) {
        printf("error creating window\n");
        return -1;
    }

    while (start_frame()) {
        auto& io = ImGui::GetIO();

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("###fullscreen", NULL,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBringToFrontOnFocus
        );

        // Reset state
        memset(arr, false, sizeof(arr));
        visited = 0;
        visited_unique = 0;

        // Update view paramters
        view_scale = ImMin(io.DisplaySize.x, io.DisplaySize.y) / size;
        ImVec2 view_pos = io.MousePos / view_scale;
        static float view_angle = 0;
        view_angle += io.MouseWheel * 3.14 / 16;
        ImVec2 view_dir = ImRotate({ 1, 0 }, ImCos(view_angle), ImSin(view_angle));

        ImVec2 a = ImRotate(view_dir, ImCos(fov / 2), ImSin(fov / 2));
        ImVec2 b = ImRotate(view_dir, ImCos(-fov / 2), ImSin(-fov / 2));

        for (int i = 0; i < resolution; i++) {
            float angle = (i / (float)resolution - 0.5) * fov;
            walk_ray(view_pos, ImRotate(view_dir, ImCos(angle), ImSin(angle)));
        }

        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                draw_square(ImVec2(x, y), arr[x][y] ? col_visited : col_empty);
            }
        }

        ImGui::Begin("Stats", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("View pos: {%f, %f}\n", view_pos.x, view_pos.y);
        ImGui::Text("View angle: %f {%f, %f}\n", view_angle, view_dir.x, view_dir.y);
        float size = 30;
        ImVec2 origin = ImGui::GetCursorScreenPos() + ImVec2{ size, size };
        ImGui::Dummy({ size * 2, size * 2 });
        auto& drw = *ImGui::GetWindowDrawList();
        drw.AddCircle(origin, size, col_empty, 30);
        drw.AddLine(origin, origin + view_dir * size, col_visited, 4);
        ImGui::Text("Visisted: %d\nUnique: %d (%.1f%%)", visited, visited_unique, visited_unique / (float)visited * 100);
        ImGui::InputFloat("FOV", &fov, 0.1);
        ImGui::InputInt("Resolution", &resolution);
        ImGui::End();

        ImGui::End();
        render();
    }

    shutdown();

    return 0;
}
