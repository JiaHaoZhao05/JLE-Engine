#pragma once
#include "../Component.h"
#include <glad/glad.h>
#include "IL/il.h"
#include "../Texture.h"
#include <vector>

class MaterialComponent : public Component {
public:
	MaterialComponent() { type = ComponentType::MATERIAL; }
	MaterialComponent(GameObject* owner) { this->owner = owner; type = ComponentType::MATERIAL; }
	~MaterialComponent() {}
	
	void Enable() { active = true; }
	void Update() {}
	void Disable() { active = false; }

	void CleanUp() override;

	ILint width = 1;
	ILint height = 1;
	GLuint id = 0;

	bool changeToCheckers = false;
	bool checkersTexture = false;
	bool hasTexture = false;
	std::vector<Texture*>  m_Textures;
private:
	void CreateDefaultTexture();
};