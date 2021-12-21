#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;

uniform vec4 l_pos;
uniform vec4 l_dir;

in vec4 position;
//in vec4 direction;
in vec4 normal;    //por causa do gerador de geometria

out Data {
	vec3 normal;
	vec3 eye;
	vec3 allLights[2];
} DataOut;

out vec3 vertex_color;

void main () {

	vec4 pos = m_viewModel * position;

	DataOut.normal = normalize(m_normal * normal.xyz);
	DataOut.allLights[0] = vec3(l_dir);
	DataOut.allLights[1] = vec3(l_pos - pos);

	DataOut.eye = vec3(-pos);


	gl_Position = m_pvm * position;	

}