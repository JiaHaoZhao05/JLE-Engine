#include "Scene.h"
#include "../Modules/Scene.h"
#include "../GameObject.h"
#include <iostream>
#include "../../Engine/Components/MeshComponent.h"
#include "../../Engine/Components/TransformComponent.h"
#include "../../Engine/Components/MaterialComponent.h"
#include "../../Editor/App.h"
#include "../../Editor/Modules/ModelLoader.h"
#include "../../Editor/Modules/TextureLoader.h"
#include "Render.h"

#include "../Primitives/Cube.h"

Scene::Scene() {
	name = "Scene";
}

Scene::~Scene() {}

std::vector<GameObject*> Scene::GetGameObjects()
{
	if (!rootGameObject) {
		
		return std::vector<GameObject*>();
	}
	std::vector<GameObject*> ret;
	rootGameObject->GetAllGameObjects(ret);
	return ret;
}

void Scene::AddGameObject(std::unique_ptr<GameObject> gameObject)
{
	if (!rootGameObject) {
		rootGameObject = std::move(gameObject);
		return;
	}
	rootGameObject->children.emplace_back(std::move(gameObject));
}

std::unique_ptr<GameObject> Scene::CreatePrimitive(Primitive& primitive, glm::vec3 position, std::string name) {
	auto newGameObject = std::make_unique<GameObject>();
	newGameObject->name = name;
	newGameObject->CreateComponents({ ComponentType::TRANSFORM, ComponentType::MESH, ComponentType::MATERIAL });

	auto* transformComponent = static_cast<TransformComponent*>(newGameObject->GetComponent(ComponentType::TRANSFORM));
	auto* meshComponent = static_cast<MeshComponent*>(newGameObject->GetComponent(ComponentType::MESH));
	auto* materialComponent = static_cast<MaterialComponent*>(newGameObject->GetComponent(ComponentType::MATERIAL));

	transformComponent->position = position;

	meshComponent->mesh = App::GetInstance().modelLoader->LoadPrimitive(primitive.vertex, primitive.index); //function tdwadsao make the mesh of primitives. This is because they are not leaded from the FBX so they need to have their VAO binded.
	App::GetInstance().textureLoader->ChangeTexture(materialComponent->m_Textures, 0, primitive.texturePath, materialComponent->hasTexture);// same with textures, as they come in the FBX for the normal GameObjects
	
	//add indices
	meshComponent->mesh.m_Indices.clear();
	for (const auto& index : primitive.index) {
		meshComponent->mesh.m_Indices.emplace_back((unsigned int)index);
	}

	App::GetInstance().modelLoader->LoadFaceNormals(meshComponent->mesh);
	
	meshComponent->mesh.PopulateFaceNormalsBuffers();
	
	return newGameObject;
}

bool Scene::Awake() {
	//parse scene jsons to load GameObjects
	return true;
}

bool Scene::Start() {

	rootGameObject = std::make_unique<GameObject>();
	rootGameObject->CreateComponents({ ComponentType::MESH, ComponentType::TRANSFORM, ComponentType::MATERIAL });

	auto* meshComponent = static_cast<MeshComponent*>(rootGameObject->GetComponent(ComponentType::MESH));
	auto* transformComponent = static_cast<TransformComponent*>(rootGameObject->GetComponent(ComponentType::TRANSFORM));
	auto* materialComponent = static_cast<MaterialComponent*>(rootGameObject->GetComponent(ComponentType::MATERIAL));

	meshComponent->mesh = App::GetInstance().modelLoader->LoadMesh("Data/BakerHouse.fbx");
	meshComponent->SetData(*meshComponent);
	transformComponent->position = { 0,0,0 };
	materialComponent->m_Textures = App::GetInstance().textureLoader->InitMaterials("Data/BakerHouse.fbx", materialComponent->hasTexture);

	auto newGameObject = std::make_unique<GameObject>();
	newGameObject->name = "Lash";
	newGameObject->CreateComponents({ ComponentType::MESH, ComponentType::TRANSFORM, ComponentType::MATERIAL });

	auto* meshComponent1 = static_cast<MeshComponent*>(newGameObject->GetComponent(ComponentType::MESH));
	auto* transformComponent1 = static_cast<TransformComponent*>(newGameObject->GetComponent(ComponentType::TRANSFORM));
	auto* materialComponent1 = static_cast<MaterialComponent*>(newGameObject->GetComponent(ComponentType::MATERIAL));

	meshComponent1->mesh = App::GetInstance().modelLoader->LoadMesh("Data/lash.fbx");
	meshComponent1->SetData(*meshComponent1);
	transformComponent1->position = { -10,0,0 };
	materialComponent1->m_Textures = App::GetInstance().textureLoader->InitMaterials("Data/lash.fbx", materialComponent1->hasTexture);
	if (!materialComponent1->hasTexture) {
		App::GetInstance().textureLoader->ChangeTexture(materialComponent1->m_Textures, 0, App::GetInstance().textureLoader->GetCheckersPath(), materialComponent1->hasTexture);
	}
	App::GetInstance().scene->AddGameObject(std::move(newGameObject));

	auto newGameObject2 = std::make_unique<GameObject>();
	newGameObject2->name = "Lash";
	newGameObject2->CreateComponents({ ComponentType::MESH, ComponentType::TRANSFORM, ComponentType::MATERIAL });

	auto* meshComponent2 = static_cast<MeshComponent*>(newGameObject2->GetComponent(ComponentType::MESH));
	auto* transformComponent2 = static_cast<TransformComponent*>(newGameObject2->GetComponent(ComponentType::TRANSFORM));
	auto* materialComponent2 = static_cast<MaterialComponent*>(newGameObject2->GetComponent(ComponentType::MATERIAL));

	meshComponent2->mesh = App::GetInstance().modelLoader->LoadMesh("Data/Sr.China.fbx");
	meshComponent2->SetData(*meshComponent2);
	transformComponent2->position = { 10,0,0 };
	materialComponent2->m_Textures = App::GetInstance().textureLoader->InitMaterials("Data/Sr.China.fbx", materialComponent2->hasTexture);
	if (!materialComponent2->hasTexture) {
		App::GetInstance().textureLoader->ChangeTexture(materialComponent2->m_Textures, 0, App::GetInstance().textureLoader->GetCheckersPath(), materialComponent2->hasTexture);
	}
	App::GetInstance().scene->AddGameObject(std::move(newGameObject2));
	return true;
}

bool Scene::Update() {
	if(rootGameObject) rootGameObject->Update();
	return true;
}