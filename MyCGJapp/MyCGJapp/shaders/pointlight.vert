#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;

// TODO nao consegui meter isto tudo num array porque acho que
// nao da com os datatypes disto 
uniform vec4 l_pos_1;
uniform vec4 l_pos_2;
uniform vec4 l_pos_3;
uniform vec4 l_pos_4;
uniform vec4 l_pos_5;
uniform vec4 l_pos_6;

uniform vec4 l_dir;

in vec4 position;
in vec4 normal;    //por causa do gerador de geometria

out Data {
	vec3 normal;
	vec3 eye;
	vec3 allLights[7];
	float lightCount;
} DataOut;

out vec3 vertex_color;

void main () {

	vec4 pos = m_viewModel * position;

	DataOut.normal = normalize(m_normal * normal.xyz);
	DataOut.allLights[0] = vec3(l_dir);

	if(l_pos_1.w == 1.0f){
		DataOut.allLights[1] = vec3(l_pos_1 - pos);
		DataOut.allLights[2] = vec3(l_pos_2 - pos);
		DataOut.allLights[3] = vec3(l_pos_3 - pos);
		DataOut.allLights[4] = vec3(l_pos_4 - pos);
		DataOut.allLights[5] = vec3(l_pos_5 - pos);
		DataOut.allLights[6] = vec3(l_pos_6 - pos);
		DataOut.lightCount = 7;
	}
	else{
		DataOut.lightCount = 1;
	}

	DataOut.eye = vec3(-pos);
	gl_Position = m_pvm * position;	

}