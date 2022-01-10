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
#include "Texture_Loader.h"
#include "avtFreeType.h"

#ifdef _WIN32
#define M_PI       3.14159265358979323846f
#endif

static inline float
DegToRad(float degrees)
{
	return (float)(degrees * (M_PI / 180.0f));
};

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

/// Array of boolean values of length 256 (0-255)
bool* keyStates = new bool[256];
void keyOperations();

GLint pvm_uniformId;
GLint vm_uniformId;
GLint normal_uniformId;
GLint lPos_uniformId[6];
GLint lDir_uniformId;
GLint slDir_uniformId[2];
GLint slPos_uniformId[2];
GLint slCutOffAngle_uniformId;
GLint tex_loc, tex_loc1;
GLint texMode_uniformId;

GLuint TextureArray[2];
	
// Camera Position
float camX, camY, camZ;

//float lookAtX, lookAtY, lookAtZ = 0.0f;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = -90.0f, beta = 0.0f;
float alpha_cam3 = -90.0f, beta_cam3 = 0.0f;


float r = 10.0f;

// Frame counting and FPS computation
long myTime,timebase = 0,frame = 0;
char s[32];
float tableX = 100;
float tableY = 0.5;
float tableZ = 100;

float lightDir[4] = { tableX / 2, tableY , tableZ / 2, 0.0f };
float nolightDir[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

float pointLightsHight = 5.f;

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

float roadWidth = 10;
//float roadTurn = sqrt((roadWidth * roadWidth) + (roadWidth * roadWidth));
float roadTurn = 5;

int numObjects = 0;
const int numButter = 5;
const int numOranges = 5;
int mapRoad[10][10] = { {1, 1, 1, 0, 0, 0, 0, 0, 0, 0},//1
						{0, 0, 1, 0, 0, 1, 1, 1, 1, 1},//2
						{0, 0, 1, 0, 0, 1, 0, 0, 0, 1},//3
						{0, 0, 1, 1, 1, 1, 0, 0, 0, 1},//4
						{0, 0, 0, 0, 0, 0, 0, 1, 1, 1},//5
						{1, 1, 1, 1, 1, 1, 1, 1, 0, 0},//6
						{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},//7
						{1, 0, 0, 1, 1, 1, 1, 0, 0, 0},//8
						{1, 0, 0, 1, 0, 0, 1, 0, 0, 0},//9
						{1, 1, 1, 1, 0, 0, 1, 1, 1, 1} };//10

int mapRows = sizeof(mapRoad) / sizeof(mapRoad[0]);
int mapCols = sizeof(mapRoad[0]) / sizeof(mapRoad[0][0]);
int numRoads = 0;
//CalcRoads();

int CalcRoads() {
	int count = 0;
	for (int i = 0; i < mapRows; i++) {
		for (int j = 0; j < mapCols; j++) {
			if (mapRoad[i][j] != 0) {
				count++;
			}
		}
	};
	printf("%d", count);
	return count;
}

/*
class RoadSegment {
	RoadID _id;
	RoadID _connections[2];
	Vec3D _positions[2];
	
};

class RoadID {
	unsigned short X;

};
*/
bool isDirectionalLightOn = false;
bool isPointLightsOn = false;
bool isSpotLightsOn = false;

bool CheckCollision(int oneXP, int oneYP, int oneXS, int oneYS, int twoXP, int twoYP, int twoXS, int twoYS); // AABB - AABB collision

struct Spotlight
{
	float pos[4];
	float dir[4];
	float ang;
};
class Car {
	public:
		float position[3] = { 0.0f, 0.0f, 0.0f }; 
		float minVelocity = 0.0f;
		float maxVelocity = 0.01f;
		float velocity = 0.0f;
		float direction[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
		float directionAngle = 0;
		float rotationAngle = 1.5f;
		float isForward = true;
		struct Spotlight *spotlights[2];
		
		float torusY = 1.0f; //z in world coords
		float carBodyX = 1.5f;
		float carBodyY = 3.0f;
		float jointCarGap = -0.5f;

		Car() {
			float marginX[2] = { carBodyY, carBodyY};
			float marginY[2] = { 0.8f, 0.8f };
			float marginZ[2] = { 0.f, carBodyX };
			for (int i = 0; i < 2; i++) {
				spotlights[i] = new Spotlight;

				// SPOTLIGHTS CUT OFF ANGLE OF CONE 
				spotlights[i]->ang = cos(DegToRad(8.5f));
				// W
				spotlights[i]->pos[3] = 0.f;

				spotlights[i]->pos[0] = position[0] + marginX[i];
				// Y
				spotlights[i]->pos[1] = position[1] + marginY[i];
				// Z
				spotlights[i]->pos[2] = position[2] + marginZ[i];
			}
			updateSpotlights();
		};

		void updateSpotlights() {
			float marginX[2] = { carBodyY , carBodyY };
			float marginY[2] = { 1.f, 1.f };
			float marginZ[2] = { 0.f, carBodyX };

			float forward;
			if (!isForward) {
				forward = -1;
			}
			else {
				forward = 1;
			}
			for (int i = 0; i < 2; i++) {
				// SPOTLIGHTS DIRECTION
				for (int j = 0; j < 4; j++) {

					spotlights[i]->dir[j] = forward*direction[j];
				}
			}
			// RIGHT SPOTLIGHT 
			// X
			spotlights[1]->pos[0] = position[0] + marginZ[1] * sin(DegToRad(directionAngle)) +  marginX[1] * cos(DegToRad(directionAngle));
			// Y
			spotlights[1]->pos[1] = position[1] + marginY[1];
			// Z
			spotlights[1]->pos[2] = position[2] - marginX[1] * sin(DegToRad(directionAngle)) + marginZ[1] * cos(DegToRad(directionAngle));

			// LEFT SPOTLIGHT 
			// X
			spotlights[0]->pos[0] = position[0] + marginX[0] * cos(DegToRad(directionAngle));
			// Y
			spotlights[0]->pos[1] = position[1] + marginY[0];
			// Z
			spotlights[0]->pos[2] = position[2] - marginX[0] * sin(DegToRad(directionAngle));

		}

		void reset() {
			direction[0] = 1; 
			for (int i = 0; i < 3; i++) {
				position[i] = 0;
				direction[i + 1] = 0;
			}
			directionAngle = 0;
			alpha_cam3 = -90.0f;
			beta_cam3 = 0.0f;
			updateSpotlights();
		}

		void move() {
			position[0] += direction[0] * velocity;
			position[1] += direction[1] * velocity;
			position[2] += direction[2] * velocity;
			updateSpotlights();
		};

		void changeDirection(float isRight) {
			float* newDir;
			if (isRight) {
				alpha_cam3 -= rotationAngle;
				directionAngle -= rotationAngle;
				newDir = rotateVec4(direction, -rotationAngle, 0, 1, 0);
				//rotateSpotlights(-rotationAngle);
			}
			else {
				directionAngle += rotationAngle;
				alpha_cam3 += rotationAngle;
				newDir = rotateVec4(direction, rotationAngle, 0, 1, 0);

				//rotateSpotlights(rotationAngle);
			}
			for (int i = 0; i < 3; i++) {
				direction[i] = newDir[i];
			}
			updateSpotlights();
		}

		void setDirection(float newDirection[3]) {
			direction[0] = newDirection[0];
			direction[1] = newDirection[1];
			direction[2] = newDirection[2];
		}
};

class Orange{
public:
	const float velocityIncrease = 0.005f;
	const float veloctityIntervalIncrease = 0.001;
	float maxVelocity = 0.03f;
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
		direction[0] = -1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1 - (-1))));
		direction[2] = -1 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (1 - (-1))));
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
	
	void renderOrange() {
		move();
		if (!isOnTable()) {
			//printf("fora da mesa\n");
			reset();
		}
		translate(MODEL, position[0], position[1], position[2]);
		roll();
		rotate(MODEL, rotationAngle, direction[0], direction[1], direction[2]);
		accelerate();
		//printf("velocity of the orange %.10f", velocity);
	}
};

class Butter {
public:
	float position[3] = { 0.0f, 1.5f, 0.0f };
	float rotationAngle = 0;

	Butter() {
		// set position random
		randomPosition();
		reset();
	};
	
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

	void reset() {
		randomPosition();
	}
};


class Game {
public:Car car;
public:Orange orange[numOranges];
public:Butter butter[numButter];
public:float finishLineDimensions[3] = { 1, 1, roadWidth };
public:float finishLinePos[3] = { tableX - finishLineDimensions[0] - 1, 1, tableZ - finishLineDimensions[2] - 0.5f };
public:bool isFinished = false;
public:bool win;
	  void checkFinish(float* q) {
		  if (CheckCollision(car.position[0], car.position[2], q[0], q[1], finishLinePos[0], finishLinePos[2], finishLineDimensions[0], finishLineDimensions[2])) {
			  printf("FIM DA CORRIDA\n");
			  finishGame(true);
		  }
	  }

	  void finishGame(bool win) {
		  if (win) {
			  cam = 1;
			  this->win = win;
			  RenderText(shaderText, "You win!", 25.0f, 25.0f, 1.0f, 0.5f, 0.8f, 0.2f);
			  isFinished = true;
		  }
		  else {
			  cam = 3;
			  car.reset();
		  }
	  }

	  void renderGameEnd() {
		  if (win) {
			  cam = 1;
			  RenderText(shaderText, "You win!", 25.0f, 25.0f, 1.0f, 0.5f, 0.8f, 0.2f);
		  }
	  }

	  void createFinishLine() {

		  float amb1[] = { 0.3f, 0.0f, 0.0f, 1.0f };
		  float diff1[] = { 0.8f, 0.1f, 0.1f, 1.0f };
		  float spec1[] = { 0.0f, 0.9f, 0.9f, 1.0f };
		  float shininess = 200.0;

		  float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		  int texcount = 0;

		  MyMesh amesh;

		  amesh = createCube();
		  memcpy(amesh.mat.ambient, amb1, 4 * sizeof(float));
		  memcpy(amesh.mat.diffuse, diff1, 4 * sizeof(float));
		  memcpy(amesh.mat.specular, spec1, 4 * sizeof(float));
		  memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
		  amesh.mat.shininess = shininess;
		  amesh.mat.texCount = texcount;
		  myMeshes.push_back(amesh);
		  numObjects++;
	  }

	  void createRoadCheerios() {

	  }

	  void renderFinishLine() {
		  translate(MODEL, finishLinePos[0], finishLinePos[1], finishLinePos[2]);
		  //rotate(MODEL, 180.f, 0.0f, 1.0f, 0.0f);
		  scale(MODEL, finishLineDimensions[0], finishLineDimensions[1], finishLineDimensions[2]);
	  }

	  void renderRoadCheerios(int id) { 

	  }
};

Game game;

float lookAtX = game.car.position[0];
float lookAtY = game.car.position[1];
float lookAtZ = game.car.position[2];


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

	float inclination = 10.f;
	float height = 7;
	camX = r * sin(alpha_cam3 * 3.14f / 180.0f) * cos(beta_cam3 * 3.14f / 180.0f) + game.car.position[0];
	camZ = r * cos(alpha_cam3 * 3.14f / 180.0f) * cos(beta_cam3 * 3.14f / 180.0f) + game.car.position[2];
	camY = r * sin(inclination * 3.14f / 180.0f) + height;

	lookAtX = game.car.position[0];
	lookAtY = game.car.position[1];
	lookAtZ = game.car.position[2];
	cam = 3;
};


//Verifies if two objectes are coliding
bool CheckCollision(int oneXP, int oneYP, int oneXS, int oneYS, int twoXP, int twoYP, int twoXS, int twoYS) // AABB - AABB collision
{
	int oneXP2 = oneXP - oneXS / 2;
	int oneYP2 = oneYP - oneYS / 2;
	int twoXP2 = twoXP - twoXS / 2;
	int twoYP2 = twoYP - twoYS / 2;


	// collision x-axis?
	bool collisionX = oneXP + oneXS >= twoXP &&
		twoXP + twoXS >= oneXP;
	// collision y-axis?
	bool collisionY = oneYP + oneYS >= twoYP &&
		twoYP + twoYS >= oneYP;
	// collision only if on both axes
	return collisionX && collisionY;
}

float *getCarSize() {
	float angle = game.car.rotationAngle;
	float radAngle = DegToRad(angle);
	float co = cos(radAngle);
	float si = sin(radAngle);
	float c = co *game.car.carBodyY;
	float a = si * game.car.carBodyX;

	float angle2 = 90.0 - ((int) angle % 90);
	float radAngle2 = DegToRad(angle2);
	float co2 = cos(radAngle2);
	float si2 = sin(radAngle2);
	float d = co2 * game.car.carBodyX;
	float b = si2 * game.car.carBodyY;

	float x = a + b;
	float y = c + d;
	float* q = new float[2];
	q[0] = x;
	q[1] = y;
	return q;

	
}

// ------------------------------------------------ ------------
//
// Render stufff
//

void renderScene(void) {

	keyOperations();
	GLint loc;
	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// load identity matrices
	loadIdentity(VIEW);
	loadIdentity(MODEL);
	// set the camera using a function similar to gluLookAt

	lookAt(camX, camY, camZ, lookAtX, lookAtY, lookAtZ, 0,1,0);
	
	if (!game.isFinished) {
	
		// use our shader
		glUseProgram(shader.getProgramIndex());

		//send the light position in eye coordinates
		//glUniform4fv(lPos_uniformId, 1, lightPos); //efeito capacete do mineiro, ou seja lighPos foi definido em eye coord 

		float res[6][4];
		float res2[4];

		for (int i = 0; i < 6; i++) {
			//printf("Point light = { %.1f, %.1f, %.1f, %.1f}\n", lightPos[i][0], lightPos[i][1], lightPos[i][2], lightPos[i][3]);
			multMatrixPoint(VIEW, lightPos[i], res[i]);   //lightPos definido em World Coord so is converted to eye space
			//printf("WORLD COORDS Point light = { %.1f, %.1f, %.1f, %.1f}\n", res[i][0], res[i][1], res[i][2], res[i][3]);
			//printf("Point light uniform ID= { %d }\n", lPos_uniformId[i]);

			glUniform4fv(lPos_uniformId[i], 1, res[i]);

		}
		//printf("\n\n");		
		//printf("Directional light = { %.1f, %.1f, %.1f, %.1f}", directionalLight[0], directionalLight[1], directionalLight[2], directionalLight[3]);
		
		multMatrixPoint(VIEW, directionalLight, res2);   //lightPos definido em World Coord so is converted to eye space
		//printf("WORLD COORDS DIRECTIONAL light = { %.1f, %.1f, %.1f, %.1f}\n", res2[0], res2[1], res2[2], res2[3]);
		glUniform4fv(lDir_uniformId, 1, res2);
		
		//printf("spotlight direction = { %.1f, %.1f, %.1f, %.1f}\n", game.car.spotlights[0]->dir[0], game.car.spotlights[0]->dir[1], game.car.spotlights[0]->dir[2], game.car.spotlights[0]->dir[3]);
		//printf("spotlight position = { %.1f, %.1f, %.1f, %.1f}\n", game.car.spotlights[0]->pos[0], game.car.spotlights[0]->pos[1], game.car.spotlights[0]->pos[2], game.car.spotlights[0]->pos[3]);
		//printf("spotlight cut angle = { %.10f }\n", game.car.spotlights[0]->ang);
		float res3[4];

	
		multMatrixPoint(VIEW, game.car.spotlights[0]->dir, res3);
		//printf("spotlight 0 dir = { %.1f, %.1f, %.1f, %.1f}\n", res2[0], res2[1], res2[2], res2[3]);
		glUniform4fv(slDir_uniformId[0], 1, res3);


		float res4[4];

		multMatrixPoint(VIEW, game.car.spotlights[1]->dir, res4);
		//printf("spotlight dir= { %.1f, %.1f, %.1f, %.1f}\n", game.car.spotlights[1]->dir[0], game.car.spotlights[1]->dir[1], game.car.spotlights[1]->dir[2], game.car.spotlights[1]->dir[3]);
		//printf("spotlight dir res = { %.1f, %.1f, %.1f, %.1f}\n", res2[0], res2[1], res2[2], res2[3]);
		glUniform4fv(slDir_uniformId[1], 1, res4);

		float res5[4];


	multMatrixPoint(VIEW, game.car.spotlights[0]->pos, res5);
	glUniform4fv(slPos_uniformId[0], 1, res5);

		float res6[4];

	multMatrixPoint(VIEW, game.car.spotlights[1]->pos, res6);
	glUniform4fv(slPos_uniformId[1], 1, res6);
	
	

		glUniform1f(slCutOffAngle_uniformId, game.car.spotlights[0]->ang);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, TextureArray[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, TextureArray[1]);


	//Indicar aos dois samplers do GLSL quais os Texture Units a serem usados
	glUniform1i(tex_loc, 1);
	glUniform1i(tex_loc1, 0);

	float* q = getCarSize();
	for (int j = 0; j < numButter + numOranges; j++) {
		if (j < numOranges) {

				if (CheckCollision(game.car.position[0], game.car.position[2], q[0], q[1], game.orange[j].position[0], game.orange[j].position[2], 2, 2)) {
					printf("COLISAO_ORANGE\n");
					game.finishGame(false); 
				}	
			}
			else {
				if (CheckCollision(game.car.position[0], game.car.position[2], q[0], q[1], game.butter[j-numOranges].position[0], game.butter[j-numOranges].position[2], 1, 1)) {
					printf("COLISAO_BUTTER\n");
				}
			}
		}
		game.checkFinish(q);

		int objId=0; //id of the object mesh - to be used as index of mesh: Mymeshes[objID] means the current mesh
		for (int i = 0 ; i < numObjects; ++i) {
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
				float carBodyZ = 1.5f;
				float carBodyX = 3.0f;
				float jointCarGap = 0.5f;
			
				//game.car.move();
				float* position = game.car.position;

				float p;
				float q;
			
				switch (objId) {
				case 0:
					//table
					translate(MODEL, 0.0f, 0.0f, 0.0f);
					scale(MODEL, tableX, tableY, tableZ);

					break;
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
					//orange
					game.orange[objId - 1].renderOrange();
					break;
				case 6:
					//car wheel torus RIGHT TOP
					translate(MODEL, position[0] + (carBodyZ)*sin(DegToRad(game.car.directionAngle)) + (carBodyX - jointCarGap) * cos(DegToRad(game.car.directionAngle)), position[1] + torusY, position[2] + carBodyZ * cos(DegToRad(game.car.directionAngle)) - (carBodyX - jointCarGap) * sin(DegToRad(game.car.directionAngle)));
					rotate(MODEL, game.car.directionAngle, 0.0f, 1.0f, 0.0f);
					rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);

					break;
				case 7:
					//car wheel torus LEFT TOP
					translate(MODEL, position[0] + (carBodyX - jointCarGap) * cos(DegToRad(game.car.directionAngle)), position[1] + torusY, position[2] - (carBodyX - jointCarGap) * sin(DegToRad(game.car.directionAngle)));
					rotate(MODEL, game.car.directionAngle, 0.0f, 1.0f, 0.0f);
					rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);

					break;

				case 8:
					//car wheel torus RIGHT BOTTOM
				
					translate(MODEL, position[0] + carBodyZ * sin(DegToRad(game.car.directionAngle)) + (jointCarGap * cos(DegToRad(game.car.directionAngle))), position[1] + torusY, position[2] + (carBodyZ* cos(game.car.directionAngle * 3.14159265358979323846f / 180)) - (jointCarGap * sin(game.car.directionAngle * 3.14159265358979323846f / 180)));
					rotate(MODEL, game.car.directionAngle, 0.0f, 1.0f, 0.0f);
					rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);

					break;
				case 9:
					//car wheel torus LEFT BOTTOM

					translate(MODEL, position[0] + jointCarGap * cos(DegToRad(game.car.directionAngle)), position[1] + torusY, position[2] - jointCarGap * sin(DegToRad(game.car.directionAngle)));
					rotate(MODEL, game.car.directionAngle, 0.0f, 1.0f, 0.0f);
					rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);
					break;
				case 10:
					//car body
					translate(MODEL, position[0], position[1] + torusY - 0.2f, position[2]);
					rotate(MODEL, game.car.directionAngle, 0.0f, 1.0f, 0.0f);
					scale(MODEL, carBodyX, 0.5, carBodyZ);
					break;
			
				case 11:
					game.renderFinishLine();
					break;
				};

			
				// send matrices to OGL
				computeDerivedMatrix(PROJ_VIEW_MODEL);
				glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
				glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
				computeNormalMatrix3x3();
				glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

			// Render mesh
			if (objId == 0) glUniform1i(texMode_uniformId, 1);
			else glUniform1i(texMode_uniformId, 2);

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

		for (int y = 0; y < numButter; y++) {
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

			game.car.move();
			float* position = game.car.position;

			//butter
			translate(MODEL, game.butter[y].position[0], game.butter[y].position[1], game.butter[y].position[2]);
			//scale(MODEL, 4, 2, 5);

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

		}

		for (int j = 0; j < mapRows; j++) {
			for (int k = 0; k < mapCols; k++) {
				if (mapRoad[j][k] == 0) {
					continue;
				}
				//printf("numR:  %d", numRoads);
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

				game.car.move();
				float* position = game.car.position;

				if (mapRoad[j][k] == 1) {
					translate(MODEL, roadWidth * k, 0.1f, roadWidth * j);
					scale(MODEL, roadWidth, 0.5, roadWidth);
				}
				else if (mapRoad[j][k] == 2) {
					translate(MODEL, roadTurn * k, 0.1f, roadTurn * j);
					scale(MODEL, roadTurn, 0.5, roadTurn);
				}
				// send matrices to OGL
				computeDerivedMatrix(PROJ_VIEW_MODEL);
				glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
				glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
				computeNormalMatrix3x3();
				glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

				// Render mesh
				glUniform1i(texMode_uniformId, 0);
				glBindVertexArray(myMeshes[objId].vao);

				if (!shader.isProgramValid()) {
					printf("Program Not Valid!\n");
					exit(1);
				}
				glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes, GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);

				popMatrix(MODEL);
				objId++;
			}
		}
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
	}
	if (game.isFinished) {
		game.renderGameEnd();
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
				for (int i = 0; i < 2; i++) {
					game.car.spotlights[i]->pos[3] = 0.0f;
				}
				isSpotLightsOn = false;
			}
			else {
				for (int i = 0; i < 2; i++) {
					game.car.spotlights[i]->pos[3] = 1.0f;
				}
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

//
// Events from the Keyboard
//

void keyOperations() {

	float* direction = game.car.direction;

	if (keyStates[27]) {
		glutLeaveMainLoop();
	}

	if (keyStates['q']) {
		//move forward

		if (game.car.isForward == false) {  //if car wants to go forward when it's reversing
			game.car.velocity -= 0.00032;
			if (game.car.velocity < 0) {
				game.car.velocity = 0;
				for (int i = 0; i < 3; i++) {
					direction[i] = -direction[i];
				}
				game.car.isForward = true;
			}
		}
		else{
			game.car.velocity += 0.00008;
			if (game.car.velocity > game.car.maxVelocity) {
				game.car.velocity = game.car.maxVelocity;
			}
		}
		game.car.move();
	}

	if (keyStates['a']) {
		//move backwards
		if (game.car.isForward == true) {  //if car wants to go backward (similar to braking)
			game.car.velocity -= 0.00032;
			if (game.car.velocity < 0) {
				game.car.velocity = 0;
				for (int i = 0; i < 3; i++) {
					direction[i] = -direction[i];
				}
				game.car.isForward = false;
			}
		}
		else {
			game.car.velocity += 0.00008;
			if (game.car.velocity > game.car.maxVelocity) {
				game.car.velocity = game.car.maxVelocity;
			}
		}
		game.car.move();
	}

	
	if (keyStates['p'] && game.car.velocity > 0) {
		//move left
		if (!keyStates['q'] && !keyStates['a']) {
			game.car.velocity -= 0.00016;
			if (game.car.velocity < 0) {
				game.car.velocity = 0;
			}
		}
		if (game.car.isForward == true && game.car.velocity > 0) {
			game.car.changeDirection(true);
		}
		else if (game.car.isForward == false && game.car.velocity > 0) {
			game.car.changeDirection(false);
		}
	}

	if (keyStates['o'] && game.car.velocity > 0) {
		//move left
		if (!keyStates['q'] && !keyStates['a']) {
			game.car.velocity -= 0.00008;
			if (game.car.velocity < 0) {
				game.car.velocity = 0;
			}
		}
		if (game.car.isForward == true && game.car.velocity > 0) {
			game.car.changeDirection(false);
		}
		else if (game.car.isForward == false && game.car.velocity > 0) {
			game.car.changeDirection(true);
		}
	}

	for (int i = 0; i < 256; i++) {
		if (keyStates[i] == true) {
			return;
		}
	}

	game.car.velocity -= 0.00016;
	if (game.car.velocity < 0) {
		game.car.velocity = 0;
	}
	else if (game.car.velocity > game.car.maxVelocity) {
		game.car.velocity = game.car.maxVelocity;
	}
	game.car.updateSpotlights();

}

void keyPressed(unsigned char key, int xx, int yy) {

	keyStates[key] = true;
}

void keyUp(unsigned char key, int x, int y) {
	switch (key) {
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
			// SPOTLIGHTS --> ILUMINACAO COMO SE FOSSE AS LUZES FRONTEIRAS DO CARRO
		if (isSpotLightsOn) {
			for (int i = 0; i < 2; i++) {
				game.car.spotlights[i]->pos[3] = 0.0f;
			}
			isSpotLightsOn = false;
		}
		else {
			for (int i = 0; i < 2; i++) {
				game.car.spotlights[i]->pos[3] = 1.0f;
			}
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
	case '4':   //DEBUG MOVIMENTO DO CARRO
		for (int i = 0; i < 3; i++) {
			cout << "\ndir " << game.car.direction[i];
		}
		cout << "\nis Forward  " << game.car.isForward;

		break;
	default:
		cout << "tecla carregada = " << key;
		break;
	}
	keyStates[key] = false;
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
	float alphaAux = 0;
	float betaAux = 0;
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
		alpha_cam3 = alphaAux;
		//beta_cam3 = betaAux;
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


	texMode_uniformId = glGetUniformLocation(shader.getProgramIndex(), "texMode"); // different modes of texturing
	pvm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_pvm");
	vm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_viewModel");
	normal_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_normal");

	lPos_uniformId[0] = glGetUniformLocation(shader.getProgramIndex(), "l_pos[0]");
	lPos_uniformId[1] = glGetUniformLocation(shader.getProgramIndex(), "l_pos[1]");
	lPos_uniformId[2] = glGetUniformLocation(shader.getProgramIndex(), "l_pos[2]");
	lPos_uniformId[3] = glGetUniformLocation(shader.getProgramIndex(), "l_pos[3]");
	lPos_uniformId[4] = glGetUniformLocation(shader.getProgramIndex(), "l_pos[4]");
	lPos_uniformId[5] = glGetUniformLocation(shader.getProgramIndex(), "l_pos[5]");
	
	lDir_uniformId = glGetUniformLocation(shader.getProgramIndex(), "l_dir");
	

	slPos_uniformId[0] = glGetUniformLocation(shader.getProgramIndex(), "sl_pos[0]");
	slPos_uniformId[1] = glGetUniformLocation(shader.getProgramIndex(), "sl_pos[1]");

	slDir_uniformId[0] = glGetUniformLocation(shader.getProgramIndex(), "sl_dir[0]");
	slDir_uniformId[1] = glGetUniformLocation(shader.getProgramIndex(), "sl_dir[1]");

	slCutOffAngle_uniformId = glGetUniformLocation(shader.getProgramIndex(), "sl_cut_off_ang");

	tex_loc = glGetUniformLocation(shader.getProgramIndex(), "texmap");
	tex_loc1 = glGetUniformLocation(shader.getProgramIndex(), "texmap1");
	
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

	for (int i = 0; i < 256; i++) {
		keyStates[i] = false;
	}

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
	camY = r * sin(beta * 3.14f / 180.0f);

	glGenTextures(2, TextureArray);
	Texture2D_Loader(TextureArray, "grass.jpg", 0);
	Texture2D_Loader(TextureArray, "road.jpg", 1);


	numRoads = CalcRoads();


	float amb[] = { 0.2f, 0.15f, 0.1f, 1.0f };
	float diff[] = { 0.8f, 0.6f, 0.4f, 1.0f };
	float spec[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 100.0f;
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
	numObjects++;


	float amb1[] = { 0.3f, 0.0f, 0.0f, 1.0f };
	float diff1[] = { 0.8f, 0.1f, 0.1f, 1.0f };
	float spec1[] = { 0.0f, 0.9f, 0.9f, 1.0f };
	shininess = 200.0;



	float amb2[] = { 1.0f, 0.647f, 0.0f, 1.0f };
	// ORANGE
	for (int i = 0; i < numOranges; i++) {
		amesh = createSphere(1.0f, 20);
		memcpy(amesh.mat.ambient, amb2, 4 * sizeof(float));
		memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
		memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
		memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
		amesh.mat.shininess = shininess;
		amesh.mat.texCount = texcount;
		myMeshes.push_back(amesh);
		numObjects++;
	}

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
		numObjects++;
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
	numObjects++;

	game.createFinishLine();

	//Butter
	for (int i = 0; i < numButter; i++) {
		amesh = createCube();
		memcpy(amesh.mat.ambient, amb4, 4 * sizeof(float));
		memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
		memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
		memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
		amesh.mat.shininess = shininess;
		amesh.mat.texCount = texcount;
		myMeshes.push_back(amesh);
		//numObjects++;
	};

	// ROAD
	for (int i = 0; i < numRoads; i++) {
		amesh = createCube();
		memcpy(amesh.mat.ambient, amb1, 4 * sizeof(float));
		memcpy(amesh.mat.diffuse, diff1, 4 * sizeof(float));
		memcpy(amesh.mat.specular, spec1, 4 * sizeof(float));
		memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
		amesh.mat.shininess = 500;
		amesh.mat.texCount = texcount;
		myMeshes.push_back(amesh);
		//numObjects++;
	};

	
	// some GL settings
	//glEnable(GL_DEPTH_TEST);
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

	glutKeyboardFunc(keyPressed);
	glutKeyboardUpFunc(keyUp);
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