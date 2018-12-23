#include "rubix.h"

double x,y;
bool mouseActive;
Axis rotMode;
float colors[Faces][3] = {{1,0,0},{0,0,1},{1,1,1},{0,.5,0},{1,.75,0},{1,1,0}};

int main(int argc, char *argv[]) {
  GLFWwindow* window = prepareGlfw();
  if (window == NULL) {
    return EXIT_FAILURE;
  }

  mouseActive = false;
  rotMode = Front;
  unsigned int n;
  if (argc < 2 || sscanf(argv[1], "%i", &n) != 1) {
    n = 3;
  }
  float scale = (float)1/(2*n);
  glScalef(scale, scale, scale);
  while (!glfwWindowShouldClose(window)) {
    render(window, n);
    glfwPollEvents();
  }
  glfwTerminate();
  return EXIT_SUCCESS;
}

void render(GLFWwindow* window, unsigned int n) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (mouseActive) {
    double curx, cury;
    glPushMatrix();
    glLoadIdentity();
    glBegin(GL_POINTS);
      glfwGetCursorPos(window, &curx, &cury);
      glColor3f(0, .75, 1);
      glVertex3f((curx - x)/100, (y - cury)/100, 0);
    glEnd();
    glPopMatrix();
  }

  cube(n);

  if (!mouseActive) {
    switch (rotMode) {
      case Front:
        glRotatef(1, 0, 0, 1);
        break;
      case Right:
        glRotatef(1, 1, 0, 0);
        break;
      case Upper:
        glRotatef(1, 0, 1, 0);
        break;
    }
  }

  glfwSwapBuffers(window);
}

void insert(Face f, float a, float b) {
  switch(f.axis) {
    case Front:
      glVertex3f(a, b, f.offset);
      break;
    case Right:
      glVertex3f(f.offset, a, b);
      break;
    case Upper:
      glVertex3f(b, f.offset, a);
      break;
  }
}

void square(Face f, float *p, RubixCol c) {
  glBegin(GL_TRIANGLES);
  glColor3fv(colors[c]);

  insert(f, p[0]-1, p[1]-1);
  insert(f, p[0]-1, p[1]+1);
  insert(f, p[0]+1, p[1]-1);

  insert(f, p[0]+1, p[1]+1);
  insert(f, p[0]-1, p[1]+1);
  insert(f, p[0]+1, p[1]-1);

  glEnd();
}

Face canonicalFace(RubixCol c, float offset) {
  Face f;
  f.axis = c % (Faces/2);
  f.offset = (c/(Faces/2)) ? -offset : offset;
  return f;
}

void cube(unsigned int n) {
  float p[2];
  float spacing, maxDim;

  spacing = 1.1;
  maxDim = spacing * (n - 1) + 1;
  for (int i = 0; i < n; i++) {
    p[0] = spacing * (2*i - (int)n + 1);
    for (int j = 0; j < n; j++) {
      p[1] = spacing * (2*j - (int)n + 1);
      for (unsigned int f = 0; f < Faces; f++) {
        square(canonicalFace(f, maxDim), p, f);
      }
    }
  }
}

Axis keyToAxis(int key) {
  switch (key) {
    case GLFW_KEY_F:
      return Front;
    case GLFW_KEY_R:
      return Right;
    case GLFW_KEY_U:
    default:
      return Upper;
  }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action != GLFW_PRESS) {
    return;
  }
  switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
      glfwSetWindowShouldClose(window, GLFW_TRUE);
      break;
    case GLFW_KEY_A:
      mouseActive = true;
      glfwGetCursorPos(window, &x, &y);
      printf("activating mouse: (x = %f, y = %f)\n", x, y);
      break;
    case GLFW_KEY_D:
      printf("deactivating mouse\n");
      mouseActive = false;
      break;
    case GLFW_KEY_F:
    case GLFW_KEY_R:
    case GLFW_KEY_U:
      rotMode = keyToAxis(key);
      break;
  }
}

GLFWwindow* prepareGlfw() {
  GLFWmonitor* monitor;
  const GLFWvidmode* mode;
  GLFWwindow* window;
  if (!glfwInit()) {
    return NULL;
  }
  monitor = glfwGetPrimaryMonitor();
  mode = glfwGetVideoMode(monitor);
  window = glfwCreateWindow(10, 10, WINDOW_TITLE, NULL, NULL);
  if (!window) {
    glfwTerminate();
    return NULL;
  }
  glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPos(window, 0, 0);
  glfwSetKeyCallback(window, key_callback);
  glfwMakeContextCurrent(window);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glPointSize(35);
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  return window;
}