//
// CGJ: Phong Shading and Text rendered with FreeType library
// The text rendering was based on https://learnopengl.com/In-Practice/Text-Rendering
// This demo was built for learning purposes only.
// Some code could be severely optimised, but I tried to
// keep as simple and clear as possible.
//
// The code comes with no warranties, use it at your own risk.
// You may use it, or parts of it, wherever you want.
// 
// Author: Jo�o Madeiras Pereira
//

#include <math.h>
#include <iostream>
#include <sstream>
#include <string>

// include GLEW to access OpenGL 3.3 functions
#include <GL/glew.h>


// GLUT is the toolkit to interface with the OS
#include <GL/freeglut.h>

#include <IL/il.h>


// Use Very Simple Libs
#include "VSShaderlib.h"
#include "AVTmathLib.h"
#include "VertexAttrDef.h"
#include "geometry.h"

#include "avtFreeType.h"

using namespace std;

#define CAPTION "CGJ Demo: Phong Shading and Text rendered with FreeType"
int WindowHandle = 0;
int WinX = 1024, WinY = 768;

unsigned int FrameCount = 0;
int cam = 3;
//shaders
VSShaderLib shader;  //geometry
VSShaderLib shaderGlobal;  //geometry directional light
VSShaderLib shaderText;  //render bitmap text

//File with the font
const string font_name = "fonts/arial.ttf";

//Vector with meshes
vector<struct MyMesh> myMeshes;

//External array storage defined in AVTmathLib.cpp

/// The storage for matrices
extern float mMatrix[COUNT_MATRICES][16];
extern float mCompMatrix[COUNT_COMPUTED_MATRICES][16];

/// The normal matrix
extern float mNormal3x3[9];

GLint pvm_uniformId;
GLint vm_uniformId;
GLint normal_uniformId;
GLint lPos_uniformId[6];
GLint lDir_uniformId;
GLint tex_loc, tex_loc1, tex_loc2;
	
// Camera Position
float camX, camY, camZ;

//float lookAtX, lookAtY, lookAtZ = 0.0f;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = -90.0f, beta = 0.0f;

float r = 10.0f;

// Frame counting and FPS computation
long myTime,timebase = 0,frame = 0;
char s[32];
float tableX = 100;
float tableY = 0.5;
float tableZ = 100;

float lightDir[4] = { tableX / 2, tableY , tableZ / 2, 0.0f };
float nolightDir[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

float pointLightsHight = 2.f;

const float plGrid[2] = { 4,3 }; // This means that the point lights will be devided 
						// |_|_|_|_| like this so the first value is the number of 
						// |_|_|_|_| collumns and the second is the number of rows 
						// |_|_|_|_| (x,z)
					

float lightPos[6][4] = {{tableX / plGrid[0], tableY + pointLightsHight, tableZ / plGrid[1], 0.0f},
						{(tableX / plGrid[0]) * 2, tableY + pointLightsHight, (tableZ / plGrid[1]), 0.0f},
						{(tableX / plGrid[0]) * 3, tableY + pointLightsHight, (tableZ / plGrid[1]), 0.0f}, 
						{(tableX / plGrid[0]), tableY + pointLightsHight, (tableZ / plGrid[1]) * 2, 0.0f}, 
						{(tableX / plGrid[0]) * 2, tableY + pointLightsHight, (tableZ / plGrid[1]) * 2, 0.0f},
						{(tableX / plGrid[0]) * 3, tableY + pointLightsHight, (tableZ / plGrid[1]) * 2, 0.0f}, };

float* directionalLight = nolightDir;


bool isDirectionalLightOn = false;
bool isPointLightsOn = false;
bool isSpotLightsOn = false;




class Car {
	public:
		float position[3] = { 0.0f, 0.0f, 0.0f };
		float velocity = 0.01f;
		float direction[3] = { 1.0f, 0.0f, 0.0f };

		Car() {};
		void move() {
			position[0] += direction[0] * velocity;
			position[1] += direction[1] * velocity;
			position[2] += direction[2] * velocity;
		};
		void setVelocitity(float velocityNew) {
			velocity = velocityNew;
		}
};

class Orange{
public:
	const float velocityIncrease = 0.008f;
	const float veloctityIntervalIncrease = 0.001;
	float maxVelocity = 0.1f;
	float minVelocity = 0.01f;
	float position[3] = { 0.0f, 1.5f, 0.0f };
	float velocity;
	float direction[3] = { 1.0f, 0.0f, 0.0f };
	float rotationAngle = 0;

	Orange() {
		// set position random
		reset();
	};
	
	void move() {
		position[0] += direction[0] * velocity;
		position[1] += direction[1] * velocity;
		position[2] += direction[2] * velocity;
	};

	void roll() {
		rotationAngle = velocity * 10;
	}

	//given a certain table size 
	void randomPosition() {
		position[0] = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / tableX));
		position[2] = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / tableZ));
	}

	bool isOnTable() {
		if (position[0] > tableX || position[0] < 0) 
			return false;
		
		if (position[2] > tableZ || position[0] < 0)
			return false;

		return true;
	}

	void randomDirection() {
		direction[0] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		direction[2] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	}

	void randomVelocity() {
		velocity = minVelocity + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (maxVelocity - minVelocity)));
	}

	void reset() {
		randomPosition();
		randomDirection();
		randomVelocity();
	}

	void accelerate() {
		//new max and min velocity
		minVelocity += veloctityIntervalIncrease;
		maxVelocity += veloctityIntervalIncrease;

		//acceleration part
		velocity += velocityIncrease;
		if (velocity > maxVelocity) {
			velocity = maxVelocity;
		}
	}
};

Car car;
Orange orange;

float lookAtX = car.position[0];
float lookAtY = car.position[1];
float lookAtZ = car.position[2];


void timer(int value)
{
	std::ostringstream oss;
	oss << CAPTION << ": " << FrameCount << " FPS @ (" << WinX << "x" << WinY << ")";
	std::string s = oss.str();
	glutSetWindow(WindowHandle);
	glutSetWindowTitle(s.c_str());
    FrameCount = 0;
    glutTimerFunc(1000, timer, 0);
}

void refresh(int value)
{
	//PUT YOUR CODE HERE
	glutTimerFunc(1000/60, refresh, 0);
	glutPostRedisplay();
}

// ------------------------------------------------------------
//
// Reshape Callback Function
//

void changeSize(int w, int h) {

	float ratio;
	// Prevent a divide by zero, when window is too short
	if(h == 0)
		h = 1;
	// set the viewport to be the entire window
	glViewport(0, 0, w, h);
	// set the projection matrix
	ratio = (1.0f * w) / h;
	loadIdentity(PROJECTION);
	perspective(53.13f, ratio, 0.1f, 1000.0f);
}

void cam1() {
	// Ortogonal Projection Top View Cam
	// set the camera position based on its spherical coordinates
	camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f) + 50;
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f) + 50;
	camY = 110;

	lookAtX = 50.0f;
	lookAtY = 0.0f;
	lookAtZ = 50.0f;
	cam = 1;
};

void cam2() {
	//Prespective Top View Cam

	camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f) + 50;
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f) + 50;
	camY = 110;

	lookAtX = 50.0f;
	lookAtY = 0.0f;
	lookAtZ = 50.0f;
	//perspective(1.0f, 1.0f, -1, 1);
	cam = 2;

};

void cam3() {
	// prespective Car Cam
	// this values rotate the cam in x z WC
	//inclination 'i' of camera from above
	//  cam
	//      \
	//       \
	//    _ _ i\
	//          object;

	float inclination = 55.f;
	float hight = 10;
	camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f) + car.position[0];
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f) + car.position[2];
	camY = r * sin(inclination * 3.14f / 180.0f) + hight;

	lookAtX = car.position[0];
	lookAtY = car.position[1];
	lookAtZ = car.position[2];
	cam = 3;
};

// ------------------------------------------------ ------------
//
// Render stufff
//

void renderScene(void) {

	GLint loc;
	//printf("i am always being called biacht\n");
	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// load identity matrices
	loadIdentity(VIEW);
	loadIdentity(MODEL);
	// set the camera using a function similar to gluLookAt

	lookAt(camX, camY, camZ, lookAtX, lookAtY, lookAtZ, 0,1,0);
	
	
	// use our shader
	glUseProgram(shader.getProgramIndex());

		//send the light position in eye coordinates
		//glUniform4fv(lPos_uniformId, 1, lightPos); //efeito capacete do mineiro, ou seja lighPos foi definido em eye coord 

		float res[6][4];
		float res2[4];

			for (int i = 0; i < 6; i++) {
				printf("Point light = { %.1f, %.1f, %.1f, %.1f}\n", lightPos[i][0], lightPos[i][1], lightPos[i][2], lightPos[i][3]);
				multMatrixPoint(VIEW, lightPos[i], res[i]);   //lightPos definido em World Coord so is converted to eye space
				printf("WORLD COORDS Point light = { %.1f, %.1f, %.1f, %.1f}\n", res[i][0], res[i][1], res[i][2], res[i][3]);

				glUniform4fv(lPos_uniformId[i], 1, res[i]);

			}
			printf("\n\n");		
		//printf("Directional light = { %.1f, %.1f, %.1f, %.1f}", directionalLight[0], directionalLight[1], directionalLight[2], directionalLight[3]);
		
		multMatrixPoint(VIEW, directionalLight, res2);   //lightPos definido em World Coord so is converted to eye space
		printf("WORLD COORDS DIRECTIONAL light = { %.1f, %.1f, %.1f, %.1f}\n", res2[0], res2[1], res2[2], res2[3]);
		glUniform4fv(lDir_uniformId, 1, res2);
		
		
	int objId=0; //id of the object mesh - to be used as index of mesh: Mymeshes[objID] means the current mesh

	for (int i = 0 ; i < 8; ++i) {
//		for (int j = 0; j < 2; ++j) {

			// send the material
			loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
			glUniform4fv(loc, 1, myMeshes[objId].mat.ambient);
			loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
			glUniform4fv(loc, 1, myMeshes[objId].mat.diffuse);
			loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
			glUniform4fv(loc, 1, myMeshes[objId].mat.specular);
			loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
			glUniform1f(loc, myMeshes[objId].mat.shininess);
			pushMatrix(MODEL);
			
			// TODO THIS IS SHIT I WANT THIS WHEN DEFINING THE VO in the init() func
			// Values correspond to 0,0 on the table coords = wc
			float torusY = 1.0f; //z in world coords
			float carBodyX = 1.5f;
			float carBodyY = 3.0f;
			float jointCarGap = -0.5f;
			
			car.move();
			float* position = car.position;
			
			switch (objId) {
			case 0:
				//table
				translate(MODEL, 0.0f, 0.0f, 0.0f);
				scale(MODEL, tableX, tableY, tableZ);
				
				break;
			case 1:
				//road
				translate(MODEL, 0.0f, 0.1f, 40.0f);
				scale(MODEL, 100, 0.5, 5);
				break;
			case 2:
				//orange
				orange.move();
				if (!orange.isOnTable()) {
					orange.reset();
				}
				translate(MODEL, orange.position[0], orange.position[1], orange.position[2]);
				orange.roll();
				rotate(MODEL, orange.rotationAngle, orange.direction[0], orange.direction[1], orange.direction[2]);
				orange.accelerate();
				break;
			case 3:
				//car wheel torus RIGHT TOP
				translate(MODEL, position[0] + carBodyY + jointCarGap, position[1] + torusY, position[2] + carBodyX);
				rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);
				break;
			case 4:
				//car wheel torus RIGHT BOTTOM
				translate(MODEL, position[0] + carBodyY + jointCarGap, position[1] + torusY, position[2]);
				rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);
				break;
			case 5:
				//car wheel torus LEFT TOP
				translate(MODEL, position[0] - jointCarGap, position[1] + torusY, position[2] + carBodyX);
				rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);
				//rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);

				break;
			case 6:
				//car wheel torus LEFT BOTTOM
				translate(MODEL, position[0] + 0.0f - jointCarGap, position[1] + torusY, position[2]);
				rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);

				break;
			case 7:
				//car body
				translate(MODEL, position[0], position[1] + torusY - 0.2f, position[2]);
				scale(MODEL, carBodyY, 0.5, carBodyX);
				break;
			}

			// send matrices to OGL
			computeDerivedMatrix(PROJ_VIEW_MODEL);
			glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
			glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
			computeNormalMatrix3x3();
			glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

			// Render mesh
			glBindVertexArray(myMeshes[objId].vao);
			
			if (!shader.isProgramValid()) {
				printf("Program Not Valid!\n");
				exit(1);	
			}
			glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);

			popMatrix(MODEL);
			objId++;
		//}
	}

	//Render text (bitmap fonts) in screen coordinates. So use ortoghonal projection with viewport coordinates.
	glDisable(GL_DEPTH_TEST);
	//the glyph contains background colors and non-transparent for the actual character pixels. So we use the blending
	glEnable(GL_BLEND);  
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	//viewer at origin looking down at  negative z direction
	pushMatrix(MODEL);
	loadIdentity(MODEL);
	pushMatrix(PROJECTION);
	loadIdentity(PROJECTION);
	pushMatrix(VIEW);
	loadIdentity(VIEW);

	switch (cam)
	{
	case 1:
		ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
		//printf("cam1");
		break;
	case 2:
		perspective(120.0f, 1.33f, 15.0, 120.0);
		//printf("cam2");
		break;
	case 3:
		perspective(120.0f, 1.33f, 15.0, 120.0);
		cam3();
		//printf("cam3");
		break;
	default:
		break;
	}
	//RenderText(shaderText, "This is a sample text", 25.0f, 25.0f, 1.0f, 0.5f, 0.8f, 0.2f);
	//RenderText(shaderText, "CGJ Light and Text Rendering Demo", 440.0f, 570.0f, 0.5f, 0.3, 0.7f, 0.9f);
	popMatrix(PROJECTION);
	popMatrix(VIEW);
	popMatrix(MODEL);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glutSwapBuffers();
}

// ------------------------------------------------------------
//
// Events from the Keyboard
//

void processKeys(unsigned char key, int xx, int yy)
{
	switch(key) {
		case 27:
			glutLeaveMainLoop();
			break;
		case 'c': // POINTING LIGHTS --> LUZES PARA ILUMINAR A MESA
			//printf("Camera Spherical Coordinates (%f, %f, %f)\n", alpha, beta, r);
			if (isPointLightsOn) {
				for (int i = 0; i < 6; i++) {
					lightPos[i][3] = 0.0f;
				}
				isPointLightsOn = false;
			}
			else {
				for (int i = 0; i < 6; i++) {
					lightPos[i][3] = 1.0f;
				}
				isPointLightsOn = true;
			}
			break;
		case 'm': glEnable(GL_MULTISAMPLE); break;
		case 'n': /*glDisable(GL_MULTISAMPLE);*/
			// DIRECTIONAL LIGHT --> ILUMINACAO GERAL TIPO DIA E NOITE
			if (isDirectionalLightOn) {
				directionalLight = nolightDir;
				isDirectionalLightOn = false;
			}
			else {
				directionalLight = lightDir;
				isDirectionalLightOn = true;
			}
			break;
		case 'h':
			// SPOTLIGHTS --> ILUMINACAO COMO SE FOSSE AS LUZES FRONTEIRAS DO CARRO
			if (isSpotLightsOn) {
				directionalLight = nolightDir;
				isSpotLightsOn = false;
			}
			else {
				directionalLight = lightDir;
				isSpotLightsOn = true;
			}
			break;
		case '1': 
			alpha = 0.0f;
			beta = 90.0f;
			cout << "tecla carregada = " << key;
			cam1();
			break;
		case '2': 
			alpha = 0.0f;
			beta = 90.0f;
			cout << "tecla carregada = " << key;
			cam2();
			break;
		case '3': 
			cout << "tecla carregada = " << key;
			//ESTA ATRIBUICAO DE VALORES E FEITA AQUI POR CAUSA TO MOVIMENTO DE CAMERA ATRAVES DO RATO
			alpha = -90.0f, beta = 0.0f;
			cam3();

			break;
		default:
			cout << "tecla carregada = " << key;
			break;
	}
}




// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{
	// start tracking the mouse
	if (state == GLUT_DOWN)  {
		startX = xx;
		startY = yy;
		if (button == GLUT_LEFT_BUTTON)
			tracking = 1;
		else if (button == GLUT_RIGHT_BUTTON)
			tracking = 2;
	}

	//stop tracking the mouse
	else if (state == GLUT_UP) {
		if (tracking == 1) {
			alpha -= (xx - startX);
			beta += (yy - startY);
		}
		else if (tracking == 2) {
	/*		r += (yy - startY) * 0.01f;
			if (r < 0.1f)
				r = 0.1f;
	*/	}
		tracking = 0;
	}
}

// Track mouse motion while buttons are pressed

void processMouseMotion(int xx, int yy)
{

	int deltaX, deltaY;
	float alphaAux, betaAux;
	float rAux;
	float slowDown = 0.2f;
	deltaX =  (- xx + startX) * slowDown;
	deltaY =  (  yy - startY) * slowDown;

	// left mouse button: move camera
	if (tracking == 1) {


		alphaAux = alpha + deltaX;
		betaAux = beta + deltaY;

		if (betaAux > 85.0f)
			betaAux = 85.0f;
		else if (betaAux < -85.0f)
			betaAux = -85.0f;
		rAux = r;
	}
	// right mouse button: zoom
	else if (tracking == 2) {

		alphaAux = alpha;
		betaAux = beta;
		rAux = r + (deltaY * 0.01f);
		if (rAux < 0.1f)
			rAux = 0.1f;
	}
	
	switch (cam) {
	case 1:
		alpha = alphaAux;
		beta = betaAux;
		cam1();
		break;
	case 2:
		alpha = alphaAux;
		beta = betaAux;
		cam2();
		break;
	case 3:
		//float inclination = 55.f;
		//float hight = 10;
		alpha = alphaAux;
		beta = betaAux;
		cam3();
		break;
	}
	//camX = rAux * sin(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	//camZ = rAux * cos(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	//camY = rAux *   						       sin(betaAux * 3.14f / 180.0f);

//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}


void mouseWheel(int wheel, int direction, int x, int y) {

	r += direction * 0.1f;
	if (r < 0.1f)
		r = 0.1f;

	camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camY = r *   						     sin(beta * 3.14f / 180.0f);

//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}

// --------------------------------------------------------
//
// Shader Stuff
//


GLuint setupShaders() {

	// Shader for models
	shader.init();
	shader.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/pointlight.vert");
	shader.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/pointlight.frag");

	// set semantics for the shader variables
	glBindFragDataLocation(shader.getProgramIndex(), 0,"colorOut");
	glBindAttribLocation(shader.getProgramIndex(), VERTEX_COORD_ATTRIB, "position");
	glBindAttribLocation(shader.getProgramIndex(), NORMAL_ATTRIB, "normal");
	//glBindAttribLocation(shader.getProgramIndex(), TEXTURE_COORD_ATTRIB, "texCoord");

	glLinkProgram(shader.getProgramIndex());

	pvm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_pvm");
	vm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_viewModel");
	normal_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_normal");

	lPos_uniformId[0] = glGetUniformLocation(shader.getProgramIndex(), "l_pos_1");
	lPos_uniformId[1] = glGetUniformLocation(shader.getProgramIndex(), "l_pos_2");
	lPos_uniformId[2] = glGetUniformLocation(shader.getProgramIndex(), "l_pos_3");
	lPos_uniformId[3] = glGetUniformLocation(shader.getProgramIndex(), "l_pos_4");
	lPos_uniformId[4] = glGetUniformLocation(shader.getProgramIndex(), "l_pos_5");
	lPos_uniformId[5] = glGetUniformLocation(shader.getProgramIndex(), "l_pos_6");

	lDir_uniformId = glGetUniformLocation(shader.getProgramIndex(), "l_dir");
	tex_loc = glGetUniformLocation(shader.getProgramIndex(), "texmap");
	tex_loc1 = glGetUniformLocation(shader.getProgramIndex(), "texmap1");
	tex_loc2 = glGetUniformLocation(shader.getProgramIndex(), "texmap2");
	
	printf("InfoLog for Per Fragment Phong Lightning Shader\n%s\n\n", shader.getAllInfoLogs().c_str());

	// Shader for bitmap Text
	shaderText.init();
	shaderText.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/text.vert");
	shaderText.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/text.frag");

	glLinkProgram(shaderText.getProgramIndex());
	printf("InfoLog for Text Rendering Shader\n%s\n\n", shaderText.getAllInfoLogs().c_str());
	
	//// Shader for models
	//shaderGlobal.init();
	//shaderGlobal.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/pointlight.vert");
	//shaderGlobal.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/pointlight.frag");

	//// set semantics for the shader variables
	//glBindFragDataLocation(shaderGlobal.getProgramIndex(), 0, "colorOut");
	//glBindAttribLocation(shaderGlobal.getProgramIndex(), VERTEX_COORD_ATTRIB, "position");
	//glBindAttribLocation(shaderGlobal.getProgramIndex(), NORMAL_ATTRIB, "normal");
	////glBindAttribLocation(shader.getProgramIndex(), TEXTURE_COORD_ATTRIB, "texCoord");

	//glLinkProgram(shaderGlobal.getProgramIndex());

	//pvm_uniformId = glGetUniformLocation(shaderGlobal.getProgramIndex(), "m_pvm");
	//vm_uniformId = glGetUniformLocation(shaderGlobal.getProgramIndex(), "m_viewModel");
	//normal_uniformId = glGetUniformLocation(shaderGlobal.getProgramIndex(), "m_normal");
	//lDir_uniformId = glGetUniformLocation(shaderGlobal.getProgramIndex(), "l_dir");
	//tex_loc = glGetUniformLocation(shaderGlobal.getProgramIndex(), "texmap");
	//tex_loc1 = glGetUniformLocation(shaderGlobal.getProgramIndex(), "texmap1");
	//tex_loc2 = glGetUniformLocation(shaderGlobal.getProgramIndex(), "texmap2");

	return(shader.isProgramLinked() && shaderText.isProgramLinked() /*&& shaderGlobal.isProgramLinked()*/);
}

// ------------------------------------------------------------
//
// Model loading and OpenGL setup
//

void init()
{
	srand(static_cast <unsigned> (time(0)));

	MyMesh amesh;

	/* Initialization of DevIL */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		exit(0);
	}
	ilInit();

	/// Initialization of freetype library with font_name file
	freeType_init(font_name);

	// set the camera position based on its spherical coordinates
	camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camY = r *   						     sin(beta * 3.14f / 180.0f);

	
	float amb[]= {0.2f, 0.15f, 0.1f, 1.0f};
	float diff[] = {0.8f, 0.6f, 0.4f, 1.0f};
	float spec[] = {0.8f, 0.8f, 0.8f, 1.0f};
	float emissive[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float shininess= 100.0f;
	int texcount = 0;

	//// create geometry and VAO of the pawn
	//amesh = createPawn();
	//memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	//memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	//memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	//memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	//amesh.mat.shininess = shininess;
	//amesh.mat.texCount = texcount;
	//myMeshes.push_back(amesh);

	//float amb1[]= {0.3f, 0.0f, 1.0f, 0.0f};
	//float diff1[] = {0.8f, 0.1f, 0.1f, 1.0f};
	//float spec1[] = {0.9f, 0.9f, 0.9f, 1.0f};
	//shininess=100.0;

	//// create geometry and VAO of the cylinder
	//amesh = createCylinder(1.5f, 0.5f, 20);
	//memcpy(amesh.mat.ambient, amb1, 4 * sizeof(float));
	//memcpy(amesh.mat.diffuse, diff1, 4 * sizeof(float));
	//memcpy(amesh.mat.specular, spec1, 4 * sizeof(float));
	//memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	//amesh.mat.shininess = shininess;
	//amesh.mat.texCount = texcount;
	//myMeshes.push_back(amesh);



	//// create geometry and VAO of the 
	//amesh = createCone(1.5f, 0.5f, 20);
	//memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	//memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	//memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	//memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	//amesh.mat.shininess = shininess;
	//amesh.mat.texCount = texcount;
	//myMeshes.push_back(amesh);

	// create geometry and VAO of the cube
	// TABLE
	amesh = createCube();
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);


	float amb1[] = { 0.3f, 0.0f, 0.0f, 1.0f };
	float diff1[] = { 0.8f, 0.1f, 0.1f, 1.0f };
	float spec1[] = { 0.0f, 0.9f, 0.9f, 1.0f };
	shininess = 200.0;

	// ROAD
	amesh = createCube();
	memcpy(amesh.mat.ambient, amb1, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff1, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec1, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);


	float amb2[] = { 1.0f, 0.647f, 0.0f, 1.0f };
	// ORANGE
	amesh = createSphere(1.0f, 20);
	memcpy(amesh.mat.ambient, amb2, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);

	// Car
	float amb3[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float diff2[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float spec2[] = { 0.9f, 0.9f, 0.9f, 1.0f };
	for (int i = 0; i < 4; i++) {
		amesh = createTorus(0.1f, 0.5f, 20, 20);
		memcpy(amesh.mat.ambient, amb3, 4 * sizeof(float));
		memcpy(amesh.mat.diffuse, diff2, 4 * sizeof(float));
		memcpy(amesh.mat.specular, spec2, 4 * sizeof(float));
		memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
		amesh.mat.shininess = shininess;
		amesh.mat.texCount = texcount;
		myMeshes.push_back(amesh);
	}
	float amb4[] = { 0.0f, 1.0f, 0.647f, 0.0f };
	amesh = createCube();
	memcpy(amesh.mat.ambient, amb4, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);
	
	// some GL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


}

// ------------------------------------------------------------
//
// Main function
//


int main(int argc, char **argv) {

//  GLUT initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA|GLUT_MULTISAMPLE);

	glutInitContextVersion (4, 3);
	glutInitContextProfile (GLUT_CORE_PROFILE );
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);

	glutInitWindowPosition(100,100);
	glutInitWindowSize(WinX, WinY);
	WindowHandle = glutCreateWindow(CAPTION);


//  Callback Registration
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);

	glutTimerFunc(0, timer, 0);
	//glutIdleFunc(renderScene);  // Use it for maximum performance
	glutTimerFunc(0, refresh, 0);    //use it to to get 60 FPS whatever
	//glutIdleFunc(renderScene);  // Use it for maximum performance


//	Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeys);
	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glutMouseWheelFunc ( mouseWheel ) ;
	

//	return from main loop
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

//	Init GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	printf ("Vendor: %s\n", glGetString (GL_VENDOR));
	printf ("Renderer: %s\n", glGetString (GL_RENDERER));
	printf ("Version: %s\n", glGetString (GL_VERSION));
	printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));

	if (!setupShaders())
		return(1);

	init();

	//  GLUT main loop
	glutMainLoop();

	return(0);
}



