#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>

#include "glad.h"
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct SPF_Entry {
    char path[128];
    int offset;
    int length;
    int index;
};

static char SPF_Path[1024];
static std::vector<SPF_Entry> files;
static int selected = 0;

static GLuint image;
static int width, height;
static int image_has_data = 0;

// https://stackoverflow.com/questions/744766/how-to-compare-ends-of-strings-in-c
static int EndsWith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

static void ExtractImage(SPF_Entry& ent) {
    char *l = strrchr(ent.path, '/');
    *l = 0;

    std::filesystem::create_directories(ent.path);
    *l = '/';

    std::ifstream in(SPF_Path, std::ios::binary);
    std::ofstream out(ent.path, std::ios::binary);
    if (in.is_open() && out.is_open()) {
        in.seekg(ent.offset);
        char *buf = new char[ent.length];
        in.read(buf, ent.length);
        out.write(buf, ent.length);
        delete [] buf;
    }
}

static void ShowTreeWindow() {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Tree", nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::End();
        return;
    }

    // ImGui::InputText returns true when text changes
    if (ImGui::InputText("<- Input SPF Path", SPF_Path, sizeof(SPF_Path))) {
        std::ifstream spf(SPF_Path, std::ios::binary);
        // On Linux, ifstream could even open folders! So the latter check is a must.
        if (spf.is_open() && std::filesystem::is_regular_file(SPF_Path)) {
            SPF_Entry ent;

            // Read the second last entry: the very last entry is dummy
            spf.seekg(-280, std::ios::end);
            spf.read((char*)&ent, sizeof(ent));

            int first_entry_pos = ent.offset + ent.length;
            spf.seekg(first_entry_pos);

            files.clear();
            while (true) { 
                spf.read((char*)&ent, sizeof(ent));

                if (ent.offset == 0 && ent.length == 0)
                    break;

                files.emplace_back(ent);
            }
        }
        else {
            files.clear();
            selected = 0;
            image_has_data = 0;
            glBindTexture(GL_TEXTURE_2D, image);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
    }

    if (!std::filesystem::exists(SPF_Path)) {
        ImGui::Text("Specified file not found");
    }
    else {
        ImGui::Text("Specified file found");
        if (std::filesystem::is_directory(SPF_Path) || (!EndsWith(SPF_Path, ".SPF") && !EndsWith(SPF_Path, ".spf"))) {
            ImGui::Text("But it's not an SPF file");
        }
        else {
            ImGui::Text("Here is its contents:");
        }
    }

    // Returns true when selection changes
    bool changed = ImGui::ListBox("<- Files in SPF", &selected,
        [](void *data, int index, const char **output){
            auto ent = (SPF_Entry *)data;
            *output = ent[index].path;
            return true;
        },
        files.data(), files.size(), 30);

    if (changed) {
        std::ifstream spf(SPF_Path, std::ios::binary);

        auto& ent = files[selected];
        spf.seekg(ent.offset);

        stbi_uc *buf = new stbi_uc[ent.length];
        spf.read((char*)buf, ent.length);

        stbi_uc *data = stbi_load_from_memory(buf, ent.length, &width, &height, NULL, 4);

        glBindTexture(GL_TEXTURE_2D, image);
        if (data) {
            image_has_data = 1;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        else {
            image_has_data = 0;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }

        stbi_image_free(data);
        delete [] buf;
    }

    ImGui::End();
}

static void ShowImageWindow() {
    ImGui::SetNextWindowPos(ImVec2(400, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Image", nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {
        if (image_has_data) {
            if (ImGui::Button("Extract this image")) {
                ExtractImage(files[selected]);
            }
            ImGui::Image((ImTextureID)(intptr_t)image, ImVec2(width, height));
        }
        else {
            ImGui::Text("No image selected or image not displayable.");
        }
    }
    ImGui::End();
}

int main() {
    glfwInit();
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *win = glfwCreateWindow(900, 600, "SPF Viewer", nullptr, nullptr);
    glfwMakeContextCurrent(win);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "SPF_Viewer.ini";
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init();

    gladLoadGL(); // imgui has it own loader, so no need to put this line before imgui initialization code
    glfwShowWindow(win);

    glGenTextures(1, &image);
    glBindTexture(GL_TEXTURE_2D, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    while (!glfwWindowShouldClose(win)) {
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ShowTreeWindow();
        ShowImageWindow();

        ImGui::Render(); // Generate draw data
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(win);
        glfwWaitEvents(); // Sleep, reduce cpu load.
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(win);
    glfwTerminate();
}
