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

/////////////////////////////////////////////////////

#ifdef TEXTURE_DEPTHSTENCIL

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};
void main()
{
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////
// TODO: Write your fragment shader here
void main()
{
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
	vNormal		= normalize(vec3(uWorldMatrix * vec4(aNormal, 0.0)));
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
	nColor = vec4(vNormal,1.0);
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

out vec3 vPosition;
out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
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

in vec2 vTexCoord;

void main()
{
	vec2 texCoord = vTexCoord;

	vec3 albedo = texture(uTextureAlbedo, texCoord).xyz;
	vec3 normal = texture(uTextureNormal, texCoord).xyz;
	vec3 depth = texture(uTextureDepth, texCoord).xyz;
	vec3 pos = texture(uTexturePos, texCoord).xyz;

	//Ambient
	float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * uLight.col;

	//Diffuse
	vec3 lightDir = normalize(uLight.dir); 
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * uLight.col;

	//Specular
	vec3 viewDir = normalize(uCameraPosition - pos); 
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir),0.0),128);
	vec3 specular = spec * uLight.col; 

	vec3 resultCol = (ambient + diffuse + specular) * albedo;

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

void main()
{
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

	vec3 albedo = texture(uTextureAlbedo, texCoord).xyz;
	vec3 normal = texture(uTextureNormal, texCoord).xyz;
	vec3 depth = texture(uTextureDepth, texCoord).xyz;
	vec3 pos = texture(uTexturePos, texCoord).xyz;

	//Ambient
	float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * uLight.col;

	//Diffuse
	vec3 lightDir = normalize(uLight.pos - pos);
	float diff = max(dot(normal, lightDir),0.0);
	vec3 diffuse = diff * uLight.col;

	//Specular
	vec3 viewDir = normalize(uCameraPosition - pos); 
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir),0.0),128);
	vec3 specular = spec * uLight.col;

	//Final
	vec3 resultCol;
	float dist = length(uLight.pos - pos);
	float atten = 1.0 / (uLight.attenuation.x + uLight.attenuation.y * dist + uLight.attenuation.z * (dist * dist));

	diffuse *= atten;

	resultCol = (ambient + diffuse + specular) * albedo;

	oColor = vec4(resultCol, 1.0);		
}

#endif
#endif

/////////////////////////////////////////////////////

#ifdef TEXTURE_NORMALMAPPING

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
out mat3 vTBN;

void main()
{

	vTexCoord	= aTexCoord;
	vPosition	= vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal		= normalize(vec3(uWorldMatrix * vec4(aNormal, 0.0)));
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);

	vec3 T = normalize(vec3(uWorldMatrix * vec4(aTangent,0.0)));
	vec3 B = normalize(vec3(uWorldMatrix * vec4(aBitangent,0.0)));
	vTBN = mat3(T, B, vNormal);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in mat3 vTBN;

uniform sampler2D uTexture;
uniform sampler2D uWormalMap;

layout(location=0) out vec4 oColor;
layout(location=1) out vec4 albedoColor;
layout(location=2) out vec4 nColor;
layout(location=3) out vec4 depthColor;
layout(location=4) out vec4 positionColor;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

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
	//nColor = vec4(vNormal,1.0);
	depthColor = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far),1.0);
	positionColor = vec4(vPosition,1.0);
	oColor = albedoColor;

	// Convert normal from tangent space to local space and view space
	vec3 normal = texture(uWormalMap, vTexCoord).rgb;
	normal = normal * 2.0 - 1.0;   
	normal = normalize(vTBN * normal); 

	nColor = vec4(normal, 1.0);
}

#endif
#endif