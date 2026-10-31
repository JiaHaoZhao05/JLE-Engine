#pragma once
#include "../Module.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>

#include "../settings.h"

using json = nlohmann::json;

class FileReader {
public:
	FileReader() {};
	~FileReader() {}

	static RenderSettings GetRenderSettings();
	static void SetRenderSettings(const RenderSettings& renderSettings);

	static EditorSettings GetEditorSettings();
	static void SetEditorSettings(const EditorSettings& editorSettings);

	static WindowSettings GetWindowSettings();
	static void SetWindowSettings(const WindowSettings& windowSettings);

private:
	static json GetFile(const char* path);

};