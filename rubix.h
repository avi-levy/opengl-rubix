// gcc -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -Wall rubix.c -o rubix.gcc
// clang -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -Wall rubix.c -o rubix.clang

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h> // for sqrt
#include <string.h> // for memcpy, memset

#define WINDOW_TITLE "RubixCube"
#define LINE_WIDTH 10
#define DEGREES_IN_CIRCLE 360

typedef enum Face {
  Blue,
  White,
  Red,
  Green,
  Yellow,
  Orange,
  Faces // number of faces (6)
} Face;

//                                 Blue     White    Red      Green     Yellow   Orange    Black
const float colors[Faces+1][3] = {{0,0,1}, {1,1,1}, {1,0,0}, {0,.5,0}, {1,1,0}, {1,.5,0}, {0,0,0}};

// Standard ("Singmaster") Rubik's cube face notation
const int FaceToKey[Faces] = {
  GLFW_KEY_R, // starts blue
  GLFW_KEY_U, // starts white
  GLFW_KEY_F, // starts red
  GLFW_KEY_L, // starts green
  GLFW_KEY_D, // starts yellow
  GLFW_KEY_B, // starts orange
};

Face *data;

// There are two faces per axis
typedef enum axis {
  Right, // R (+) and L (-)
  Upper, // U (+) and D (-)
  Front, // F (+) and B (-)
  Axes // number of axes (3)
} Axis;

#define FACES_PER_AXIS (Faces / Axes)
#define MATRIX_SIZE ((Axes + 1) * (Axes + 1))
#define CORNERS (1 << Axes)
#define BORDER (1 << (Axes - 1))

// camera view directed at (1, 1, 1)
float isometric[MATRIX_SIZE] = {
  -1  , 1   , 1 , 0   ,
  0   , -2  , 1 , 0   ,
  1   , 1   , 1 , 0   ,
  0   , 0   , 0 , -1  ,
};

typedef enum orientation {
  Counterclockwise,
  Clockwise
} Orientation;

typedef enum action {
  Twist, // face rotation
  Turn, // entire cube rotation
  Actions // number of actions (2)
} Action;

typedef struct move {
  Face f;
  Orientation o;
  float angle;
} Move;

typedef struct faceGeom {
  Face adj[Faces-FACES_PER_AXIS]; // number of adjacent faces (4)
  float offset;
  float vector[Axes];
} FaceGeom;

typedef struct corner {
  int index;
  int orientation;
} Corner;

typedef struct phys {
  FaceGeom faces[Faces];
  unsigned int cubies; // number of subcubes per edge (n)
  unsigned int cubiesPerFace; // n^2
  float spacing; // gap between adjacent cubies divided by cubie size
  float boundingBox; // half the cube side length
  float scale; // zoom applied to viewport to accommodate the whole cube
} Phys;

typedef struct animationState {
  Move actions[Actions]; // records progress of pending actions
  Corner corner; // records pending view corner centering operation
} AnimationState;

#define USAGE                                                           \
  "command line usage: rubix <n>\n"                                     \
  "\trenders an interactive Rubik's cube with n cubies per side\n\n"    \
  "Keyboard controls the following operations:\n"                       \
  "\tClockwise face twists (R, U, F, L, D, B)\n"                        \
  "\tClockwise cube turns (Shift + corresponding face twist key)\n"     \
  "\tCounterclockwise by adding Control\n"                              \
  "\tCenter view on corner (1 - 8, Shift/Control to set orientation)\n"

GLFWwindow* prepareGlfw();
bool initCube(const unsigned int n, const float spacing);
void reset();
void render(GLFWwindow* window);
void centerOnCorner();
void rotate(Action a);
void cube(Action a);