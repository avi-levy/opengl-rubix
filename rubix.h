// gcc -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo rubix.c -o rubix.out

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h> // for sqrt
#include <string.h> // for memcpy, memset

#define WINDOW_TITLE "RubixCube"
#define MATRIX_SIZE 16
#define CORNERS 8

//                   Blue     White    Red      Green     Yellow   Orange
#define CUBE_COLORS {{0,0,1}, {1,1,1}, {1,0,0}, {0,.5,0}, {1,1,0}, {1,.65,0}}

typedef enum faceName {
  Blue,
  White,
  Red,
  Green,
  Yellow,
  Orange,
  Faces // number of faces (6)
} FaceName;

// Standard ("singmaster") Rubik's cube face notation
const int RubixColKey[Faces] = {
  GLFW_KEY_R, // starts blue
  GLFW_KEY_U, // starts white
  GLFW_KEY_F, // starts red
  GLFW_KEY_L, // starts green
  GLFW_KEY_D, // starts yellow
  GLFW_KEY_B, // starts orange
};

// There are two faces per axis
typedef enum axis {
  Right, // R (+) and L (-)
  Upper, // U (+) and D (-)
  Front, // F (+) and B (-)
  Axes // number of axes (3)
} Axis;

typedef struct face {
  Axis axis;
  float offset;
  float vector[Axes];
} Face;

typedef struct corner {
  int index;
  int orientation;
} Corner;

typedef struct phys {
  Face faces[Faces];
  unsigned int cubies;
  float spacing;
  float boundingBox;
  float scale;
  float sensitivity;
  int viewport[2];
  double mouse[4];
} Phys;

typedef struct animationState {
  FaceName face;
  FaceName view;
  unsigned int duration;
  Corner corner;
  bool mouseActive;
} AnimationState;

#define USAGE                                                           \
  "command line usage: rubix <n>\n"                                     \
  "\trenders an interactive n by n by n Rubik's cube\n\n"               \
  "Keyboard controls the following operations:\n"                       \
  "\tFace twists (R, U, F, L, D, B)\n"                                  \
  "\tCube rotations (shift + corresponding face twist key)\n"           \
  "\tCenter view on corner (1 - 8, shift/control to set orientation)\n" \
  "\tToggle mouse activation with A\n"

GLFWwindow* prepareGlfw();
void render(GLFWwindow* window);
void cube();
void inputCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void minimizeCallback(GLFWwindow* window, int iconified);
void centerOnCorner();
void insert(FaceName f, float a[Axes-1], float b, float c);
void axisInsert(Axis a, float* b, float c, float* d);
void square(FaceName f, float *p, FaceName c);
void rotateFace(FaceName f, float angle);
FaceName keyToFaceName(int key);
void initCube(const unsigned int n, const float spacing);