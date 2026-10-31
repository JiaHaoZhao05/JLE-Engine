#pragma once
#include "../../Engine/Module.h"
#include "../../Engine/Mesh.h"
#include "glm/glm.hpp"
class Mesh;
/// <summary>
/// Also known as Assimp loader
/// </summary>
class ModelLoader : public Module {
public:
	ModelLoader();

	~ModelLoader();

	Mesh LoadMesh(const std::string& filename);

	Mesh LoadPrimitive(const std::vector<float>& vertices, const std::vector<int>& indices);

	void LoadFaceNormals(Mesh& mesh);

private:
	bool InitFromScene(Mesh& mesh, const aiScene* pScene, const std::string& filename);

	void CountVerticesAndIndices(Mesh& mesh, const aiScene* pScene, unsigned int& numVertices, unsigned int& numIndices);

	void ReserveSpace(Mesh& mesh, unsigned int numVertices, unsigned int numIndices);

	void InitAllMeshes(Mesh& mesh, const aiScene* pScene);

	void InitSingleMesh(Mesh& mesh, const aiMesh* paiMesh);

	void PopulateBuffers(Mesh& mesh);

private:
	//FaceNormal
	FaceNormal GetNormal(const glm::vec3& A, const glm::vec3 B, const glm::vec3 C);
};