#include "TextureLoader.h"
#include "../../Engine/Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <filesystem>
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../Log.h"
#include <il.h>

#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)

TextureLoader::TextureLoader() : Module() {
	name = "TextureLoader";
}

TextureLoader::~TextureLoader() {

}
bool TextureLoader::Awake() {
	ilInit();

	return true;
}

bool TextureLoader::CleanUp() {
	ilShutDown();

	return true;
}

std::vector<Texture*> TextureLoader::InitMaterials(const std::string& filename, bool& hasTexture) {
	std::vector<Texture*> newTextures;

	Assimp::Importer importer;
	const aiScene* pScene = importer.ReadFile(filename.c_str(), ASSIMP_LOAD_FLAGS);

	//newTextures.resize(pScene->mNumMaterials); //copied but causes crash crash

	//Extract the directory part from the file name
	std::filesystem::path filePath = filename;
	std::filesystem::path fileDir = filePath.parent_path();
	bool ret = true;

	//Init materials
	for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
		const aiMaterial* pMaterial = pScene->mMaterials[i];

		if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString path;
			if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
				std::string p(path.C_Str());
				if (p.empty()) {
					LOG("No Textures path in this model");
					return {};
				}
				std::replace(p.begin(), p.end(), '\\', '/');
				std::filesystem::path dirTextPath = p;
				// Resolve relative paths against the FBX directory.
				if (dirTextPath.is_relative())
				{
					dirTextPath = fileDir / dirTextPath; 
				}
				//Load texture
				if (i >= newTextures.size()) {
					newTextures.resize(i + 1, nullptr);
				}
				if (newTextures[i] == nullptr) {
					newTextures[i] = new Texture(dirTextPath.string().c_str());
				}
				if (!LoadFromFile(newTextures[i],newTextures[i]->m_fileName, hasTexture)) {
					std::cerr << "Error loading texture: " << dirTextPath.string() << std::endl;
					delete newTextures[i];
					newTextures[i] = NULL;
					return {};
				}
				else {
					std::cerr << "Loaded texture: " << dirTextPath.string() << std::endl;
				}
			}
		}
	}
	return newTextures;
}

bool TextureLoader::LoadFromFile(Texture* tex,const std::string& fileName, bool& hasTexture) {
	tex->m_fileName = fileName;

	ILuint imageID = 0;
	ilGenImages(1, &imageID);
	ilBindImage(imageID);

	if (!ilLoadImage(tex->m_fileName.c_str())) {
		std::cout << "Failed to load Texture:" << tex->m_fileName << std::endl;
		ilDeleteImages(1, &imageID);
		return false;
	}

	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	//texID -> m_textureObj m stands for mesh
	glGenTextures(1, &tex->m_textureObj);
	glBindTexture(GL_TEXTURE_2D, tex->m_textureObj);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//UVs
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenerateMipmap(GL_TEXTURE_2D);

	// Release the CPU-side image.
	ilDeleteImages(1, &imageID);

	glBindTexture(GL_TEXTURE_2D, 0);

	hasTexture = (tex->m_textureObj != 0);
	return hasTexture;
}

bool TextureLoader::ChangeTexture(std::vector<Texture*>& m_Tex, unsigned int materialIndex, const std::string& fileName, bool& hasTexture) {
	if (materialIndex >= m_Tex.size()) {
		m_Tex.resize(materialIndex + 1, nullptr);
	}

	if (m_Tex[materialIndex] == nullptr) {
		m_Tex[materialIndex] = new Texture(fileName);
	}
	// change material
	if (!LoadFromFile(m_Tex[materialIndex], fileName, hasTexture)) {
		std::cerr << "Error loading texture: "	<< fileName.c_str() << std::endl;
		delete m_Tex[materialIndex];
		m_Tex[materialIndex] = nullptr;
		hasTexture = false;
		return false;
	}

	return true;
}