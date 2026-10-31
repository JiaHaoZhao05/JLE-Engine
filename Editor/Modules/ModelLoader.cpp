#include "ModelLoader.h"
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

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)

ModelLoader::ModelLoader() : Module() {
	name = "ModelLoader";
}

ModelLoader::~ModelLoader() {

}

Mesh ModelLoader::LoadMesh(const std::string& filename) {
	Mesh newMesh;
	glGenVertexArrays(1, &newMesh.m_VAO);

	glGenBuffers(1, &newMesh.m_VBO);

	glGenBuffers(1, &newMesh.m_EBO);

	glBindVertexArray(newMesh.m_VAO);

	glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(newMesh.m_buffers), newMesh.m_buffers);

	bool ret = false;
	Assimp::Importer importer;

	//search for model and texture
	const aiScene* pScene = importer.ReadFile(filename.c_str(), ASSIMP_LOAD_FLAGS);

	if (pScene) {
		ret = InitFromScene(newMesh, pScene, filename);
	}
	else {
		std::cout << "Error parsing " << filename.c_str() << ": " << importer.GetErrorString() << std::endl;
	}
	if (!ret) {
		std::cout << "Failed to init the model" << std::endl;
	}
	glBindVertexArray(0);

	//triangle/face normals
	LoadFaceNormals(newMesh);
	newMesh.PopulateFaceNormalsBuffers();
	newMesh.PopulateVertexNormalBuffers();

	return newMesh;
}
Mesh ModelLoader::LoadPrimitive(const std::vector<float>& vertices, const std::vector<int>& indices) {
	Mesh newMesh;

	glGenVertexArrays(1, &newMesh.m_VAO);
	glGenBuffers(1, &newMesh.m_VBO);
	glGenBuffers(1, &newMesh.m_EBO);

	glBindVertexArray(newMesh.m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, newMesh.m_VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newMesh.m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	MeshEntry entry;
	entry.numIndices = (unsigned int)indices.size();
	entry.baseVertex = 0;
	entry.baseIndex = 0;
	entry.materialIndex = 0;
	newMesh.m_Meshes.assign(1, entry);

	//primitive vertices <-> mesh positions
	for (size_t i = 0; i + 7 < vertices.size(); i += 8) {
		newMesh.m_Positions.emplace_back(vertices[i], vertices[i + 1], vertices[i + 2]);
	}

	return newMesh;
}
bool ModelLoader::InitFromScene(Mesh& mesh, const aiScene* pScene, const std::string& filename) {
	mesh.m_Meshes.resize(pScene->mNumMeshes);

	unsigned int numVertices = 0;
	unsigned int numIndices = 0;

	CountVerticesAndIndices(mesh, pScene, numVertices, numIndices);

	ReserveSpace(mesh, numVertices, numIndices);

	InitAllMeshes(mesh, pScene);

	PopulateBuffers(mesh);

	return true;
}

void ModelLoader::CountVerticesAndIndices(Mesh& mesh, const aiScene* pScene, unsigned int& numVertices, unsigned int& numIndices) {
	for (unsigned int i = 0; i < mesh.m_Meshes.size(); i++) {
		mesh.m_Meshes[i].materialIndex = pScene->mMeshes[i]->mMaterialIndex;
		mesh.m_Meshes[i].numIndices = pScene->mMeshes[i]->mNumFaces * 3;
		mesh.m_Meshes[i].baseVertex = numVertices;
		mesh.m_Meshes[i].baseIndex = numIndices;

		numVertices += pScene->mMeshes[i]->mNumVertices;
		numIndices += mesh.m_Meshes[i].numIndices;
	}
}

void ModelLoader::ReserveSpace(Mesh& mesh, unsigned int numVertices, unsigned int numIndices) {
	mesh.m_Positions.reserve(numVertices);
	mesh.m_vertexNormals.reserve(numVertices);
	mesh.m_TextCoords.reserve(numVertices);
	mesh.m_Indices.reserve(numIndices);
}

void ModelLoader::InitAllMeshes(Mesh& mesh, const aiScene* pScene) {
	for (unsigned int i = 0; i < mesh.m_Meshes.size(); i++) {
		const aiMesh* paiMesh = pScene->mMeshes[i];
		InitSingleMesh(mesh, paiMesh);
	}
}

void ModelLoader::InitSingleMesh(Mesh& mesh, const aiMesh* paiMesh) {
	const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

	//populate the vertex attribute vectors
	for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
		const aiVector3D& pPos = paiMesh->mVertices[i];
		const aiVector3D& pNormal = paiMesh->mNormals[i];
		const aiVector3D& pTextCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i] : Zero3D;

		mesh.m_Positions.push_back({ pPos.x, pPos.y, pPos.z });
		mesh.m_vertexNormals.push_back({ pNormal.x, pNormal.y, pNormal.z });
		mesh.m_TextCoords.push_back(Vector2f(pTextCoord.x, pTextCoord.y));
	}

	//populate the index buffer
	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
		const aiFace& face = paiMesh->mFaces[i];
		assert(face.mNumIndices == 3);
		mesh.m_Indices.push_back(face.mIndices[0]);
		mesh.m_Indices.push_back(face.mIndices[1]);
		mesh.m_Indices.push_back(face.mIndices[2]);
	}
}

void ModelLoader::PopulateBuffers(Mesh& mesh) {
	glBindBuffer(GL_ARRAY_BUFFER, mesh.m_buffers[mesh.Buffer_Type::POS_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mesh.m_Positions[0]) * mesh.m_Positions.size(), mesh.m_Positions.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDisableVertexAttribArray(1);
	glVertexAttrib3f(1, 1.0f, 1.0f, 1.0f);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.m_buffers[mesh.Buffer_Type::TEXCOORD_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mesh.m_TextCoords[0]) * mesh.m_TextCoords.size(), mesh.m_TextCoords.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.m_buffers[mesh.Buffer_Type::INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mesh.m_Indices[0]) * mesh.m_Indices.size(), mesh.m_Indices.data(), GL_STATIC_DRAW);
}


FaceNormal ModelLoader::GetNormal(const glm::vec3& A, const glm::vec3 B, const glm::vec3 C) { //TODO: move the FaceNormal code to mesh and load the FaceNormal vector after creating the mesh
	//calculate normal
	glm::vec3 vector1 = B - A;
	glm::vec3 vector2 = C - A;

	glm::vec3 normal = glm::cross(vector1, vector2);
	normal = glm::normalize(normal);

	//calculate incenter
	float a = glm::distance(B, C);
	float b = glm::distance(A, C);
	float c = glm::distance(A, B);

	if (a + b + c == 0.0f) return FaceNormal({ 0,0,0 }, { 0,0,0 }, 1);

	glm::vec3 position = (a * A + b * B + c * C) / (a + b + c);

	return FaceNormal(normal, position, 1);
}

void ModelLoader::LoadFaceNormals(Mesh& mesh)
{
	mesh.m_triangleNormals.clear();

	const auto& vertices = mesh.m_Positions;
	for (const MeshEntry& meshEntry : mesh.m_Meshes) {
		for (size_t i = 0; i + 2 < meshEntry.numIndices; i += 3) {
			unsigned int index = meshEntry.baseIndex + i;

			unsigned int i1 = meshEntry.baseVertex + mesh.m_Indices[index];
			unsigned int i2 = meshEntry.baseVertex + mesh.m_Indices[index + 1];
			unsigned int i3 = meshEntry.baseVertex + mesh.m_Indices[index + 2];

			if (i1 >= vertices.size() || i2 >= vertices.size() || i3 >= vertices.size()) {
				continue;
			}

			mesh.m_triangleNormals.emplace_back(GetNormal(vertices[i1], vertices[i2], vertices[i3]));
		}
	}
}