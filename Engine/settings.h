#pragma once
struct RenderSettings {
	uint16_t windowsSizeX;
	uint16_t windowsSizeY;

	RenderSettings() {
		windowsSizeX = 1920;
		windowsSizeY = 1080;
	}

	RenderSettings(const int& x, const int& y) {
		windowsSizeX = x;
		windowsSizeY = y;
	}
};

struct EditorSettings {
	bool showAbout;
	bool showConfig;
	bool showScene;
	bool showInspector;
	bool showConsole;

	EditorSettings() {
		showAbout = false;
		showConfig = true;
		showScene = true;
		showInspector = true;
		showConsole = true;
	}

	EditorSettings(const bool& showAbout, const bool& showConfig, const bool& showScene, const bool& showInspector, const bool& showConsole) {
		this->showAbout = showAbout;
		this->showConfig = showConfig;
		this->showScene = showScene;
		this->showInspector = showInspector;
		this->showConsole = showConsole;
	}
};

struct WindowSettings {
	bool fullScreen;
	bool resizable;
	bool borderless;

	WindowSettings() {
		fullScreen = true;
		resizable = true;
		borderless = true;
	}

	WindowSettings(const bool& fullScreen, const bool& resizable, const bool& borderless) {
		this->fullScreen = fullScreen;
		this->resizable = resizable;
		this->borderless = borderless;
	}
};