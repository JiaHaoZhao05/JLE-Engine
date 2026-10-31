#pragma once
#include "../../Engine/Module.h"
#include "../../Engine/Texture.h"
#include <vector>
class Texture;
class TextureLoader : public Module {
public:
	TextureLoader();

	~TextureLoader();

	bool Awake();
	bool CleanUp();

	//Textures from the fbx
	std::vector<Texture*> InitMaterials(const std::string& filename, bool& hasTexture);

	bool LoadFromFile(Texture* tex, const std::string& fileName, bool& hasTexture);

	bool ChangeTexture(std::vector<Texture*>& m_Tex, unsigned int materialIndex, const std::string& fileName, bool& hasTexture);


};