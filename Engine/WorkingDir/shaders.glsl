///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

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

#ifdef TEXTURED_PATRICIO

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

struct Light
{
    vec3 col;
    vec3 dir;
    vec3 pos;
    unsigned int type;
	float range;
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
	nColor = vec4(vNormal,1.0);
	albedoColor = oColor;
	depthColor = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far),1.0);

	for(int i = 0; i < uLightCount; ++i) {

		Light currentLight = uLight[i];
		float distance = length(currentLight.pos - vPosition);
		vec3  lDir = normalize(currentLight.pos - vPosition);
		float intensity;
	
		if(currentLight.type == 0) 
		{
			intensity = dot(vNormal, currentLight.dir);
			oColor += vec4(currentLight.col * intensity, 1.0);
		}
		else if(currentLight.type == 1)
		{
			float cons = currentLight.dir.x;
			float linear = currentLight.dir.y;
			float quadratic = currentLight.dir.z;
			float attenuation = 1.0 / (cons + linear * distance + quadratic * (distance * distance));
			intensity = dot(vNormal, lDir) * attenuation;
			oColor += vec4(currentLight.col * intensity, 1.0);
		}
		else if(currentLight.type == 2)
		{
			float cutOff = cos(radians(20.0));
			float theta = dot(lDir,normalize(currentLight.dir));

			if(theta > cutOff)
			{
				float attenuation = 1.0 - (1.0 - theta) / (1.0 - cutOff);
				intensity = dot(vNormal, lDir) * attenuation;
				oColor += vec4(currentLight.col * intensity, 1.0);
			}
		}

	}

}

#endif
#endif