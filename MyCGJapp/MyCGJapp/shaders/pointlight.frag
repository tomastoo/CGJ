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
uniform sampler2D texmap3;
uniform sampler2D texmap4;
uniform samplerCube cubeMap;
uniform sampler2D texmap6;
uniform sampler2D texmap7;

uniform int texMode;
uniform bool shadowMode;
uniform uint diffMapCount;

uniform vec4 sl_dir[2];
uniform float sl_cut_off_ang;


uniform vec4 sl_dir_texture;
uniform float sl_cut_off_ang_texture;

uniform int is_fog_on;
uniform float fog_maxdist;

in Data {
	vec3 normal;
	vec3 eye;
	vec3 DirectionalLight;
	vec3 PointLights[6];
	vec3 SpotLights[2];
	vec3 TextureSpotlight;
	flat int lights[3];
	vec2 tex_coord;
	vec3 skyboxTexCoord;
} DataIn;

vec4 calcFog(vec3 position){
	//float fog_maxdist = 100.0;
	float fog_mindist = 0.1;
	vec4  fog_colour = vec4(0.4, 0.4, 0.4, 1.0);

	vec4 color  = colorOut.xyzw;
	
	// Calculate fog
	float dist = length(position.xyz);
	float fog_factor = (fog_maxdist - dist) /
					  (fog_maxdist - fog_mindist);
	fog_factor = clamp(fog_factor, 0.0, 1.0);

	return mix(fog_colour, color, fog_factor);
}

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
vec4 diff;

void main() {
	
	vec4 texel, texel1;
	vec3 n;
	normalize(DataIn.normal);

	
	if(texMode == 6)  // lookup normal from normal map, move from [0,1] to [-1, 1] range, normalize
		n = normalize(2.0 * texture(texmap6, DataIn.tex_coord).rgb - 1.0);
	else
		n = normalize(DataIn.normal);

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

	// Spotlight texture
	if(DataIn.lights[2] == 1){
		intensity_spec += calcSpotLight(DataIn.TextureSpotlight, n, e, sl_dir_texture, sl_cut_off_ang_texture);
	}

	// Textures
	if(shadowMode)  //constant color
		colorOut = vec4(0.5, 0.5, 0.5, 1.0);
	else{
		//if(mat.texCount == 0){
		if(0==0){
			if(texMode == 0) // modulate diffuse color with texel color
			{
				if (intensity_spec[0] > 0){
				texel = texture(texmap, DataIn.tex_coord);  // texel from lightwood.tga
				colorOut = max(intensity_spec[0]*texel + intensity_spec[1], 0.07*texel) + vec4(0.1, 0.1, 0.1, 0);
				}
			}
			else if (texMode == 1) // modulate diffuse color with texel1 color
			{
				if (intensity_spec[0] > 0){
				texel = texture(texmap1, DataIn.tex_coord);  // texel from road.jpg
				colorOut = max(intensity_spec[0]*texel + intensity_spec[1], 0.07*texel)+ vec4(0.1, 0.1, 0.1, 0);
				}
			}
			else if (texMode == 2) // modulate diffuse color with texel2 color
			{
				if (intensity_spec[0] > 0){
				texel = texture(texmap2, DataIn.tex_coord);  // texel from finishline.jpg
				colorOut = max(intensity_spec[0]*texel + intensity_spec[1], 0.07*texel)+ vec4(0.1, 0.1, 0.1, 0);
				}
			}
			else if (texMode == 3) 	// modulated texture for particle (texel3 color)
			{
				texel = texture(texmap3, DataIn.tex_coord);    //texel from particle.tga

				if((texel.a == 0.0) || (mat.diffuse.a == 0.0) ) discard;

				else colorOut = mat.diffuse * texel;
				}

			else if (texMode == 4) 	// modulate color with texel4 color
			{
				texel = texture(texmap4, DataIn.tex_coord);    //texel from tree.tga

				if (texel.a == 0.0) discard;

				else 
					colorOut = vec4(max(intensity_spec[0]*texel.rgb + intensity_spec[1], 0.1*texel.rgb), texel.a);
				}

			else if (texMode == 5) //SkyBox
			{
				texel = texture(cubeMap, DataIn.skyboxTexCoord);

				if (texel.a == 0.0) discard;

				else 
					colorOut = texel;
				}
			else if (texMode == 6) // modulate diffuse color with texel1 color
			{
				if (intensity_spec[0] > 0){
					colorOut = max(intensity_spec[0]*mat.diffuse + intensity_spec[1], mat.ambient);// + vec4(0.1, 0.1, 0.1, 0);
				}
			}
			else if (texMode == 7) // modulate diffuse color with texel1 color
			{
				if (intensity_spec[0] > 0){
				texel = texture(texmap7, DataIn.tex_coord);  // texel from stone.tga
				colorOut = vec4((max(intensity_spec[0]*texel + intensity_spec[1], 0.2*texel)).rgb, 1.0);
				}
			}
			else{
				if(diffMapCount == 0){
			diff = mat.diffuse;
			if (intensity_spec[0] > 0.0) {
				colorOut = vec4((max(intensity_spec[0] * diff, diff*0.15) + intensity_spec[1]).rgb, 1.0);
		
			}
			}
			else{if (intensity_spec[0] > 0){
					colorOut = max(intensity_spec[0]*mat.diffuse + intensity_spec[1], mat.ambient) + vec4(0.1, 0.1, 0.1, 0);
				}}
			}

			if(is_fog_on == 1){
				colorOut = calcFog(DataIn.eye);
			}
		}
		else{
			if(diffMapCount == 0)
			diff = mat.diffuse;
			if (intensity_spec[0] > 0.0) {
				colorOut = vec4((max(intensity_spec[0] * diff, diff*0.15) + intensity_spec[1]).rgb, 1.0);
		
			}
			
		}
		
	}
	
}