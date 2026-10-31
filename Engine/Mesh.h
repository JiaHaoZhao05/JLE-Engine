#pragma once
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// OpenGL
#include <glad/glad.h>
#include <SDL3/SDL.h>

//GLM
#include "glm/glm.hpp"

#define INVALID_MATERIAL 0xFFFFFFFF

struct Vector3f {
	union {
		struct {
			float x, y, z;
		};
		struct {
			float data[3];
		};
	};
	Vector3f() : Vector3f(0, 0, 0) {}
	Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}
};
struct Vector2f {
	union {
		struct {
			float x, y;
		};
		struct {
			float data[2];
		};
	};
	Vector2f() : Vector2f(0, 0) {}
	Vector2f(float x, float y) : x(x), y(y) {}
};
struct MeshEntry {
	MeshEntry() {
		numIndices = 0;
		baseVertex = 0;
		baseIndex = 0;
		materialIndex = INVALID_MATERIAL;
	}
	unsigned int numIndices;
	unsigned int baseVertex;
	unsigned int baseIndex;
	unsigned int materialIndex;
};
#define FACE_NORMAL_LENGTH 1

struct FaceNormal {
	glm::vec3 normal;
	glm::vec3 position;
	float length;

	FaceNormal(const glm::vec3& normal, const glm::vec3& position, const float& length) {
		this->normal = normal;
		this->position = position;
		this->length = length;
	}
};

class Mesh {
public:
	Mesh() {};
	~Mesh();

	//WorldTrans& GetWorldTransform(){ return m_worldTransform; }

	void Clear();
public:
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_EBO = 0;

	enum Buffer_Type {
		INDEX_BUFFER = 0,
		POS_VB = 1,
		TEXCOORD_VB = 2,
		NORMAL_VB = 3,
		WVP_MAT_VB = 4,
		WORLD_MAT_VB = 5,
		NUM_BUFFERS = 6
	};

	//WorldTrans m_worldTransform;
	
	GLuint m_buffers[NUM_BUFFERS] = { 0 };

	std::vector<MeshEntry> m_Meshes;

	std::vector<glm::vec3> m_Positions; //vertex positions
	std::vector<Vector2f> m_TextCoords; //UVs 
	std::vector<unsigned int> m_Indices;


	//triangle normals
	std::vector<FaceNormal> m_triangleNormals;

	GLuint m_FaceNormalsVAO = 0;
	GLuint m_FaceNormalsVBO = 0;

	void PopulateFaceNormalsBuffers();

	//vertex normals
	std::vector<glm::vec3> m_vertexNormals; //vertex normals loaded with Assimp

	GLuint m_VertexNormalsVAO = 0;
	GLuint m_VertexNormalsVBO = 0;

	void PopulateVertexNormalBuffers();
};