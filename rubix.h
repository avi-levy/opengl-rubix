// gcc -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo rubix.c -o rubix.out

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>

#define WINDOW_TITLE "RubixCube"
#define CUBE_COLORS {{0,0,1}, {1,1,1}, {1,0,0}, {1,.75,0}, {1,1,0}, {0,.5,0}}
#define USAGE                                                         \
  "command line usage: rubix <n>\n"                                   \
  "\trenders an interactive n by n by n Rubik's cube\n\n"             \
  "operation:\t(shift is denoted by \u21e7)\n"                        \
  "\ta\tactivates mouse and pauses view rotation\n"                   \
  "\t\u21e7a\tdeactivates mouse and resumes rotation\n"               \
  "\tb\trotate bottom cube face counterclockwise in initial view\n"   \
  "\t\u21e7b\ttoggle cube rotation along bottom face\n"               \
  "\td\trotate downward cube face counterclockwise in initial view\n" \
  "\t\u21e7d\ttoggle cube rotation along downward face\n"             \
  "\tf\trotate front cube face counterclockwise in initial view\n"    \
  "\t\u21e7f\ttoggle cube rotation along front face\n"                \
  "\tl\trotate left cube face counterclockwise in initial view\n"     \
  "\t\u21e7l\ttoggle cube rotation along left face\n"                 \
  "\tr\trotate right cube face counterclockwise in initial view\n"    \
  "\t\u21e7r\ttoggle cube rotation along right face\n"                \
  "\tu\trotate upward cube face counterclockwise in initial view\n"   \
  "\t\u21e7u\ttoggle cube rotation along upward face\n"

typedef enum axis {
  Right,
  Upper,
  Front,
  Axes
} Axis;

typedef struct face {
  Axis axis;
  float offset;
} Face;

typedef enum rubixCol {
  Blue,
  White,
  Red,
  Orange,
  Yellow,
  Green,
  Faces
} RubixCol;

typedef struct animationState {
  Face face;
  Face view;
  unsigned int duration;
} AnimationState;

GLFWwindow* prepareGlfw();
void render(GLFWwindow* window, unsigned int n);
void cube(unsigned int n, float spacing);
Face canonicalFace(RubixCol c, float offset);
void square(Face f, float *p, RubixCol c);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void centerOnCorner(unsigned char c);
void insert(Face f, float a[Axes-1], float b, float c);
void axisInsert(Axis a, float* b, float c, float* d);
void faceVector(Face a, int* b);
Face keyToFace(int key);
bool equals(Face a, Face b);
bool isFaceActive(Face f);
void invalidateFace(Face *f);
void invalidateAnimationState();