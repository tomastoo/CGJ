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

in Data {
	vec3 normal;
	vec3 eye;
	vec3 allLights[2];
} DataIn;

void main() {

	vec4 spec = vec4(0.0);

	vec3 n = normalize(DataIn.normal);
	vec3 e = normalize(DataIn.eye);
	int i = 0;

	while(i < 2){

		vec3 l = normalize(DataIn.allLights[i]);		
		
		float intensity = max(dot(n,l), 0.0);
		if (intensity > 0.0) {
			vec3 h = normalize(l + e);
			float intSpec = max(dot(h,n), 0.0);
			spec = mat.specular * pow(intSpec, mat.shininess);
			if(i == 0){
				colorOut = max(intensity * mat.diffuse + spec, mat.ambient);
			}
			else{
				colorOut += max(intensity * mat.diffuse + spec, mat.ambient);
			}
		}
		i++;
	}
}
