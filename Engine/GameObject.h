#pragma once
#include <iostream>
#include "../Engine/Component.h"
#include <vector>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class GameObject {
public:
	GameObject();
	GameObject(std::string name) { this->name = name; }
	~GameObject();

	void CreateComponents(const std::vector<ComponentType>& newComponents);
	glm::vec3 GetPosition();

	void Update();
	
	void Render();

	void RenderWireframe();
	void RenderNormalsPerTriangle();
	void RenderVertexNormals();

public:
	Component* GetComponent(ComponentType type);
	void GetAllGameObjects(std::vector<GameObject*>& vec); //call only with the rootGameObject

	bool HasChildren() { return !children.empty(); }

	std::string name = "GameObject";
	bool active = true;

	std::vector<std::unique_ptr<Component>> components;

	std::vector<std::unique_ptr<GameObject>> children;
	GameObject* parent = nullptr;

private:
	Component& CreateComponent(ComponentType type);
};