#version 430

out vec4 colorOut;

struct Materials {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	vec4 emissive;
	float shininess;
	int texCount;
};

uniform Materials mat;

uniform sampler2D texmap;
uniform sampler2D texmap1;
uniform sampler2D texmap2;
uniform int texMode;

uniform vec4 sl_dir[2];
uniform float sl_cut_off_ang;

in Data {
	vec3 normal;
	vec3 eye;
	vec3 DirectionalLight;
	vec3 PointLights[6];
	vec3 SpotLights[2];
	flat int lights[2];
	vec2 tex_coord;
} DataIn;

//Dir or Point
vec2 calcLight(vec3 lightDir, vec3 n, vec3 e){
		vec4 spec = vec4(0.0);
		vec3 l = normalize(lightDir);
		float intensity = max(dot(n,l), 0.0);
		if (intensity > 0.0) {
			vec3 h = normalize(l + e);
			float intSpec = max(dot(h,n), 0.0);
			spec = mat.specular * pow(intSpec, mat.shininess);

			return vec2(intensity, spec);
		}
		else{
			return vec2(0,0);
		}
}

vec2 calcSpotLight(vec3 lightPos, vec3 n, vec3 e, vec4 sl_direction, float cutOff){
		vec4 spec = vec4(0.0);
		vec3 l = normalize(lightPos);
		vec3 s = normalize(vec3(-sl_direction));
		
		if(dot(l, s) > cutOff){
				float intensity = max(dot(n,l), 0.0);
				if (intensity > 0.0) {
					vec3 h = normalize(l + e);
					float intSpec = max(dot(h,n), 0.0);
					spec = mat.specular * pow(intSpec, mat.shininess);
					return vec2(intensity, spec);
				}
		}
		else{
			return vec2(0,0);
		}
}

void main() {
	
	vec4 texel, texel1;
	vec3 n = normalize(DataIn.normal);
	vec3 e = normalize(DataIn.eye);
	//Directional Light
	vec2 intensity_spec = calcLight(DataIn.DirectionalLight, n, e);
	//Point Lights
	if(DataIn.lights[0] == 1){
		for(int i = 0; i < 6; i++){
			intensity_spec += calcLight(DataIn.PointLights[i], n, e);
		}
	}
	// Spotlights
	if(DataIn.lights[1] == 1){
		intensity_spec += calcSpotLight(DataIn.SpotLights[0], n, e, sl_dir[0], sl_cut_off_ang);
		intensity_spec += calcSpotLight(DataIn.SpotLights[1], n, e, sl_dir[1], sl_cut_off_ang);
	}

	// Textures
	vec3 l = normalize(DataIn.DirectionalLight);
	if(texMode == 0) // modulate diffuse color with texel color
	{
		if (intensity_spec[0] > 0){
		texel = texture(texmap, DataIn.tex_coord);  // texel from lightwood.tga
		colorOut = max(intensity_spec[0]*texel + intensity_spec[1], 0.07*texel);
		}
	}
	else if (texMode == 1) // modulate diffuse color with texel1 color
	{
		if (intensity_spec[0] > 0){
		texel = texture(texmap1, DataIn.tex_coord);  // texel from road.jpg
		colorOut = max(intensity_spec[0]*texel + intensity_spec[1], 0.07*texel);
		}
	}
	else if (texMode == 2) // modulate diffuse color with texel1 color
	{
		if (intensity_spec[0] > 0){
		texel = texture(texmap2, DataIn.tex_coord);  // texel from finishline.jpg
		colorOut = max(intensity_spec[0]*texel + intensity_spec[1], 0.07*texel);
		}
	}
	else{
		if (intensity_spec[0] > 0){
			colorOut = max(intensity_spec[0]*mat.diffuse + intensity_spec[1], mat.ambient);
			}
		}
	}







