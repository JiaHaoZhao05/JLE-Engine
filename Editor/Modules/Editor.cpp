#include "Editor.h"
#include "../App.h"
#include "../Log.h"
#include "../../Engine/Modules/Camera.h"
#include "../../Engine/Modules/Render.h"
#include "../../Engine/Modules/Windows.h"
#include "../../Engine/Modules/Input.h"
#include "../../Engine/FileReader.h"
#include "../../Engine/PrimitiveManager.h"
#include "../Engine/GameObject.h"
#include "ModelLoader.h"
#include "TextureLoader.h"

#include <iostream>
#include <memory>

// OpenGL
#include <glad/glad.h>
#include <SDL3/SDL.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGui
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include "ImGuizmo.h"

//GameObject
#include "../Engine/GameObject.h"
#include "../Engine/Components/MaterialComponent.h"
#include "../Engine/Components/MeshComponent.h"
#include "../Engine/Components/TransformComponent.h"

//Primitives
#include "../Engine/Primitives/Cube.h"
#include "../Engine/Primitives/Icosahedron.h"
#include "../Engine/Primitives/Pyramid.h"
#include "../Engine/Primitives/Cylinder.h"
#include "../Engine/Primitives/Tetrahedron.h"
#include "../Engine/Primitives/Sphere.h"

//Scene
#include "../Engine/Modules/Scene.h"

//Additional C++ Headers
#include <vector>
#include <dxgi1_4.h> //<----- needed to see VRAM values
#include <cpuinfo_x86.h> //<----- needed to check the caps of the cpu
#include <filesystem>

Editor::Editor() : Module() {
    name = "Editor";
}

Editor::~Editor() {

}

bool Editor::Awake() {
    SDL_Window* window = App::GetInstance().windows->window;
    SDL_GLContext glContext = App::GetInstance().render->GetGLContext();

    // IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    LOG("ImGui created context");
    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init();

    flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;// | ImGuiConfigFlags_DockingEnable;
    
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
   
    editorSettings = FileReader::GetEditorSettings(); //get editor settings before updating

    if (SDL_EventEnabled(SDL_EVENT_DROP_FILE))
        LOG("SDL_EVENT_DROP_FILE is enabled");
    else LOG("SDL_EVENT_DROP_FILE is not enabled");

    return true;
}

bool Editor::PreUpdate() {
    if (!fileToLoad3D.empty()) {
        ProcessDropFile(fileToLoad3D);
        fileToLoad3D.clear();
    }
    const bool* keyboard = SDL_GetKeyboardState(nullptr);
    if (keyboard[SDL_SCANCODE_X]) {
        return false;
    }
    const std::vector<SDL_Event> events = App::GetInstance().input->GetEvents();
    std::string dropFileDir;
    for (const auto& event : events) {
        switch (event.type) {
        case SDL_EVENT_WINDOW_RESIZED:
            int w, h;
            SDL_GetWindowSize(App::GetInstance().windows->window, &w, &h);
            ResizeWindows(ImVec2(w, h));
            break;
        case SDL_EVENT_DROP_FILE:
            dropFileDir = event.drop.data;
            LOG("Recieved path: " + dropFileDir);
            ProcessDropFile(dropFileDir);
            dropFileDir.clear();
            break;
        default:
            break;
        }
    }
    return true;
}

bool Editor::PostUpdate() {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    ImGui::DockSpaceOverViewport();

    SDL_Window* window = App::GetInstance().windows->window;
    const bool gizmoActive = ImGuizmo::IsOver() || ImGuizmo::IsUsing();
    if (gizmoActive)
    {
        flags |= ImGuiWindowFlags_NoMove;
    }
    if (showScene) {
        
        ImGui::Begin("Scene", nullptr, flags);
        AddWindow("Scene");
        ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
        ImVec2 newSceneWindowSize = ImGui::GetContentRegionAvail();
        App::GetInstance().render->shouldRefreshSceneWindow = (newSceneWindowSize.x != App::GetInstance().render->sceneWindowSize.x || newSceneWindowSize.y != App::GetInstance().render->sceneWindowSize.y);
        App::GetInstance().render->sceneWindowSize = newSceneWindowSize;
        ImGui::Image(App::GetInstance().render->frameBufferObject.RENDER_TO_TEXTURE_ID, newSceneWindowSize, ImVec2(0, 1), ImVec2(1, 0));
        App::GetInstance().input->isHoveringScene = ImGui::IsWindowHovered(0);
        glDisable(GL_DEPTH_TEST);
        ImGuizmo::SetRect(cursorScreenPos.x, cursorScreenPos.y, newSceneWindowSize.x, newSceneWindowSize.y);
        ImGuizmo::SetDrawlist();
        ImGuizmo::Manipulate(glm::value_ptr(App::GetInstance().render->viewMatrix), glm::value_ptr(App::GetInstance().render->projectionMatrix), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, glm::value_ptr(App::GetInstance().render->modelMatrix));
        glEnable(GL_DEPTH_TEST);
        ImGui::End();
    }

    if (editorSettings.ImGuiActive) {
        if (showConsole) {
            App::GetInstance().log.DrawConsole();
        }

        // Header Menu Bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    exit(0);
                }
                if (ImGui::MenuItem("Import 3D")) {
                    FileExplorerFor3D();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Configuraton Window")) {
                    showConfig = !showConfig;
                }
                if (ImGui::MenuItem("Scene")) {
                    showScene = !showScene;
                }
                if (ImGui::MenuItem("Inspector")) {
                    showInspector = !showInspector;
                }
                if (ImGui::MenuItem("Console")) {
                    showConsole = !showConsole;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Github Documentation")) {
                    SDL_OpenURL("https://github.com/ErikArgemi/JLE-Engine#gameenginetemplate");
                }
                if (ImGui::MenuItem("Report a Bug")) {
                    SDL_OpenURL("https://github.com/ErikArgemi/JLE-Engine/issues");
                }
                if (ImGui::MenuItem("Download Latest:")) {
                    SDL_OpenURL("https://github.com/ErikArgemi/JLE-Engine");
                }
                if (ImGui::MenuItem("About")) {
                    showAbout = !showAbout;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        //about us window with license and credits
        if (showAbout)
        {
            ImGui::Begin("About", &showAbout);
            AddWindow("About");

            ImGui::Text("JLE Engine v0.1");
            ImGui::Text("Welcome to the glorious JLE Engine, JLE stands for Jia, Luying and Erik.");
            ImGui::Text("By Group 1: Jia Hao Zhao Deng, Luying Bao Cheng and Erik Argemí Chinchilla");
            ImGui::TextUnformatted(R"(3rd Party Libraries Used:
            - SDL
            - glm
            - sdl3
            - opengl
            - glad
            - imgui
            - imguizmo
            )");
            ImGui::TextUnformatted(R"(MIT License

            Copyright (c) 2026 Jia Hao Zhao Deng & Luying Bao Cheng & Erik Argemí Chinchilla

            Permission is hereby granted, free of charge, to any person obtaining a copy
            of this software and associated documentation files (the "Software"), to deal
            in the Software without restriction, including without limitation the rights
            to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
            copies of the Software, and to permit persons to whom the Software is
            furnished to do so, subject to the following conditions:

            The above copyright notice and this permission notice shall be included in all
            copies or substantial portions of the Software.

            THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
            IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
            FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
            AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
            LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
            OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
            SOFTWARE.)");
            ImGui::End();
        }

        //Configuration window
        if (showConfig) {
            ImGui::Begin("Configuration window", nullptr, flags);
            AddWindow("Configuration window");
            if (ImGui::CollapsingHeader("Application"))
            {
                ImGui::Text("JLE-Engine");
                ImGui::Text("UPC CITM");

                int max_fps_int = App::GetInstance().max_fps;
                ImGui::SliderInt("Max FPS", &max_fps_int, 0, 120, ImGuiSliderFlags(ImGuiSliderFlags_None));
                App::GetInstance().max_fps = max_fps_int;

                char title[30];
                sprintf_s(title, 30, "Framerate %.f", App::GetInstance().fps_log[App::GetInstance().fps_log.size() - 1]);
                ImGui::PlotHistogram("##framerate", &App::GetInstance().fps_log[0], App::GetInstance().fps_log.size(), 0, title, 0.0f, 120.0f, ImVec2(310, 100));
                sprintf_s(title, 30, "Milliseconds %.f", App::GetInstance().ms_log[App::GetInstance().ms_log.size() - 1]);
                ImGui::PlotHistogram("##milliseconds", &App::GetInstance().ms_log[0], App::GetInstance().ms_log.size(), 0, title, 0.0f, 40.0f, ImVec2(310, 100));
            }
            if (ImGui::CollapsingHeader("Window"))
            {
                ImGui::SliderFloat("Brightness", &brightness, 0.1, 1.0, ImGuiSliderFlags(ImGuiSliderFlags_None));

                static int currentSize = 1;
                if (ImGui::Combo("Window sizes", &currentSize, windowSizes, GLM_COUNTOF(windowSizes))) {
                    SDL_SetWindowSize(window, windowSizesValues[currentSize].x, windowSizesValues[currentSize].y);
                    glViewport(0, 0, windowSizesValues[currentSize].x, windowSizesValues[currentSize].y);
                }

                ImGui::Text("Refresh rate: %i", (int)App::GetInstance().fps);

                if (ImGui::Checkbox("Fullscreen", &fullscreen)) {
                    SDL_SetWindowFullscreen(window, fullscreen);
                }

                if (ImGui::Checkbox("Resizable", &resizable)) {
                    SDL_SetWindowResizable(window, resizable);
                }

                if (ImGui::Checkbox("Borderless", &borderless)) {
                    SDL_SetWindowBordered(window, !borderless);
                }
            }
            if (ImGui::CollapsingHeader("Hardware")) {
                //get info from the cpu
                const cpu_features::X86Features features = cpu_features::GetX86Info().features;
                std::string capsCPU = GetCapsFromCpu(features);
                pFactory.reset();
                adapter.reset();

                HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&pFactory);
                pFactory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter**>(&adapter));
                pFactory->Release();
                DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
                adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
                adapter->Release();
                size_t usedVRAM = videoMemoryInfo.CurrentUsage / 1024 / 1024;
                //end of stack overflow copy-paste
                size_t availableVRAM = videoMemoryInfo.AvailableForReservation / 1024 / 1024;
                size_t budgetVRAM = videoMemoryInfo.Budget / 1024 / 1024;
                size_t reservedVRAM = videoMemoryInfo.CurrentReservation / 1024 / 1024;

                ImGui::Separator();
                ImGui::Text("CPUs: %i (Cache: %ikb)", SDL_GetNumLogicalCPUCores(), SDL_GetCPUCacheLineSize());
                int systemRAM = SDL_GetSystemRAM();
                ImGui::Text("System RAM: %.2f %s", (systemRAM > 1024) ? (float)systemRAM / 1024 : (float)systemRAM, (systemRAM > 1024) ? "Gb" : "Mb");
                ImGui::Text("Caps: %s", capsCPU.c_str());
                ImGui::Separator();
                ImGui::Text("Vendor: %s", glGetString(GL_VENDOR));
                ImGui::Text("Brand: %s", glGetString(GL_RENDERER));
                ImGui::Text("VRAM budget: %.2f %s", (budgetVRAM > 1024) ? (float)budgetVRAM / 1024 : (float)budgetVRAM, (budgetVRAM > 1024) ? "Gb" : "Mb");
                ImGui::Text("VRAM usage: %.2f %s", (usedVRAM > 1024) ? (float)usedVRAM / 1024 : (float)usedVRAM, (usedVRAM > 1024) ? "Gb" : "Mb");
                ImGui::Text("VRAM available: %.2f %s", (availableVRAM > 1024) ? (float)availableVRAM / 1024 : (float)availableVRAM, (availableVRAM > 1024) ? "Gb" : "Mb");
                ImGui::Text("VRAM reserved: %.2f %s", (reservedVRAM > 1024) ? (float)reservedVRAM / 1024 : (float)reservedVRAM, (reservedVRAM > 1024) ? "Gb" : "Mb");

            }
            if (ImGui::CollapsingHeader("Camera")) {
                Camera& camera = *App::GetInstance().camera;
                ImGui::SliderFloat("Movement Speed", &camera.movementSpeed, 0.1, 15.0, ImGuiSliderFlags(ImGuiSliderFlags_None));
                ImGui::SliderFloat("Mouse Sensitivity", &camera.sensitivity, 0.1, 2.0, ImGuiSliderFlags(ImGuiSliderFlags_None));
                ImGui::SliderFloat("Panning Sensitivity", &camera.panningSensitivity, 0.01, 1.0, ImGuiSliderFlags(ImGuiSliderFlags_None));

                if (ImGui::Button("Reset To Default Values")) {
                    camera.movementSpeed = 5.0f;
                    camera.sensitivity = 0.5f;
                    camera.panningSensitivity = 0.1f;
                }
            }
            if (ImGui::CollapsingHeader("Primitives")) {
                if (ImGui::Button("Icosahedron")) {
                    App::GetInstance().scene->AddGameObject(App::GetInstance().scene->CreatePrimitive(Icosahedron(), { 0,0,0 }, "Icosahedron"));
                    LOG("Icosahdron created.\n");
                }
                if (ImGui::Button("Cube")) {
                    App::GetInstance().scene->AddGameObject(App::GetInstance().scene->CreatePrimitive(Cube(), { 0,0,0 }, "Cube"));
                    LOG("Cube created.\n");
                }
                if (ImGui::Button("Pyramid")) {
                    App::GetInstance().scene->AddGameObject(App::GetInstance().scene->CreatePrimitive(Pyramid(), { 0,0,0 }, "Pyramid"));
                    LOG("Pyramid created.\n");
                }
                if (ImGui::Button("Sphere")) {
                    App::GetInstance().scene->AddGameObject(App::GetInstance().scene->CreatePrimitive(Sphere(), { 0,0,0 }, "Sphere"));
                    LOG("Sphere created.\n");
                }
                if (ImGui::Button("Cylinder")) {
                    App::GetInstance().scene->AddGameObject(App::GetInstance().scene->CreatePrimitive(Cylinder(), { 0,0,0 }, "Cylinder"));
                    LOG("Cylinder created.\n");
                }
                if (ImGui::Button("Tetrahedron")) {
                    App::GetInstance().scene->AddGameObject(App::GetInstance().scene->CreatePrimitive(Tetrahedron(), { 0,0,0 }, "Tetrahedron"));
                    LOG("Tetrahedron created.\n");
                }
            }
            if (ImGui::CollapsingHeader("Hierarchy")) {
                ShowGameObjectHierarchy(App::GetInstance().scene->GetRootGameObject());
            }
            ImGui::End();
        }

		//Inspector window
        if (showInspector) {
            if (ImGui::Begin("Inspector")) {

                //need the scene to get the currently selected gameobject
                GameObject* selectedObject = App::GetInstance().scene->GetSelectedGameObject();

                if (selectedObject != nullptr) {
                    ImGui::Text("Name: %s", selectedObject->name.c_str());
                    auto* transform = static_cast<TransformComponent*>(selectedObject->GetComponent(ComponentType::TRANSFORM));

                    ImGui::SliderFloat("Transform Increments", &transformIncrements, 0.1, 5.0, ImGuiSliderFlags(ImGuiSliderFlags_None));

                    ImGui::Text("TRAMSFORM");
                    ImGui::Text("");
                    //Position
                    ImGui::Text("Position");
                    ImGui::DragFloat3("P", &transform->position.x, transformIncrements); //DO NOT REMOVE THE STRING IN ANY OF THESE DRAGFLOAT3
                    ImGui::Text("X: %f", transform->position.x);
                    ImGui::Text("Y: %f", transform->position.y);
                    ImGui::Text("Z: %f", transform->position.z);
                    ImGui::Text("");
                    //Rotation
                    ImGui::Text("Rotation");
                    ImGui::DragFloat3("R", &transform->rotation.x, transformIncrements);
                    ImGui::Text("X: %f", transform->rotation.x);
                    ImGui::Text("Y: %f", transform->rotation.y);
                    ImGui::Text("Z: %f", transform->rotation.z);
                    ImGui::Text("");
                    //Scale
                    ImGui::Text("Scale");
                    ImGui::DragFloat3("S", &transform->scale.x, transformIncrements);
                    ImGui::Text("X: %f", transform->scale.x);
                    ImGui::Text("Y: %f", transform->scale.y);
                    ImGui::Text("Z: %f", transform->scale.z);
                    ImGui::Text("");
                    if (ImGui::Button("RESET TO DEFAULT")) {
						transform->position = glm::vec3(0.0f);
                        transform->rotation = glm::vec3(0.0f);
                        transform->scale = glm::vec3(1.0f);
                    }

                    // Mesh information
                    if (ImGui::CollapsingHeader("MESH", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto* meshComponent = static_cast<MeshComponent*>(selectedObject->GetComponent(ComponentType::MESH));
                        if (meshComponent) {
                            Mesh& mesh = meshComponent->mesh;

                            ImGui::Text("Vertices: %i", mesh.m_Positions.size());
                            ImGui::Text("Normals: %i", mesh.m_vertexNormals.size());
                            ImGui::Text("Indices: %i", mesh.m_Indices.size());
                            ImGui::Text("Submeshes: %i", mesh.m_Meshes.size());
                            //ImGui::Text("Textures: %i", mesh.m_Textures.size());

                            ImGui::Text("");

                            ImGui::Checkbox("Show Vertex Normals", &meshComponent->showVertexNormals);
                            ImGui::Checkbox("Show Face Normals", &meshComponent->showFaceNormals);
                        }
                        else {
                            ImGui::Text("No mesh");
                        }
                    }

                    if (ImGui::CollapsingHeader("MATERIAL", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto* materialComponent = static_cast<MeshComponent*>(selectedObject->GetComponent(ComponentType::MESH));
                        if (materialComponent) {

                        }
                        else {
                            ImGui::Text("No material");
                        }
                    }

                }
                else {
                    ImGui::Text("No object selected.");
                }
            }
            ImGui::End();
        }
    }
    // Dark block for brightness
    float opacity = 1.0f - brightness;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::GetForegroundDrawList(viewport)->AddRectFilled(viewport->Pos, ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y), IM_COL32(0, 0, 0, (int)(opacity * 255.0f)));

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    //ImGui::EndFrame();
    // Swap window
    SDL_GL_SwapWindow(window);

    return true;
}

bool Editor::CleanUp() {
    // Deinit ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return true;
}

void Editor::AddWindow(std::string newWindow) {
    for (const auto& window : windows) {
        if (window == newWindow) return;
    }
    windows.emplace_back(newWindow);
}

void Editor::ResizeWindows(ImVec2 windowSize) {
    currentWindowSize = windowSize;
    for (auto& window : windows) {
        RescaleWindow(window);
    }
    prevWindowsSize = currentWindowSize;
}

//privates
void Editor::RescaleWindow(std::string window) {
    ImGuiWindow* win = ImGui::FindWindowByName(window.c_str());
    if (!win) return;

    float scaleX = (float)currentWindowSize.x / (float)prevWindowsSize.x;
    float scaleY = (float)currentWindowSize.y / (float)prevWindowsSize.y;

    ImVec2 newPos(win->Pos.x * scaleX, win->Pos.y * scaleY);
    ImVec2 newSize(win->Size.x * scaleX, win->Size.y * scaleY);

    ImGui::SetWindowPos(window.c_str(), newPos, ImGuiCond_Always);
    ImGui::SetWindowSize(window.c_str(), newSize, ImGuiCond_Always);
}

std::string Editor::GetCapsFromCpu(const cpu_features::X86Features& features) {
    std::string listOfFeatures = "";
    if (features.sse) { listOfFeatures += "SSE, "; }
    if (features.sse2) { listOfFeatures += "SSE2, "; }
    if (features.sse3) { listOfFeatures += "SSE3, "; }
    if (features.ssse3) { listOfFeatures += "SSSE3, "; }
    if (features.sse4_1) { listOfFeatures += "SSE4.1, "; }
    if (features.sse4_2) { listOfFeatures += "SSE4.2, "; }
    if (features.avx) { listOfFeatures += "AVX, "; }
    if (features.avx2) { listOfFeatures += "AVX2, "; }
    if (features.avx512f) { listOfFeatures += "AVX-512F, "; }
    if (features.avx512cd) { listOfFeatures += "AVX-512CD, "; }
    if (features.avx512er) { listOfFeatures += "AVX-512ER, "; }
    if (features.avx512pf) { listOfFeatures += "AVX-512PF, "; }
    if (features.avx512vl) { listOfFeatures += "AVX-512VL, "; }
    if (features.avx512dq) { listOfFeatures += "AVX-512DQ, "; }
    if (features.avx512bw) { listOfFeatures += "AVX-512BW, "; }
    if (features.avx512ifma) { listOfFeatures += "AVX-512IFMA, "; }
    if (features.avx512vbmi) { listOfFeatures += "AVX-512VBMI, "; }
    if (features.avx512_4vnniw) { listOfFeatures += "AVX-512 4VNNIW, "; }
    if (features.avx512_4fmaps) { listOfFeatures += "AVX-512 4FMAPS, "; }
    if (features.avx512vpopcntdq) { listOfFeatures += "AVX-512VPOPCNTDQ, "; }
    if (features.avx512vnni) { listOfFeatures += "AVX-512VNNI, "; }
    if (features.avx512vbmi2) { listOfFeatures += "AVX-512VBMI2, "; }
    if (features.avx512bitalg) { listOfFeatures += "AVX-512BITALG, "; }
    if (features.avx512_vp2intersect) { listOfFeatures += "AVX-512 VP2INTERSECT, "; }
    listOfFeatures.pop_back();
    listOfFeatures.pop_back();
    return listOfFeatures;
}

void Editor::ShowGameObjectHierarchy(GameObject* gameObject)
{
    ImGui::PushID(gameObject);

    App::GetInstance().camera->orbitCentre = static_cast<TransformComponent*>(gameObject->GetComponent(ComponentType::TRANSFORM))->position;

    if (!gameObject->HasChildren()) {
        if (ImGui::Selectable(gameObject->name.c_str(), gameObject == App::GetInstance().scene->GetSelectedGameObject())) {
            App::GetInstance().scene->SetSelectedGameObject(gameObject);
        }
    }
    else {
        ImGuiTreeNodeFlags selected = ImGuiTreeNodeFlags_None;
        if (gameObject == App::GetInstance().scene->GetSelectedGameObject()) {
            selected = ImGuiTreeNodeFlags_Selected;
        }

        if (ImGui::TreeNodeEx(gameObject->name.c_str(), selected)) {
            if (ImGui::IsItemClicked()) {
                App::GetInstance().scene->SetSelectedGameObject(gameObject);
            }

            for (auto& child : gameObject->children) {
                ShowGameObjectHierarchy(child.get());
            }

            ImGui::TreePop();
        }
    }

    ImGui::PopID();
}

void Editor::ProcessDropFile(const std::string fileFullPath) {
    std::filesystem::path fileName = fileFullPath;
    if (fileName.extension() == ".fbx" || 
        fileName.extension() == ".obj") {

        LOG("Loading 3D Model: "+ fileName.filename().string());
        auto newGameObject = std::make_unique<GameObject>();
        newGameObject->name = fileName.filename().string().substr(0, fileName.filename().string().size() - fileName.extension().string().size());
        newGameObject->CreateComponents({ ComponentType::MESH, ComponentType::TRANSFORM, ComponentType::MATERIAL });

        auto* meshComponent = static_cast<MeshComponent*>(newGameObject->GetComponent(ComponentType::MESH));
        auto* transformComponent = static_cast<TransformComponent*>(newGameObject->GetComponent(ComponentType::TRANSFORM));
        auto* materialComponent = static_cast<MaterialComponent*>(newGameObject->GetComponent(ComponentType::MATERIAL));

        meshComponent->mesh = App::GetInstance().modelLoader->LoadMesh(fileFullPath);
        meshComponent->SetData(*meshComponent);
        transformComponent->position = { 0,0,0 };
        materialComponent->m_Textures = App::GetInstance().textureLoader->InitMaterials(fileFullPath, materialComponent->hasTexture);
        if (!materialComponent->hasTexture) {
            App::GetInstance().textureLoader->ChangeTexture(materialComponent->m_Textures, 0, "Data/PrimitiveTextures/checkerboard.dds", materialComponent->hasTexture);
        }
        App::GetInstance().scene->AddGameObject(std::move(newGameObject));
    }
    else if (fileName.extension() == ".png" || 
            fileName.extension() == ".dds") {

        auto target = App::GetInstance().scene->GetSelectedGameObject();
        if (!target) {
            LOG("No Target to update texture");
            return;
        }
        auto* materialComponent = static_cast<MaterialComponent*>(target->GetComponent(ComponentType::MATERIAL));
        if (!materialComponent) {
            LOG("No texture to update"); 
        }
        else {
            App::GetInstance().textureLoader->ChangeTexture(materialComponent->m_Textures, 0, fileFullPath, materialComponent->hasTexture);
        }
    }
    else {
        LOG("Unsupported file format");
    }
}

void Editor::FileExplorerFor3D() {
    std::string selectedPath;
    const SDL_DialogFileFilter filters[] = {
        { "3D Models", "fbx;obj" }
    };

    SDL_ShowOpenFileDialog([](void* userdata, const char* const* filelist, int filter) {
        if (!filelist) {
            LOG("An error occured: %s", SDL_GetError());
            return;
        }
        if (filelist[0] == NULL) {
            return;
        }
        std::string path = filelist[0];
        LOG("Loaded file path: " + path);
        App::GetInstance().editor->Set3DFileToLoad(path);
        },
        nullptr, nullptr, filters, 1, nullptr, false
    );
}