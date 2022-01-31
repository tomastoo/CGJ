//
// CGJ: Phong Shading and Text rendered with FreeType library
// The text rendering was based on
// https://learnopengl.com/In-Practice/Text-Rendering This demo was built for
// learning purposes only. Some code could be severely optimised, but I tried to
// keep as simple and clear as possible.
//
// The code comes with no warranties, use it at your own risk.
// You may use it, or parts of it, wherever you want.
//
// Author: Jo�o Madeiras Pereira
//

#include <iostream>
#include <math.h>
#include <sstream>
#include <string>

// include GLEW to access OpenGL 3.3 functions
#include <GL/glew.h>

// GLUT is the toolkit to interface with the OS
#include <GL/freeglut.h>

#include <IL/il.h>

// Use Very Simple Libs
#include "AVTmathLib.h"
#include "Texture_Loader.h"
#include "VSShaderlib.h"
#include "VertexAttrDef.h"
#include "avtFreeType.h"
#include "geometry.h"

#ifdef _WIN32
#define M_PI 3.14159265358979323846f
#define frand() ((float)rand() / RAND_MAX)
#define MAX_PARTICULAS 1500
#endif

#ifndef __FLARE_H
#define __FLARE_H

/* --- Defines --- */

#define FLARE_MAXELEMENTSPERFLARE 15
#define NTEXTURES 5

/* --- Types --- */

typedef struct FLARE_ELEMENT_DEF {

  float fDistance;     // Distance along ray from source (0.0-1.0)
  float fSize;         // Size relative to flare envelope (0.0-1.0)
  float matDiffuse[4]; // color
  int textureId;
} FLARE_ELEMENT_DEF;

typedef struct FLARE_DEF {
  float fScale;   // Scale factor for adjusting overall size of flare elements.
  float fMaxSize; // Max size of largest element, as proportion of screen width
                  // (0.0-1.0)
  int nPieces;    // Number of elements in use
  FLARE_ELEMENT_DEF element[FLARE_MAXELEMENTSPERFLARE];
} FLARE_DEF;

char *flareTextureNames[NTEXTURES] = {"crcl", "flar", "hxgn", "ring", "sun"};

void render_flare(FLARE_DEF *flare, int lx, int ly, int *m_viewport);
void loadFlareFile(FLARE_DEF *flare, char *filename);

#endif

inline double clamp(const double x, const double min, const double max) {
  return (x < min ? min : (x > max ? max : x));
}

inline int clampi(const int x, const int min, const int max) {
  return (x < min ? min : (x > max ? max : x));
}

static inline float DegToRad(float degrees) {
  return (float)(degrees * (M_PI / 180.0f));
};

using namespace std;

#define CAPTION "Projeto CGJ Grupo 06 2021/22"
int WindowHandle = 0;
int WinX = 1024, WinY = 768;

// Fireworks

typedef struct {
  float life;         // vida
  float fade;         // fade
  float r, g, b;      // color
  GLfloat x, y, z;    // posicao
  GLfloat vx, vy, vz; // velocidade
  GLfloat ax, ay, az; // aceleracao
} Particle;

Particle particula[MAX_PARTICULAS];
int dead_num_particles = 0;
int fireworks = 0;

unsigned int FrameCount = 0;
int cam = 3;
// shaders
VSShaderLib shader;       // geometry
VSShaderLib shaderGlobal; // geometry directional light
VSShaderLib shaderText;   // render bitmap text

// File with the font
const string font_name = "fonts/arial.ttf";

// Vector with meshes
vector<struct MyMesh> myMeshes;

// External array storage defined in AVTmathLib.cpp

/// The storage for matrices
extern float mMatrix[COUNT_MATRICES][16];
extern float mCompMatrix[COUNT_COMPUTED_MATRICES][16];
/// The normal matrix
extern float mNormal3x3[9];

/// Array of boolean values of length 256 (0-255)
bool *keyStates = new bool[256];
void keyOperations();

GLint pvm_uniformId;
GLint vm_uniformId;
GLint normal_uniformId;
GLint model_uniformId;
GLint lPos_uniformId[6];
GLint lDir_uniformId;
GLint slDir_uniformId[2];
GLint slPos_uniformId[2];
GLint slCutOffAngle_uniformId;

GLint slPosTexture_uniformId;
GLint slDirTexture_uniformId;
GLint slCutOffAngleTexture_uniformId;

GLint is_fog_on_uniformId;
GLint fog_maxdist_uniformId;

GLint tex_loc, tex_loc1, tex_loc2, tex_loc3, tex_loc4, tex_cube_loc;
GLint texMode_uniformId;

GLuint TextureArray[10];
GLuint FlareTextureArray[5];

// Flare effect
FLARE_DEF AVTflare;
float lightScreenPos[3]; // Position of the light in Window Coordinates

bool flareEffect = false;

// Camera Position
float camX, camY, camZ;

// float lookAtX, lookAtY, lookAtZ = 0.0f;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = -90.0f, beta = 0.0f;
float alpha_cam3 = -90.0f, beta_cam3 = 0.0f;

float r = 10.0f;

// Frame counting and FPS computation
long myTime, timebase = 0, frame = 0;
char s[32];
float tableX = 100;
float tableY = 0.5;
float tableZ = 100;

float lightDir[4] = {tableX / 2, tableY, tableZ / 2, 0.0f};
float nolightDir[4] = {0.0f, 0.0f, 0.0f, 0.0f};

float pointLightsHight = 5.f;

const float plGrid[2] = {
    4, 3}; // This means that the point lights will be devided
           // |_|_|_|_| like this so the first value is the number of
           // |_|_|_|_| collumns and the second is the number of rows
           // |_|_|_|_| (x,z)

float lightPos[6][4] = {
    {tableX / plGrid[0], tableY + pointLightsHight, tableZ / plGrid[1], 0.0f},
    {(tableX / plGrid[0]) * 2, tableY + pointLightsHight, (tableZ / plGrid[1]),
     0.0f},
    {(tableX / plGrid[0]) * 3, tableY + pointLightsHight, (tableZ / plGrid[1]),
     0.0f},
    {(tableX / plGrid[0]), tableY + pointLightsHight, (tableZ / plGrid[1]) * 2,
     0.0f},
    {(tableX / plGrid[0]) * 2, tableY + pointLightsHight,
     (tableZ / plGrid[1]) * 2, 0.0f},
    {(tableX / plGrid[0]) * 3, tableY + pointLightsHight,
     (tableZ / plGrid[1]) * 2, 0.0f},
};

float *directionalLight = nolightDir;

float roadWidth = 5;
// float roadTurn = sqrt((roadWidth * roadWidth) + (roadWidth * roadWidth));
float roadTurn = 5;

int numObjects = 0;
const int numButter = 5;
const int numOranges = 5;

int mapRoad[21][21] = {
    {
        1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    }, // 1
    {
        1, 1, 1, 1, 1, 1, 2, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    },
    {
        2, 2, 2, 2, 1, 1, 2, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    }, // 2
    {
        0, 0, 0, 2, 1, 1, 2, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    },
    {
        0, 0, 0, 2, 1, 1, 2, 0, 0, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1, 1,
    }, // 3
    {
        0, 0, 0, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 0, 0, 0, 0, 2, 1, 1,
    }, // 3
    {
        0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 2, 1, 1,
    }, // 4
    {
        0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 1,
    }, // 4
    {
        0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
    }, // 5
    {
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
    }, // 5
    {
        2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
    }, // 6
    {
        2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0,
    }, // 6
    {
        2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0,
    }, // 7
    {
        2, 1, 1, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0,
    }, // 7
    {
        2, 1, 1, 2, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0,
    }, // 8
    {
        2, 1, 1, 2, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0,
    }, // 8
    {
        2, 1, 1, 2, 0, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 0, 0, 0, 0, 0,
    }, // 9
    {
        2, 1, 1, 2, 2, 2, 1, 1, 2, 0, 0, 2, 1, 1, 2, 2, 2, 2, 2, 2,
    }, // 9
    {
        2, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1,
    }, // 10
    {
        2, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1,
    }};

// int mapRoad[10][10] = { {1, 1, 1, 0, 0, 0, 0, 0, 0, 0},//1
//						{0, 0, 1, 0, 0, 1, 1, 1, 1,
// 1},//2 						{0, 0, 1, 0, 0, 1, 0, 0, 0, 1},//3
// {0, 0, 1, 1, 1, 1, 0, 0, 0, 1},//4 						{0, 0, 0, 0, 0, 0, 0, 1, 1, 1},//5
// {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},//6 						{1, 0,
// 0, 0, 0, 0, 0, 0, 0, 0},//7 						{1, 0, 0, 1, 1, 1, 1, 0, 0, 0},//8
// {1, 0, 0, 1, 0, 0, 1, 0, 0, 0},//9 						{1, 1,
// 1, 1, 0, 0, 1, 1, 1, 0}
//};//10

int mapRows = sizeof(mapRoad) / sizeof(mapRoad[0]);
int mapCols = sizeof(mapRoad[0]) / sizeof(mapRoad[0][0]);
int numRoads = 0;

int CalcRoads() {
  int count = 0;
  for (int i = 0; i < mapRows; i++) {
    for (int j = 0; j < mapCols; j++) {
      if (mapRoad[i][j] == 1) {
        count++;
      }
    }
  };
  printf("%d\n", count);
  return count;
}

int CalcCheerios() {
  int count2 = 0;
  for (int i = 0; i < mapRows; i++) {
    for (int j = 0; j < mapCols; j++) {
      if (mapRoad[i][j] == 2) {
        count2++;
      }
    }
  };
  printf("number of cheerios: %d\n", count2);
  return count2;
}

int numCheerios = CalcCheerios();
const int numCheerios2 = numCheerios;

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
bool isFogOn = false;

bool CheckCollision(int oneXP, int oneYP, int oneXS, int oneYS, int twoXP,
                    int twoYP, int twoXS, int twoYS); // AABB - AABB collision

struct Spotlight {
  float pos[4];
  float dir[4];
  float ang;
};

class Car {
public:
  float position[3] = {0.0f, 0.0f, 0.0f};
  float minVelocity = 0.0f;
  float maxVelocity = 0.1f;
  float velocity = 0.0f;
  float direction[4] = {1.0f, 0.0f, 0.0f, 0.0f};
  float directionAngle = 0;
  float rotationAngle = 1.5f;
  float isForward = true;
  struct Spotlight *spotlights[2];

  float torusY = 1.0f; // z in world coords
  float carBodyX = 1.5f;
  float carBodyY = 3.0f;
  float jointCarGap = -0.5f;

  Car() {
    float marginX[2] = {carBodyY, carBodyY};
    float marginY[2] = {0.8f, 0.8f};
    float marginZ[2] = {0.f, carBodyX};
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
    float marginX[2] = {carBodyY, carBodyY};
    float marginY[2] = {1.f, 1.f};
    float marginZ[2] = {0.f, carBodyX};

    float forward;

    if (!isForward) {
      forward = -1;
    } else {
      forward = 1;
    }

    for (int i = 0; i < 2; i++) {
      // SPOTLIGHTS DIRECTION
      for (int j = 0; j < 4; j++) {

        spotlights[i]->dir[j] = forward * direction[j];
      }
    }
    // RIGHT SPOTLIGHT
    // X
    spotlights[1]->pos[0] = position[0] +
                            marginZ[1] * sin(DegToRad(directionAngle)) +
                            marginX[1] * cos(DegToRad(directionAngle));
    // Y
    spotlights[1]->pos[1] = position[1] + marginY[1];
    // Z
    spotlights[1]->pos[2] = position[2] -
                            marginX[1] * sin(DegToRad(directionAngle)) +
                            marginZ[1] * cos(DegToRad(directionAngle));

    // LEFT SPOTLIGHT
    // X
    spotlights[0]->pos[0] =
        position[0] + marginX[0] * cos(DegToRad(directionAngle));
    // Y
    spotlights[0]->pos[1] = position[1] + marginY[0];
    // Z
    spotlights[0]->pos[2] =
        position[2] - marginX[0] * sin(DegToRad(directionAngle));
  }

  bool isOnTable() {
    if (position[0] > tableX || position[0] < 0)
      return false;

    if (position[2] > tableZ || position[2] < 0)
      return false;

    return true;
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
    isForward = true;
    velocity = 0;
    updateSpotlights();
  }

  void changeVelocity(float velocityChange) { velocity += velocityChange * 4; }

  void move() {
    position[0] += direction[0] * velocity;
    position[1] += direction[1] * velocity;
    position[2] += direction[2] * velocity;
    updateSpotlights();
  };

  void changeDirection(float isRight) {
    float *newDir;
    if (isRight) {
      alpha_cam3 -= rotationAngle;
      directionAngle -= rotationAngle;
      newDir = rotateVec4(direction, -rotationAngle, 0, 1, 0);
      // rotateSpotlights(-rotationAngle);
    } else {
      directionAngle += rotationAngle;
      alpha_cam3 += rotationAngle;
      newDir = rotateVec4(direction, rotationAngle, 0, 1, 0);

      // rotateSpotlights(rotationAngle);
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

class Orange {
public:
  const float velocityIncrease = 0.00005f;
  const float veloctityIntervalIncrease = 0.00001f;
  float maxVelocity = 0.03f;
  float minVelocity = 0.01f;
  float position[3] = {0.0f, 1.5f, 0.0f};
  float velocity;
  float direction[3] = {1.0f, 0.0f, 0.0f};
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

  void roll() { rotationAngle = velocity * 10; }

  // given a certain table size
  void randomPosition() {
    position[0] =
        static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / tableX));
    position[2] =
        static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / tableZ));
  }

  bool isOnTable() {
    if (position[0] > tableX || position[0] < 0)
      return false;

    if (position[2] > tableZ || position[2] < 0)
      return false;

    return true;
  }

  void randomDirection() {
    direction[0] = -1 + static_cast<float>(rand()) /
                            (static_cast<float>(RAND_MAX / (1 - (-1))));
    direction[2] = -1 + static_cast<float>(rand()) /
                            (static_cast<float>(RAND_MAX / (1 - (-1))));
  }

  void randomVelocity() {
    velocity = minVelocity +
               static_cast<float>(rand()) /
                   (static_cast<float>(RAND_MAX / (maxVelocity - minVelocity)));
  }

  void reset() {
    randomPosition();
    randomDirection();
    randomVelocity();
  }

  void accelerate() {
    // new max and min velocity
    minVelocity += veloctityIntervalIncrease;
    maxVelocity += veloctityIntervalIncrease;

    // acceleration part
    velocity += velocityIncrease;
    if (velocity > maxVelocity) {
      velocity = maxVelocity;
    }
  }

  void animateOrange() {
    move();
    if (!isOnTable()) {
      // printf("fora da mesa\n");
      reset();
    }
  }

  void renderOrange() {
    translate(MODEL, position[0], position[1], position[2]);
    roll();
    rotate(MODEL, rotationAngle, direction[0], direction[1], direction[2]);
    accelerate();
    // printf("velocity of the orange %.10f", velocity);
  }
};

class Butter {
public:
  float position[3] = {0.0f, 0.5f, 0.0f};
  float rotationAngle = 0;

  Butter() {
    // set position random
    randomPosition();
    // reset();
  };

  // given a certain table size
  void randomPosition() {
    int a = rand() % mapRows;
    int b = rand() % mapCols;
    while (mapRoad[a][b] != 1) {
      a = rand() % mapRows;
      b = rand() % mapCols;
    }

    position[0] =
        b * 5 + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 5));
    position[2] =
        a * 5 + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 5));
    printf("butter, p0: %f, p1: %f", position[0], position[1]);
  }

  bool isOnTable() {
    if (position[0] > tableX || position[0] < 0)
      return false;

    if (position[2] > tableZ || position[0] < 0)
      return false;

    return true;
  }

  void reset() { randomPosition(); }
};

class Cheerio {
public:
  float position[3];
  float rotationAngle = 0;

  Cheerio(){
      // set position random
      // randomPosition();
      // reset();
  };

  // given a certain table size
  void randomPosition() {
    int a = rand() % mapRows;
    int b = rand() % mapCols;
    while (mapRoad[a][b] != 1) {
      a = rand() % mapRows;
      b = rand() % mapCols;
    }

    position[0] =
        b * 5 + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 5));
    position[2] =
        a * 5 + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 5));
    // printf("butter, p0: %f, p1: %f", position[0], position[1]);
  }

  bool isOnTable() {
    if (position[0] > tableX || position[0] < 0)
      return false;

    if (position[2] > tableZ || position[0] < 0)
      return false;

    return true;
  }

  void reset(int k, int j) {
    position[0] = roadWidth * j + roadWidth / 2;
    position[1] = 1.0f;
    position[2] = roadWidth * k + roadWidth / 2;
    // randomPosition();
  }
};

float lookAtX = 0;
float lookAtY = 0;
float lookAtZ = 0;

class Game {
public:
  Car car;

public:
  Orange orange[numOranges];

public:
  Butter butter[numButter];

public:
  Spotlight textureSl;

public:
  bool isTextureSpOn = false;

public:
  Cheerio cheerio[141];

public:
  float finishLineDimensions[3] = {2 * roadWidth, 0.6, 2 * roadWidth};

public:
  float finishLinePos[3] = {tableX - finishLineDimensions[0], 0,
                            tableZ - finishLineDimensions[2]};

public:
  bool isFinished = false;

public:
  bool win;

public:
  float fogMaxDistance = 100;

public:
  bool isGamePaused = false;

public:
  int lives = 5;

public:
  int points = 0;

  Game() {
    textureSl.pos[0] = car.position[0];
    textureSl.pos[1] = car.position[0];
    textureSl.pos[2] = car.position[0];
    textureSl.pos[3] = 0;

    textureSl.dir[0] = 0;
    textureSl.dir[1] = 0;
    textureSl.dir[2] = 0;
    textureSl.dir[3] = 0;

    textureSl.ang = cos(DegToRad(12.5f));
  }

  // update texture spotlight key l
  void updateTextureSl() {
    textureSl.pos[0] = lookAtX;
    textureSl.pos[1] = lookAtY;
    textureSl.pos[2] = lookAtZ;

    textureSl.dir[0] = camX;
    textureSl.dir[1] = camY;
    textureSl.dir[2] = camZ;
  }

  void checkFinish(float *q, float *p) {
    if (CheckCollision(car.position[0], car.position[2], q[0], q[1],
                       finishLinePos[0], finishLinePos[2],
                       finishLineDimensions[0], finishLineDimensions[2])) {
      printf("FIM DA CORRIDA\n");
      finishGame(true);
    }
  }

  void finishGame(bool win) {
    this->win = win;
    if (win) {
      cam = 3;
      isFinished = true;
      points += 100;
    } else {
      cam = 3;
      loseLife();
      car.reset();
      int k = 0;
      for (int i = 0; i < mapRows; i++) {
        for (int j = 0; j < mapCols; j++) {
          if (mapRoad[i][j] == 2) {
            cheerio[k].reset(i, j);
            k++;
          }
        }
      }
      for (int i = 0; i < numOranges; i++) {
        orange[i].reset();
      }
    }
  }

  void loseLife() {
    lives -= 1;
    points -= 10;
    if (lives <= 0) {
      isFinished = true;
      win = false;
    }
  }

  void createFinishLine() {

    float amb1[] = {0.3f, 0.0f, 0.0f, 1.0f};
    float diff1[] = {0.8f, 0.1f, 0.1f, 1.0f};
    float spec1[] = {0.0f, 0.9f, 0.9f, 1.0f};
    float shininess = 200.0;
    float emissive[] = {0.0f, 0.0f, 0.0f, 1.0f};
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

  void renderFinishLine() {
    translate(MODEL, finishLinePos[0], finishLinePos[1], finishLinePos[2]);
    // rotate(MODEL, 180.f, 0.0f, 1.0f, 0.0f);
    scale(MODEL, finishLineDimensions[0], finishLineDimensions[1],
          finishLineDimensions[2]);
  }

  void colisionButterCheerio(float *q, float *p) {
    float butterCollisionVelocity = 0.8f;
    for (int i = 0; i < numButter; i++) {
      if (CheckCollision(p[0], p[1], q[0], q[1], butter[i].position[0],
                         butter[i].position[2], 1, 1)) {
        // printf("COLISAO_BUTTER CPX: %f, CPY: %f, p0: %f, p1: %f, q0: %f,
        // q1:%f\n", car.position[0], car.position[2], p[0],p[1],q[0],q[1]);
        car.velocity = 0;
        butter[i].position[0] += car.direction[0] * butterCollisionVelocity;
        butter[i].position[1] += car.direction[1] * butterCollisionVelocity;
        butter[i].position[2] += car.direction[2] * butterCollisionVelocity;
      }
    }
    for (int i = 0; i < numCheerios; i++) {
      if (CheckCollision(car.position[0], car.position[2], q[0], q[1],
                         cheerio[i].position[0], cheerio[i].position[2], 1,
                         1)) {
        car.velocity = 0;
        cheerio[i].position[0] += car.direction[0] * butterCollisionVelocity;
        cheerio[i].position[1] += car.direction[1] * butterCollisionVelocity;
        cheerio[i].position[2] += car.direction[2] * butterCollisionVelocity;
      }
    }
  }
};

Game game;

void timer(int value) {
  std::ostringstream oss;
  oss << CAPTION << ": " << FrameCount << " FPS @ (" << WinX << "x" << WinY
      << ")";
  std::string s = oss.str();
  glutSetWindow(WindowHandle);
  glutSetWindowTitle(s.c_str());
  FrameCount = 0;
  glutTimerFunc(1000, timer, 0);
}

void refresh(int value) {
  // PUT YOUR CODE HERE
  glutTimerFunc(1000 / 60, refresh, 0);
  glutPostRedisplay();
}

// ------------------------------------------------------------
//
// Reshape Callback Function
//

void changeSize(int w, int h) {

  float ratio;
  // Prevent a divide by zero, when window is too short
  if (h == 0)
    h = 1;
  // set the viewport to be the entire window
  glViewport(0, 0, w, h);
  // set the projection matrix
  ratio = (1.0f * w) / h;
  loadIdentity(PROJECTION);
  perspective(53.13f, ratio, 0.1f, 1000.0f);
}

void updateParticles() {
  int i;
  float h;

  /* Método de Euler de integração de eq. diferenciais ordinárias
  h representa o step de tempo; dv/dt = a; dx/dt = v; e conhecem-se os valores
  iniciais de x e v */

  // h = 0.125f;
  h = 0.033;
  if (fireworks) {

    for (i = 0; i < MAX_PARTICULAS; i++) {
      particula[i].x += (h * particula[i].vx);
      particula[i].y += (h * particula[i].vy);
      particula[i].z += (h * particula[i].vz);
      particula[i].vx += (h * particula[i].ax);
      particula[i].vy += (h * particula[i].ay);
      particula[i].vz += (h * particula[i].az);
      particula[i].life -= particula[i].fade;
    }
  }
}

void iniParticles(void) {
  GLfloat v, theta, phi;
  int i;

  for (i = 0; i < MAX_PARTICULAS; i++) {
    v = 0.8 * frand() + 0.2;
    phi = frand() * M_PI;
    theta = 2.0 * frand() * M_PI;

    particula[i].x = tableX - 5;
    particula[i].y = 5.0f;
    particula[i].z = tableZ - 5;
    particula[i].vx = v * cos(theta) * sin(phi);
    particula[i].vy = v * cos(phi);
    particula[i].vz = v * sin(theta) * sin(phi);
    particula[i].ax = 0.1f;   /* simular um pouco de vento */
    particula[i].ay = -0.15f; /* simular a aceleração da gravidade */
    particula[i].az = 0.0f;

    /* tom amarelado que vai ser multiplicado pela textura que varia entre
     * branco e preto */
    particula[i].r = 0.882f;
    particula[i].g = 0.552f;
    particula[i].b = 0.211f;

    particula[i].life = 1.0f; /* vida inicial */
    particula[i].fade =
        0.0025f; /* step de decréscimo da vida para cada iteração */
  }
}

void render_flare(FLARE_DEF *flare, int lx, int ly,
                  int *m_viewport) { // lx, ly represent the projected position
                                     // of light on viewport

  int dx, dy; // Screen coordinates of "destination"
  int px, py; // Screen coordinates of flare element
  int cx, cy;
  float maxflaredist, flaredist, flaremaxsize, flarescale, scaleDistance;
  int width, height, alpha; // Piece parameters;
  int i;
  float diffuse[4];

  GLint loc;

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  int screenMaxCoordX = m_viewport[0] + m_viewport[2] - 1;
  int screenMaxCoordY = m_viewport[1] + m_viewport[3] - 1;

  // viewport center
  cx = m_viewport[0] + (int)(0.5f * (float)m_viewport[2]) - 1;
  cy = m_viewport[1] + (int)(0.5f * (float)m_viewport[3]) - 1;

  // Compute how far off-center the flare source is.
  maxflaredist = sqrt(cx * cx + cy * cy);
  flaredist = sqrt((lx - cx) * (lx - cx) + (ly - cy) * (ly - cy));
  scaleDistance = (maxflaredist - flaredist) / maxflaredist;
  flaremaxsize = (int)(m_viewport[2] * flare->fMaxSize);
  flarescale = (int)(m_viewport[2] * flare->fScale);

  // Destination is opposite side of centre from source
  dx = clampi(cx + (cx - lx), m_viewport[0], screenMaxCoordX);
  dy = clampi(cy + (cy - ly), m_viewport[1], screenMaxCoordY);

  // Render each element. To be used Texture Unit 6

  glUniform1i(texMode_uniformId, 3); // draw modulated textured particles
  glUniform1i(tex_loc, 6);           // use TU 6

  for (i = 0; i < flare->nPieces; ++i) {
    // Position is interpolated along line between start and destination.
    px = (int)((1.0f - flare->element[i].fDistance) * lx +
               flare->element[i].fDistance * dx);
    py = (int)((1.0f - flare->element[i].fDistance) * ly +
               flare->element[i].fDistance * dy);
    px = clampi(px, m_viewport[0], screenMaxCoordX);
    py = clampi(py, m_viewport[1], screenMaxCoordY);

    // Piece size are 0 to 1; flare size is proportion of screen width; scale by
    // flaredist/maxflaredist.
    width = (int)(scaleDistance * flarescale * flare->element[i].fSize);

    // Width gets clamped, to allows the off-axis flaresto keep a good size
    // without letting the elements get big when centered.
    if (width > flaremaxsize)
      width = flaremaxsize;

    height = (int)((float)m_viewport[3] / (float)m_viewport[2] * (float)width);
    memcpy(diffuse, flare->element[i].matDiffuse, 4 * sizeof(float));
    diffuse[3] *= scaleDistance; // scale the alpha channel

    if (width > 1) {
      // send the material - diffuse color modulated with texture
      loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
      glUniform4fv(loc, 1, diffuse);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D,
                    FlareTextureArray[flare->element[i].textureId]);
      pushMatrix(MODEL);
      translate(MODEL, (float)(px - width * 0.0f), (float)(py - height * 0.0f),
                0.0f);
      scale(MODEL, (float)width, (float)height, 1);
      computeDerivedMatrix(PROJ_VIEW_MODEL);
      glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
      glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE,
                         mCompMatrix[PROJ_VIEW_MODEL]);
      computeNormalMatrix3x3();
      glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

      glBindVertexArray(myMeshes[7].vao);
      glDrawElements(myMeshes[7].type, myMeshes[7].numIndexes, GL_UNSIGNED_INT,
                     0);
      glBindVertexArray(0);
      popMatrix(MODEL);
    }
  }
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_BLEND);
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
  game.fogMaxDistance = 300;
  cam = 1;
};

void cam2() {
  // Prespective Top View Cam
  if (r < 0.1f)
    r = 0.1f;

  camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
  camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
  camY = 10;

  lookAtX = 50.0f;
  lookAtY = 0.0f;
  lookAtZ = 50.0f;
  // perspective(1.0f, 1.0f, -1, 1);

  game.fogMaxDistance = 100;

  cam = 2;
};

void cam3() {
  // prespective Car Cam
  // this values rotate the cam in x z WC
  // inclination 'i' of camera from above
  //  cam
  //      \
	//       \
	//    _ _ i\
	//          object;

  float inclination = 10.f;
  float height = 7;
  camX =
      r * sin(alpha_cam3 * 3.14f / 180.0f) * cos(beta_cam3 * 3.14f / 180.0f) +
      game.car.position[0];
  camZ =
      r * cos(alpha_cam3 * 3.14f / 180.0f) * cos(beta_cam3 * 3.14f / 180.0f) +
      game.car.position[2];
  camY = r * sin(inclination * 3.14f / 180.0f) + height;

  lookAtX = game.car.position[0];
  lookAtY = game.car.position[1];
  lookAtZ = game.car.position[2];

  game.fogMaxDistance = 50;

  cam = 3;
};

void camRearViewMirror() {
  // prespective Car Cam
  // this values rotate the cam in x z WC
  // inclination 'i' of camera from above
  //  cam
  //      \
  	//       \
  	//    _ _ i\
  	//          object;

  float inclination = 10.f;
  float height = 7;
  camX =
      r * sin(-alpha_cam3 * 3.14f / 180.0f) * cos(-beta_cam3 * 3.14f / 180.0f) +
      game.car.position[0];
  camZ =
      r * cos(-alpha_cam3 * 3.14f / 180.0f) * cos(-beta_cam3 * 3.14f / 180.0f) +
      game.car.position[2];
  camY = r * sin(inclination * 3.14f / 180.0f) + height;

  lookAtX = game.car.position[0];
  lookAtY = game.car.position[1];
  lookAtZ = game.car.position[2];

  game.fogMaxDistance = 50;
};

// Verifies if two objectes are coliding
bool CheckCollision(int oneXP, int oneYP, int oneXS, int oneYS, int twoXP,
                    int twoYP, int twoXS, int twoYS) // AABB - AABB collision
{
  int oneXP2 = oneXP - oneXS / 2;
  int oneYP2 = oneYP - oneYS / 2;

  // collision x-axis?
  bool collisionX = oneXP2 + oneXS >= twoXP && twoXP + twoXS >= oneXP2;
  // collision y-axis?
  bool collisionY = oneYP2 + oneYS >= twoYP && twoYP + twoYS >= oneYP2;
  // collision only if on both axes
  return collisionX && collisionY;
}

float *getCarCenter() {
  float *Dir = new float[2];
  float *p = new float[2];
  // float radAndgle = DegToRad(car.carBodyX / car.carBodyY);
  float angle = DegToRad(-27);
  float co = cos(angle);
  float si = sin(angle);

  Dir[0] = game.car.direction[0];
  Dir[1] = game.car.direction[2];
  float x = Dir[0];
  float y = Dir[1];
  Dir[0] = x * co + y * si;
  Dir[1] = -x * si + y * co;

  float mag = sqrt(Dir[0] * Dir[0] + Dir[1] * Dir[1]);

  Dir[0] /= mag;
  Dir[1] /= mag;

  // printf("d[0]: % f, d[1] : % f\n", car.direction[0], car.direction[1]);
  // float dist = sqrt(car.carBodyX * car.carBodyX + car.carBodyY *
  // car.carBodyY) / 2;
  float dist = 1.677;
  p[0] = game.car.position[0] + Dir[0] * dist;
  p[1] = game.car.position[2] + Dir[1] * dist;

  return p;
}

float *getCarSize() {
  float angle = game.car.rotationAngle;
  float radAngle = DegToRad(angle);
  float co = cos(radAngle);
  float si = sin(radAngle);

  float c = co * game.car.carBodyY;
  float a = si * game.car.carBodyY;

  float angle2 = 90.0 - ((int)angle % 90);
  float radAngle2 = DegToRad(angle2);
  float co2 = cos(radAngle2);
  float si2 = sin(radAngle2);

  float d = co2 * game.car.carBodyX;
  float b = si2 * game.car.carBodyY;

  float y = a + b;
  float x = c + d;
  float *q = new float[2];
  q[0] = x;
  q[1] = y;
  return q;
}

void renderFog() {
  if (isFogOn) {
    glUniform1f(fog_maxdist_uniformId, game.fogMaxDistance);
    glUniform1i(is_fog_on_uniformId, 1);
  } else {
    glUniform1i(is_fog_on_uniformId, 0);
  }
}

void renderTextures() {
  float res1[5];
  multMatrixPoint(VIEW, game.textureSl.dir, res1);
  // printf("spotlight 0 dir = { %.1f, %.1f, %.1f, %.1f}\n", res2[0], res2[1],
  // res2[2], res2[3]);
  glUniform4fv(slDirTexture_uniformId, 1, res1);

  multMatrixPoint(VIEW, game.textureSl.pos, res1);
  glUniform4fv(slPosTexture_uniformId, 1, res1);

  glUniform1f(slCutOffAngleTexture_uniformId, game.textureSl.ang);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, TextureArray[0]);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, TextureArray[1]);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, TextureArray[2]);

  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, TextureArray[3]);

  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, TextureArray[4]);

  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_CUBE_MAP, TextureArray[5]);

  // Indicar aos samplers do GLSL quais os Texture Units a serem usados
  glUniform1i(tex_loc, 0);
  glUniform1i(tex_loc1, 1);
  glUniform1i(tex_loc2, 2);
  glUniform1i(tex_loc3, 3);
  glUniform1i(tex_loc4, 4);
  glUniform1i(tex_cube_loc, 5);
}

void renderLights() {
  float res[6][4];
  float res2[4];

  for (int i = 0; i < 6; i++) {
    multMatrixPoint(VIEW, lightPos[i],
                    res[i]); // lightPos definido em World Coord so is converted
                             // to eye space
    glUniform4fv(lPos_uniformId[i], 1, res[i]);
  }

  multMatrixPoint(VIEW, directionalLight, res2);
  glUniform4fv(lDir_uniformId, 1, res2);

  multMatrixPoint(VIEW, game.car.spotlights[0]->dir, res2);
  glUniform4fv(slDir_uniformId[0], 1, res2);

  multMatrixPoint(VIEW, game.car.spotlights[1]->dir, res2);
  glUniform4fv(slDir_uniformId[1], 1, res2);

  multMatrixPoint(VIEW, game.car.spotlights[0]->pos, res2);
  glUniform4fv(slPos_uniformId[0], 1, res2);

  multMatrixPoint(VIEW, game.car.spotlights[1]->pos, res2);
  glUniform4fv(slPos_uniformId[1], 1, res2);

  glUniform1f(slCutOffAngle_uniformId, game.car.spotlights[0]->ang);
}

/*----------------------------------------------------------------
True billboarding. With the spherical version the object will
always face the camera. It requires more computational effort than
the cylindrical billboard though. The parameters camX,camY, and camZ,
are the target, i.e. a 3D point to which the object will point.
----------------------------------------------------------------*/

void l3dBillboardSphericalBegin(float *cam, float *worldPos) {

  float lookAt[3] = {0, 0, 1}, objToCamProj[3], objToCam[3], upAux[3],
        angleCosine;

  // objToCamProj is the vector in world coordinates from the local origin to
  // the camera projected in the XZ plane
  objToCamProj[0] = cam[0] - worldPos[0];
  objToCamProj[1] = 0;
  objToCamProj[2] = cam[2] - worldPos[2];

  // normalize both vectors to get the cosine directly afterwards
  normalize(objToCamProj);

  // easy fix to determine wether the angle is negative or positive
  // for positive angles upAux will be a vector pointing in the
  // positive y direction, otherwise upAux will point downwards
  // effectively reversing the rotation.

  crossProduct(lookAt, objToCamProj, upAux);

  // compute the angle
  angleCosine = dotProduct(lookAt, objToCamProj);

  // perform the rotation. The if statement is used for stability reasons
  // if the lookAt and v vectors are too close together then |aux| could
  // be bigger than 1 due to lack of precision
  if ((angleCosine < 0.99990) && (angleCosine > -0.9999))
    rotate(MODEL, acos(angleCosine) * 180 / 3.14, upAux[0], upAux[1], upAux[2]);

  // The second part tilts the object so that it faces the camera

  // objToCam is the vector in world coordinates from the local origin to the
  // camera
  objToCam[0] = cam[0] - worldPos[0];
  objToCam[1] = cam[1] - worldPos[1];
  objToCam[2] = cam[2] - worldPos[2];

  // Normalize to get the cosine afterwards
  normalize(objToCam);

  // Compute the angle between v and v2, i.e. compute the
  // required angle for the lookup vector
  angleCosine = dotProduct(objToCamProj, objToCam);

  // Tilt the object. The test is done to prevent instability when objToCam and
  // objToCamProj have a very small angle between them
  if ((angleCosine < 0.99990) && (angleCosine > -0.9999))
    if (objToCam[1] < 0)
      rotate(MODEL, acos(angleCosine) * 180 / 3.14, 1, 0, 0);
    else
      rotate(MODEL, acos(angleCosine) * 180 / 3.14, -1, 0, 0);
}

// ------------------------------------------------ ------------
//
// Render stufff
//

void renderHUD() {
  GLint loc;
  int m_viewport[4];
  glGetIntegerv(GL_VIEWPORT, m_viewport);
  pushMatrix(MODEL);
  loadIdentity(MODEL);
  pushMatrix(PROJECTION);
  loadIdentity(PROJECTION);
  pushMatrix(VIEW);
  loadIdentity(VIEW);

  ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1],
        m_viewport[1] + m_viewport[3] - 1, -1, 1);

  //  printf("1 = %d, 2 = %d, 3 = %d, 4 = %d\n", m_viewport[0], m_viewport[1],
  //  m_viewport[2], m_viewport[3]);
  string livesStr = "Lives " + to_string(game.lives);
  string pointsStr = "Points " + to_string(game.points);
  RenderText(shaderText, livesStr, m_viewport[1], m_viewport[3] - 50, 0.8f,
             0.5f, 0.8f, 0.2f);
  RenderText(shaderText, pointsStr, m_viewport[1], m_viewport[3] - 100, 0.8f,
             0.5f, 0.8f, 0.2f);

  popMatrix(PROJECTION);
  popMatrix(MODEL);
  popMatrix(VIEW);
}

void sendMatricesToOGL() {
  computeDerivedMatrix(PROJ_VIEW_MODEL);
  // glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
  glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
  computeNormalMatrix3x3();
  glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);
}

// meio buggy vou fazer so com o text

void renderStencil() {
  GLint loc;
  int m_viewport[4];

  glGetIntegerv(GL_VIEWPORT, m_viewport);
  float w = static_cast<float>(m_viewport[2]);
  float h = static_cast<float>(m_viewport[3]);

  pushMatrix(PROJECTION);
  loadIdentity(PROJECTION);
  pushMatrix(MODEL);
  loadIdentity(MODEL);
  pushMatrix(VIEW);
  loadIdentity(VIEW);
  float x;
  float y;
  if (w <= h) {
    x = -2.0 * (GLfloat)h / (GLfloat)w;
    y = 2.0 * (GLfloat)h / (GLfloat)w;
    ortho(-2.0, 2.0, -2.0 * (GLfloat)h / (GLfloat)w,
          2.0 * (GLfloat)h / (GLfloat)w, -10, 10);
  } else {
    x = -2.0 * (GLfloat)w / (GLfloat)h;
    y = 2.0 * (GLfloat)w / (GLfloat)h;

    ortho(-2.0 * (GLfloat)w / (GLfloat)h, 2.0 * (GLfloat)w / (GLfloat)h, -2.0,
          2.0, -10, 10);
  }
  // printf("x = %.3f, y = %.3f\n", x, y);

  translate(MODEL, -.75, 1.5f, 0.5f);
  scale(MODEL, 1.5, 0.6f, 0.5f);

  sendMatricesToOGL();

  glClear(GL_STENCIL_BUFFER_BIT);

  glStencilFunc(GL_NEVER, 0x1, 0x1);
  glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
  MyMesh amesh = createCube();

  glBindVertexArray(amesh.vao);
  glDrawElements(amesh.type, amesh.numIndexes, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);

  popMatrix(PROJECTION);
  popMatrix(MODEL);
  popMatrix(VIEW);
}

// 0 - paused
// 1 - gameOver
// 2 - you win
void renderOffGameMessage(int messageCase) {
  GLint loc;
  int m_viewport[4];
  glGetIntegerv(GL_VIEWPORT, m_viewport);
  pushMatrix(MODEL);
  loadIdentity(MODEL);
  pushMatrix(PROJECTION);
  loadIdentity(PROJECTION);
  pushMatrix(VIEW);
  loadIdentity(VIEW);

  ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1],
        m_viewport[1] + m_viewport[3] - 1, -1, 1);

  //  printf("1 = %d, 2 = %d, 3 = %d, 4 = %d\n", m_viewport[0], m_viewport[1],
  //  m_viewport[2], m_viewport[3]);

  switch (messageCase) {
  case (0):
    RenderText(shaderText, "PAUSED", m_viewport[2] / 2 - 133,
               m_viewport[3] / 2 - 15, 1.5f, 0.5f, 0.8f, 0.2f);
    break;
  case (1):
    RenderText(shaderText, "GAME OVER", m_viewport[2] / 2 - 201.5,
               m_viewport[3] / 2 - 15, 1.5f, 0.5f, 0.8f, 0.2f);
    break;
  case (2):
    RenderText(shaderText, "YOU WIN", m_viewport[2] / 2 - 151,
               m_viewport[3] / 2 - 15, 1.5f, 0.5f, 0.8f, 0.2f);
    break;
  }
  // printf("1 = %d, 2 = %d, 3 = %d, 4 = %d\n", m_viewport[0], m_viewport[1],
  // m_viewport[2], m_viewport[3]);

  popMatrix(PROJECTION);
  popMatrix(MODEL);
  popMatrix(VIEW);
}

void renderMeshes() {
  GLint loc;

  float pos[3];
  float camh[3] = {camX, camY, camZ};
  int objId = 0; // id of the object mesh - to be used as index of mesh:
                 // Mymeshes[objID] means the current mesh

  for (int i = 0; i < numObjects; ++i) {
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
    float torusY = 1.0f; // z in world coords
    float carBodyZ = 1.5f;
    float carBodyX = 3.0f;
    float jointCarGap = 0.5f;

    // game.car.move();
    float *position = game.car.position;

    float p;
    float q;

    switch (objId) {
    case 0:
      // table
      translate(MODEL, 0.0f, 0.0f, 0.0f);
      scale(MODEL, tableX, tableY, tableZ);

      break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      // orange
      game.orange[objId - 1].renderOrange();
      break;
    case 6:
      // car wheel torus RIGHT TOP
      translate(
          MODEL,
          position[0] + (carBodyZ)*sin(DegToRad(game.car.directionAngle)) +
              (carBodyX - jointCarGap) * cos(DegToRad(game.car.directionAngle)),
          position[1] + torusY,
          position[2] + carBodyZ * cos(DegToRad(game.car.directionAngle)) -
              (carBodyX - jointCarGap) *
                  sin(DegToRad(game.car.directionAngle)));
      rotate(MODEL, game.car.directionAngle, 0.0f, 1.0f, 0.0f);
      rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);

      break;
    case 7:
      // car wheel torus LEFT TOP
      translate(MODEL,
                position[0] + (carBodyX - jointCarGap) *
                                  cos(DegToRad(game.car.directionAngle)),
                position[1] + torusY,
                position[2] - (carBodyX - jointCarGap) *
                                  sin(DegToRad(game.car.directionAngle)));
      rotate(MODEL, game.car.directionAngle, 0.0f, 1.0f, 0.0f);
      rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);

      break;

    case 8:
      // car wheel torus RIGHT BOTTOM

      translate(
          MODEL,
          position[0] + carBodyZ * sin(DegToRad(game.car.directionAngle)) +
              (jointCarGap * cos(DegToRad(game.car.directionAngle))),
          position[1] + torusY,
          position[2] +
              (carBodyZ *
               cos(game.car.directionAngle * 3.14159265358979323846f / 180)) -
              (jointCarGap *
               sin(game.car.directionAngle * 3.14159265358979323846f / 180)));
      rotate(MODEL, game.car.directionAngle, 0.0f, 1.0f, 0.0f);
      rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);

      break;
    case 9:
      // car wheel torus LEFT BOTTOM

      translate(
          MODEL,
          position[0] + jointCarGap * cos(DegToRad(game.car.directionAngle)),
          position[1] + torusY,
          position[2] - jointCarGap * sin(DegToRad(game.car.directionAngle)));
      rotate(MODEL, game.car.directionAngle, 0.0f, 1.0f, 0.0f);
      rotate(MODEL, 90.0f, 1.0f, 0.0f, 0.0f);
      break;

    case 10:
      // car body
      translate(MODEL, position[0], position[1] + torusY - 0.2f, position[2]);
      rotate(MODEL, game.car.directionAngle, 0.0f, 1.0f, 0.0f);
      scale(MODEL, carBodyX, 0.5, carBodyZ);
      break;

    case 11:
      game.renderFinishLine();
      break;

    case 12:

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUniform1i(texMode_uniformId, 4); // draw textured quads

        pushMatrix(MODEL);
        pos[0] = 35.0;
        pos[1] = 4.0;
        pos[2] = 45.0;

        translate(MODEL, pos[0], pos[1], pos[2]);

        l3dBillboardSphericalBegin(camh, pos);

        // diffuse and ambient color are not used in the tree quads
        loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
        glUniform4fv(loc, 1, myMeshes[objId].mat.specular);
        loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
        glUniform1f(loc, myMeshes[objId].mat.shininess);

        pushMatrix(MODEL);

        computeDerivedMatrix(PROJ_VIEW_MODEL);

        glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
        glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
        computeNormalMatrix3x3();
        glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);
        glBindVertexArray(myMeshes[objId].vao);
        glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes,GL_UNSIGNED_INT, 0);
        popMatrix(MODEL);
        popMatrix(MODEL);
        glDisable(GL_BLEND);
        break;

    case 13:

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUniform1i(texMode_uniformId, 4); // draw textured quads

        pushMatrix(MODEL);
        pos[0] = 45.0;
        pos[1] = 4.0;
        pos[2] = 45.0;

        translate(MODEL, pos[0], pos[1], pos[2]);

        l3dBillboardSphericalBegin(camh, pos);

        // diffuse and ambient color are not used in the tree quads
        loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
        glUniform4fv(loc, 1, myMeshes[objId].mat.specular);
        loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
        glUniform1f(loc, myMeshes[objId].mat.shininess);

        pushMatrix(MODEL);

        computeDerivedMatrix(PROJ_VIEW_MODEL);

        glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
        glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
        computeNormalMatrix3x3();
        glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);
        glBindVertexArray(myMeshes[objId].vao);
        glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes,GL_UNSIGNED_INT, 0);
        popMatrix(MODEL);
        popMatrix(MODEL);
        glDisable(GL_BLEND);
        break;

    case 14:     // TREE WITH BILLBOARDING

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glUniform1i(texMode_uniformId, 4); // draw textured quads

      pushMatrix(MODEL);
      pos[0] = 55.0;
      pos[1] = 4.0;
      pos[2] = 45.0;

      translate(MODEL, pos[0], pos[1], pos[2]);

      l3dBillboardSphericalBegin(camh, pos);

       // diffuse and ambient color are not used in the tree quads
      loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
      glUniform4fv(loc, 1, myMeshes[objId].mat.specular);
      loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
      glUniform1f(loc, myMeshes[objId].mat.shininess);

      pushMatrix(MODEL);

      computeDerivedMatrix(PROJ_VIEW_MODEL);

      glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
      glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
      computeNormalMatrix3x3();
      glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);
      glBindVertexArray(myMeshes[objId].vao);
      glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes,GL_UNSIGNED_INT, 0);
      popMatrix(MODEL);
      popMatrix(MODEL);
      glDisable(GL_BLEND);
      break;

    case 15: // SKYBOX

      glUniform1i(texMode_uniformId, 5);

      // it won't write anything to the zbuffer; all subsequently drawn scenery
      // to be in front of the sky box.
      glDepthMask(GL_FALSE);
      glFrontFace(GL_CW); // set clockwise vertex order to mean the front

      pushMatrix(MODEL);
      pushMatrix(VIEW); // se quiser anular a translação

      //  Fica mais realista se não anular a translação da câmara
      // Cancel the translation movement of the camera - de acordo com o
      // tutorial do Antons
      mMatrix[VIEW][12] = 0.0f;
      mMatrix[VIEW][13] = 0.0f;
      mMatrix[VIEW][14] = 0.0f;

      scale(MODEL, 200.0f, 20.0f, 200.0f);
      translate(MODEL, -0.5f, -0.5f, -0.5f);

      // send matrices to OGL
      glUniformMatrix4fv(model_uniformId, 1, GL_FALSE,
                         mMatrix[MODEL]); // Transformação de modelação do cubo
                                          // unitário para o "Big Cube"
      computeDerivedMatrix(PROJ_VIEW_MODEL);
      glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE,
                         mCompMatrix[PROJ_VIEW_MODEL]);

      glBindVertexArray(myMeshes[objId].vao);
      glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes,
                     GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
      popMatrix(MODEL);
      popMatrix(VIEW);

      glFrontFace(
          GL_CCW); // restore counter clockwise vertex order to mean the front
      glDepthMask(GL_TRUE);
    };

    // send matrices to OGL
    computeDerivedMatrix(PROJ_VIEW_MODEL);
    glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
    glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE,
                       mCompMatrix[PROJ_VIEW_MODEL]);
    computeNormalMatrix3x3();
    glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

    // Render mesh
    if (objId == 0)
      glUniform1i(texMode_uniformId, 0);
    else if (objId == 11)
      glUniform1i(texMode_uniformId, 2);
    else if (objId == 12)
      glUniform1i(texMode_uniformId, 4);
    else
      glUniform1i(texMode_uniformId, 6);

    glBindVertexArray(myMeshes[objId].vao);

    if (!shader.isProgramValid()) {
      printf("Program Not Valid!\n");
      exit(1);
    }
    glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes,
                   GL_UNSIGNED_INT, 0);
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
    float torusY = 1.0f; // z in world coords
    float carBodyX = 1.5f;
    float carBodyY = 3.0f;
    float jointCarGap = -0.5f;

    // game.car.move();
    float *position = game.car.position;

    // butter
    translate(MODEL, game.butter[y].position[0], game.butter[y].position[1],
              game.butter[y].position[2]);
    // scale(MODEL, 4, 2, 5);

    // send matrices to OGL
    computeDerivedMatrix(PROJ_VIEW_MODEL);
    glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
    glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE,
                       mCompMatrix[PROJ_VIEW_MODEL]);
    computeNormalMatrix3x3();
    glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

    // Render mesh
    glUniform1i(texMode_uniformId, 6);
    glBindVertexArray(myMeshes[objId].vao);

    if (!shader.isProgramValid()) {
      printf("Program Not Valid!\n");
      exit(1);
    }
    glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes,
                   GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    popMatrix(MODEL);
    objId++;
  }

  for (int j = 0; j < mapRows; j++) {
    for (int k = 0; k < mapCols; k++) {
      if (mapRoad[j][k] != 1) {
        continue;
      }
      // printf("numR:  %d", numRoads);
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
      float torusY = 1.0f; // z in world coords
      float carBodyX = 1.5f;
      float carBodyY = 3.0f;
      float jointCarGap = -0.5f;

      // game.car.move();
      float *position = game.car.position;

      if (mapRoad[j][k] == 1) {
        translate(MODEL, roadWidth * k, 0.1f, roadWidth * j);
        scale(MODEL, roadWidth, 0.5, roadWidth);
      } else if (mapRoad[j][k] == 27) {
        translate(MODEL, roadTurn * k, 0.1f, roadTurn * j);
        scale(MODEL, roadTurn, 0.5, roadTurn);
      }
      // send matrices to OGL
      computeDerivedMatrix(PROJ_VIEW_MODEL);
      glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
      glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE,
                         mCompMatrix[PROJ_VIEW_MODEL]);
      computeNormalMatrix3x3();
      glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

      // Render mesh
      glUniform1i(texMode_uniformId, 1);
      glBindVertexArray(myMeshes[objId].vao);

      if (!shader.isProgramValid()) {
        printf("Program Not Valid!\n");
        exit(1);
      }
      glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes,
                     GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);

      popMatrix(MODEL);
      objId++;
    }
  }

  int iteCheerio = 0;

  for (int j = 0; j < mapRows; j++) {
    for (int k = 0; k < mapCols; k++) {
      if (mapRoad[j][k] != 2) {
        continue;
      }

      // printf("numR:  %d", numRoads);
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

      //game.car.move();
      float *position = game.car.position;

      if (mapRoad[j][k] == 2) {
        translate(MODEL, game.cheerio[iteCheerio].position[0],
                  game.cheerio[iteCheerio].position[1],
                  game.cheerio[iteCheerio].position[2]);
        scale(MODEL, roadWidth - 2, 1.5, roadWidth - 2);
        iteCheerio++;
      }

      // send matrices to OGL
      computeDerivedMatrix(PROJ_VIEW_MODEL);
      glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
      glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE,
                         mCompMatrix[PROJ_VIEW_MODEL]);
      computeNormalMatrix3x3();
      glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

      // Render mesh
      glUniform1i(texMode_uniformId, 1);
      glBindVertexArray(myMeshes[objId].vao);

      if (!shader.isProgramValid()) {
        printf("Program Not Valid!\n");
        exit(1);
      }
      glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes,
                     GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);

      popMatrix(MODEL);
      objId++;
    }
  }
}

void renderPawn() {

  float amb[] = {0.2f, 0.15f, 0.1f, 1.0f};
  float diff[] = {0.8f, 0.6f, 0.4f, 1.0f};
  float spec[] = {0.8f, 0.8f, 0.8f, 1.0f};
  float emissive[] = {0.0f, 0.0f, 0.0f, 1.0f};
  float shininess = 100.0f;
  int texcount = 0;
  GLint loc;
  // create geometry and VAO of the pawn
  MyMesh amesh = createPawn();
  memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
  memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
  memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
  memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
  amesh.mat.shininess = shininess;
  amesh.mat.texCount = texcount;

  loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
  glUniform4fv(loc, 1, amesh.mat.ambient);
  loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
  glUniform4fv(loc, 1, amesh.mat.diffuse);
  loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
  glUniform4fv(loc, 1, amesh.mat.specular);
  loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
  glUniform1f(loc, amesh.mat.shininess);
  pushMatrix(MODEL);
  // send matrices to OGL
  computeDerivedMatrix(PROJ_VIEW_MODEL);
  glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
  glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
  computeNormalMatrix3x3();
  glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);
}
// ------------------------------------------------ ------------
//
// Render stufff
//

void renderScene(void) {

  GLint loc;
  float particle_color[4];
  FrameCount++;
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // load identity matrices
  loadIdentity(VIEW);
  loadIdentity(MODEL);

  // set the camera using a function similar to gluLookAt

  lookAt(camX, camY, camZ, lookAtX, lookAtY, lookAtZ, 0, 1, 0);

  // use our shader
  glUseProgram(shader.getProgramIndex());
  game.updateTextureSl();

  renderLights();
  renderFog();
  renderTextures();
  // renderStencil();

  if (!game.isFinished) {
    if (!game.isGamePaused) {
      keyOperations();
      game.car.move();
      float *p = getCarCenter();
      float *q = getCarSize();
      for (int j = 0; j < numButter + numOranges; j++) {
        if (j < numOranges) {
          if (CheckCollision(game.car.position[0], game.car.position[2], q[0],
                             q[1], game.orange[j].position[0],
                             game.orange[j].position[2], 2, 2)) {
            printf("COLISAO_ORANGE\n");
            game.finishGame(false);
          }
        }
      }
      if (!game.car.isOnTable()) {
        game.finishGame(false);
      }
      game.checkFinish(q, p);
      game.colisionButterCheerio(q, p);
      for (int i = 0; i < numOranges; i++) {
        game.orange[i].animateOrange();
      }
    }
  }
  // camRearViewMirror();

  // glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);
  // draw
  renderMeshes();

  // Render text (bitmap fonts) in screen coordinates. So use ortoghonal
  // projection with viewport coordinates.
  if (game.isFinished && game.win) {
      if (fireworks == 0) {
          iniParticles();
          fireworks = 1;
      }
  }

  if (fireworks) {


    printf("entrei \n");

    updateParticles();

    // draw fireworks particles
    int objId = 20; // quad for particle

    glBindTexture(GL_TEXTURE_2D,
                  TextureArray[3]); // particle.tga associated to TU3

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDepthMask(GL_FALSE); // Depth Buffer Read Only

    glUniform1i(texMode_uniformId, 3); // draw modulated textured particles

    for (int i = 0; i < MAX_PARTICULAS; i++) {
      if (particula[i].life > 0.0f) /* só desenha as que ainda estão vivas */
      {

        /* A vida da partícula representa o canal alpha da cor. Como o blend
        está activo a cor final é a soma da cor rgb do fragmento multiplicada
        pelo alpha com a cor do pixel destino */

        particle_color[0] = particula[i].r;
        particle_color[1] = particula[i].g;
        particle_color[2] = particula[i].b;
        particle_color[3] = particula[i].life;

        // send the material - diffuse color modulated with texture
        loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
        glUniform4fv(loc, 1, particle_color);

        pushMatrix(MODEL);
        translate(MODEL, particula[i].x, particula[i].y, particula[i].z);

        // send matrices to OGL
        computeDerivedMatrix(PROJ_VIEW_MODEL);
        glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
        glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE,
                           mCompMatrix[PROJ_VIEW_MODEL]);
        computeNormalMatrix3x3();
        glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

        glBindVertexArray(myMeshes[objId].vao);
        glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes,
                       GL_UNSIGNED_INT, 0);
        popMatrix(MODEL);
      } else
        dead_num_particles++;
    }

    glDepthMask(GL_TRUE); // make depth buffer again writeable

    if (dead_num_particles == MAX_PARTICULAS) {
      fireworks = 0;
      dead_num_particles = 0;
      printf("All particles dead\n");
    }
  }

  if (flareEffect) {

    int flarePos[2];
    int m_viewport[4];
    glGetIntegerv(GL_VIEWPORT, m_viewport);

    pushMatrix(MODEL);
    loadIdentity(MODEL);
    computeDerivedMatrix(PROJ_VIEW_MODEL); // pvm to be applied to lightPost.
                                           // pvm is used in project function

    if (!project(lightPos[0], lightScreenPos, m_viewport))
      printf(
          "Error in getting projected light in screen\n"); // Calculate the
                                                           // window Coordinates
                                                           // of the light
                                                           // position: the
                                                           // projected position
                                                           // of light on
                                                           // viewport
    flarePos[0] = clampi((int)lightScreenPos[0], m_viewport[0],
                         m_viewport[0] + m_viewport[2] - 1);
    flarePos[1] = clampi((int)lightScreenPos[1], m_viewport[1],
                         m_viewport[1] + m_viewport[3] - 1);
    popMatrix(MODEL);

    // viewer looking down at  negative z direction
    pushMatrix(PROJECTION);
    loadIdentity(PROJECTION);
    pushMatrix(VIEW);
    loadIdentity(VIEW);
    ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1],
          m_viewport[1] + m_viewport[3] - 1, -1, 1);
    render_flare(&AVTflare, flarePos[0], flarePos[1], m_viewport);
    popMatrix(PROJECTION);
    popMatrix(VIEW);
  }

  // Render text (bitmap fonts) in screen coordinates. So use ortoghonal
  // projection with viewport coordinates.
  glDisable(GL_DEPTH_TEST);
  // the glyph contains background colors and non-transparent for the actual
  // character pixels. So we use the blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  int m_viewport[4];
  glGetIntegerv(GL_VIEWPORT, m_viewport);

  // viewer at origin looking down at  negative z direction
  pushMatrix(MODEL);
  loadIdentity(MODEL);
  pushMatrix(PROJECTION);
  loadIdentity(PROJECTION);
  pushMatrix(VIEW);
  loadIdentity(VIEW);

  switch (cam) {
  case 1:
    ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1],
          m_viewport[1] + m_viewport[3] - 1, -1, 1);
    break;
  case 2:
    perspective(120.0f, 1.33f, 15.0, 120.0);
    break;
  case 3:
    perspective(120.0f, 1.33f, 15.0, 120.0);
    cam3();
    break;
  }

  popMatrix(PROJECTION);
  popMatrix(VIEW);
  popMatrix(MODEL);

  renderHUD();

  if (game.isGamePaused) {
    // renderStencil();
    // glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);
    renderOffGameMessage(0);
  } else if (game.isFinished && game.win) {
    renderOffGameMessage(2);
  } else if (game.isFinished && !game.win) {
    renderOffGameMessage(1);
  }

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glutSwapBuffers();
}

// ------------------------------------------------------------
//
// Events from the Keyboard
//

void processKeys(unsigned char key, int xx, int yy) {
  switch (key) {
  case 27:
    glutLeaveMainLoop();
    break;
  case 'c': // POINTING LIGHTS --> LUZES PARA ILUMINAR A MESA
    // printf("Camera Spherical Coordinates (%f, %f, %f)\n", alpha, beta, r);
    if (isPointLightsOn) {
      for (int i = 0; i < 6; i++) {
        lightPos[i][3] = 0.0f;
      }
      isPointLightsOn = false;
    } else {
      for (int i = 0; i < 6; i++) {
        lightPos[i][3] = 1.0f;
      }
      isPointLightsOn = true;
    }
    break;
  case 'm':
    glEnable(GL_MULTISAMPLE);
    break;
  case 'n': /*glDisable(GL_MULTISAMPLE);*/
    // DIRECTIONAL LIGHT --> ILUMINACAO GERAL TIPO DIA E NOITE
    if (isDirectionalLightOn) {
      directionalLight = nolightDir;
      isDirectionalLightOn = false;
    } else {
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
    } else {
      for (int i = 0; i < 2; i++) {
        game.car.spotlights[i]->pos[3] = 1.0f;
      }
      isSpotLightsOn = true;
    }
    break;

  case 'l':
    // SPOTLIGHTS --> ILUMINACAO COMO SE FOSSE AS LUZES FRONTEIRAS DO CARRO
    if (game.isTextureSpOn) {
      game.textureSl.pos[3] = 0.0f;
      game.isTextureSpOn = false;
    } else {
      game.textureSl.pos[3] = 1.0f;
      game.isTextureSpOn = true;
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
    // ESTA ATRIBUICAO DE VALORES E FEITA AQUI POR CAUSA TO MOVIMENTO DE CAMERA
    // ATRAVES DO RATO
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

  float *direction = game.car.direction;

  if (keyStates[27]) {
    glutLeaveMainLoop();
  }

  if (keyStates['q']) {
    // move forward

    if (game.car.isForward ==
        false) { // if car wants to go forward when it's reversing
      game.car.changeVelocity(-0.00032);
      if (game.car.velocity < 0) {
        game.car.velocity = 0;
        for (int i = 0; i < 3; i++) {
          direction[i] = -direction[i];
        }
        game.car.isForward = true;
      }
    } else {
      game.car.changeVelocity(0.00008);
      if (game.car.velocity > game.car.maxVelocity) {
        game.car.velocity = game.car.maxVelocity;
      }
    }
    game.car.move();
  }

  if (keyStates['a']) {
    // move backwards
    if (game.car.isForward ==
        true) { // if car wants to go backward (similar to braking)
      game.car.changeVelocity(-0.00032);
      if (game.car.velocity < 0) {
        game.car.velocity = 0;
        for (int i = 0; i < 3; i++) {
          direction[i] = -direction[i];
        }
        game.car.isForward = false;
      }
    } else {
      game.car.changeVelocity(0.00008);
      if (game.car.velocity > game.car.maxVelocity) {
        game.car.velocity = game.car.maxVelocity;
      }
    }
    game.car.move();
  }

  if (keyStates['p'] && game.car.velocity > 0) {
    // move left
    if (!keyStates['q'] && !keyStates['a']) {
      game.car.changeVelocity(-0.00016);
      if (game.car.velocity < 0) {
        game.car.velocity = 0;
      }
    }
    if (game.car.isForward == true && game.car.velocity > 0) {
      game.car.changeDirection(true);
    } else if (game.car.isForward == false && game.car.velocity > 0) {
      game.car.changeDirection(false);
    }
  }

  if (keyStates['o'] && game.car.velocity > 0) {
    // move left
    if (!keyStates['q'] && !keyStates['a']) {
      game.car.changeVelocity(-0.00008);
      if (game.car.velocity < 0) {
        game.car.velocity = 0;
      }
    }
    if (game.car.isForward == true && game.car.velocity > 0) {
      game.car.changeDirection(false);
    } else if (game.car.isForward == false && game.car.velocity > 0) {
      game.car.changeDirection(true);
    }
  }

  for (int i = 0; i < 256; i++) {
    if (keyStates[i] == true) {
      return;
    }
  }

  game.car.changeVelocity(-0.00016);
  if (game.car.velocity < 0) {
    game.car.velocity = 0;
  } else if (game.car.velocity > game.car.maxVelocity) {
    game.car.velocity = game.car.maxVelocity;
  }
  game.car.updateSpotlights();
}

void keyPressed(unsigned char key, int xx, int yy) {
  if (key == 's') {
    keyStates[key] = true;

    // this is necessary because other wise there can be a case where
    // you pause the game while clicking at the same time on one of theese keys
    // and when that hapens when you unpause the game does not process the
    // keydown so you get a key stuck in true for ever unless you click it
    // again.
    keyStates['q'] = false;
    keyStates['a'] = false;
    keyStates['p'] = false;
    keyStates['o'] = false;
  } else if (!game.isGamePaused) {
    keyStates[key] = true;
  }
}

void keyUp(unsigned char key, int x, int y) {
    if (key == 's') {
        // PAUSE
        if (game.isGamePaused) {
            game.isGamePaused = false;
        }
        else {
            game.isGamePaused = true;
        }
        keyStates[key] = false;
    }

    else if (!game.isGamePaused) {
        switch (key) {

        case 'c': // POINTING LIGHTS --> LUZES PARA ILUMINAR A MESA
          // printf("Camera Spherical Coordinates (%f, %f, %f)\n", alpha, beta, r);
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
        case 'f':
            // FOG
            if (isFogOn) {
                isFogOn = false;
            }
            else {
                isFogOn = true;
            }
            break;
        case 'r':
            if (game.isFinished && !game.win) {
                game.isFinished = false;
                game.lives = 5;
                game.points = 0;
            }
            else if (game.isFinished && game.win) {
                game.isFinished = false;
                game.lives += 1;
                game.points += 10;
                game.finishGame(false);
                fireworks = 0;
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
            // ESTA ATRIBUICAO DE VALORES E FEITA AQUI POR CAUSA TO MOVIMENTO DE
            // CAMERA ATRAVES DO RATO
            alpha = -90.0f, beta = 0.0f;
            cam3();

            break;
        case '4': // DEBUG MOVIMENTO DO CARRO
            for (int i = 0; i < 3; i++) {
                cout << "\ndir " << game.car.direction[i];
            }
            cout << "\nis Forward  " << game.car.isForward;

            break;

        case 'e':
            fireworks = 1;
            iniParticles();
            break;

        case 'l':
            if (flareEffect)
                flareEffect = false;
            else
                flareEffect = true;
            break;

        default:
            cout << "tecla carregada = " << key;
            break;
        }
        keyStates[key] = false;
    }
}
  // ------------------------------------------------------------
  //
  // Mouse Events
  //

  void processMouseButtons(int button, int state, int xx, int yy) {
    // start tracking the mouse
    if (state == GLUT_DOWN) {
      startX = xx;
      startY = yy;
      if (button == GLUT_LEFT_BUTTON)
        tracking = 1;
      else if (button == GLUT_RIGHT_BUTTON)
        tracking = 2;
    }

    // stop tracking the mouse
    else if (state == GLUT_UP) {
      if (tracking == 1) {
        alpha -= (xx - startX);
        beta += (yy - startY);
      } else if (tracking == 2) {
        /*		r += (yy - startY) * 0.01f;
                        if (r < 0.1f)
                                r = 0.1f;
        */
      }
      tracking = 0;
    }
  }

  // Track mouse motion while buttons are pressed

  void processMouseMotion(int xx, int yy) {

    int deltaX, deltaY;
    float alphaAux = 0;
    float betaAux = 0;
    float rAux;
    float slowDown = 0.2f;
    deltaX = (-xx + startX) * slowDown;
    deltaY = (yy - startY) * slowDown;

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
      // float inclination = 55.f;
      // float hight = 10;
      alpha_cam3 = alphaAux;
      // beta_cam3 = betaAux;
      cam3();
      break;
    }
    // camX = rAux * sin(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f /
    // 180.0f); camZ = rAux * cos(alphaAux * 3.14f / 180.0f) * cos(betaAux
    // * 3.14f / 180.0f); camY = rAux *
    // sin(betaAux * 3.14f / 180.0f);

    //  uncomment this if not using an idle or refresh func
    //	glutPostRedisplay();
  }

  void mouseWheel(int wheel, int direction, int x, int y) {

    r += direction * 0.1f;
    if (r < 0.1f)
      r = 0.1f;

    camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
    camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
    camY = r * sin(beta * 3.14f / 180.0f);

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
    glBindFragDataLocation(shader.getProgramIndex(), 0, "colorOut");
    glBindAttribLocation(shader.getProgramIndex(), VERTEX_COORD_ATTRIB,
                         "position");
    glBindAttribLocation(shader.getProgramIndex(), NORMAL_ATTRIB, "normal");
    glBindAttribLocation(shader.getProgramIndex(), TEXTURE_COORD_ATTRIB,
                         "texCoord");

    glLinkProgram(shader.getProgramIndex());

    texMode_uniformId = glGetUniformLocation(
        shader.getProgramIndex(), "texMode"); // different modes of texturing
    pvm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_pvm");
    vm_uniformId =
        glGetUniformLocation(shader.getProgramIndex(), "m_viewModel");
    normal_uniformId =
        glGetUniformLocation(shader.getProgramIndex(), "m_normal");
    model_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_Model");

    lPos_uniformId[0] =
        glGetUniformLocation(shader.getProgramIndex(), "l_pos[0]");
    lPos_uniformId[1] =
        glGetUniformLocation(shader.getProgramIndex(), "l_pos[1]");
    lPos_uniformId[2] =
        glGetUniformLocation(shader.getProgramIndex(), "l_pos[2]");
    lPos_uniformId[3] =
        glGetUniformLocation(shader.getProgramIndex(), "l_pos[3]");
    lPos_uniformId[4] =
        glGetUniformLocation(shader.getProgramIndex(), "l_pos[4]");
    lPos_uniformId[5] =
        glGetUniformLocation(shader.getProgramIndex(), "l_pos[5]");

    lDir_uniformId = glGetUniformLocation(shader.getProgramIndex(), "l_dir");

    slPos_uniformId[0] =
        glGetUniformLocation(shader.getProgramIndex(), "sl_pos[0]");
    slPos_uniformId[1] =
        glGetUniformLocation(shader.getProgramIndex(), "sl_pos[1]");

    slDir_uniformId[0] =
        glGetUniformLocation(shader.getProgramIndex(), "sl_dir[0]");
    slDir_uniformId[1] =
        glGetUniformLocation(shader.getProgramIndex(), "sl_dir[1]");

    slCutOffAngle_uniformId =
        glGetUniformLocation(shader.getProgramIndex(), "sl_cut_off_ang");

    slPosTexture_uniformId =
        glGetUniformLocation(shader.getProgramIndex(), "sl_pos_texture");
    slDirTexture_uniformId =
        glGetUniformLocation(shader.getProgramIndex(), "sl_dir_texture");
    slCutOffAngleTexture_uniformId = glGetUniformLocation(
        shader.getProgramIndex(), "sl_cut_off_ang_texture");

    is_fog_on_uniformId =
        glGetUniformLocation(shader.getProgramIndex(), "is_fog_on");
    fog_maxdist_uniformId =
        glGetUniformLocation(shader.getProgramIndex(), "fog_maxdist");

    tex_loc = glGetUniformLocation(shader.getProgramIndex(), "texmap");
    tex_loc1 = glGetUniformLocation(shader.getProgramIndex(), "texmap1");
    tex_loc2 = glGetUniformLocation(shader.getProgramIndex(), "texmap2");
    tex_loc3 = glGetUniformLocation(shader.getProgramIndex(), "texmap3");
    tex_loc4 = glGetUniformLocation(shader.getProgramIndex(), "texmap4");
    tex_cube_loc = glGetUniformLocation(shader.getProgramIndex(), "cubeMap");

    printf("InfoLog for Per Fragment Phong Lightning Shader\n%s\n\n",
           shader.getAllInfoLogs().c_str());

    // Shader for bitmap Text
    shaderText.init();
    shaderText.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/text.vert");
    shaderText.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/text.frag");

    glLinkProgram(shaderText.getProgramIndex());
    printf("InfoLog for Text Rendering Shader\n%s\n\n",
           shaderText.getAllInfoLogs().c_str());

    //// Shader for models
    // shaderGlobal.init();
    // shaderGlobal.loadShader(VSShaderLib::VERTEX_SHADER,
    // "shaders/pointlight.vert");
    // shaderGlobal.loadShader(VSShaderLib::FRAGMENT_SHADER,
    // "shaders/pointlight.frag");

    //// set semantics for the shader variables
    // glBindFragDataLocation(shaderGlobal.getProgramIndex(), 0, "colorOut");
    // glBindAttribLocation(shaderGlobal.getProgramIndex(), VERTEX_COORD_ATTRIB,
    // "position"); glBindAttribLocation(shaderGlobal.getProgramIndex(),
    // NORMAL_ATTRIB, "normal");
    ////glBindAttribLocation(shader.getProgramIndex(), TEXTURE_COORD_ATTRIB,
    ///"texCoord");

    // glLinkProgram(shaderGlobal.getProgramIndex());

    // pvm_uniformId = glGetUniformLocation(shaderGlobal.getProgramIndex(),
    // "m_pvm"); vm_uniformId =
    // glGetUniformLocation(shaderGlobal.getProgramIndex(), "m_viewModel");
    // normal_uniformId = glGetUniformLocation(shaderGlobal.getProgramIndex(),
    // "m_normal"); lDir_uniformId =
    // glGetUniformLocation(shaderGlobal.getProgramIndex(), "l_dir"); tex_loc =
    // glGetUniformLocation(shaderGlobal.getProgramIndex(), "texmap"); tex_loc1
    // = glGetUniformLocation(shaderGlobal.getProgramIndex(), "texmap1");
    // tex_loc2 = glGetUniformLocation(shaderGlobal.getProgramIndex(),
    // "texmap2");

    return (shader.isProgramLinked() &&
            shaderText.isProgramLinked() /*&& shaderGlobal.isProgramLinked()*/);
  }

  // ------------------------------------------------------------
  //
  // Model loading and OpenGL setup
  //

  void init() {
      srand(static_cast<unsigned>(time(0)));

      MyMesh amesh;

      for (int i = 0; i < 256; i++) {
          keyStates[i] = false;
      }

      /* Initialization of DevIL */
      if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION) {
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

      glGenTextures(5, TextureArray);
      Texture2D_Loader(TextureArray, "lightwood.tga", 0);
      Texture2D_Loader(TextureArray, "road.jpg", 1);
      Texture2D_Loader(TextureArray, "finishline.jpg", 2);
      Texture2D_Loader(TextureArray, "particle.tga", 3);
      Texture2D_Loader(TextureArray, "tree.tga", 4);

      // Flare elements textures
      glGenTextures(5, FlareTextureArray);
      Texture2D_Loader(FlareTextureArray, "crcl.tga", 0);
      Texture2D_Loader(FlareTextureArray, "flar.tga", 1);
      Texture2D_Loader(FlareTextureArray, "hxgn.tga", 2);
      Texture2D_Loader(FlareTextureArray, "ring.tga", 3);
      Texture2D_Loader(FlareTextureArray, "sun.tga", 4);

      const char* filenames[] = { "posx.jpg", "negx.jpg", "posy.jpg",
                                 "negy.jpg", "posz.jpg", "negz.jpg" };
      TextureCubeMap_Loader(TextureArray, filenames, 5);

      numRoads = CalcRoads();      // 176
      printf("%d\n", numCheerios); // 141

      numRoads = CalcRoads();
      // numCheerios = CalcCheerios();

      float amb[] = { 0.2f, 0.15f, 0.1f, 1.0f };
      float diff[] = { 0.8f, 0.6f, 0.4f, 1.0f };
      float spec[] = { 0.8f, 0.8f, 0.8f, 1.0f };
      float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
      float shininess = 100.0f;
      int texcount = 0;

      //// create geometry and VAO of the pawn
      // amesh = createPawn();
      // memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
      // memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
      // memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
      // memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
      // amesh.mat.shininess = shininess;
      // amesh.mat.texCount = texcount;
      // myMeshes.push_back(amesh);

      // float amb1[]= {0.3f, 0.0f, 1.0f, 0.0f};
      // float diff1[] = {0.8f, 0.1f, 0.1f, 1.0f};
      // float spec1[] = {0.9f, 0.9f, 0.9f, 1.0f};
      // shininess=100.0;

      //// create geometry and VAO of the cylinder
      // amesh = createCylinder(1.5f, 0.5f, 20);
      // memcpy(amesh.mat.ambient, amb1, 4 * sizeof(float));
      // memcpy(amesh.mat.diffuse, diff1, 4 * sizeof(float));
      // memcpy(amesh.mat.specular, spec1, 4 * sizeof(float));
      // memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
      // amesh.mat.shininess = shininess;
      // amesh.mat.texCount = texcount;
      // myMeshes.push_back(amesh);

      //// create geometry and VAO of the
      // amesh = createCone(1.5f, 0.5f, 20);
      // memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
      // memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
      // memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
      // memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
      // amesh.mat.shininess = shininess;
      // amesh.mat.texCount = texcount;
      // myMeshes.push_back(amesh);

      // create geometry and VAO of the cube
      // TABLE  id = 0
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
      // ORANGE  id = 1 to 5
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
          // id = 6 to 9
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

      // id = 10
      amesh = createCube();
      memcpy(amesh.mat.ambient, amb4, 4 * sizeof(float));
      memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
      memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
      memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
      amesh.mat.shininess = shininess;
      amesh.mat.texCount = texcount;
      myMeshes.push_back(amesh);
      numObjects++;

      // id = 11
      game.createFinishLine();

      // create geometry and VAO of the quad for trees id = 12
      // tree specular color
      float tree_spec[] = { 0.2f, 0.2f, 0.2f, 1.0f };
      float tree_shininess = 30.0f;

      for (int i = 0; i < 3; i++){
          amesh = createQuad(6, 6);
          memcpy(amesh.mat.specular, tree_spec, 4 * sizeof(float));
          memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
          amesh.mat.shininess = tree_shininess;
          amesh.mat.texCount = texcount;
          myMeshes.push_back(amesh);
          numObjects++;
  }

    // create geometry and VAO of the cube for skybox, objId=13;
    amesh = createCube();
    memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
    memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
    memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
    memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
    amesh.mat.shininess = shininess;
    amesh.mat.texCount = texcount;
    myMeshes.push_back(amesh);
    numObjects++;

    // Butter = 15 to 19
    for (int i = 0; i < numButter; i++) {
      amesh = createCube();
      memcpy(amesh.mat.ambient, amb4, 4 * sizeof(float));
      memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
      memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
      memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
      amesh.mat.shininess = shininess;
      amesh.mat.texCount = texcount;
      myMeshes.push_back(amesh);
      // numObjects++;
    };

    // ROAD 20 to 195
    for (int i = 0; i < numRoads; i++) {
      amesh = createCube();
      memcpy(amesh.mat.ambient, amb1, 4 * sizeof(float));
      memcpy(amesh.mat.diffuse, diff1, 4 * sizeof(float));
      memcpy(amesh.mat.specular, spec1, 4 * sizeof(float));
      memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
      amesh.mat.shininess = 500;
      amesh.mat.texCount = texcount;
      myMeshes.push_back(amesh);
      // numObjects++;
    };

    // Cheerios 196 to 336
    for (int i = 0; i < numCheerios; i++) {

      amesh = createTorus(0.1f, 0.5f, 20, 20);
      memcpy(amesh.mat.ambient, amb3, 4 * sizeof(float));
      memcpy(amesh.mat.diffuse, diff2, 4 * sizeof(float));
      memcpy(amesh.mat.specular, spec2, 4 * sizeof(float));
      memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
      amesh.mat.shininess = shininess;
      amesh.mat.texCount = texcount;
      myMeshes.push_back(amesh);
      // numObjects++;
    };
    int iteCheerio = 0;
    for (int j = 0; j < mapRows; j++) {
      for (int k = 0; k < mapCols; k++) {
        if (mapRoad[j][k] == 2) {
          game.cheerio[iteCheerio].position[0] = roadWidth * k + roadWidth / 2;
          game.cheerio[iteCheerio].position[1] = 1.0f;
          game.cheerio[iteCheerio].position[2] = roadWidth * j + roadWidth / 2;
          iteCheerio++;
        }
      }
    }

    // create geometry and VAO of the quad for particles id = 338
    amesh = createQuad(2, 2);
    amesh.mat.texCount = texcount;
    myMeshes.push_back(amesh);
    // numObjects++;

    // create geometry and VAO of the quad for flare elements objId = 14
    amesh = createQuad(1, 1);
    myMeshes.push_back(amesh);

    // Load flare from file
    loadFlareFile(&AVTflare, "flare.txt");

    // some GL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // set counter-clockwise vertex order to mean the front
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glClearStencil(0x0);
    glEnable(GL_STENCIL_TEST);
  }

  // ------------------------------------------------------------
  //
  // Main function
  //

  int main(int argc, char **argv) {

    //  GLUT initialization
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL |
                        GLUT_MULTISAMPLE);

    glutInitContextVersion(4, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);

    glutInitWindowPosition(100, 100);
    glutInitWindowSize(WinX, WinY);
    WindowHandle = glutCreateWindow(CAPTION);

    //  Callback Registration
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);

    glutTimerFunc(0, timer, 0);
    // glutIdleFunc(renderScene);  // Use it for maximum performance
    glutTimerFunc(0, refresh, 0); // use it to to get 60 FPS whatever
    // glutIdleFunc(renderScene);  // Use it for maximum performance

    //	Mouse and Keyboard Callbacks

    glutKeyboardFunc(keyPressed);
    glutKeyboardUpFunc(keyUp);
    glutMouseFunc(processMouseButtons);
    glutMotionFunc(processMouseMotion);
    // glutMouseWheelFunc ( mouseWheel ) ;

    //	return from main loop
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,
                  GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    //	Init GLEW
    glewExperimental = GL_TRUE;
    glewInit();

    printf("Vendor: %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    if (!setupShaders())
      return (1);

    init();

    //  GLUT main loop
    glutMainLoop();

    return (0);
  }

  unsigned int getTextureId(char *name) {
    int i;

    for (i = 0; i < NTEXTURES; ++i) {
      if (strncmp(name, flareTextureNames[i], strlen(name)) == 0)
        return i;
    }
    return -1;
  }

  void loadFlareFile(FLARE_DEF * flare, char *filename) {
    int n = 0;
    FILE *f;
    char buf[256];
    int fields;

    memset(flare, 0, sizeof(FLARE_DEF));

    f = fopen(filename, "r");
    if (f) {
      fgets(buf, sizeof(buf), f);
      sscanf(buf, "%f %f", &flare->fScale, &flare->fMaxSize);

      while (!feof(f)) {
        char name[8] = {
            '\0',
        };
        double dDist = 0.0, dSize = 0.0;
        float color[4];
        int id;

        fgets(buf, sizeof(buf), f);
        fields = sscanf(buf, "%4s %lf %lf ( %f %f %f %f )", name, &dDist,
                        &dSize, &color[3], &color[0], &color[1], &color[2]);
        if (fields == 7) {
          for (int i = 0; i < 4; ++i)
            color[i] = clamp(color[i] / 255.0f, 0.0f, 1.0f);
          id = getTextureId(name);
          if (id < 0)
            printf("Texture name not recognized\n");
          else
            flare->element[n].textureId = id;
          flare->element[n].fDistance = (float)dDist;
          flare->element[n].fSize = (float)dSize;
          memcpy(flare->element[n].matDiffuse, color, 4 * sizeof(float));
          ++n;
        }
      }

      flare->nPieces = n;
      fclose(f);
    } else
      printf("Flare file opening error\n");
  }
