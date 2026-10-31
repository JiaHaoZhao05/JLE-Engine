#include "Render.h"
#include "../../Editor/App.h"
#include "../../Editor/Log.h"
#include "Windows.h"
#include "../FileReader.h"
#include "Scene.h"

#include <iostream>
#include <memory>

// OpenGL
#include <glad/glad.h>
#include <SDL3/SDL.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//GameObjectt and Components
#include "../GameObject.h"
#include "../Components/TransformComponent.h"
#include "../Components/MeshComponent.h"
#include "../Components/MaterialComponent.h"

//Grid
#include "../Grid/Grid.h"

//TODO: Create .frag and .vert files to save the fragment and shader code
// Setup VS and PS in GLSL
const char* vertexShaderSource = "\n"
"#version 460 core\n"
"layout (location = 0) in vec3 inPos;\n"
"layout (location = 1) in vec3 inColor;\n"
"layout (location = 2) in vec2 inTexCoord;\n"
"out vec3 vertexColor;\n"
"out vec2 TexCoord;\n"
"uniform mat4 modelViewProj;\n"
"void main()\n"
"{\n"
"gl_Position = modelViewProj * vec4(inPos, 1.0);\n"
"vertexColor = inColor;\n"
"TexCoord = inTexCoord;\n"
"}\0";

const char* fragmentShaderSource = "\n"
"#version 460 core\n"
"in vec3 vertexColor;\n"
"in vec2 TexCoord;\n"
"uniform float isOutline;\n"
"uniform sampler2D tex;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(vertexColor, 1.0f) * texture(tex, TexCoord) + vec4(isOutline, isOutline, isOutline, isOutline);\n"
"}\0";

Render::Render() : Module() {
	name = "render";
}

Render::~Render() {

}

bool Render::Awake() {
	SDL_Window* window = App::GetInstance().windows->window;
	// OpenGL is context based and thread local
		// Link OpenGL context to SDL, after this you can load OpenGL functions and start rendering
		// Multi-threading -> Define multiple context and make them current
	glContext = SDL_GL_CreateContext(window);
	if (!glContext)
	{
        LOG("render glcontext failed");
		SDL_DestroyWindow(window);
		SDL_Quit();
		return false;
	}
    LOG("OpenGL linked context to SDL");
	gladLoadGL();

	// Used for mapping NDC coordinates (-1.0f to 1.0f) to pixel coordinates (e.g. 1920x1080)
    
    renderSettings = FileReader::GetRenderSettings();
	glViewport(0, 0, renderSettings.windowsSizeX, renderSettings.windowsSizeY);

    // Create & compile vertex and fragment shaders
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    // Create Program and bind shaders
    shaderProgram = glCreateProgram();

	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); //TODO: get these from files
	glCompileShader(vertexShader);

	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
    
	// Delete shaders since we've created a program already and they are contained there
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

    // Create ModelViewProjection matrix
    modelViewProjLocation = glGetUniformLocation(shaderProgram, "modelViewProj");
    isOutlineLocation = glGetUniformLocation(shaderProgram, "isOutline");

    // Rotate the cube over time
    SDL_GetCurrentTime(&App::GetInstance().prevTime);

    CreateFBO(App::GetInstance().windows->SCREEN_WIDTH, App::GetInstance().windows->SCREEN_HEIGHT, frameBufferObject);
    sceneWindowSize.x = App::GetInstance().windows->SCREEN_WIDTH;
    sceneWindowSize.y = App::GetInstance().windows->SCREEN_HEIGHT;

    projectionMatrix = glm::perspective(glm::radians(FOV), static_cast<float>(App::GetInstance().windows->SCREEN_WIDTH) / static_cast<float>(App::GetInstance().windows->SCREEN_HEIGHT), NEAR_PLANE, FAR_PLANE);

    grid = std::make_unique<Grid>();

    return true;
}

bool Render::Update() {
    // UPDATE
    Camera& camera= *App::GetInstance().camera;

    direction = camera.direction;

    viewMatrix = camera.viewMatrix;
    // Model Matrix
    modelMatrix = camera.modelMatrix;
    glm::mat4 modelViewProj = projectionMatrix * viewMatrix * modelMatrix;
    
    // Clear screen color
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Clear Color Buffer and Depth Buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // RENDER TO TEXTURE
    if (shouldRefreshSceneWindow)
    {
        CreateFBO(sceneWindowSize.x, sceneWindowSize.y, frameBufferObject);
        /*CreateFBO(sceneWindowSize.x, sceneWindowSize.y, brightnessFBO, true);*/
        shouldRefreshSceneWindow = false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject.FBO_ID);
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    glViewport(0, 0, sceneWindowSize.x, sceneWindowSize.y);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Enable Stencil test
    glEnable(GL_STENCIL_TEST);

    // Clear screen color from render to texture
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    // Clear Color Buffer and Depth Buffer from render to texture
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    //Render primitives

    glUseProgram(shaderProgram);

    if (seeEdges) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF);
    glUniform1f(isOutlineLocation, 0.0f);
    
    //grids matrix
    glm::mat4 gridModel = glm::mat4(1.0f);
    glm::mat4 gridMVP = projectionMatrix * viewMatrix * gridModel;

    glUniformMatrix4fv(modelViewProjLocation, 1, GL_FALSE, glm::value_ptr(gridMVP));

    //draw grid
    grid->Render();

    for (auto gameObject : App::GetInstance().scene->GetGameObjects()) {
        auto* transform = static_cast<TransformComponent*>(gameObject->GetComponent(ComponentType::TRANSFORM));
        auto* meshComponent = static_cast<MeshComponent*>(gameObject->GetComponent(ComponentType::MESH));
        auto* material = static_cast<MaterialComponent*>(gameObject->GetComponent(ComponentType::MATERIAL));
        if (!transform || !meshComponent) continue; // core Components that need to exist. We can change this.

        glm::mat4 mvp = projectionMatrix * viewMatrix * transform->modelMat;
        glUniformMatrix4fv(modelViewProjLocation, 1, GL_FALSE, glm::value_ptr(mvp));

        glActiveTexture(GL_TEXTURE0);
        if (material) {
            glBindTexture(GL_TEXTURE_2D, material->id);
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        gameObject->Render();
        if (meshComponent->showFaceNormals) {
            gameObject->RenderNormalsPerTriangle();
        }
        if (meshComponent->showVertexNormals) {
            gameObject->RenderVertexNormals();
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Unbind frame buffer, back to default
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

bool Render::CleanUp() {

    // Delete VAO, VBO and shader program
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    SDL_GL_DestroyContext(glContext);
    SDL_Quit();
    
    return true;
}

void Render::CreateFBO(int width, int height, FrameBufferObject& frameBufferObject)
{
    /// Generate Frame buffer textures if not done earlier
    if (frameBufferObject.FBO_ID == 0)
    {
        glGenFramebuffers(1, &frameBufferObject.FBO_ID);
        glGenTextures(1, &frameBufferObject.RENDER_TO_TEXTURE_ID);
        glGenRenderbuffers(1, &frameBufferObject.RBO_DEPTH_STENCIL_ID);
    }
    // Bind the frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject.FBO_ID);
    // Color texture
    glBindTexture(GL_TEXTURE_2D, frameBufferObject.RENDER_TO_TEXTURE_ID);
    // Setup texture filtering params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // Depth/stencil
    glBindRenderbuffer(GL_RENDERBUFFER, frameBufferObject.RBO_DEPTH_STENCIL_ID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    // Attach
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBufferObject.RENDER_TO_TEXTURE_ID, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, frameBufferObject.RBO_DEPTH_STENCIL_ID);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG("RenderModule -> CreateFBO(): Frame buffer incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

MeshComponent Render::CreateMesh(float* vertices, Uint32 vertexBytes, int* indices, GLsizei indexCount)
{
    MeshComponent meshComponent;
    meshComponent.n_index = indexCount;

    glGenVertexArrays(1, &meshComponent.mesh.m_VAO);

    glGenBuffers(1, &meshComponent.mesh.m_VBO);

    glGenBuffers(1, &meshComponent.mesh.m_EBO);

    // Bind VAO and VBO
    glBindVertexArray(meshComponent.mesh.m_VAO);

    // Link GL_ARRAY_BUFFER to vertices data
    glBindBuffer(GL_ARRAY_BUFFER, meshComponent.mesh.m_VBO);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
        
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, vertices, GL_STATIC_DRAW);


    // Link GL_ELEMENT_ARRAY_BUFFER to indices data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshComponent.mesh.m_EBO);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(int), indices, GL_STATIC_DRAW);

    // Define Vertex layout and set attribute index and set UVs
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // Unlink VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return meshComponent;
}