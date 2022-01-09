#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;

// POINT LIGHTS
uniform vec4 l_pos[6];

// DIRECTION LIGHTS
uniform vec4 l_dir;

// SPOTLIGHTS

uniform vec4 sl_pos[2];


in vec4 position;
in vec4 normal;    //por causa do gerador de geometria

out Data {
	vec3 normal;
	vec3 eye;
	vec3 DirectionalLight;
	vec3 PointLights[6];
	vec3 SpotLights[2];
	flat int lights[2];
} DataOut;

out vec3 vertex_color;

void main () {

	vec4 pos = m_viewModel * position;

	DataOut.normal = normalize(m_normal * normal.xyz);
	DataOut.DirectionalLight = vec3(l_dir);

	if(l_pos[0].w == 1.0f){

		for(int i = 0; i < 6; i++){
			DataOut.PointLights[i] = vec3(l_pos[i] - pos);
		}	
		DataOut.lights[0] = 1;
	}
	else{
		DataOut.lights[0] = 0;
	}

	if(sl_pos[0].w == 1.0f){
		for(int i = 0; i < 2; i++){
			DataOut.SpotLights[i] = vec3(sl_pos[i] - pos);
		}	
		DataOut.lights[1] = 1;
	}
	else{
		DataOut.lights[1] = 0;
	}

	DataOut.eye = vec3(-pos);
	gl_Position = m_pvm * position;	

}