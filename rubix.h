// gcc -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo rubix.c -o rubix.out

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define WINDOW_TITLE "RubixCube"

typedef enum axis {
  Front,
  Right,
  Upper,
} Axis;

typedef struct face {
  Axis axis;
  float offset;
} Face;

typedef enum rubixCol {
  Red,
  Blue,
  White,
  Green,
  Orange,
  Yellow,
  Faces
} RubixCol;

GLFWwindow* prepareGlfw();
void render(GLFWwindow* window, unsigned int n);
void cube(unsigned int n);
Face canonicalFace(RubixCol c, float offset);
void square(Face f, float *p, RubixCol c);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
