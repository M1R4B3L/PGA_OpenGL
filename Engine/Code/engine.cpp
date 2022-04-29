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
    if(range <= 13)     { return vec3(constant,0.35,   0.44); }
    if(range <= 20)     { return vec3(constant,0.22,   0.20); }
    if(range <= 32)     { return vec3(constant,0.14,   0.07); }
    if(range <= 50)     { return vec3(constant,0.09,   0.032); }
    if(range <= 65)     { return vec3(constant,0.07,   0.017); }
    if(range <= 100)    { return vec3(constant,0.045,  0.0075); }
    if(range <= 160)    { return vec3(constant,0.027,  0.0028); }
    if(range <= 200)    { return vec3(constant,0.022,  0.0019); }
    if(range <= 325)    { return vec3(constant,0.014,  0.0007); }
    if(range <= 600)    { return vec3(constant,0.007,  0.0002); }
    if(range <= 3250)   { return vec3(constant,0.0014, 0.000007); }

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

void InitializeTextureQuad(App* app)
{  
    f32 vertices[] = {  1, 1, 0.0, 1.0, 1.0,
                        1,-1, 0.0, 1.0, 0.0,
                       -1,-1, 0.0, 0.0, 0.0,
                       -1, 1, 0.0, 0.0, 1.0 };

    u16 indices[] = { 0, 1, 2,
                      0, 2, 3 };

    std::vector<float> vertices2;
    std::vector<float> indices2;

    for(int i = 0; i < 20; ++i)
        vertices2.push_back(vertices[i]);

    for (int i = 0; i < 6; ++i)
        indices2.push_back(indices[i]);

    int stride = 0;

    VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 2, sizeof(float)*3 });
    vertexBufferLayout.stride = 5 * sizeof(float);

    Submesh submesh;
    submesh.vertexBufferLayout = vertexBufferLayout;
    submesh.vertices.swap(vertices2);
    submesh.vertices.swap(indices2);

    app->meshes.push_back(Mesh{});
    Mesh& mesh = app->meshes.back();

    mesh.submeshes.push_back(submesh);

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

    app->texturedQuadProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& textureQuadProgram = app->programs[app->texturedQuadProgramIdx];
    app->textureQuadProgram_uTexture = glGetUniformLocation(textureQuadProgram.handle, "uTexture");

    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    app->mode = Mode_TexturedQuad;
}

void InitializeTextureMesh(App* app)
{
    u32 patrick = LoadModel(app, "Patrick/Patrick.obj");

    Entity enTity1 = Entity(glm::mat4(1.0f), patrick, 0,0);
    enTity1.worldMatrix = TransformPositionScale(vec3(0.0, 0.0, 0.0), vec3(0.45f));
    app->enTities.push_back(enTity1);

    Entity enTity2 = Entity(glm::mat4(1.0f), patrick, 0, 0);
    enTity2.worldMatrix = TransformPositionScale(vec3(2.0, 0.0, 5.0), vec3(0.45f));
    app->enTities.push_back(enTity2);

    Entity enTity3 = Entity(glm::mat4(1.0f), patrick, 0, 0);
    enTity3.worldMatrix = TransformPositionScale(vec3(-2.0, 0.0, 5.0), vec3(0.45f));
    app->enTities.push_back(enTity3);

    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_PATRICIO");
    Program& textureGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->textureMeshProgram_uTexture = glGetUniformLocation(textureGeometryProgram.handle, "uTexture");

    app->mode = Mode_TextureMesh;
}

u32 CreateFrameBuffers(App* app)
{
    //Create Textures
    glGenTextures(1, &app->colorAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);    //Time / Repeticion para texturas animadas
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    //U
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    //V
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->normalAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->normalAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glGenTextures(1, &app->albedoAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->albedoAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->depthColorAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthColorAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    u32 framebufferHandle;
    glGenFramebuffers(1, &framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferHandle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->colorAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->normalAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->albedoAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->depthColorAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

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

    u32 drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return framebufferHandle;
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

    InitializeTextureQuad(app);
    InitializeTextureMesh(app);

    app->framebufferHandle = CreateFrameBuffers(app);
    app->currentAttachmentHandle = app->colorAttachmentHandle;

    app->camera.pos = glm::vec3(0.0f, 1.0f, 9.0f);
    app->camera.angles = glm::vec3(0.0f);
    app->camera.target =  glm::vec3(0.0f);
    app->camera.aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;

    // Lights

    Light light;
    light.pos = vec3(0.0f,0.0f,0.0f);
    light.type = LightType::Point;
    light.col = vec3(1.0f,1.0f,1.0f);
    light.range = 200.0f;
    light.dir = vec3(1.0f);
    light.attenuation = GetAttenuation(light.range);
    app->lights.push_back(light);
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
                vec3 lightDir = app->lights[0].dir;
                vec3 ligthPos = app->lights[0].pos;
                vec3 ligthCol = app->lights[0].col;

                if (ImGui::DragFloat3("Dir ##lights", (float*)&lightDir, 0.1f))
                {
                    app->lights[0].dir = lightDir;
                }

                if (ImGui::DragFloat3("Position ##lights", (float*)&ligthPos, 0.1f))
                {
                    app->lights[0].pos = ligthPos;
                }

                if (ImGui::DragFloat3("Color ##lights", (float*)&ligthCol, 0.01f,0.0f,1.0f))
                {
                    app->lights[0].col = ligthCol;
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
            const char* items[] = { "Scene", "Normal", "Albedo", "Depth" };
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
                            app->currentAttachmentHandle = app->colorAttachmentHandle;
                        }

                        if (current == "Normal")
                        {
                            app->currentAttachmentHandle = app->normalAttachmentHandle;
                        }

                        if (current == "Albedo")
                        {
                            app->currentAttachmentHandle = app->albedoAttachmentHandle;
                        }

                        if (current == "Depth")
                        {
                            app->currentAttachmentHandle = app->depthColorAttachmentHandle;
                        }
                    }

                    if (is_selected)
                       ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                           
                      
                }
                ImGui::EndCombo();
            }

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

    //app->enTities[1].worldMatrix = glm::rotate(app->enTities[1].worldMatrix, glm::radians(app->time), glm::vec3(0.0f, 1.0f, 0.0f));
    //app->time += 1.0f;

    //Uniforms
    glBindBuffer(GL_UNIFORM_BUFFER, app->cBuffer.handle);
    app->cBuffer.data = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    app->cBuffer.head = 0;

    //Global Params
    app->gloabalParamsOffset = app->cBuffer.head;

    PushVec3(app->cBuffer, app->camera.pos);
    PushUInt(app->cBuffer, app->lights.size());

    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->cBuffer, sizeof(vec4));

        Light& light = app->lights[i];
        PushVec3(app->cBuffer, light.col);
        PushVec3(app->cBuffer, light.dir);
        PushVec3(app->cBuffer, light.pos);
        PushUInt(app->cBuffer, (u32)light.type);
        PushFloat(app->cBuffer, light.range);
    }
    app->gloabalParamsSize = app->cBuffer.head - app->gloabalParamsOffset;

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

                glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glEnable(GL_DEPTH_TEST);

                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                Program& textureMeshProgram = app->programs[app->texturedGeometryProgramIdx];
                glUseProgram(textureMeshProgram.handle);

                glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->cBuffer.handle, app->gloabalParamsOffset, app->gloabalParamsSize);

                for (int j = 0; j < app->enTities.size(); ++j)
                {
                    Model& model = app->models[app->enTities[j].modelIdx];
                    Mesh& mesh = app->meshes[model.meshIdx];

                    u32 blockOffset = app->enTities[j].localParamsOffset;
                    u32 blockSize = app->enTities[j].localParamsSize;

                    glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cBuffer.handle, blockOffset, blockSize);

                    for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                    {
                        GLuint vao = FindVAO(mesh, i, textureMeshProgram);
                        glBindVertexArray(vao);

                        u32 submeshMaterialIdx = model.materialIdx[i];
                        Material& submeshMaterial = app->materials[submeshMaterialIdx];

                        glActiveTexture(GL_TEXTURE);
                        glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                        glUniform1i(app->textureMeshProgram_uTexture, 0);

                        Submesh& submesh = mesh.submeshes[i];
                        glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                    }
                }

                glBindVertexArray(0);
                glUseProgram(0);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glUseProgram(app->programs[app->texturedQuadProgramIdx].handle);
                glBindVertexArray(app->vao);

                glUniform1i(app->textureQuadProgram_uTexture, 0);

                glBindTexture(GL_TEXTURE_2D, app->currentAttachmentHandle);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

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

