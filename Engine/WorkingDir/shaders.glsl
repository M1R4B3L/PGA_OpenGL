///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURE_FILLQUAD

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location=0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.

#ifdef TEXTURE_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in vec3 aTangent;
layout(location=4) in vec3 aBitangent;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;

void main()
{
	vTexCoord	= aTexCoord;
	vPosition	= vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal		= vec3(uWorldMatrix * vec4(aNormal, 0.0));
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;

uniform sampler2D uTexture;

layout(location=0) out vec4 oColor;
layout(location=1) out vec4 albedoColor;
layout(location=2) out vec4 nColor;
layout(location=3) out vec4 depthColor;
layout(location=4) out vec4 positionColor;

float near = 0.1; 
float far  = 100.0; 
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main()
{
	albedoColor = texture(uTexture, vTexCoord);
	nColor = vec4(normalize(vNormal),1.0);
	depthColor = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far),1.0);
	positionColor = vec4(vPosition,1.0);
	oColor = albedoColor;
}

#endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef TEXTURE_DLIGHT

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;
out vec3 vPosition;

void main()
{
	vTexCoord	= aTexCoord;
	gl_Position = vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

struct Light
{
    vec3 col;
    vec3 dir;
    vec3 pos;
	float range;
	vec3 attenuation;
};

in vec2 vTexCoord;

uniform sampler2D uTextureAlbedo;
uniform sampler2D uTextureNormal;
uniform sampler2D uTextureDepth;
uniform sampler2D uTexturePos;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
};

layout(binding = 2, std140) uniform LightParams
{
	Light uLight;
};

layout(location=0) out vec4 oColor;

void main()
{
	vec3 albedo = texture(uTextureAlbedo, vTexCoord).rgb;
	vec3 normal = texture(uTextureNormal, vTexCoord).rgb;
	vec3 depth = texture(uTextureDepth, vTexCoord).rgb;
	vec3 pos = texture(uTexturePos, vTexCoord).rgb;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	vec3 lightDir = normalize(uLight.dir);
	vec3 viewDir = normalize(uCameraPosition - pos); 
	vec3 reflectDir = reflect(-lightDir, normal);

	float diff;
	float spec;

	vec3 resultCol;

	diff = max(dot(normal, lightDir),0.0);
	spec = pow(max(dot(viewDir, reflectDir),0.0),32);

	ambient = uLight.col * 0.2;
	diffuse = diff * uLight.col * 0.7;
	specular = spec * uLight.col * 0.5;

	resultCol = (ambient + diffuse + specular) * albedo;

	oColor = vec4(resultCol,1.0);

}

#endif
#endif

#ifdef TEXTURE_PLIGHT

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;

void main()
{
	vTexCoord	= aTexCoord;

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

struct Light
{
    vec3 col;
    vec3 dir;
    vec3 pos;
	float range;
	vec3 attenuation;
};

in vec2 vTexCoord;

uniform sampler2D uTextureAlbedo;
uniform sampler2D uTextureNormal;
uniform sampler2D uTextureDepth;
uniform sampler2D uTexturePos;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
};

layout(binding = 2, std140) uniform LightParams
{
	Light uLight;
};

layout(location=0) out vec4 oColor;

void main()
{
	vec2 texCoord = gl_FragCoord.xy / textureSize(uTextureAlbedo, 0);

	vec3 albedo = texture(uTextureAlbedo, texCoord).rgb;
	vec3 normal = texture(uTextureNormal, texCoord).rgb;
	vec3 depth = texture(uTextureDepth, texCoord).rgb;
	vec3 pos = texture(uTexturePos, texCoord).rgb;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	vec3 lightDir = normalize(uLight.pos - pos);
	vec3 viewDir = normalize(uCameraPosition - pos); 
	vec3 reflectDir = reflect(-lightDir, normal);

	float diff;
	float spec;

	vec3 resultCol;

	float distance = length(uLight.pos - pos);
	float atten = 1.0 / (uLight.attenuation.x + uLight.attenuation.y * distance + uLight.attenuation.z * (distance * distance));

	diff = max(dot(normal, lightDir),0.0);
	spec = pow(max(dot(viewDir, reflectDir),0.0),32);

	ambient = uLight.col * 0.2 ;
	diffuse = diff * uLight.col * 0.7;
	specular = spec * uLight.col * 0.5; 

	resultCol = (ambient + diffuse + specular) * albedo;

	oColor = vec4(resultCol, 1.0);
}

#endif
#endif