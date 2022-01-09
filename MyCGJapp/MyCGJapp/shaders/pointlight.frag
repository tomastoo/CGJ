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

uniform vec4 sl_dir[2];
uniform float sl_cut_off_ang;

in Data {
	vec3 normal;
	vec3 eye;
	vec3 DirectionalLight;
	vec3 PointLights[6];
	vec3 SpotLights[2];
	flat int lights[2];
} DataIn;

//Dir or Point
vec4 calcLight(vec3 lightDir, vec3 n, vec3 e){
		vec4 spec = vec4(0.0);
		vec3 l = normalize(lightDir);
		float intensity = max(dot(n,l), 0.0);
		if (intensity > 0.0) {
			vec3 h = normalize(l + e);
			float intSpec = max(dot(h,n), 0.0);
			spec = mat.specular * pow(intSpec, mat.shininess);

			return max(intensity * mat.diffuse + spec, mat.ambient);
		}
}

vec4 calcSpotLight(vec3 lightPos, vec3 n, vec3 e, vec4 sl_direction, float cutOff){
		vec4 spec = vec4(0.0);
		vec3 l = normalize(lightPos);
		vec3 s = normalize(vec3(-sl_direction));
		
		if(dot(l, s) > cutOff){
				float intensity = max(dot(n,l), 0.0);
				if (intensity > 0.0) {
					vec3 h = normalize(l + e);
					float intSpec = max(dot(h,n), 0.0);
					spec = mat.specular * pow(intSpec, mat.shininess);
					return max(intensity * mat.diffuse + spec, mat.ambient);
				}
		}
}

void main() {
	vec3 n = normalize(DataIn.normal);
	vec3 e = normalize(DataIn.eye);
	//Directional Light
	colorOut = calcLight(DataIn.DirectionalLight, n, e);
	//Point Lights
	if(DataIn.lights[0] == 1){
		for(int i = 0; i < 6; i++){
			colorOut += calcLight(DataIn.PointLights[i], n, e);
		}
	}
	// Spotlights
	if(DataIn.lights[1] == 1){
		colorOut += calcSpotLight(DataIn.SpotLights[0], n, e, sl_dir[0], sl_cut_off_ang);
		colorOut += calcSpotLight(DataIn.SpotLights[1], n, e, sl_dir[1], sl_cut_off_ang);
	}
}





