#include "Mesh.h"
#include "Mesh.h"
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// OpenGL
#include <glad/glad.h>
#include <SDL3/SDL.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

#define ASSIMP_LOAD_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)
#define COLOR_TEXTURE_UNIT GL_TEXTURE0

#define POSITION_LOCATION 0
#define TEX_COORD_LOCATION 1
#define NORMAL_LOCATION 2

Mesh::~Mesh() {
	//Clear();
}

void Mesh::Clear() {
	if (m_buffers[0] != 0) {
		glDeleteBuffers(ARRAY_SIZE_IN_ELEMENTS(m_buffers), m_buffers);
	}
	if (m_VAO != 0) {
		glDeleteVertexArrays(1, &m_VAO);
		m_VAO = 0;
	}
}

void Mesh::PopulateFaceNormalsBuffers() {
	if (m_triangleNormals.empty()) return;

	std::vector<glm::vec3> vertices;
	for (const auto& triangleNormal : m_triangleNormals) {
		vertices.emplace_back(triangleNormal.position);
		vertices.emplace_back(triangleNormal.position + triangleNormal.normal * triangleNormal.length);
	}

	if (m_FaceNormalsVAO == 0) {
		glGenVertexArrays(1, &m_FaceNormalsVAO);
		glGenBuffers(1, &m_FaceNormalsVBO);
	}

	glBindVertexArray(m_FaceNormalsVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_FaceNormalsVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	glBindVertexArray(0);
}

void Mesh::PopulateVertexNormalBuffers()
{
	if (m_vertexNormals.empty()) return;

	std::vector<glm::vec3> vertices;
	size_t i = 0;
	for (const auto& vertexNormal : m_vertexNormals) {
		vertices.emplace_back(m_Positions[i]);
		vertices.emplace_back(m_Positions[i] + vertexNormal);
		++i;
	}

	if (m_VertexNormalsVAO == 0) {
		glGenVertexArrays(1, &m_VertexNormalsVAO);
		glGenBuffers(1, &m_VertexNormalsVBO);
	}

	glBindVertexArray(m_VertexNormalsVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VertexNormalsVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	glBindVertexArray(0);
}