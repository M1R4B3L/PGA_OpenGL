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

layout(binding = 0, std140) uniform GlobalParams
{
	vec3		 uCameraPosition;
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

void main()
{
	vTexCoord	= aTexCoord;
	vPosition	= vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal		= vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir	= uCameraPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;

uniform sampler2D uTexture;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3		 uCameraPosition;
};

layout(location=0) out vec4 oColor;
layout(location=1) out vec4 nColor;
layout(location=2) out vec4 albedoColor;
layout(location=3) out vec4 depthColor;
layout(location=4) out vec4 positionColor;
layout(location=5) out vec4 specularColor;

float near = 0.1; 
float far  = 100.0; 
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main()
{
	oColor = texture(uTexture, vTexCoord);
	nColor = vec4(normalize(vNormal),1.0);
	albedoColor = oColor;
	depthColor = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far),1.0);
	positionColor = vec4(vPosition,1.0);
	specularColor = vec4(vec3(vTexCoord,0.0),1.0);
}

#endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef TEXTURE_LIGHT

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

struct Light
{
    vec3 col;
    vec3 dir;
    vec3 pos;
    unsigned int type;
	float range;
	vec3 attenuation;
};

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in vec3 aTangent;
layout(location=4) in vec3 aBitangent;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3		 uCameraPosition;
	unsigned int uLightCount;
	Light		 uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

void main()
{
	vTexCoord	= aTexCoord;
	vPosition	= vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal		= vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir	= uCameraPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

struct Light
{
    vec3 col;
    vec3 dir;
    vec3 pos;
    unsigned int type;
	float range;
	vec3 attenuation;
};

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;

uniform sampler2D uTexture;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3		 uCameraPosition;
	unsigned int uLightCount;
	Light		 uLight[16];
};

layout(location=0) out vec4 oColor;
layout(location=1) out vec4 nColor;
layout(location=2) out vec4 albedoColor;
layout(location=3) out vec4 depthColor;
layout(location=4) out vec4 positionColor;
layout(location=5) out vec4 specularColor;

float near = 0.1; 
float far  = 100.0; 
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main()
{
	oColor = texture(uTexture, vTexCoord);
	nColor = vec4(normalize(vNormal),1.0);
	albedoColor = oColor;
	depthColor = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far),1.0);
	positionColor = vec4(vPosition,1.0);
	specularColor = vec4(vec3(vTexCoord,0.0),1.0);

	vec3 objectCol = vec3(oColor.x, oColor.y, oColor.z);
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	vec3 normal = normalize(vNormal);

	vec3 lightDir;
	vec3 viewDir = normalize(vViewDir); 
	vec3 reflectDir;

	float diff;
	float spec;

	vec3 resultCol;

	for(int i = 0; i < uLightCount; ++i)
	{	
		Light currentLight = uLight[i];

	    switch (currentLight.type)
		{
			case 0:
			lightDir = normalize(currentLight.dir);
			reflectDir = reflect(-lightDir, normal);

			diff = max(dot(normal, lightDir),0.0);
			spec = pow(max(dot(viewDir, reflectDir),0.0),32);

			ambient = currentLight.col * 0.2;
			diffuse = diff * currentLight.col * 0.7;
			specular = spec * currentLight.col * 0.5;

			resultCol = (ambient + diffuse + specular) * objectCol;
			break;

			case 1:
			lightDir = normalize(currentLight.pos - vPosition);
			reflectDir = reflect(-lightDir, normal);

			float distance = length(currentLight.pos - vPosition);
			float atten = 1.0 / (currentLight.attenuation.x + currentLight.attenuation.y * distance + currentLight.attenuation.z * (distance * distance));

			diff = max(dot(normal, lightDir),0.0);
			spec = pow(max(dot(viewDir, reflectDir),0.0),32);

			ambient = currentLight.col * 0.2 * atten;
			diffuse = diff * currentLight.col * 0.7 * atten;
			specular = spec * currentLight.col * 0.5 * atten; 

			resultCol += (ambient + diffuse + specular) * objectCol;
			break;
		}	
	}

	oColor = vec4(resultCol,1.0);

}

#endif
#endif