#undef GLFW_DLL

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <vector>
#include <iostream>
#include "Libs/Mesh.h"
#include "Libs/Shader.h"
#include "Libs/Window.h"
#include "ProjectPaths.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include <stb_image.h>

// Window size and uniform locations shared by the render passes.
const GLint WIDTH = 800;
const GLint HEIGHT = 600;

GLuint uniformModel = 0, uniformView = 0, uniformProjection = 0, uniformTextureData = 0;
GLuint loadedTexture;
glm::vec3 lightColour = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 lightPos = glm::vec3(2.5f, 6.0f, 4.0f);
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 6.5f);

// Offset the character group toward the centre of the scene, facing the road (+Z).
const glm::vec3 characterOffset = glm::vec3(0.207f, 0.0f, -0.769f);

// Street layout (metres). Sidewalk top is y = 0 where the characters sit.
//   wall (z < -1.4) | sidewalk (-1.4 .. 2.6) | curb (2.6 .. 2.85) | road (2.85 .. 12.85)
const float streetLength = 30.0f; // along x
const float wallFrontZ = -1.4f;
const float wallHeight = 3.0f;
const float curbZ = 2.6f;
const float curbWidth = 0.25f;
const float roadWidth = 10.0f;
const float sidewalkHeight = 0.15f; // road surface is this much lower than the sidewalk

GLuint texWall = 0, texSidewalk = 0, texAsphalt = 0;
float wallImageAspect = 1.0f; // width / height of the wall photo (set when loaded)

// Camera orientation in degrees.
float yaw = -90.0f;
float pitch = -10.0f;

std::vector<Mesh*> meshList;
std::vector<Shader*> shaderList;
Shader* depthShader;

const std::filesystem::path shaderDirectory = std::filesystem::u8path(OPENGL_STARTER_SHADER_DIR);
const std::filesystem::path textureDirectory = std::filesystem::u8path(OPENGL_STARTER_TEXTURE_DIR);
const std::filesystem::path modelDirectory = std::filesystem::u8path(OPENGL_STARTER_MODEL_DIR);

// Unused lab example: four vertices connected as a tetrahedron, not one triangle.
void CreateTriangle()
{
    // Each row contains position (x, y, z), followed by texture coordinates (u, v).
    GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // Base vertex 0.
        0.0f,  -1.0f, 1.0f, 0.5,  0.0f, // Base vertex 1.
        1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // Base vertex 2.
        0.0f,  1.0f,  0.0f, 0.5f, 1.0f  // Apex vertex 3.
    };

    unsigned int indices[] = {
        0, 3, 1, // Side face.
        1, 3, 2, // Side face.
        2, 3, 0, // Side face.
        0, 1, 2  // Base face.
    };

    Mesh* triangleMesh = new Mesh();
    triangleMesh->CreateMesh(vertices, indices, 20, 12);
    for (int i = 0; i < 10; i++)
    {
        meshList.push_back(triangleMesh);
    }
}

void CreateShaders()
{
    // Keep shader slots consistent with the lab: surface [0], light [1], background [2].
    Shader* surfaceShader = new Shader();
    surfaceShader->CreateFromFiles(shaderDirectory / "shader.vert",
                                   shaderDirectory / "shader.frag");
    shaderList.push_back(surfaceShader);

    Shader* lightShader = new Shader();
    lightShader->CreateFromFiles(shaderDirectory / "lightShader.vert",
                                 shaderDirectory / "lightShader.frag");
    shaderList.push_back(lightShader);

    Shader* backgroundShader = new Shader();
    backgroundShader->CreateFromFiles(shaderDirectory / "bgShader.vert",
                                      shaderDirectory / "bgShader.frag");
    shaderList.push_back(backgroundShader);

    depthShader = new Shader();
    depthShader->CreateFromFiles(shaderDirectory / "depthShader.vert",
                                 shaderDirectory / "depthShader.frag");
}

void CreateOBJ(const std::filesystem::path& modelPath)
{
    Mesh* obj = new Mesh();
    bool loaded = obj->CreateMeshFromOBJ(modelPath.u8string().c_str());

    if (!loaded)
    {
        std::cerr << "Failed to load model: " << modelPath << std::endl;
        return;
    }

    meshList.push_back(obj);
}

GLuint LoadTexture(const std::filesystem::path& texturePath)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* data = stbi_load(texturePath.u8string().c_str(), &width, &height, &channels, 0);
    if (data == nullptr)
    {
        std::cerr << "Failed to load texture: " << stbi_failure_reason() << "\n";
        return 0;
    }

    GLenum format = GL_RGB;
    switch (channels)
    {
    case 4:
        format = GL_RGBA;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 2:
        format = GL_RG;
        break;
    case 1:
        format = GL_RED;
        break;
    default:
        std::cerr << "Unsupported texture format\n";
        stbi_image_free(data);
        return 0;
    }

    glGenTextures(1, &loadedTexture);
    glBindTexture(GL_TEXTURE_2D, loadedTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return loadedTexture;
}

void UpdateMouseLook(Window& window)
{
    double mouseX, mouseY;
    glfwGetCursorPos(window.getWindow(), &mouseX, &mouseY);

    // Start from the first cursor sample to avoid a jump on the first update.
    static double lastX = mouseX;
    static double lastY = mouseY;

    float xOffset = static_cast<float>(mouseX - lastX);
    float yOffset = static_cast<float>(mouseY - lastY);

    float sensitivity = 0.1f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    // Store this sample for the next frame.
    lastX = mouseX;
    lastY = mouseY;

    yaw += xOffset;
    pitch -= yOffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
}

// Uniform location in the shader that is currently in use (-1 if it doesn't have it,
// so the same drawing code works for both the depth pass and the main pass)
GLint GetActiveUniformLocation(const char* name)
{
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    return glGetUniformLocation(program, name);
}

void RenderScene(glm::mat4 view, glm::mat4 projection)
{
    // Draw the character group in both the shadow-depth and main render passes.
    glUniform1i(GetActiveUniformLocation("useTexture"), 0); // Use the model's material colours.
    glUniform1i(GetActiveUniformLocation("worldUV"), 0);
    glUniform3f(GetActiveUniformLocation("flatNormal"), 0.0f, 0.0f, 0.0f);
    glUniform3f(GetActiveUniformLocation("tintColour"), 1.0f, 1.0f, 1.0f);
    glUniform1f(GetActiveUniformLocation("specularStrength"), 0.3f);

    glm::mat4 model(1.0f);
    model = glm::translate(model, characterOffset);

    glUniformMatrix4fv(GetActiveUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(GetActiveUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(GetActiveUniformLocation("projection"), 1, GL_FALSE,
                       glm::value_ptr(projection));

    meshList[0]->RenderMesh();
}

void Cleanup()
{
    for (Mesh* mesh : meshList)
        delete mesh;
    meshList.clear();

    for (Shader* shader : shaderList)
        delete shader;
    shaderList.clear();

    delete depthShader;
    depthShader = nullptr;
}

int main()
{

    Window mainWindow(WIDTH, HEIGHT, 3, 3);
    if (mainWindow.initialise() != 0)
        return 1;

    // Centre the window on the primary monitor.
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
        if (mode)
        {
            int monitorX = 0, monitorY = 0;
            glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);
            glfwSetWindowPos(mainWindow.getWindow(), monitorX + (mode->width - WIDTH) / 2,
                             monitorY + (mode->height - HEIGHT) / 2);
        }
    }

    // Load models in render-slot order: character [0], light [1], street [2].
    CreateOBJ(modelDirectory / "characters_v27_corrected_foot_pose.obj");
    CreateOBJ(modelDirectory / "cube.obj");
    CreateOBJ(modelDirectory / "cube.obj");

    CreateShaders();

    // Load textures before configuring the wall image's aspect ratio and wrapping.
    GLuint clothTexture = LoadTexture(textureDirectory / "uvmap.png");
    // Images are stored top row first, OpenGL expects bottom row first -> flip on load
    stbi_set_flip_vertically_on_load(true);

    texWall = LoadTexture(textureDirectory / "old_wall.png"); // photo, used once per wall height
    texSidewalk = LoadTexture(textureDirectory / "sidewalk.png");
    texAsphalt = LoadTexture(textureDirectory / "asphalt.png");
    if (clothTexture == 0 || texWall == 0 || texSidewalk == 0 || texAsphalt == 0)
    {
        return 1;
    }

    // Wall photo: mirror-repeat sideways (no seam), clamp top/bottom (no bleeding)
    {
        int w = 0, h = 0;
        glBindTexture(GL_TEXTURE_2D, texWall);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
        if (h > 0)
            wallImageAspect = (float)w / (float)h;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // Set up the perspective camera and its initial direction.
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (GLfloat)mainWindow.getBufferWidth() / (GLfloat)mainWindow.getBufferHeight(), 0.1f, 100.0f);

    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraDirection = glm::normalize(cameraTarget - cameraPos);

    cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraDirection.y = sin(glm::radians(pitch));
    cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraDirection = glm::normalize(cameraDirection);

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraDirection, up));
    glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraDirection));

    float lastTime = static_cast<float>(glfwGetTime());

    // Shadow-map setup: render depth into a texture attached to a framebuffer.
    GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

    // Use a depth-only texture; no colour attachment is needed.
    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // Outside the shadow map = depth 1.0 (farthest) -> never in shadow
    float borderColour[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

    // Attach the depth texture and disable colour reads/writes.
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Main loop: update input, render the shadow map, then render the visible scene.
    while (!mainWindow.getShouldClose())
    {

        // Input and camera update. Movement is scaled by elapsed time.
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        glfwPollEvents();
        UpdateMouseLook(mainWindow);

        cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraDirection.y = sin(glm::radians(pitch));
        cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraDirection = glm::normalize(cameraDirection);

        cameraRight = glm::normalize(glm::cross(cameraDirection, up));
        cameraUp = glm::normalize(glm::cross(cameraRight, cameraDirection));

        float moveSpeed = 5.0f;
        if (glfwGetKey(mainWindow.getWindow(), GLFW_KEY_W) == GLFW_PRESS)
        {
            cameraPos = cameraPos + cameraDirection * moveSpeed * deltaTime;
        }
        if (glfwGetKey(mainWindow.getWindow(), GLFW_KEY_S) == GLFW_PRESS)
        {
            cameraPos = cameraPos - cameraDirection * moveSpeed * deltaTime;
        }
        if (glfwGetKey(mainWindow.getWindow(), GLFW_KEY_A) == GLFW_PRESS)
        {
            cameraPos = cameraPos - cameraRight * moveSpeed * deltaTime;
        }
        if (glfwGetKey(mainWindow.getWindow(), GLFW_KEY_D) == GLFW_PRESS)
        {
            cameraPos = cameraPos + cameraRight * moveSpeed * deltaTime;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view(1.0f);
        glm::mat4 cameraRotationMatrix(1.0f);
        glm::mat4 cameraTranslationMatrix(1.0f);

        shaderList[0]->UseShader();
        uniformModel = shaderList[0]->GetUniformLocation("model");
        uniformView = shaderList[0]->GetUniformLocation("view");
        uniformProjection = shaderList[0]->GetUniformLocation("projection");
        uniformTextureData = shaderList[0]->GetUniformLocation("texture_data");

        glUniform1i(uniformTextureData, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, clothTexture);

        glUniform3fv(shaderList[0]->GetUniformLocation("lightColour"), 1, (GLfloat*)&lightColour);

        glUniform3fv(shaderList[0]->GetUniformLocation("lightPos"), 1, (GLfloat*)&lightPos);

        glUniform3fv(shaderList[0]->GetUniformLocation("viewPos"), 1, (GLfloat*)&cameraPos);

        cameraTranslationMatrix[3][0] = -cameraPos.x;
        cameraTranslationMatrix[3][1] = -cameraPos.y;
        cameraTranslationMatrix[3][2] = -cameraPos.z;

        cameraRotationMatrix[0] = glm::vec4(cameraRight.x, cameraUp.x, -cameraDirection.x, 0.0f);
        cameraRotationMatrix[1] = glm::vec4(cameraRight.y, cameraUp.y, -cameraDirection.y, 0.0f);
        cameraRotationMatrix[2] = glm::vec4(cameraRight.z, cameraUp.z, -cameraDirection.z, 0.0f);

        view = cameraRotationMatrix * cameraTranslationMatrix;

        glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

        // Pass 1: render the characters from the light's viewpoint into the depth map.
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Light looks at the characters; the ortho box only needs to cover them
        // A smaller orthographic box gives the 2048x2048 map more detail per metre.
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), up);
        glm::mat4 lightProjection = glm::ortho(-6.0f, 6.0f, -6.0f, 6.0f, 0.1f, 20.0f);

        depthShader->UseShader();
        uniformModel = depthShader->GetUniformLocation("model");
        uniformView = depthShader->GetUniformLocation("view");
        uniformProjection = depthShader->GetUniformLocation("projection");

        glCullFace(GL_FRONT);
        RenderScene(lightView, lightProjection);
        glCullFace(GL_BACK);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Pass 2: render the scene from the camera, using the depth map for shadows.
        glViewport(0, 0, mainWindow.getBufferWidth(), mainWindow.getBufferHeight());
        glClearColor(0.55f, 0.70f, 0.85f, 1.0f); // sky colour
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderList[0]->UseShader();
        uniformModel = shaderList[0]->GetUniformLocation("model");
        uniformView = shaderList[0]->GetUniformLocation("view");
        uniformProjection = shaderList[0]->GetUniformLocation("projection");

        glUniform3fv(GetActiveUniformLocation("lightColour"), 1, (GLfloat*)&lightColour);
        glUniform3fv(GetActiveUniformLocation("lightPos"), 1, (GLfloat*)&lightPos);
        glUniform3fv(GetActiveUniformLocation("viewPos"), 1, (GLfloat*)&cameraPos);

        // Texture unit 0 holds the surface texture; unit 1 holds the shadow map.
        glUniformMatrix4fv(GetActiveUniformLocation("lightView"), 1, GL_FALSE,
                           glm::value_ptr(lightView));
        glUniformMatrix4fv(GetActiveUniformLocation("lightProjection"), 1, GL_FALSE,
                           glm::value_ptr(lightProjection));
        glUniform1i(GetActiveUniformLocation("texture_data"), 0);
        glUniform1i(GetActiveUniformLocation("shadowMap"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        // Draw the characters before the textured street surfaces.
        RenderScene(view, projection);

        // Background, like the lab: transform cube.obj, bind texture, then draw.
        // The main shader adds textures and lighting to the lab's flat background.
        {
            shaderList[0]->UseShader();
            uniformModel = shaderList[0]->GetUniformLocation("model");
            uniformView = shaderList[0]->GetUniformLocation("view");
            uniformProjection = shaderList[0]->GetUniformLocation("projection");
            glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
            glUniform1i(shaderList[0]->GetUniformLocation("worldUV"), 1);
            glUniform1f(shaderList[0]->GetUniformLocation("specularStrength"), 0.05f);
            glActiveTexture(GL_TEXTURE0);

            // A flattened cube has overlapping front/back faces: render only the front.
            GLboolean wasCulling = glIsEnabled(GL_CULL_FACE);
            GLint previousCullFace = GL_BACK;
            glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFace);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            // Wall: flatten along Z and apply the wall photo.
            float wallTotalHeight = wallHeight + sidewalkHeight;
            float wallImageWidth = wallTotalHeight * wallImageAspect;
            glm::mat4 model(1.0f);
            model = glm::translate(
                model, glm::vec3(0.0f, (wallHeight - sidewalkHeight) / 2.0f, wallFrontZ));
            model = glm::scale(model, glm::vec3(streetLength, wallTotalHeight, 0.0f));
            glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
            // A zero scale cannot use inverse(model) for lighting normals.
            glUniform3f(shaderList[0]->GetUniformLocation("flatNormal"), 0.0f, 0.0f, 1.0f);
            glUniform1i(shaderList[0]->GetUniformLocation("useTexture"), 1);
            glUniform3f(shaderList[0]->GetUniformLocation("tintColour"), 1.0f, 1.0f, 1.0f);
            glUniform2f(shaderList[0]->GetUniformLocation("texScale"), 1.0f / wallImageWidth,
                        1.0f / wallTotalHeight);
            glUniform2f(shaderList[0]->GetUniformLocation("texOffset"), -wallImageWidth / 2.0f,
                        -sidewalkHeight);
            glBindTexture(GL_TEXTURE_2D, texWall);
            meshList[2]->RenderMesh();

            // Concrete strip above the wall: another vertical flattened cube.
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, wallHeight + 0.05f, wallFrontZ + 0.04f));
            model = glm::scale(model, glm::vec3(streetLength, 0.1f, 0.0f));
            glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(shaderList[0]->GetUniformLocation("useTexture"), 0);
            glUniform3f(shaderList[0]->GetUniformLocation("tintColour"), 0.70f, 0.70f, 0.68f);
            glBindTexture(GL_TEXTURE_2D, 0);
            meshList[2]->RenderMesh();

            // Sidewalk: rotate the flat cube onto the ground and apply its texture.
            float sidewalkDepth = curbZ - wallFrontZ;
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, wallFrontZ + sidewalkDepth / 2.0f));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(streetLength, sidewalkDepth, 0.0f));
            glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(shaderList[0]->GetUniformLocation("flatNormal"), 0.0f, 1.0f, 0.0f);
            glUniform1i(shaderList[0]->GetUniformLocation("useTexture"), 1);
            glUniform3f(shaderList[0]->GetUniformLocation("tintColour"), 1.0f, 1.0f, 1.0f);
            glUniform2f(shaderList[0]->GetUniformLocation("texScale"), 1.0f, 1.0f);
            glUniform2f(shaderList[0]->GetUniformLocation("texOffset"), 0.0f, 0.0f);
            glBindTexture(GL_TEXTURE_2D, texSidewalk);
            meshList[2]->RenderMesh();

            // Red/white curb: one vertical face and one horizontal top per block.
            glUniform1i(shaderList[0]->GetUniformLocation("useTexture"), 0);
            glUniform1f(shaderList[0]->GetUniformLocation("specularStrength"), 0.15f);
            glBindTexture(GL_TEXTURE_2D, 0);
            for (int i = 0; i < (int)streetLength; ++i)
            {
                float x = -streetLength / 2.0f + 0.5f + i;
                glm::vec3 paint =
                    (i % 2 == 0) ? glm::vec3(0.80f, 0.12f, 0.10f) : glm::vec3(0.92f, 0.92f, 0.90f);
                glUniform3fv(shaderList[0]->GetUniformLocation("tintColour"), 1,
                             glm::value_ptr(paint));

                model = glm::mat4(1.0f);
                model =
                    glm::translate(model, glm::vec3(x, -sidewalkHeight / 2.0f, curbZ + curbWidth));
                model = glm::scale(model, glm::vec3(1.0f, sidewalkHeight, 0.0f));
                glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
                glUniform3f(shaderList[0]->GetUniformLocation("flatNormal"), 0.0f, 0.0f, 1.0f);
                meshList[2]->RenderMesh();

                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(x, 0.0f, curbZ + curbWidth / 2.0f));
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::scale(model, glm::vec3(1.0f, curbWidth, 0.0f));
                glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
                glUniform3f(shaderList[0]->GetUniformLocation("flatNormal"), 0.0f, 1.0f, 0.0f);
                meshList[2]->RenderMesh();
            }

            // Road: horizontal flattened cube with asphalt texture.
            float roadStartZ = curbZ + curbWidth;
            model = glm::mat4(1.0f);
            model = glm::translate(model,
                                   glm::vec3(0.0f, -sidewalkHeight, roadStartZ + roadWidth / 2.0f));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(streetLength, roadWidth, 0.0f));
            glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(shaderList[0]->GetUniformLocation("flatNormal"), 0.0f, 1.0f, 0.0f);
            glUniform1i(shaderList[0]->GetUniformLocation("useTexture"), 1);
            glUniform1f(shaderList[0]->GetUniformLocation("specularStrength"), 0.05f);
            glUniform3f(shaderList[0]->GetUniformLocation("tintColour"), 1.0f, 1.0f, 1.0f);
            glUniform2f(shaderList[0]->GetUniformLocation("texScale"), 0.5f, 0.5f);
            glUniform2f(shaderList[0]->GetUniformLocation("texOffset"), 0.0f, 0.0f);
            glBindTexture(GL_TEXTURE_2D, texAsphalt);
            meshList[2]->RenderMesh();

            // Lane markings: white flattened cubes just above the road.
            glUniform1i(shaderList[0]->GetUniformLocation("useTexture"), 0);
            glUniform3f(shaderList[0]->GetUniformLocation("tintColour"), 0.95f, 0.95f, 0.92f);
            glBindTexture(GL_TEXTURE_2D, 0);
            for (float x = -streetLength / 2.0f + 1.5f; x < streetLength / 2.0f; x += 6.0f)
            {
                model = glm::mat4(1.0f);
                model = glm::translate(
                    model, glm::vec3(x, -sidewalkHeight + 0.004f, roadStartZ + roadWidth / 2.0f));
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::scale(model, glm::vec3(3.0f, 0.15f, 0.0f));
                glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
                meshList[2]->RenderMesh();
            }

            glUniform3f(shaderList[0]->GetUniformLocation("flatNormal"), 0.0f, 0.0f, 0.0f);
            glCullFace(previousCullFace);
            if (!wasCulling)
                glDisable(GL_CULL_FACE);
        }

        // Draw the small cube marking the light's position.
        shaderList[1]->UseShader();
        uniformModel = shaderList[1]->GetUniformLocation("model");
        uniformView = shaderList[1]->GetUniformLocation("view");
        uniformProjection = shaderList[1]->GetUniformLocation("projection");

        glm::mat4 model(1.0f);

        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(shaderList[1]->GetUniformLocation("lightColour"), 1,
                     glm::value_ptr(lightColour));

        meshList[1]->RenderMesh();

        mainWindow.swapBuffers();
    }

    // Delete OpenGL objects before the Window destructor destroys the context.
    Cleanup();
    return 0;
}
