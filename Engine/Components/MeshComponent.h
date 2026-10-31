#include "../Component.h"
#include <glad/glad.h>
#include "../Mesh.h"

class MeshComponent : public Component { //TODO: Add the assimp for loading meshes
public:
	MeshComponent() { type = ComponentType::MESH; }
	MeshComponent(GameObject* owner) {
		this->owner = owner; 
		type = ComponentType::MESH;
	}

	~MeshComponent() { }

	void Enable() { active = true; }
	void Update();
	void Disable() { active = false; }

	void CleanUp() override { mesh.Clear(); }

	void SetData(const MeshComponent& meshComponent);

	GLsizei n_index = 0;

	bool showVertexNormals = false;
	bool showTriangles = false;
	bool showFaceNormals = false;
	bool showCheckers = false;

	//Loading Meshes and textures
	Mesh mesh;
};