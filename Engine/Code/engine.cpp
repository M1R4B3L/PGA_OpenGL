//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include "assimp_model_loading.h"
#include "buffer_management.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);

    int attributeCount;
    char attributeName[128];
    int attributeNameLenght;
    int attributeSize;
    GLenum attributeType;

    int attributeLocation;

    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

    for (int i = 0; i <= attributeCount; ++i)
    {
        glGetActiveAttrib(program.handle, i,
            ARRAY_COUNT(attributeName),
            &attributeNameLenght,
            &attributeSize,
            &attributeType,
            attributeName);

        attributeLocation = glGetAttribLocation(program.handle, attributeName);

        u8 realCount = 0;

        switch (attributeType)
        {
        case GL_FLOAT:
            realCount = 1;
            break;
        case GL_FLOAT_VEC2:
            realCount = 2;
            break;
        case GL_FLOAT_VEC3:
            realCount = 3;
            break;
        default:
            realCount = 1;
            break;
        }

        program.vertexInputLayout.attributes.push_back({ (u8)attributeLocation , (u8)realCount });
    }

    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

constexpr vec3 GetAttenuation(u32 range)
{
    float constant = 1.0;
    if(range <= 7)      { return vec3(constant,0.7,    1.8); }
    else if(range <= 13)     { return vec3(constant,0.35,   0.44); }
    else if(range <= 20)     { return vec3(constant,0.22,   0.20); }
    else if(range <= 32)     { return vec3(constant,0.14,   0.07); }
    else if(range <= 50)     { return vec3(constant,0.09,   0.032); }
    else if(range <= 65)     { return vec3(constant,0.07,   0.017); }
    else if(range <= 100)    { return vec3(constant,0.045,  0.0075); }
    else if(range <= 160)    { return vec3(constant,0.027,  0.0028); }
    else if(range <= 200)    { return vec3(constant,0.022,  0.0019); }
    else if(range <= 325)    { return vec3(constant,0.014,  0.0007); }
    else if(range <= 600)    { return vec3(constant,0.007,  0.0002); }
    else if(range <= 3250)   { return vec3(constant,0.0014, 0.000007); }

}

mat4 TransformScale(const vec3& scaleFactors)
{
    mat4 transform = glm::scale(scaleFactors);
    return transform;
}

mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactors)
{
    mat4 transform = glm::translate(pos);
    transform = glm::scale(transform, scaleFactors);
    return transform;
}

mat4 UpdateMat(vec3 _pos, vec3 _rotAngle, vec3 _scale)
{
    mat4 _worldMatrix = mat4(1.0f);
    _worldMatrix = glm::translate(_worldMatrix, _pos);
    _worldMatrix = glm::rotate(_worldMatrix, glm::radians(_rotAngle.x), glm::vec3(1, 0, 0));//rotation x 
    _worldMatrix = glm::rotate(_worldMatrix, glm::radians(_rotAngle.y), glm::vec3(0, 1, 0));//rotation y 
    _worldMatrix = glm::rotate(_worldMatrix, glm::radians(_rotAngle.z), glm::vec3(0, 0, 1));//rotation z 
    _worldMatrix = glm::scale(_worldMatrix, _scale);

    return _worldMatrix;
}

void InitializeTextureQuad(App* app, const char* shaderName)
{  
    f32 vertices[] = {  -1, -1, 0.0, 0.0, 0.0,
                        1,-1, 0.0, 1.0, 0.0,
                        1, 1, 0.0, 1.0, 1.0,
                       -1, 1, 0.0, 0.0, 1.0 };

    u16 indices[] = { 0, 1, 2,
                      0, 2, 3 };

    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(f32)*5, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(f32)*5, (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    app->texturedQuadProgramIdx = LoadProgram(app, "shaders.glsl", shaderName);
    Program& textureQuadProgram = app->programs[app->texturedQuadProgramIdx];
    app->textureQuadProgram_uTexture = glGetUniformLocation(textureQuadProgram.handle, "uTexture");

    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    app->mode = Mode_TexturedQuad;
}

void InitializeDepthStencil(App* app, const char* shaderName)
{
    app->texturedDepthStencil = LoadProgram(app, "shaders.glsl", shaderName);
    Program& textureGeometryProgram = app->programs[app->texturedDepthStencil];
}

void InitializeTextureMesh(App* app, const char* shaderName)
{
    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", shaderName);
    Program& textureGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->textureMeshProgram_uTexture = glGetUniformLocation(textureGeometryProgram.handle, "uTexture");

    app->mode = Mode_TextureMesh;
}

void InitializeTextureDLight(App* app, const char* shaderName)
{
    app->texturedDLightProgramIdx = LoadProgram(app, "shaders.glsl", shaderName);
    Program& textureLightProgram = app->programs[app->texturedDLightProgramIdx];
    app->textureLightProgram_uTexture = glGetUniformLocation(textureLightProgram.handle, "uTexture");
}

void InitializeTexturePLight(App* app, const char* shaderName)
{
    app->texturedPLightProgramIdx = LoadProgram(app, "shaders.glsl", shaderName);
    Program& textureLightProgram = app->programs[app->texturedPLightProgramIdx];
    app->textureLightProgram_uTexture = glGetUniformLocation(textureLightProgram.handle, "uTexture");
}

void InitailizeTextureNormalMap(App* app, const char* shaderName)
{
    app->texturedNormalMapIdx = LoadProgram(app, "shaders.glsl", shaderName);
    Program& textureNormalMapProgram = app->programs[app->texturedNormalMapIdx];

    app->textureMeshProgram_uTexture = glGetUniformLocation(textureNormalMapProgram.handle, "uTexture");
    app->textureNormalMapProgram_uTexture = glGetUniformLocation(textureNormalMapProgram.handle, "uWormalMap");
}


u32 CreateFrameBuffers(App* app)
{
    //Create Textures
    for(int i = 0; i < 5; ++i)
    { 
        glGenTextures(1, &app->framebufferTexturesHandle[i]);
        glBindTexture(GL_TEXTURE_2D, app->framebufferTexturesHandle[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);    //Time / Repeticion para texturas animadas
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    //U
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    //V
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    u32 framebufferHandle;
    glGenFramebuffers(1, &framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferHandle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->framebufferTexturesHandle[0], 0);       //Color
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->framebufferTexturesHandle[1], 0);       //Albedo    
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->framebufferTexturesHandle[2], 0);       //Normal
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->framebufferTexturesHandle[3], 0);       //Depth 
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, app->framebufferTexturesHandle[4], 0);       //Pos
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, app->depthAttachmentHandle, 0);

    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (framebufferStatus)
        {
            case GL_FRAMEBUFFER_UNDEFINED:                      ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:          ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:  ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
            case GL_FRAMEBUFFER_UNSUPPORTED:                    ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:       ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
            default:                                            ELOG("UNKNOWN FRAMEBUFFER ERROR"); break;
        }
    }

    app->currentAttachmentHandle = app->framebufferTexturesHandle[0];

    return framebufferHandle;
}

u32 CreatePlane(App* app)
{
    app->meshes.push_back(Mesh{});
    Mesh& mesh = app->meshes.back();
    u32 meshIdx = (u32)app->meshes.size() - 1u;

    app->models.push_back(Model{});
    Model& model = app->models.back();
    model.meshIdx = meshIdx;
    u32 modelIdx = (u32)app->models.size() - 1u;

    mesh.submeshes.push_back(Submesh{});
    Submesh& subMesh = mesh.submeshes.back();

    subMesh.vertexOffset = 0;
    subMesh.indexOffset = 0;

    f32 vertices[] = {  
        /*Position*/ -1.0, 1.0, 0.0, /*Normal*/ 0, 0, 1.0, /*TextCoords*/ 0.0, 1.0, /*Tangent*/ 1,0,0, /*Bitangent*/ 0,1,0,
        /*Position*/ -1.0, -1.0, 0.0, /*Normal*/ 0, 0, 1.0, /*TextCoords*/ 0.0, 0.0, /*Tangent*/ 1,0,0, /*Bitangent*/ 0,1,0,
        /*Position*/  1.0, -1.0, 0.0, /*Normal*/ 0, 0, 1.0, /*TextCoords*/ 1.0, 0.0, /*Tangent*/ 1,0,0, /*Bitangent*/ 0,1,0,
        /*Position*/  1.0, 1.0, 0.0, /*Normal*/ 0, 0, 1.0, /*TextCoords*/ 1.0, 1.0, /*Tangent*/ 1,0,0, /*Bitangent*/ 0,1,0, };

    u16 indices[] = { 0, 1, 2,
                      0, 2, 3};

    for (int i = 0; i < 56; ++i)
    {
        subMesh.vertices.push_back(vertices[i]);
    }

    for (int i = 0; i < 6; ++i)
    {
        subMesh.indices.push_back(indices[i]);
    }

    VertexBufferLayout vertexLayout;
    vertexLayout.attributes.push_back({ 0, 3, 0 });
    vertexLayout.attributes.push_back({ 1, 3, 3 * sizeof(float) });
    vertexLayout.stride = 6 * sizeof(float);

    vertexLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, vertexLayout.stride });
    vertexLayout.stride += 2 * sizeof(float);

    vertexLayout.attributes.push_back(VertexBufferAttribute{ 3, 3, vertexLayout.stride });
    vertexLayout.stride += 3 * sizeof(float);

    vertexLayout.attributes.push_back(VertexBufferAttribute{ 4, 3, vertexLayout.stride });
    vertexLayout.stride += 3 * sizeof(float);

    subMesh.vertexBufferLayout = vertexLayout;

    u32 vertexBufferSize = 0;
    u32 indexBufferSize = 0;

    vertexBufferSize += subMesh.vertices.size() * sizeof(float);
    indexBufferSize += subMesh.indices.size() * sizeof(u32);

    glGenBuffers(1, &mesh.vertexBufferHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.indexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, NULL, GL_STATIC_DRAW);

    u32 indicesOffset = 0;
    u32 verticesOffset = 0;

    const void* verticesData = subMesh.vertices.data();
    const u32   verticesSize = subMesh.vertices.size() * sizeof(float);
    glBufferSubData(GL_ARRAY_BUFFER, verticesOffset, verticesSize, verticesData);
    subMesh.vertexOffset = verticesOffset;
    verticesOffset += verticesSize;

    const void* indicesData = subMesh.indices.data();
    const u32   indicesSize = subMesh.indices.size() * sizeof(u32);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indicesOffset, indicesSize, indicesData);
    subMesh.indexOffset = indicesOffset;
    indicesOffset += indicesSize;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    app->materials.push_back(Material{});
    Material& material = app->materials.back();
    material.name = "PlaneMat";
    material.albedo = vec3(1);
    material.albedoTextureIdx = LoadTexture2D(app, "brickwall.jpg");
    material.normalsTextureIdx = LoadTexture2D(app, "brickwall_normal.jpg");
    u32 materialIdx = app->materials.size() - 1;
    model.materialIdx.push_back(materialIdx);

    return app->models.size() - 1;
}

u32 CreateSphere(App* app)
{
    app->meshes.push_back(Mesh{});
    Mesh& mesh = app->meshes.back();
    u32 meshIdx = (u32)app->meshes.size() - 1u;

    app->models.push_back(Model{});
    Model& model = app->models.back();
    model.meshIdx = meshIdx;
    u32 modelIdx = (u32)app->models.size() - 1u;

    mesh.submeshes.push_back(Submesh{});
    Submesh& subMesh = mesh.submeshes.back();

    subMesh.vertexOffset = 0;
    subMesh.indexOffset = 0;

    const u32 H = 32;
    const u32 V = 16;
    struct Vertex { vec3 pos; vec3 norm; };
    Vertex sphere[H][V + 1];

    for (int h = 0; h < H; ++h)
    {
        for (int v = 0; v < V + 1; ++v)
        {
            float nh = float(h) / H;
            float nv = float(v) / V - 0.5f;
            float angleh = 2 * PI * nh;
            float anglev = -PI * nv;
            sphere[h][v].pos.x = sinf(angleh) * cosf(anglev);
            sphere[h][v].pos.y = -sinf(anglev);
            sphere[h][v].pos.z = cosf(angleh) * cosf(anglev);
            sphere[h][v].norm = sphere[h][v].pos;
            subMesh.vertices.push_back(sphere[h][v].pos.x);
            subMesh.vertices.push_back(sphere[h][v].pos.y);
            subMesh.vertices.push_back(sphere[h][v].pos.z);
            subMesh.vertices.push_back(sphere[h][v].norm.x);
            subMesh.vertices.push_back(sphere[h][v].norm.y);
            subMesh.vertices.push_back(sphere[h][v].norm.z);
            //TexCoords
            subMesh.vertices.push_back(0);
            subMesh.vertices.push_back(0);
            //Tangents
            subMesh.vertices.push_back(0);
            subMesh.vertices.push_back(0);
            subMesh.vertices.push_back(0);
            //Bitangets
            subMesh.vertices.push_back(0);
            subMesh.vertices.push_back(0);
            subMesh.vertices.push_back(0);
        }
    }

    u32 sphereIndices[H][V][6];

    for (u32 h = 0; h < H; ++h)
    {
        for (u32 v = 0; v < V; ++v)
        {
            sphereIndices[h][v][0] = (h + 0)     * (V + 1) + v;
            sphereIndices[h][v][1] = ((h + 1)%H) * (V + 1) + v;
            sphereIndices[h][v][2] = ((h + 1)%H) * (V + 1) + v + 1;
            sphereIndices[h][v][3] = (h + 0)     * (V + 1) + v;
            sphereIndices[h][v][4] = ((h + 1)%H) * (V + 1) + v + 1;
            sphereIndices[h][v][5] = (h + 0)     * (V + 1) + v + 1;
            subMesh.indices.push_back(sphereIndices[h][v][0]);
            subMesh.indices.push_back(sphereIndices[h][v][1]);
            subMesh.indices.push_back(sphereIndices[h][v][2]);
            subMesh.indices.push_back(sphereIndices[h][v][3]);
            subMesh.indices.push_back(sphereIndices[h][v][4]);
            subMesh.indices.push_back(sphereIndices[h][v][5]);

        }
    }

    VertexBufferLayout vertexLayout;
    vertexLayout.attributes.push_back({ 0, 3, 0 });
    vertexLayout.attributes.push_back({ 1, 3, 3 * sizeof(float) });
    vertexLayout.stride = 6 * sizeof(float);

    vertexLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, vertexLayout.stride });
    vertexLayout.stride += 2 * sizeof(float);

    vertexLayout.attributes.push_back(VertexBufferAttribute{ 3, 3, vertexLayout.stride });
    vertexLayout.stride += 3 * sizeof(float);

    vertexLayout.attributes.push_back(VertexBufferAttribute{ 4, 3, vertexLayout.stride });
    vertexLayout.stride += 3 * sizeof(float);

    subMesh.vertexBufferLayout = vertexLayout;

    u32 vertexBufferSize = 0;
    u32 indexBufferSize = 0;

    vertexBufferSize += subMesh.vertices.size() * sizeof(float);
    indexBufferSize += subMesh.indices.size() * sizeof(u32);
   
    glGenBuffers(1, &mesh.vertexBufferHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.indexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, NULL, GL_STATIC_DRAW);

    u32 indicesOffset = 0;
    u32 verticesOffset = 0;

    const void* verticesData = subMesh.vertices.data();
    const u32   verticesSize = subMesh.vertices.size() * sizeof(float);
    glBufferSubData(GL_ARRAY_BUFFER, verticesOffset, verticesSize, verticesData);
    subMesh.vertexOffset = verticesOffset;
    verticesOffset += verticesSize;

    const void* indicesData = subMesh.indices.data();
    const u32   indicesSize = subMesh.indices.size() * sizeof(u32);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indicesOffset, indicesSize, indicesData);
    subMesh.indexOffset = indicesOffset;
    indicesOffset += indicesSize;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    app->materials.push_back(Material{});
    Material& material = app->materials.back();
    material.name = "SphereMat";
    material.albedo = vec3(1);
    material.albedoTextureIdx = app->whiteTexIdx;
    material.bumpTextureIdx = app->whiteTexIdx;
    u32 materialIdx = app->materials.size() - 1;
    model.materialIdx.push_back(materialIdx);

    return app->models.size() - 1;
}

Light CreateLight(LightType type, vec3 pos, vec3 dir, vec3 col, float range)
{
    Light light;

    light.type = type;
    light.pos = pos;
    light.dir = dir;
    light.col = col;
    light.range = range;
    light.attenuation = GetAttenuation(light.range);

    light.worldMatrix = UpdateMat(pos, vec3(0.0f), vec3(range));

    return light;
}

void Init(App* app)
{
    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    //Geometry
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    glGenBuffers(1, &app->cBuffer.handle);
    glBindBuffer(GL_UNIFORM_BUFFER, app->cBuffer.handle);
    glBufferData(GL_UNIFORM_BUFFER, app->maxUniformBufferSize, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    InitializeTextureQuad(app, "TEXTURE_FILLQUAD");
    InitializeTextureMesh(app,"TEXTURE_GEOMETRY");
    InitializeDepthStencil(app, "TEXTURE_DEPTHSTENCIL");
    InitializeTextureDLight(app, "TEXTURE_DLIGHT");
    InitializeTexturePLight(app, "TEXTURE_PLIGHT");

    InitailizeTextureNormalMap(app, "TEXTURE_NORMALMAPPING");

    //Patricks
    u32 patrick = LoadModel(app, "Patrick/Patrick.obj");
    Entity enTity1 = Entity(vec3(0.0, 3.5, 0.0), vec3(0.0f), vec3(1.0f), patrick, 0, 0);
    enTity1.name = "Patrick" + std::to_string(app->enTities.size());
    app->enTities.push_back(enTity1);
    Entity enTity2 = Entity(vec3(5.0, 3.5, 5.0), vec3(0.0f), vec3(1.0f), patrick, 0, 0);
    enTity2.name = "Patrick" + std::to_string(app->enTities.size());
    app->enTities.push_back(enTity2);

    u32 cyborg = LoadModel(app, "Cyborg/cyborg.obj");
    Entity enTity3 = Entity(vec3(-5.0, 3.5, 5.0), vec3(0.0f), vec3(2.0f), cyborg, 0, 0);
    enTity3.name = "Cyborg" + std::to_string(app->enTities.size());
    app->enTities.push_back(enTity3);

    app->sphereId = CreateSphere(app);
    Entity sphere = Entity(vec3(0), vec3(0.0f), vec3(1.0f), app->sphereId, 0, 0);
    sphere.name = "Sphere" + std::to_string(app->enTities.size());
    app->enTities.push_back(sphere);

    u32 planeId = CreatePlane(app);
    Entity plane = Entity(vec3(0), vec3(0.0f), vec3(20.0f), planeId, 0, 0);
    plane.name = "Plane" + std::to_string(app->enTities.size());
    app->enTities.push_back(plane);
    Entity plane1 = Entity(vec3(0), vec3(-90.0f,0.0f,0.0f), vec3(20.0f), planeId, 0, 0);
    plane1.name = "Plane" + std::to_string(app->enTities.size());
    app->enTities.push_back(plane1);

    app->framebufferHandle = CreateFrameBuffers(app);

    app->camera.pos = glm::vec3(0.0f, 10.0f, 30.0f);
    app->camera.angles = glm::vec3(0.0f);
    app->camera.target =  glm::vec3(0.0f);
    app->camera.aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;

    // Lights
    Light light0 = CreateLight(LightType::Directional, vec3(1.0f, 0.0f, 5.0f), vec3(1.0f), vec3(1.0f, 1.0f, 1.0f), 0.0f);
    app->lights.push_back(light0);

    for (int i = 0; i < 3; ++i)
    {
        Light light2 = CreateLight(LightType::Point, vec3(-7.0f+(i*5.0f), 2.0f, 0.0f), vec3(1.0f), vec3(0.0f + i, 1.0f, 1.0f - i), 10.0f);
        app->lights.push_back(light2);
    }
}

void Docking()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    static bool docking = true;

    if (ImGui::Begin("DockSpace", &docking, window_flags)) {
        // DockSpace
        ImGui::PopStyleVar(3);
        if (docking)
        {
            ImGuiID dockspace_id = ImGui::GetID("DockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        }

        ImGui::End();
    }
}

void Gui(App* app)
{
    Docking();
    static bool demoOpen = false;
    static bool engineInfoOpen = true;
    static bool viewModeOpen = true;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Control Panel")) {

            if (ImGui::MenuItem("Info", NULL, engineInfoOpen)) {
                engineInfoOpen = !engineInfoOpen;
            }

            if (ImGui::MenuItem("View Mode", NULL, viewModeOpen)) {
                viewModeOpen = !viewModeOpen;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Create ##primitive")) {
            if (ImGui::BeginMenu("Entities"))
            {
                if (ImGui::MenuItem("Plane ##primitive")) {

                    Entity plane = Entity(vec3(0), vec3(0.0f), vec3(1.0f), 3, 0, 0);
                    plane.name = "Plane" + std::to_string(app->enTities.size());
                    app->enTities.push_back(plane);
                    app->currentEntity = nullptr;
                }
                if (ImGui::MenuItem("Sphere ##primitive")) {

                    Entity sphere = Entity(vec3(0), vec3(0.0f), vec3(1.0f), 2, 0, 0);
                    sphere.name = "Sphere" + std::to_string(app->enTities.size());
                    app->enTities.push_back(sphere);
                    app->currentEntity = nullptr;
                }
                if (ImGui::MenuItem("Patrick ##primitive")) {

                    Entity patrick = Entity(vec3(0), vec3(0.0f), vec3(1.0f), 0, 0, 0);
                    patrick.name = "Patrick" + std::to_string(app->enTities.size());
                    app->enTities.push_back(patrick);
                    app->currentEntity = nullptr;
                }
                if (ImGui::MenuItem("Cyborg ##primitive")) {

                    Entity cyborg = Entity(vec3(0), vec3(0.0f), vec3(1.0f), 1, 0, 0);
                    cyborg.name = "Cyborg" + std::to_string(app->enTities.size());
                    app->enTities.push_back(cyborg);
                    app->currentEntity = nullptr;
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Lights"))
            {
                if (ImGui::MenuItem("DLight ##primitive")) {

                    Light light = CreateLight(LightType::Directional, vec3(0.0f, 0.0f, 0.0f), vec3(1.0f), vec3(1.0f, 1.0f, 1.0f), 2.0f);
                    app->lights.push_back(light);
                }
                if (ImGui::MenuItem("PLight ##primitive")) {

                    Light light = CreateLight(LightType::Point, vec3(0.0f, 0.0f, 0.0f), vec3(1.0f), vec3(1.0f, 1.0f, 1.0f), 2.0f);
                    app->lights.push_back(light);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }


    if (engineInfoOpen)
    {
        if (ImGui::Begin("Control Panel", &engineInfoOpen)) {

            if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_CollapsingHeader))
            {
                ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
                ImGui::Text("GPU: %s", glGetString(GL_RENDERER));
                ImGui::Text("OpenGL Version: %s", glGetString(GL_VERSION));
                ImGui::Text("Mouse Pos:");
                ImGui::SameLine();
                ImGui::Text("%f,%f", app->input.mousePos.x, app->input.mousePos.y);
            }

            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_CollapsingHeader))
            {
                vec3 translate = app->camera.pos;

                if (ImGui::DragFloat3("Position ##camera", (float*)&translate, 0.1f))
                {
                    app->camera.pos = translate;
                    app->view = glm::lookAt(app->camera.pos, app->camera.target, vec3(0.0f, 1.0f, 0.0f));
                }
                if (ImGui::DragFloat3("Rotation ##camera", (float*)&app->camera.angles, 0.1f))
                {
                    
                }

            }
            if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_CollapsingHeader))
            {

                if(ImGui::TreeNode("Lights ##2"))
                { 
                    for (int i = 0; i < app->lights.size(); ++i)
                    {
                        Light* light = &app->lights[i];

                        
                        switch (light->type)
                        {
                        case LightType::Directional:
                            light->name = "Directional" + std::to_string(i);
                            break;
                        case LightType::Point:
                            light->name = "Point" + std::to_string(i);
                            break;
                        }
                        
                        if (ImGui::TreeNodeEx(light->name.c_str(),ImGuiTreeNodeFlags_Leaf))
                        {
                            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                                app->currentLight = i;

                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }

                if (app->currentLight != -1)
                {
                    Light currentLight = app->lights[app->currentLight];
                    switch (currentLight.type)
                    {
                        case LightType::Directional:
                        {
                            ImGui::Separator();
                            ImVec4 col = ImVec4(currentLight.col.x, currentLight.col.y, currentLight.col.z, 1.0);
                            ImGui::TextColored(col, currentLight.name.c_str());
                            ImGui::Separator();
                            ImGui::DragFloat3("Dir##lights", (float*)&currentLight.dir, 0.01f, -1.0f, 1.0f);
                            ImGui::ColorEdit3("Color##lights", (float*)&currentLight.col);
                            if (ImGui::Button("Remove##lights"))
                            {
                                app->lights.erase(app->lights.begin() + app->currentLight);
                                app->currentLight = -1;
                            }

                            if (app->currentLight != -1)
                            {
                                app->lights[app->currentLight] = currentLight;
                            }

                            break;
                        }

                        case LightType::Point:
                        {
                            ImGui::Separator();
                            ImVec4 col = ImVec4(currentLight.col.x, currentLight.col.y, currentLight.col.z, 1.0);
                            ImGui::TextColored(col, currentLight.name.c_str());
                            ImGui::Separator();
                            ImGui::DragFloat3("Position##lights2", (float*)&currentLight.pos, 0.1f);
                            static float range = currentLight.range;
                            if (ImGui::DragFloat("Range##lights2", &range, 0.01f, 1.0f, 3250.0f))
                            {
                                currentLight.range = range;
                                currentLight.attenuation = GetAttenuation(currentLight.range);
                            }
                            ImGui::ColorEdit3("Color##lights2", (float*)&currentLight.col);
                            currentLight.worldMatrix = UpdateMat(currentLight.pos, vec3(0.0f), vec3(currentLight.range));

                            if (ImGui::Button("Remove##lights"))
                            {
                                app->lights.erase(app->lights.begin() + app->currentLight);
                                app->currentLight = -1;
                            }

                            if (app->currentLight != -1)
                            {
                                app->lights[app->currentLight] = currentLight;
                            }
                            break;

                        }
                    }
                }

                if (ImGui::Button("Remove All Lights##lights"))
                {
                    app->lights.clear();
                    app->currentLight = -1;
                }
             
            }
            if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_CollapsingHeader))
            {
                if (ImGui::TreeNode("Entities ##2"))
                {
                    for (int i = 0; i < app->enTities.size(); ++i)
                    {
                        Entity* entity = &app->enTities[i];

                        if (ImGui::TreeNodeEx(entity->name.c_str(), ImGuiTreeNodeFlags_Leaf))
                        {
                            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                                app->currentEntity = entity;

                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }

                if (app->currentEntity != nullptr)
                {
                    mat4 entityMat = app->currentEntity->worldMatrix;

                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.0,1.0,1.0,1.0), app->currentEntity->name.c_str());
                    ImGui::Separator();

                    static int id = app->currentEntity->modelIdx;
                    if(ImGui::DragInt("MeshId", &id, 1, 0, 3))
                    {
                        if (id > 3)
                        {
                            id = 3;
                            app->currentEntity->modelIdx = id;
                        }
                        else if (id < 0)
                        {
                            id = 0;
                            app->currentEntity->modelIdx = id;
                        }
                        else
                        {
                            app->currentEntity->modelIdx = id;
                        }
                    }

                    ImGui::DragFloat3("Position ##entity", (float*)&app->currentEntity->pos, 0.1f);
                    ImGui::DragFloat3("Rotation ##entity", (float*)&app->currentEntity->rotAngle, 0.1f);
                    ImGui::DragFloat3("Scale ##entity", (float*)&app->currentEntity->scale, 0.1f);

                    app->currentEntity->worldMatrix = UpdateMat(app->currentEntity->pos, app->currentEntity->rotAngle, app->currentEntity->scale);

                }

                if (ImGui::Button("Remove All Entities##entities"))
                {
                    app->enTities.clear();
                    app->currentEntity = nullptr;
                }

            }
            ImGui::End();
        }
    }

    if (viewModeOpen)
    {
        if (ImGui::Begin("Options", &viewModeOpen)) {

            ImGui::Text("Render Mode");
            ImGui::Separator();
            const char* items[] = { "Scene", "Normal", "Albedo", "Depth", "Position"};
            static const char* current = "Scene";

            if (ImGui::BeginCombo("##views Mode", current))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    bool is_selected = (current == items[n]); // You can store your selection however you want, outside or inside your objects
                    if (ImGui::Selectable(items[n], is_selected))
                    {
                        current = items[n];
                        if (current == "Scene")
                        {
                            app->currentAttachmentHandle = app->framebufferTexturesHandle[0];
                        }
                        if (current == "Albedo")
                        {
                            app->currentAttachmentHandle = app->framebufferTexturesHandle[1];
                        }
                        if (current == "Normal")
                        {
                            app->currentAttachmentHandle = app->framebufferTexturesHandle[2];
                        }
                        if (current == "Depth")
                        {
                            app->currentAttachmentHandle = app->framebufferTexturesHandle[3];
                        }   
                        if (current == "Position")
                        {
                            app->currentAttachmentHandle = app->framebufferTexturesHandle[4];
                        }  
                    }

                    if (is_selected)
                       ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                           
                      
                }
                ImGui::EndCombo();
            }

            ImGui::Checkbox("Normal Mapping", &app->isNormalMap);

            ImGui::End();
        }
    }

    if (demoOpen) {
        ImGui::ShowDemoWindow(&demoOpen);
    }

 
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here

    app->projection = glm::perspective(glm::radians(60.0f), app->camera.aspectRatio, app->camera.zNear, app->camera.zFar);
    app->view = glm::lookAt(app->camera.pos, app->camera.target, vec3(0.0f, 1.0f, 0.0f));

    //Uniforms
    glBindBuffer(GL_UNIFORM_BUFFER, app->cBuffer.handle);
    app->cBuffer.data = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    app->cBuffer.head = 0;

    //Global Params
    app->gloabalParamsOffset = app->cBuffer.head;

    PushVec3(app->cBuffer, app->camera.pos);

    app->gloabalParamsSize = app->cBuffer.head - app->gloabalParamsOffset;

    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->cBuffer, app->uniformBlockAlignment);

        Light& light = app->lights[i];

        light.lightParamsOffset = app->cBuffer.head;
        PushVec3(app->cBuffer, light.col);
        PushVec3(app->cBuffer, light.dir);
        PushVec3(app->cBuffer, light.pos);
        PushFloat(app->cBuffer, light.range);
        PushVec3(app->cBuffer, light.attenuation);
        light.lightParamsSize = app->cBuffer.head - light.lightParamsOffset;
    }
    
    //Local Params
    for (u32 i = 0; i < app->enTities.size(); ++i)
    {
        AlignHead(app->cBuffer, app->uniformBlockAlignment);

        Entity& entity = app->enTities[i];
        mat4    world = entity.worldMatrix;
        mat4    worldViewProjection = app->projection * app->view * world;

        entity.localParamsOffset = app->cBuffer.head;
        PushMat4(app->cBuffer, world);
        PushMat4(app->cBuffer, worldViewProjection);
        entity.localParamsSize = app->cBuffer.head - entity.localParamsOffset;
    }

    //Push light Matrices

    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->cBuffer, app->uniformBlockAlignment);

        Light&  light = app->lights[i];
        mat4    world = light.worldMatrix;
        mat4    worldViewProjection = app->projection * app->view * world;
        
        light.localParamsOffset = app->cBuffer.head;
        PushMat4(app->cBuffer, world);
        PushMat4(app->cBuffer, worldViewProjection);
        light.localParamsSize = app->cBuffer.head - light.localParamsOffset;
    }

    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;
    }

    GLuint vaoHandle = 0;

    //Create a Vao
    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

    for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
    {
        bool attributeWasLinked = false;

        for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
        {
            if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
            {
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 compCount = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
                const u32 stride = submesh.vertexBufferLayout.stride;

                glVertexAttribPointer(index, compCount, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
        }

        assert(attributeWasLinked);
    }

    glBindVertexArray(0);

    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

void DeferredShadingGeometryPass(App* app)
{
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

    u32 drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, 
        GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };

    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

    glStencilMask(GL_TRUE);
    glDepthMask(GL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glDisable(GL_BLEND);

    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    Program* textureMeshProgram = &app->programs[app->texturedGeometryProgramIdx];
    glUseProgram(textureMeshProgram->handle);

    for (int j = 0; j < app->enTities.size(); ++j)
    {
        Model& model = app->models[app->enTities[j].modelIdx];
        Mesh& mesh = app->meshes[model.meshIdx];

        u32 blockOffset = app->enTities[j].localParamsOffset;
        u32 blockSize = app->enTities[j].localParamsSize;

        glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cBuffer.handle, blockOffset, blockSize);

        for (u32 i = 0; i < mesh.submeshes.size(); ++i)
        {
            u32 submeshMaterialIdx = model.materialIdx[i];
            Material* submeshMaterial = &app->materials[submeshMaterialIdx];

            if (submeshMaterial->normalsTextureIdx != 0 && app->isNormalMap == true)
            {
                textureMeshProgram = &app->programs[app->texturedNormalMapIdx];
                glUseProgram(textureMeshProgram->handle);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial->normalsTextureIdx].handle);
                glUniform1i(app->textureNormalMapProgram_uTexture, 1);
            }

            GLuint vao = FindVAO(mesh, i, *textureMeshProgram);
            glBindVertexArray(vao);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial->albedoTextureIdx].handle);
            glUniform1i(app->textureMeshProgram_uTexture, 0);
            
            Submesh& submesh = mesh.submeshes[i];
            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

            textureMeshProgram = &app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(textureMeshProgram->handle);
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void DeferredShadingLightPass(App* app)
{
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glStencilMask(GL_FALSE);
    glDepthMask(GL_FALSE);

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->cBuffer.handle, app->gloabalParamsOffset, app->gloabalParamsSize);

    for (int j = 0; j < app->lights.size(); ++j)
    {
        Program* textureLightProgram = &app->programs[app->texturedDLightProgramIdx];

        //Change Program based on light type
        
        if (app->lights[j].type == LightType::Point)
        {
            textureLightProgram = &app->programs[app->texturedPLightProgramIdx];
        }
        else
            textureLightProgram = &app->programs[app->texturedDLightProgramIdx];

        glUseProgram(textureLightProgram->handle);

        u32 loc = glGetUniformLocation(textureLightProgram->handle, "uTextureAlbedo");
        glUniform1i(loc, 0);
        loc = glGetUniformLocation(textureLightProgram->handle, "uTextureNormal");
        glUniform1i(loc, 1);                         
        loc = glGetUniformLocation(textureLightProgram->handle, "uTextureDepth");
        glUniform1i(loc, 2);                       
        loc = glGetUniformLocation(textureLightProgram->handle, "uTexturePos");
        glUniform1i(loc, 3);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, app->framebufferTexturesHandle[1]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, app->framebufferTexturesHandle[2]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, app->framebufferTexturesHandle[3]);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, app->framebufferTexturesHandle[4]);

        u32 lightBlockOffset = app->lights[j].lightParamsOffset;
        u32 lightBlockSize = app->lights[j].lightParamsSize;
        glBindBufferRange(GL_UNIFORM_BUFFER, 2, app->cBuffer.handle, lightBlockOffset, lightBlockSize);

        if (app->lights[j].type == LightType::Point)
        {
            glDisable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_STENCIL_TEST);

            glStencilMask(GL_TRUE);
            glStencilFunc(GL_ALWAYS, 0, 0);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
            glDrawBuffer(GL_NONE);

            glUseProgram(app->programs[app->texturedDepthStencil].handle);
    
            Model& model = app->models[app->sphereId];
            Mesh& mesh = app->meshes[model.meshIdx];
            u32 blockOffset = app->lights[j].localParamsOffset;
            u32 blockSize = app->lights[j].localParamsSize;
            glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cBuffer.handle, blockOffset, blockSize);
            for (u32 i = 0; i < mesh.submeshes.size(); ++i)
            {
                GLuint vao = FindVAO(mesh, i, app->programs[app->texturedDepthStencil]);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshMaterial = app->materials[submeshMaterialIdx];

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
            }

            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
            glStencilMask(GL_FALSE);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);

            glDisable(GL_DEPTH_TEST);

            glUseProgram(textureLightProgram->handle);

            for (u32 i = 0; i < mesh.submeshes.size(); ++i)
            {
                GLuint vao = FindVAO(mesh, i, *textureLightProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshMaterial = app->materials[submeshMaterialIdx];

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
            }

            glCullFace(GL_BACK);
            glDisable(GL_STENCIL_TEST);
            glStencilMask(GL_TRUE);
            glClear(GL_STENCIL_BUFFER_BIT);
        }
        else
        {
            glBindVertexArray(app->vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
        }
    }
}

void Render(App* app)
{
    switch (app->mode)
    {
        case Mode_TexturedQuad:
            {
                // TODO: Draw your textured quad here!
                // - clear the framebuffer
                // - set the viewport
                // - set the blending state
                // - bind the texture into unit 0
                // - bind the program 
                //   (...and make its texture sample from unit 0)
                // - bind the vao
                // - glDrawElements() !!!

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                Program& programTextureGeometry = app->programs[app->texturedQuadProgramIdx];
                glUseProgram(programTextureGeometry.handle);
                glBindVertexArray(app->vao);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glUniform1i(app->textureQuadProgram_uTexture, 0);
                glActiveTexture(GL_TEXTURE);
                GLuint textureHandle = app->textures[app->normalTexIdx].handle;
                glBindTexture(GL_TEXTURE_2D, textureHandle);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

                glBindVertexArray(0);
                glUseProgram(0);
            }
            break;

        case Mode_TextureMesh:
            {

                DeferredShadingGeometryPass(app);

                DeferredShadingLightPass(app);

                glBindVertexArray(0);
                glUseProgram(0);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

                glUseProgram(app->programs[app->texturedQuadProgramIdx].handle);
                glBindVertexArray(app->vao);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->currentAttachmentHandle);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
                glBindTexture(GL_TEXTURE_2D, 0);

                glBindVertexArray(0);
                glUseProgram(0);
            }
            break;

        case Mode_Count:
            {

            }
            break;

        default:;
    }
}

