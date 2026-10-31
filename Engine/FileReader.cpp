#include "FileReader.h"
#include <fstream>

json FileReader::GetFile(const char* path) {
	std::ifstream file(path);
	json to_json;
	file >> to_json;
	if (to_json.empty()) {
		//log: the json file is not correct
		return json();
	}

	return to_json;
}

RenderSettings FileReader::GetRenderSettings()
{
	json file = GetFile("Editor/configFile/render_settings.json");

	int x = file["currentResolution"]["x"];
	int y = file["currentResolution"]["y"];

	return RenderSettings(x, y);
}

void FileReader::SetRenderSettings(const RenderSettings& renderSettings)
{
	const char* path = "Editor/configFile/render_settings.json";
	json renderSettingsFile = GetFile(path);

	renderSettingsFile["currentResolution"]["x"] = renderSettings.windowsSizeX;
	renderSettingsFile["currentResolution"]["y"] = renderSettings.windowsSizeY;

	std::ofstream out(path);
	out << std::setw(4) << renderSettingsFile << std::endl;
}

EditorSettings FileReader::GetEditorSettings()
{
	json file = GetFile("Editor/configFile/editor_settings.json");

	bool showAbout = file["showAbout"];
	bool showConfig = file["showConfig"];
	bool showScene = file["showScene"];
	bool showInspector = file["showInspector"];
	bool showConsole = file["showConsole"];
	bool showHierarchy = file["showHierarchy"];

	return EditorSettings(showAbout, showConfig, showScene, showInspector, showConsole, showHierarchy);
}

void FileReader::SetEditorSettings(const EditorSettings& editorSettings)
{
	const char* path = "Editor/configFile/editor_settings.json";
	json editorSettingsFile = GetFile(path);

	editorSettingsFile["showAbout"] = editorSettings.showAbout;
	editorSettingsFile["showConfig"] = editorSettings.showConfig;
	editorSettingsFile["showScene"] = editorSettings.showScene;
	editorSettingsFile["showInspector"] = editorSettings.showInspector;
	editorSettingsFile["showConsole"] = editorSettings.showConsole;
	editorSettingsFile["showHierarchy"] = editorSettings.showHierarchy;

	std::ofstream out(path);
	out << std::setw(4) << editorSettingsFile << std::endl;
}

WindowSettings FileReader::GetWindowSettings() {
	json file = GetFile("Editor/configFile/window_settings.json");

	bool fullScreen = file["fullScreen"];
	bool resizable = file["resizable"];
	bool borderless = file["borderless"];

	return WindowSettings(fullScreen, resizable, borderless);
}

void FileReader::SetWindowSettings(const WindowSettings& windowSettings)
{
	const char* path = "Editor/configFile/window_settings.json";
	json windowSettingsFile = GetFile(path);

	windowSettingsFile["fullScreen"] = windowSettings.fullScreen;
	windowSettingsFile["resizable"] = windowSettings.resizable;
	windowSettingsFile["borderless"] = windowSettings.borderless;

	std::ofstream out(path);
	out << std::setw(4) << windowSettingsFile << std::endl;
}