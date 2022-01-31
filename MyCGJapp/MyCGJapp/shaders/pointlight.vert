#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;
uniform mat4 m_Model;   //por causa do cubo para a skybox


// POINT LIGHTS
uniform vec4 l_pos[6];

// DIRECTION LIGHTS
uniform vec4 l_dir;

// SPOTLIGHTS

uniform vec4 sl_pos[2];

// TEXTURE SPOTLIGHT

uniform vec4 sl_pos_texture;


in vec4 position;
in vec4 normal;    //por causa do gerador de geometria
in vec4 texCoord;

out Data {
	vec3 normal;
	vec3 eye;
	vec3 DirectionalLight;
	vec3 PointLights[6];
	vec3 SpotLights[2];
	vec3 TextureSpotlight;
	flat int lights[3];
	vec2 tex_coord;
	vec3 skyboxTexCoord;

} DataOut;

out vec3 vertex_color;

void main () {

	vec4 pos = m_viewModel * position;

	DataOut.skyboxTexCoord = vec3(m_Model * position);	//Transformação de modelação do cubo unitário 
	DataOut.skyboxTexCoord.x = - DataOut.skyboxTexCoord.x; //Texturas mapeadas no interior logo negar a coordenada x
	DataOut.tex_coord = texCoord.st;

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

	if(sl_pos_texture.w == 1.0f){

		DataOut.TextureSpotlight = vec3(sl_pos_texture - pos);
		DataOut.lights[2] = 1;
	}
	else{
		DataOut.lights[2] = 0;
	}

	DataOut.eye = vec3(-pos);
	DataOut.tex_coord = texCoord.st;
	gl_Position = m_pvm * position;	

}