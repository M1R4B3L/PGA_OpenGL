//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;
typedef glm::mat4  mat4;

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

//VBO
struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexInputLayout;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_TextureMesh,
    Mode_Count
};

struct Buffer
{
    GLuint handle;
    GLenum type;
    u32	   size;
    u32	   head;
    void* data;
};

struct OpenGLInfo
{

};

//Models & Materials
struct Model
{
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32> indices;
    u32 vertexOffset;
    u32 indexOffset;
    std::vector<Vao> vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
    GLuint vertexBufferHandle;
    GLuint indexBufferHandle;
};

struct Material
{
    std::string name;
    vec3 albedo;
    vec3 emissive;
    f32 smoothness;
    u32 albedoTextureIdx;
    u32 emissiveTextureIdx;
    u32 specularTextureIdx;
    u32 normalsTextureIdx;
    u32 bumpTextureIdx;
};

struct Entity
{
    std::string name;
    vec3 pos;
    vec3 rotAngle;
    vec3 scale;
    glm::mat4 worldMatrix = mat4(1.0f);
    u32 modelIdx;

    u32 localParamsOffset;
    u32 localParamsSize;
    
    Entity(vec3 _pos, vec3 _rotAngle, vec3 _scale, u32 _modelIdx, u32 _localParamsOffset,u32 _localParamsSize)
    {
        pos = _pos;
        rotAngle = _rotAngle;
        scale = _scale;
        worldMatrix = glm::translate(worldMatrix, pos);
        worldMatrix = glm::rotate(worldMatrix, glm::radians(_rotAngle.x), glm::vec3(1, 0, 0));//rotation x 
        worldMatrix = glm::rotate(worldMatrix, glm::radians(_rotAngle.y), glm::vec3(0, 1, 0));//rotation y 
        worldMatrix = glm::rotate(worldMatrix, glm::radians(_rotAngle.z), glm::vec3(0, 0, 1));//rotation z 
        worldMatrix = glm::scale(worldMatrix, scale);
        modelIdx = _modelIdx;
        localParamsOffset = _localParamsOffset;
        localParamsSize = _localParamsSize;
    };
};

enum class LightType
{
    Directional,
    Point,
    Spot
};

struct Light
{
    vec3 col;
    vec3 dir;
    vec3 pos;
    LightType type;
    std::string name;
    float range;
    vec3 attenuation;

    glm::mat4 worldMatrix = mat4(1.0f);

    u32 lightParamsOffset;
    u32 lightParamsSize;
    u32 localParamsOffset;
    u32 localParamsSize;
};

struct Camera
{
    vec3 pos;
    vec3 angles;
    vec3 target;
    f32  aspectRatio;
    f32  zNear = 0.01f;
    f32  zFar = 10000.0f;
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;
    
    f32 time = 0.0f;

    // Input
    Input input;

    // Camera
    Camera camera;

    mat4 projection;
    mat4 view;

    // Lights
    std::vector<Light> lights;
    Light* currentLight = nullptr;

    Entity* currentEntity = nullptr;

    u32 sphereId;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Material>  materials;
    std::vector<Mesh>  meshes;
    std::vector<Model>  models;
    std::vector<Program>  programs;

    std::vector<Entity> enTities;

    // program indices
    u32 texturedQuadProgramIdx;              // Location of the texture uniform in the textured quad shader
    u32 texturedGeometryProgramIdx;
    u32 texturedDLightProgramIdx;
    u32 texturedPLightProgramIdx;

    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    //Texture
    u32 textureQuadProgram_uTexture;
    u32 textureMeshProgram_uTexture;
    u32 textureLightProgram_uTexture;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    // Uniforms
    i32 maxUniformBufferSize;
    i32 uniformBlockAlignment;
    u32 bufferHandle;

    // Buffer
    Buffer cBuffer;  //Constant Buffer

    int gloabalParamsOffset;
    int gloabalParamsSize;

    // Framebruffers
    u32 currentAttachmentHandle;

    u32 framebufferTexturesHandle[6];

    /*u32 colorAttachmentHandle;
    u32 normalAttachmentHandle;
    u32 albedoAttachmentHandle;
    u32 depthColorAttachmentHandle;
    u32 positionColorAttachmentHandle;
    u32 specularColorAttachmentHandle;*/
    u32 depthAttachmentHandle;

    u32 framebufferHandle;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

void Render(App* app);

u32 LoadTexture2D(App* app, const char* filepath);

constexpr vec3 GetAttenuation(u32 range);

void CreateQuat();