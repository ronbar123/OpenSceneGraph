#version 430
struct LightSource
{
	vec3 position;
	vec3 color;
};

layout(location = 0) in vec4 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 color;
layout(location = 3) in vec2 fragTexCoords;

uniform sampler2D baseTexture;
layout(location = 0) uniform mat4 osg_ModelViewProjectionMatrix;
layout(location = 1) uniform mat3 osg_NormalMatrix;
layout(location = 3) uniform mat4 NormalMatrix;
layout(location = 4) uniform int numOfLightSources;
layout(location = 5) uniform mat4 osg_ViewMatrix;
layout(location = 6) uniform mat4 ModelMatrix;
layout(location = 7) uniform LightSource[2] lightSource; // consumes 2 * 2 locations


layout(location = 0) out vec4 FragColor;


void main(void)
{
	for (int i = 0; i <= numOfLightSources - 1; i++)
	{
		// ambient
		float ambientStrength = 0.001;
		vec3 ambient = ambientStrength * texture(baseTexture, fragTexCoords).rgb * lightSource[i].color;
		
		//diffuse			
		float diffuseStrength = 0.9f;
		vec4 norm = normalize(NormalMatrix * vec4(fragNormal, 1.0f));
		vec3 lightDir = normalize(lightSource[i].position - fragPos.xyz);
		float diff = max(dot(norm.xyz, lightDir), 0.0);
		vec3 diffuse = diffuseStrength * diff * lightSource[i].color * texture(baseTexture, fragTexCoords).rgb;

		vec3 finalColor = ambient + diffuse;
		FragColor += vec4(finalColor, 1.0f);

	}
}