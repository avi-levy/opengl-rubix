#include "rubix.h"

Phys phys;
AnimationState state;

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
  initCube(n, 1.1);
  if (data != NULL && faceShift != NULL && edgeShift != NULL) {
    while (!glfwWindowShouldClose(window)) {
      render(window);
      glfwPollEvents();
    }
  }
  destroyCube();
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
    if (state.duration >= 90) {
      twist(state.face);
      state.duration = 0;
      state.face = Faces;
    } else {
      state.duration++;
    }
  }
  if (state.view < Faces) {
    rotateFace(state.view, 1);
  }
  glfwSwapBuffers(window);
}

//
// Insert at "a" the value "b" and populate the rest from "c" in cyclic order to generate "d".
//
void axisInsert(Axis a, float b, float c[Axes-1], float d[Axes]) {
  int j;
  for (unsigned int i = 0; i < Axes; i++) {
    if (i == a) {
      d[i] = b;
    } else {
      j = i - a - 1 + ((i > a) ? 0 : Axes);
      d[i] = (b > 0) ? c[j] : -c[j];
    }
  }
}

void insert(FaceName f, float a[Axes-1], float b, float c) {
  float d[Axes-1], e[Axes];
  d[0] = a[0] + b;
  d[1] = a[1] + c;
  axisInsert(phys.faces[f].axis, phys.faces[f].offset, d, e);
  glVertex3fv(e);
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
void physFaceCoords(int *c, float *p, unsigned int len, bool flip) {
  int n = phys.cubies;
  for (unsigned int i = 0; i < len; i++) {
    // p[i] = phys.spacing * (2 * (flip ? c[i] : n - 1 - c[i]) - (int)phys.cubies + 1);
    p[i] = phys.spacing * (2 * c[i] - (int)phys.cubies + 1);
  }
}

//
// returns the direction (in face coords of "from") to the face "to"
//
int faceDir(FaceName to, FaceName from) {
  to %= Axes;
  from %= Axes;
  return (to > from) ? to - from - 1 : Axes + to - from - 1;
}

int edgeDir(FaceName to, FaceName from) {
  return (!(to < Axes) == !(from < Axes)) ? phys.cubies - 1 : 0;
}

bool onBorder(FaceName to, FaceName from, int c[Axes-1]) {
  return c[faceDir(to, from)] == edgeDir(to, from);
}

//
// cube color data getter and setter
//
void set(FaceName f, int c[Axes-1], FaceName color) {
  int n = phys.cubies;
  data[c[0] + c[1]*n + f*n*n] = color;
}

void setCubie(Cubie *c, FaceName color) {
  set(c->face, c->coord, color);
}

FaceName get(FaceName f, int c[Axes-1]) {
  int n = phys.cubies;
  return data[c[0] + c[1]*n + f*n*n];
}

FaceName getCubie(Cubie *c) {
  return get(c->face, c->coord);
}

int push(Cubie *c, FaceName f, int coord[Axes-1], int t) {
  c[t].face = f;
  memcpy(c[t].coord, coord, (Axes-1) * sizeof(int));
  return t + 1;
}

//
// Stores in g the cyclic ordering of faces adjacent to f.
//
void perm(FaceName f, FaceName *g) {
  int j, k = 0;
  for (unsigned int i = 0; i < Faces; i++) {
    j = (f % FACES_PER_AXIS) ? Faces - 1 - i : i;
    if (j % Axes == f % Axes) continue;
    g[k++] = j;
  }
}

//
// Computes the coordinates in face b of the i^th cubie
// along the edge shared between faces a and b, oriented
// such that i = 0 corresponds to the corner shared by
// faces a, b, and c.
//
void coords(FaceName a, FaceName b, FaceName c, int e[Axes-1], int i) {
  int n = phys.cubies, f = faceDir(a, b);
  e[f] = edgeDir(a, b);
  e[1-f] = edgeDir(b, c) ? i : n - 1 - i;
}

//
// This method requires c to be an array of length g * s.
// It cyclically shift elements of the array forward by s.
//
void blockCyclicShift(Cubie *c, int g, int s) {
  FaceName last, cur;
  int t;
  for (int i = 0; i < g; i++) {
    t = i + (s - 1) * g;
    last = getCubie(c + t);
    for (int j = 0, t = i; j < s; j++, t += g) {
      cur = getCubie(c + t);
      setCubie(c + t, last);
      last = cur;
    }
  }
}

FaceName getFaceCyclic(FaceName f, int i) {
  return phys.faces[f].adj[i % (Faces - FACES_PER_AXIS)];
}

void twist(FaceName f) {
  int t = 0, u = 0, n = phys.cubies, c[Axes-1];
  FaceName adj, next;
  for (unsigned int g = 0; g < Faces - FACES_PER_AXIS; g++) {
    adj = getFaceCyclic(f, g);
    next = getFaceCyclic(f, g + 1);
    for (int i = 0; i < n; i++) {
      coords(f, adj, next, c, i);
      push(edgeShift, adj, c, t++);
      if (i == n-1) continue;
      coords(adj, f, next, c, i);
      push(faceShift, f, c, u++);
    }
  }
  blockCyclicShift(edgeShift, n, 4);
  blockCyclicShift(faceShift, n - 1, 4);
}

void cube() {
  float p[Axes-1];
  int c[Axes-1], d, n = phys.cubies;

  for (unsigned int f = 0; f < Faces; f++) {
    glPushMatrix();
    if (f == state.face) {
      rotateFace(state.face, state.duration);
      if (state.mouseActive) {
        dot(f, White);
      }
    }
    for (c[0] = 0; c[0] < n; c[0]++) {
      for (c[1] = 0; c[1] < n; c[1]++) {
        glPushMatrix();
        physFaceCoords(c, p, Axes - 1, f < Axes);
        if (state.face < Faces && state.face % Axes != f % Axes) {
          if (onBorder(state.face, f, c)) {
            rotateFace(state.face, state.duration);
          }
        }
        border(f, p, Faces);
        square(f, p, get(f, c));
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

void normalizeIsometric() {
  float norm[4] = {0};
  for (int i = 0; i < MATRIX_SIZE; i++) {
    norm[i % 4] += isometric[i] * isometric[i];
  }
  for (int i = 0; i < MATRIX_SIZE; i++) {
    isometric[i] /= sqrt(norm[i % 4]);
  }
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
  state.corner.index = state.corner.orientation = 0;
  normalizeIsometric(); // required before centering on a corner
  initPhys(n, spacing);
  state.duration = 0;
  state.mouseActive = false;

  data = malloc(Faces * n * n * sizeof(int));
  // of the n^2 cubies on each face, 4(n-1) are adjacent to another face
  faceShift = malloc(4 * (n-1) * sizeof(Cubie));
  // each face is adjacent to 4n cubies on other faces
  edgeShift = malloc(4 * n * sizeof(Cubie));;
  if (!data || !faceShift || !edgeShift) {
    return;
  }

  int c[Axes-1];
  for (unsigned int f = 0; f < Faces; f++) {
    perm(f, phys.faces[f].adj);
    for (c[0] = 0; c[0] < n; c[0]++) {
      for (c[1] = 0; c[1] < n; c[1]++) {
        set(f, c, f);
      }
    }
  }
}

void destroyCube() {
  free(data);
  free(faceShift);
  free(edgeShift);
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
  //glLineWidth(50);
  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  return window;
}