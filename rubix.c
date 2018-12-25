#include "rubix.h"

float colors[Faces][3] = CUBE_COLORS;
Phys phys;
AnimationState state;

// camera view directed at (1, 1, 1)
float isometric[MATRIX_SIZE] = {
  -1  , 1   , 1 , 0   ,
  0   , -2  , 1 , 0   ,
  1   , 1   , 1 , 0   ,
  0   , 0   , 0 , -1  ,
};

void normalizeIsometric() {
  float norm[4] = {0};
  for (int i = 0; i < MATRIX_SIZE; i++) {
    norm[i % 4] += isometric[i] * isometric[i];
  }
  for (int i = 0; i < MATRIX_SIZE; i++) {
    isometric[i] /= sqrt(norm[i % 4]);
  }
}

int main(int argc, char *argv[]) {
  unsigned int n;
  GLFWwindow* window = prepareGlfw(&phys.viewport);
  if (window == NULL) {
    return EXIT_FAILURE;
  }
  printf("OpenGL %s\n", glGetString(GL_VERSION));
  if (argc < 2 || sscanf(argv[1], "%i", &n) != 1) {
    n = 3;
    printf(USAGE);
  }
  normalizeIsometric();
  initCube(n, 1.1);
  while (!glfwWindowShouldClose(window)) {
    render(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return EXIT_SUCCESS;
}

void render(GLFWwindow* window) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glfwGetCursorPos(window, phys.mouse + 2, phys.mouse + 3);
  glMatrixMode(GL_PROJECTION);
  centerOnCorner();
  glMatrixMode(GL_MODELVIEW);
  cube();
  if (state.face < Faces) {
    state.duration++;
  }
  if (state.view < Faces) {
    rotateFace(state.view, 1);
  }
  glfwSwapBuffers(window);
}

//
// Insert at location a in array b the value c and populate the rest from d in cyclic order.
// Assume b has length equal to Axes, d has length one smaller, and a is a valid axis.
//
// Ordering is done as follows. Axes - a - 1, ... , Axes - 2, a, 0, 1, ... , Axes - a - 2
//
void axisInsert(Axis a, float* b, float c, float* d) {
  for (unsigned int i = 0; i < Axes; i++) {
    if (i == a) {
      b[i] = c;
    }
    if (i > a) {
      b[i] = d[i - a - 1];
    }
    if (i < a) {
      b[i] = d[i + Axes - a - 1];
    }
  }
}

void insert(FaceName f, float a[Axes-1], float b, float c) {
  float d[Axes-1];
  float e[Axes];
  d[0] = a[0] + b;
  d[1] = a[1] + c;
  axisInsert(phys.faces[f].axis, e, phys.faces[f].offset, d);
  glVertex3f(e[0], e[1], e[2]);
}

void rotateFace(FaceName f, float angle) {
  float *v = phys.faces[f].vector;
  glRotatef(angle, v[0], v[1], v[2]);
}

void clamp(float *f) {
  float m = phys.boundingBox;
  if (*f > m) *f = m;
  if (*f < -m) *f = -m;
}

void dot(FaceName f, FaceName c) {
  float a[2], p[2] = {0, 0};

  glBegin(GL_POINTS);
  glColor3fv(colors[c]);

  for (int i = 0; i < 2; i++) {
    a[i] = phys.mouse[i+2] - phys.mouse[i];
    a[i] *= phys.sensitivity;
    clamp(a + i);
  }

  insert(f, p, a[0], a[1]);
  glEnd();
}

void border(FaceName f, float *p, FaceName c) {
  glBegin(GL_LINE_LOOP);
  glColor3fv(colors[c]);

  insert(f, p, -1, -1);
  insert(f, p, -1, 1);
  insert(f, p, 1, 1);
  insert(f, p, 1, -1);

  glEnd();
}

//
// The color to draw the face is determined by the face name initial color.
//
void square(FaceName f, float *p, FaceName c) {
  glBegin(GL_TRIANGLES);
  glColor3fv(colors[c]);

  insert(f, p, -1, -1);
  insert(f, p, -1, 1);
  insert(f, p, 1, -1);

  insert(f, p, 1, 1);
  insert(f, p, -1, 1);
  insert(f, p, 1, -1);

  glEnd();
}

//
// in face coordinates, converts a cubie multi-index ("c")
// into the physical offset of its center ("p")
//
void physFaceCoords(int *c, float *p, unsigned int len) {
  for (unsigned int i = 0; i < len; i++) {
    p[i] = phys.spacing * (2 * c[i] - (int)phys.cubies + 1);
  }
}

//
// returns the direction (in face coords of "from") to the face "to"
//
int faceDir(Axis to, Axis from) {
  return (to > from) ? to - from - 1 : Axes + to - from - 1;
}

void cube() {
  float p[Axes-1];
  int c[Axes-1], d, n = phys.cubies;
  //faceVector(state.face, b);

  for (unsigned int f = 0; f < Faces; f++) {
    glPushMatrix();
    if (f == state.face) {
      rotateFace(state.face, state.duration);
      //glRotatef(state.duration, b[0], b[1], b[2]);
      if (state.mouseActive) {
        dot(f, White);
      }
    }
    for (c[0] = 0; c[0] < n; c[0]++) {
      for (c[1] = 0; c[1] < n; c[1]++) {
        glPushMatrix();
        physFaceCoords(c, p, Axes - 1);
        if (state.face < Faces && state.face % Axes != f % Axes) {
          d = faceDir(state.face % Axes, f % Axes);
          // check if the current square is adjacent to the face in motion
          if (c[d] == ((state.face < Axes) ? n - 1 : 0)) {
            rotateFace(state.face, state.duration);
            //glRotatef(state.duration, b[0], b[1], b[2]);
            border(f, p, state.mouseActive ? Blue : White);
          }
        }
        square(f, p, f); // identity state until data update is added
        glPopMatrix();
      }
    }
    glPopMatrix();
  }
}

void centerOnCorner() {
  int c, i, x[4];
  float isom[MATRIX_SIZE];
  c = state.corner.index;
  // the corner is represented by an index and an orientation
  // even though there are 8 corners, we use 4 bits to represent it
  // for convenience when performing matrix operations below.
  for (i = 0; i < 4; i++) {
    x[i] = (c % 2) ? 1 : -1;
    c /= 2;
  }
  // load the isometric matrix centered on the (1,1,1) corner
  memcpy(isom, isometric, MATRIX_SIZE * sizeof(float));
  // recenter the matrix on the current corner
  for (i = 0; i < MATRIX_SIZE; i++) {
    isom[i] *= x[i/4];
  }
  glLoadMatrixf(isom);
  glRotatef(state.corner.orientation * 120, x[0], x[1], x[2]);
}

//
// Face operations
//
void faceVector(FaceName f, int* b) {
  for (unsigned int i = 0; i < Axes; i++) {
    b[i] = (i == f % Axes) * ((f < Axes) ? -1 : 1);
  }
}

void initializeCorner(Corner *c) {
  c->index = 0;
  c->orientation = 0;
}

void initPhys(const unsigned int n, const float spacing) {
  float s = (float)1/(2 * n);
  phys.cubies = n;
  phys.spacing = spacing;
  phys.boundingBox = spacing * (n - 1) + 1;
  phys.scale = s;
  phys.sensitivity = (float)1/20;
  glScalef(s, s, s);
  for (int i = 0; i < Faces; i++) {
    phys.faces[i].axis = i % Axes;
    phys.faces[i].offset = (i < Axes) ? phys.boundingBox : -phys.boundingBox;
    for (int j = 0; j < Axes; j++) {
      phys.faces[i].vector[j] = (j == i % Axes) * phys.faces[i].offset;
    }
  }
  memset(&phys.mouse, 0, 4 * sizeof(int));
}

void initCube(const unsigned int n, const float spacing) {
  state.face = state.view = Faces;
  initializeCorner(&state.corner);
  initPhys(n, spacing);
  state.duration = 0;
  state.mouseActive = false;
}

FaceName keyToFaceName(const int key) {
  for (unsigned int i = 0; i < Faces; i++) {
    if (key > 0 && key == RubixColKey[i]) {
      return i;
    }
  }
  return Faces;
}

void toggleAssignFace(FaceName *old, FaceName new) {
  *old = (*old == new) ? Faces : new;
}

void inputCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action != GLFW_PRESS) {
    return;
  }
  if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) {
    state.corner.index = key - GLFW_KEY_1;
    state.corner.orientation = 0;
    if (mods & GLFW_MOD_SHIFT) {
      state.corner.orientation++;
    }
    if (mods & GLFW_MOD_CONTROL) {
      state.corner.orientation--;
    }
    return;
  }
  switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
      glfwSetWindowShouldClose(window, GLFW_TRUE);
      break;
    case GLFW_KEY_A:
      glfwGetCursorPos(window, phys.mouse, phys.mouse + 1);
      state.mouseActive ^= true;
      break;
    default:
      for (int i = 0; i < Faces; i++) {
        if (key == RubixColKey[i]) {
          if (mods & GLFW_MOD_SHIFT) {
            toggleAssignFace(&state.view, i);
            break;
          }
          state.duration = 0;
          toggleAssignFace(&state.face, i);
          break;
        }
      }
  }
}

void minimizeCallback(GLFWwindow* window, int minimized) {
    if (minimized) { // close when the window is minimized
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

GLFWwindow* prepareGlfw(int *dim) {
  GLFWmonitor* monitor;
  const GLFWvidmode* mode;
  GLFWwindow* window;
  if (!glfwInit()) {
    return NULL;
  }
  monitor = glfwGetPrimaryMonitor();
  if (!monitor) {
    return NULL;
  }
  mode = glfwGetVideoMode(monitor);
  if (!mode) {
    return NULL;
  }
  dim[0] = mode->width;
  dim[1] = mode->height;
  window = glfwCreateWindow(1, 1, WINDOW_TITLE, NULL, NULL);
  if (!window) {
    glfwTerminate();
    return NULL;
  }
  glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPos(window, 0, 0);
  glfwSetKeyCallback(window, inputCallback);
  glfwSetWindowIconifyCallback(window, minimizeCallback);
  glfwMakeContextCurrent(window);
  glDepthFunc(GL_LEQUAL);
  glPointSize(35);
  glLineWidth(10);
  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  return window;
}