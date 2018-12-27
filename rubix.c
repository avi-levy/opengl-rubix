#include "rubix.h"

Phys phys;
AnimationState state;

int main(int argc, char *argv[]) {
  unsigned int n;
  GLFWwindow* w = prepareGlfw();
  if (w == NULL) {
    return EXIT_FAILURE;
  }
  printf("OpenGL %s\n", glGetString(GL_VERSION));
  if (argc < 2 || sscanf(argv[1], "%i", &n) != 1) {
    n = 3;
    printf(USAGE);
  }
  if (initCube(n, 1.1)) {
    while (!glfwWindowShouldClose(w)) {
      render(w);
      glfwPollEvents();
    }
  }
  free(data);
  glfwTerminate();
  return EXIT_SUCCESS;
}

void render(GLFWwindow* w) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  if (state.corner.index < CORNERS) {
    centerOnCorner();
    state.corner.index = CORNERS;
  }
  if (state.actions[Turn].f < Faces) {
    rotate(Turn);
  }
  glMatrixMode(GL_MODELVIEW);
  cube(Twist);
  glfwSwapBuffers(w);
}

//
// returns the direction (in face coords of "from") to the face "to"
//
int faceDir(Face to, Face from) {
  to %= Axes;
  from %= Axes;
  return (to > from) ? to - from - 1 : Axes + to - from - 1;
}

int edgeDir(Face to, Face from) {
  return ((to < Axes) == (from < Axes)) ? phys.cubies - 1 : 0;
}

bool onBorder(Face to, Face from, int c[Axes-1]) {
  return to < Faces && to % Axes != from % Axes && c[faceDir(to, from)] == edgeDir(to, from);
}

void set(Face f, const int c[Axes-1], Face v) {
  int n = phys.cubies;
  for (Axis j = 0; j < Axes - 1; j++) {
    f *= n;
    f += c[j];
  }
  data[f] = v;
}

Face get(Face f, const int c[Axes-1]) {
  int n = phys.cubies;
  for (Axis j = 0; j < Axes - 1; j++) {
    f *= n;
    f += c[j];
  }
  return data[f];
}

//
// Computes the coordinates in face b of the i^th cubie
// along the edge shared between faces a and b, oriented
// such that i = 0 corresponds to the corner shared by
// faces a, b, and c.
//
void coords(Face a, Face b, Face c, int e[Axes-1], int i) {
  int n = phys.cubies, f = faceDir(a, b);
  e[f] = edgeDir(a, b);
  e[1-f] = edgeDir(b, c) ? i : n - 1 - i;
}

void getAdjacent(Face f, int i, int j, Face *e) {
  for (Face k = i; k < i + j; k++) {
    e[k - i] = phys.faces[f].adj[k % (Faces - FACES_PER_AXIS)];
  }
}

void swapCubie(Face a, Face b, Face c, int i, Face *v) {
  int d[Axes-1];
  coords(a, b, c, d, i);
  Face t = get(b, d);
  if (*v < Faces) {
    set(b, d, *v);
  }
  *v = t;
}

void twist(Move *m) {
  Face f = m->f, inner = Faces, outer = Faces, adj[2];
  for (int i = 0; i < phys.cubies; i++) {
    for (Face g = 0; g <= Faces - FACES_PER_AXIS; g++) {
      getAdjacent(f, m->o ? Faces - FACES_PER_AXIS - g : g, 2, adj);
      swapCubie(f, adj[0], adj[1], i, &inner);
      if (i) swapCubie(adj[0], f, adj[1], i, &outer);
    }
  }
}

void rotateFace(Face f, float angle) {
  float *v = phys.faces[f].vector;
  glRotatef(angle, v[0], v[1], v[2]);
}

void rotate(Action a) {
  Move *m = &state.actions[a];
  rotateFace(m->f, m->o ? (-1.0 * m->angle) : m->angle);
}

//
// Insert at "a" the value "b" and populate the rest from "c" in cyclic order to generate "d".
//
void axisInsert(Axis a, float b, float c[Axes-1], float d[Axes]) {
  int j;
  for (Axis i = 0; i < Axes; i++) {
    if (i == a) {
      d[i] = b;
    } else {
      j = i - a - 1 + ((i > a) ? 0 : Axes);
      d[i] = (b > 0) ? c[j] : -c[j];
    }
  }
}

void insert(Face f, float a[Axes-1], int b[Axes-1]) {
  float d[Axes-1], e[Axes];
  for (Axis i = 0; i < Axes-1; i++) {
    d[i] = a[i] + b[i];
  }
  axisInsert(phys.faces[f].axis, phys.faces[f].offset, d, e);
  glVertex3fv(e);
}

void square(Face f, const int c[Axes-1]) {
  float p[Axes-1]; // stores coordinates of center of cubie c
  int i, d[Axes-1];
  for (Axis a = 0; a < Axes - 1; a++) {
    p[a] = phys.spacing * (2 * c[a] - (int)phys.cubies + 1);
  }
  glBegin(GL_TRIANGLES); // draws a square by stitching two triangles
  glColor3fv(colors[get(f, c)]);
  for (i = 0; i < BORDER; i++) {
    for (Axis a = 0; a < Axes - 1; a++) d[a] = ((i + a) % BORDER < BORDER/2) ? 1 : -1;
    insert(f, p, d);
    if (!(i % 2)) {
      for (Axis a = 0; a < Axes - 1; a++) d[a] = -d[a];
      insert(f, p, d);
    }
  }
  glEnd();
  glBegin(GL_LINE_LOOP); // outer border
  glColor3fv(colors[Faces]);
  for (i = 0; i < BORDER; i++) {
    for (Axis a = 0; a < Axes - 1; a++) {
      d[a] = ((i + a) % BORDER < BORDER/2) ? 1 : -1;
    }
    insert(f, p, d);
  }
  glEnd();
}

void cube(Action a) {
  int c[Axes-1], n = phys.cubies;
  Move *m = &state.actions[a];
  if (m->f < Faces) {
    if (m->angle > 0) {
      m->f = Faces;
    } else {
      m->angle++;
    }
  }
  for (Face f = 0; f < Faces; f++) {
    for (c[0] = 0; c[0] < n; c[0]++) {
      for (c[1] = 0; c[1] < n; c[1]++) {
        glPushMatrix();
        if (f == m->f || onBorder(m->f, f, c)) rotate(a);
        square(f, c);
        glPopMatrix();
      }
    }
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
  glRotatef(state.corner.orientation * DEGREES_IN_CIRCLE / 3, x[0], x[1], x[2]);
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
  phys.cubiesPerFace = 1;
  for (Axis a = 0; a < Axes - 1; a++) {
    phys.cubiesPerFace *= n;
  }
  phys.spacing = spacing;
  phys.boundingBox = spacing * (n - 1) + 1;
  phys.scale = s;
  glScalef(s, s, s);
  for (Face f = 0; f < Faces; f++) {
    phys.faces[f].axis = f % Axes;
    phys.faces[f].offset = (f < Axes) ? phys.boundingBox : -phys.boundingBox;
    for (Axis a = 0; a < Axes; a++) {
      phys.faces[f].vector[a] = (a == f % Axes) * phys.faces[f].offset;
    }
  }
}

void reset() {
  for (int i = 0; i < Faces * phys.cubiesPerFace; i++) {
    data[i] = i / phys.cubiesPerFace;
  }
}

bool initCube(const unsigned int n, const float spacing) {
  Face h;
  for (Action a = 0; a < Actions; a++) {
    state.actions[a].f = Faces;
  }
  state.corner.index = state.corner.orientation = 0;
  normalizeIsometric();
  initPhys(n, spacing);
  data = malloc(Faces * phys.cubiesPerFace * sizeof(int));
  if (data != NULL) {
    reset(); // set cube to initial state
    for (Face f = 0; f < Faces; f++) { // populate face adjacency graph
      for (Face g = 0, k = 0; g < Faces; g++) {
        h = (f % FACES_PER_AXIS) ? Faces - 1 - g : g;
        if (h % Axes == f % Axes) continue;
        phys.faces[f].adj[k++] = h;
      }
    }
  }
  return data != NULL;
}

Face keyToFace(const int key) {
  Face f = 0;
  for (; f < Faces; f++) {
    if (key == FaceToKey[f]) break;
  }
  return f;
}

void update(Action a, Face f, Orientation o) {
  if (f == Faces) return;
  Move *m = &state.actions[a];
  if (a == Turn && m->f == f && m->o == o) {
    m->f = Faces; // toggle turn state if pressed twice
    return;
  }
  m->f = f;
  m->o = o;
  if (a == Twist) {
    twist(m);
    m->angle = -DEGREES_IN_CIRCLE / 4;
  } else {
    m->angle = 1;
  }
}

void input(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action != GLFW_PRESS) {
    return;
  }
  if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) {
    state.corner.index = key - GLFW_KEY_1;
    state.corner.orientation = (!!(mods & GLFW_MOD_SHIFT) - !!(mods & GLFW_MOD_CONTROL));
    return;
  }
  switch (key) {
  case GLFW_KEY_ESCAPE:
  case GLFW_KEY_Q:
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    break;
  case GLFW_KEY_C:
    reset();
    break;
  default:
    update((mods & GLFW_MOD_CONTROL) ? Turn : Twist, keyToFace(key),
      (mods & GLFW_MOD_SHIFT) ? Counterclockwise : Clockwise);
    break;
  }
}

void focus(GLFWwindow* w, int focused) {
  if (!focused) glfwSetWindowShouldClose(w, GLFW_TRUE);
}

GLFWwindow* prepareGlfw() {
  GLFWmonitor* monitor;
  const GLFWvidmode* mode;
  GLFWwindow* w;
  if (!glfwInit()) goto Fail;
  monitor = glfwGetPrimaryMonitor();
  if (!monitor) goto Fail;
  mode = glfwGetVideoMode(monitor);
  if (!mode) goto Fail;
  w = glfwCreateWindow(1, 1, WINDOW_TITLE, NULL, NULL);
  if (!w) goto Fail;
  glfwSetWindowMonitor(w, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
  glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetKeyCallback(w, input);
  glfwSetWindowFocusCallback(w, focus);
  glfwMakeContextCurrent(w);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glLineWidth(10);
  return w;
Fail:
  glfwTerminate();
  return NULL;
}