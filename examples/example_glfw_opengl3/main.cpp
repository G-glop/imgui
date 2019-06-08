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

#include <vector>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "imstb_rectpack.h"

#undef min
#undef max
#undef INFINITE

#include "../msdfgen/msdfgen.h"
#include "../msdfgen/msdfgen-ext.h"
#include "../msdfgen/core/estimate-sdf-error.h"
#pragma comment(lib, "../../examples/Debug Library/msdfgen.lib")
#pragma comment(lib, "../../msdfgen/freetype/win32/freetype.lib")

#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, (a))

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static bool check_shader(GLuint handle, const char* desc)
{
    GLint status = 0, log_length = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr, "ERROR: failed to compile %s!\n", desc);
    if (log_length > 1)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetShaderInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

// If you get an error please report on GitHub. You may try different GL context version or GLSL version.
static bool check_program(GLuint handle, const char* desc)
{
    GLint status = 0, log_length = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr, "ERROR: failed to link %s!\n", desc);
    if (log_length > 1)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glGetProgramInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

int main(int, char**) {
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

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
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
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
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    //int x, y;
    //void* image = stbi_load("../../output.png", &x, &y, NULL, 3);
    //GLuint msdf_texture;
    //glGenTextures(1, &msdf_texture);
    //glBindTexture(GL_TEXTURE_2D, msdf_texture);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

    const GLchar* msdf_vs_source = R"END(
    #version 130
    uniform mat4 ProjMtx;
    in vec2 Position;
    in vec2 UV;
    in vec4 Color;
    out vec2 Frag_UV;
    out vec4 Frag_Color;
    void main(){
        Frag_UV = UV;
        Frag_Color = Color;
        gl_Position = ProjMtx * vec4(Position.xy,0,1);
    };
)END";

    const GLchar* msdf_fs_source = R"END(
    #version 130
    uniform sampler2D Texture;
    uniform float pxRange;

    in vec2 Frag_UV;
    in vec4 Frag_Color;
    out vec4 Out_Color;

    float median(float r, float g, float b) {
        return max(min(r, g), min(max(r, g), b));
    }

    void main(){

        vec3 sample = texture(Texture, Frag_UV).rgb;
        float sigDist = median(sample.r, sample.g, sample.b) - 0.5;
        vec2 msdfUnit = pxRange/vec2(textureSize(Texture, 0));
        sigDist *= dot(msdfUnit, 0.5/fwidth(Frag_UV));
        float opacity = clamp(sigDist + 0.5, 0.0, 1.0);
        Out_Color = Frag_Color * opacity;

        //Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
    };
)END";

    GLuint vs, fs, msdf_shader;
    vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &msdf_vs_source, 0);
    glCompileShader(vs);
    check_shader(vs, "vertex shader");

    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &msdf_fs_source, 0);
    glCompileShader(fs);
    check_shader(fs, "fragment shader");

    msdf_shader = glCreateProgram();
    glAttachShader(msdf_shader, vs);
    glAttachShader(msdf_shader, fs);
    glLinkProgram(msdf_shader);
    check_program(msdf_shader, "msdf program");

    GLuint msdf_texture;
    glGenTextures(1, &msdf_texture);
    glBindTexture(GL_TEXTURE_2D, msdf_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load glyph data
    namespace df = msdfgen;

    struct Shape {
        df::Shape shape; ImVec2 off, size;
    };
    std::vector<Shape> shapes;

    {
        auto* freetype = df::initializeFreetype();
        auto* font = df::loadFont(freetype, "../../misc/fonts/Cousine-Regular.ttf");

        //for (int i = 33; i < 127; i++) {
        for (int i = 'A'; i < 'A' + 1; i++) {
            df::Shape shape;
            if (df::loadGlyph(shape, font, i)) {
                shape.inverseYAxis = true;
                shape.normalize();
                df::edgeColoringSimple(shape, 3.0);

                double l = DBL_MAX, b = DBL_MAX, r = -DBL_MAX, t = -DBL_MAX;
                shape.bounds(l, b, r, t);
                ImVec2 off((float)l, (float)b);
                ImVec2 size = ImVec2((float)r, (float)t) - off;
                printf("character: %c, %f %f %f %f\n", i, l, b, r, t);
                shapes.push_back({ shape, off, size });
            }
        }
        //df::destroyFont(font);
        //df::deinitializeFreetype(freetype);
    }

    float scale = 1;
    int padding = 2;
    int wh;

    auto run_packing = [&]() {
        // Pack glyph areas into an atlas
        ImVector<stbrp_rect> rects;
        {
            rects.resize(shapes.size());
            int area = 0;
            for (size_t i = 0; i < shapes.size(); i++) {
                ImVec2 size = shapes[i].size;
                stbrp_rect& r = rects[i];
                r.w = (stbrp_coord)ceil(size.x * scale + padding * 2);
                r.h = (stbrp_coord)ceil(size.y * scale + padding * 2);
                area += r.w * r.h;
            }
            wh = (int)ceil(sqrt(area * 1.5));
            //wh = (int)ceil(sqrt(area * 2));

            ImVector<stbrp_node> nodes;

            nodes.resize(wh * 2);
            stbrp_context cont = { 0 };
            stbrp_init_target(&cont, wh, wh, nodes.begin(), nodes.size());
            stbrp_pack_rects(&cont, rects.begin(), rects.size());
        }

        // Render them
        {
            df::Bitmap<ImU8, 3> image(ALIGN_UP(wh, 4), wh);
            auto conv = [](float x) {
                return (ImU8)fminf(fmaxf(x*256.0f, 0.0f), 255.0f);
            };

            for (size_t i = 0; i < shapes.size(); i++) {
                Shape& s = shapes[i];
                stbrp_rect r = rects[i];
                if (!r.was_packed)
                    continue;
                df::Bitmap<float, 3> msdf(r.w, r.h);

                ImVec2 glyph_center = s.size / 2 + s.off;
                ImVec2 image_center = ImVec2(r.w, r.h) / 2;
                ImVec2 tr = image_center / scale - glyph_center;

                df::generateMSDF(msdf, s.shape, 2, scale, { tr.x, tr.y });
                for (int y = 0; y < msdf.height(); y++) {
                    for (int x = 0; x < msdf.width(); x++) {
                        float *a = msdf(x, y);
                        ImU8 *b = image(r.x + x, r.y + y);
                        b[0] = conv(a[0]);
                        b[1] = conv(a[1]);
                        b[2] = conv(a[2]);
                    }
                }
            }

            glBindTexture(GL_TEXTURE_2D, msdf_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.width(), image.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, (ImU8*)image);
        }
    };

    run_packing();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        ImGui::ShowDemoWindow();

        /* TODO:
        minimize SDF size for each glyph
        -automatically - would need a better way to evaluate accuracy than the scanline API
        -manually - only ~100 glyphs for ASCII, persistent storeage
        parallelize + async
        -or persistently cache the SDF
        */

        ImGui::Begin("MSDF test");

        //double min = 1e-3;
        //if (ImGui::DragFloat("PX range", &pxrange, 0.1f, 1, 100)) {
        //    run_packing();
        //}
        //if (ImGui::DragScalar("Scale", ImGuiDataType_Double, &scale.x, 0.01f, &min)) {
        //    scale.y = scale.x;
        //    run_packing();
        //}
        //if (ImGui::DragScalarN("Translate", ImGuiDataType_Double, &trans.x, 2, 0.1f)) {
        //    run_packing();
        //}

        static bool show_raw = false;
        ImGui::Checkbox("Show raw MSDF", &show_raw);

        if (ImGui::DragFloat("Scale", &scale, 0.01f, 0.1f, 10))
            run_packing();
        //if (ImGui::DragInt("Padding", &padding, 0.1f, 1, 100))
        //    run_packing();

        static float px_range = 2;
        ImGui::SliderFloat("Shader PX range", &px_range, 0.01f, 10.0f);

        int w, h;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
        ImGui::Text("Atlas size: %d:%d, %d bytes", w, h, w * h * 3);

        if (!show_raw) {
            auto cb = [&]() {
                GLint prog;
                glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
                int proj = glGetUniformLocation(prog, "ProjMtx");
                GLfloat matrix[4 * 4];
                glGetUniformfv(prog, proj, matrix);
                glUseProgram(msdf_shader);
                int proj_new = glGetUniformLocation(msdf_shader, "ProjMtx");
                glUniformMatrix4fv(proj_new, 1, GL_FALSE, matrix);
                glUniform1f(glGetUniformLocation(msdf_shader, "pxRange"), px_range);
            };

            auto& drw = *ImGui::GetWindowDrawList();
            drw.AddCallback([](const ImDrawList*, const ImDrawCmd* cmd) {
                (*static_cast<decltype(cb)*>(cmd->UserCallbackData))();
            }, &cb);
            ImGui::Image((ImTextureID)msdf_texture, ImGui::GetContentRegionAvail());
            drw.AddCallback(ImDrawCallback_ResetRenderState, NULL);
        }
        else {
            ImGui::Image((ImTextureID)msdf_texture, ImGui::GetContentRegionAvail());
        }

        ImGui::End();

        // Rendering
        int display_w, display_h;
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
