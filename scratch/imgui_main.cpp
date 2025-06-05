#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "TextEditor.h"
#include <stdio.h>
#include <string>
#include <sstream>

#include "ast_printer.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include "vm/compiler.hpp"
#include "vm/vm.hpp"

#include "file.hpp"

bool noLoop = false;
bool debug_trace_exeuction = false;
bool disassemble = false;

void runSource(const std::string& text);


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

std::stringstream consoleOutput;
std::string consoleText;

// File management variables
std::string currentFilePath = "untitled.rhy";
char filePathBuffer[512] = "untitled.rhy";
bool hasUnsavedChanges = false;
std::string lastSavedContent = "";


// Custom streambuf to redirect stdout
class ConsoleBuffer : public std::streambuf {
public:
    ConsoleBuffer() {
        original = std::cout.rdbuf(this);
    }

    ~ConsoleBuffer() override {
        std::cout.rdbuf(original);
    }

protected:
    int overflow(int c) override {
        if (c != EOF) {
            consoleOutput << static_cast<char>(c);
        }
        return c;
    }

private:
    std::streambuf* original;
};


int main() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
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


    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL2 example", nullptr, nullptr);
    if (window == nullptr)
        return 2;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    auto font_cfg = ImFontConfig();
    font_cfg.RasterizerDensity = 2;
    ImFont* font = io.Fonts->AddFontFromFileTTF("/tmp/Roboto-Medium.ttf", 16.0f,&font_cfg);
    IM_ASSERT(font != nullptr);
    ImFont* monoFont = io.Fonts->AddFontFromFileTTF("/tmp/LiberationMono-Regular.ttf", 16.0f, &font_cfg);
    IM_ASSERT(monoFont != nullptr);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    TextEditor editor;
    editor.SetReadOnly(false);
    editor.SetText("Hello, World!");
    auto lang = TextEditor::LanguageDefinition::Rhythm();
    editor.SetLanguageDefinition(lang);
    editor.SetShowWhitespaces(false);
    editor.SetHandleKeyboardInputs(true);
    std::string display;
    bool autoScroll = true;


    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Console", nullptr, ImGuiWindowFlags_MenuBar);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::MenuItem("Clear")) {
                    consoleOutput.str("");
                    consoleOutput.clear();
                    consoleText.clear();
                }
                ImGui::Checkbox("Auto-scroll", &autoScroll);
                ImGui::EndMenuBar();
            }

            // Update console text from buffer
            consoleText = consoleOutput.str();

            // Console output area
            ImGui::BeginChild("ConsoleOutput", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(consoleText.c_str());

            // Auto scroll to bottom
            if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }

            ImGui::EndChild();
            ImGui::End();
        }

        {
            // Window title with unsaved changes indicator
            std::string windowTitle = "TextEditor - " + currentFilePath;
            if (hasUnsavedChanges) {
                windowTitle += " *";
            }
            ImGui::Begin(windowTitle.c_str(), 0, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New", "Ctrl+N")) {
                        editor.SetText("");
                        currentFilePath = "untitled.rhythm";
                        strcpy(filePathBuffer, currentFilePath.c_str());
                        lastSavedContent = "";
                        hasUnsavedChanges = false;
                    }

                    if (ImGui::MenuItem("Open", "Ctrl+O")) {
                        std::string content;
                        if (loadFile(filePathBuffer, content)) {
                            editor.SetText(content);
                            currentFilePath = filePathBuffer;
                            lastSavedContent = content;
                            hasUnsavedChanges = false;
                            consoleOutput << "File loaded: " << currentFilePath << std::endl;
                        } else {
                            consoleOutput << "Failed to load file: " << filePathBuffer << std::endl;
                        }
                    }

                    if (ImGui::MenuItem("Save", "Ctrl+S")) {
                        std::string content = editor.GetText();
                        if (saveFile(currentFilePath, content)) {
                            lastSavedContent = content;
                            hasUnsavedChanges = false;
                            consoleOutput << "File saved: " << currentFilePath << std::endl;
                        } else {
                            consoleOutput << "Failed to save file: " << currentFilePath << std::endl;
                        }
                    }

                    if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
                        std::string content = editor.GetText();
                        if (saveFile(filePathBuffer, content)) {
                            currentFilePath = filePathBuffer;
                            lastSavedContent = content;
                            hasUnsavedChanges = false;
                            consoleOutput << "File saved as: " << currentFilePath << std::endl;
                        } else {
                            consoleOutput << "Failed to save file: " << filePathBuffer << std::endl;
                        }
                    }

                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
           // File path input
            ImGui::Text("File:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-150);
            if (ImGui::InputText("##filepath", filePathBuffer, sizeof(filePathBuffer))) {
                // Update current file path when user types
            }

            ImGui::SameLine();
            if (ImGui::Button("Run")) {
                ConsoleBuffer consoleBuffer;

                // Clear previous output
                consoleOutput.str("");
                consoleOutput.clear();

                auto text = editor.GetText();
                try {
                    std::cout << "=== Running Code ===\n";
                    const auto start{std::chrono::steady_clock::now()};
                    runSource(text);
                    const auto finish{std::chrono::steady_clock::now()};
                    std::cout << "=== Execution Complete ===\n";
                    const std::chrono::duration<double> elapsed_seconds{finish - start};
                    std::cout << "Time elapsed(s) " << elapsed_seconds << '\n'; // C++20's chrono::duration operator<<
                } catch (const std::exception& e) {
                    std::cout << "Error: " << e.what() << "\n";
                } catch (...) {
                    std::cout << "Unknown error occurred during execution.\n";
                }
            }

            // Quick save/load buttons
            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                std::string content = editor.GetText();
                if (saveFile(currentFilePath, content)) {
                    lastSavedContent = content;
                    hasUnsavedChanges = false;
                    consoleOutput << "File saved: " << currentFilePath << std::endl;
                } else {
                    consoleOutput << "Failed to save file: " << currentFilePath << std::endl;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                std::string content;
                if (loadFile(filePathBuffer, content)) {
                    editor.SetText(content);
                    currentFilePath = filePathBuffer;
                    lastSavedContent = content;
                    hasUnsavedChanges = false;
                    consoleOutput << "File loaded: " << currentFilePath << std::endl;
                } else {
                    consoleOutput << "Failed to load file: " << filePathBuffer << std::endl;
                }
            }
            ImGui::PushFont(monoFont);
            editor.Render("Some text to edit");
            ImGui::PopFont();
            ImGui::End();
        }
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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


void runSource(const std::string& text) {
    auto scanner = Scanner(text);
    std::vector<Token> tokens = scanner.scanTokens();
    auto parser = Parser(tokens);
    auto stmts = parser.parse();
    AstPrinter printer;
    // printer.print(stmts);
    VM vm{};

    Compiler compiler{nullptr};
    compiler.clear();
    auto block =  BlockStmt::create(std::move(stmts),0);
    auto script = compiler.compileBeatFunction(block, "", 0, BeatFunctionType::SCRIPT);

    // script->chunk.disassembleChunk("test chunk");
    vm.run(new BeatClosure(script)); // leaking? probably fine if this is a script
}