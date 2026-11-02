#pragma once
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include "../../Engine/Module.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <SDL3/SDL.h>

#include <dxgi1_4.h> //<----- needed to see VRAM values
#include <cpuinfo_x86.h> //<----- needed to check the caps of the cpu

#include "../../Engine/settings.h"

class GameObject;

class Editor : public Module {
public:
    Editor();

    ~Editor();

    // Called before render is available
    bool Awake();

    // Called each loop iteration before the main update
    bool PreUpdate();

    // Called each loop iteration after the main update 
    bool PostUpdate();

    // Called before quitting
    bool CleanUp();

    //Called to add a window from ImGui
    void AddWindow(std::string newWindow);

    //Called when ALL the windows are in need of a resize
    void ResizeWindows(ImVec2 windowSize);

    //For Loading
    void Set3DFileToLoad(std::string name) { fileToLoad3D = name; }
    void SetTextureToLoad(std::string name) { fileTextureToChange = name; }
private:
    //Called to rescale a window
    void RescaleWindow(std::string window);

    //checks individually each and every feature avalailable from the cpu and returns a string of all of them
    std::string GetCapsFromCpu(const cpu_features::X86Features& features);

    //for showing in the Hierarchy
    void ShowGameObjectHierarchy(GameObject* node);

    //Process dropfile
    void ProcessDropFile(const std::string fileFullPath);

    //Open File Explorer to import models
    void FileExplorerFor3D();

    //OpenFile Explorer to Import/Change Textures
    void FileExplorerTextures();
public:
    // Window Resolution
    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;

    //ImGui flags
    ImGuiWindowFlags flags;

private:
    std::string fileToLoad3D;
    std::string fileTextureToChange;
    std::vector<std::string> windows;
    ImVec2 currentWindowSize;
    ImVec2 prevWindowsSize{ 1920, 1080 };

    //maybe return them as pointers
    std::shared_ptr<IDXGIFactory4> pFactory;
    std::shared_ptr<IDXGIAdapter3> adapter;

    const ImVec2 windowSizesValues[4] = {
                ImVec2(1080, 720),
                ImVec2(1920, 1080),
                ImVec2(2560, 1440),
                ImVec2(3840, 2160)
    };
    const char* windowSizes[4] = {
        "1080x720",
        "1920x1080",
        "2560x1440",
        "3840x2160"
    };

    //Windows settings
    WindowSettings windowSettings;

    float brightness = 1.0f;

    float transformIncrements = 0.1f;

    //EditorSettings
    EditorSettings editorSettings;
};