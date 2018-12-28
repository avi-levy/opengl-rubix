#include "rubix.h"

Phys phys;
AnimationState state;

//
// Entry point; build cube and run main loop.
//
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
  if (initCube(n, .1)) {
    reset();
    while (!glfwWindowShouldClose(w)) {
      render(w);
      glfwPollEvents();
    }
  }
  free(data);
  glfwTerminate();
  return EXIT_SUCCESS;
}

//
// Render the cube.
//
void render(GLFWwindow* w) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  centerOnCorner();
  rotate(Turn);
  glMatrixMode(GL_MODELVIEW);
  cube(Twist);
  glfwSwapBuffers(w);
}

//
// Compute direction in "from" face coords to the "to" face.
//
int faceDir(Face to, Face from) {
  to %= Axes;
  from %= Axes;
  return (to > from) ? to - from - 1 : Axes + to - from - 1;
}

//
// Compute first index along oriented edge between to and from.
//
int edgeDir(Face to, Face from) {
  return ((to < Axes) == (from < Axes)) ? phys.cubies - 1 : 0;
}

//
// Returns whether the specified cubie on face "to" lies on the
// border with face "from".
//
bool onBorder(Face to, Face from, int c[Axes-1]) {
  return to < Faces && to % Axes != from % Axes && c[faceDir(to, from)] == edgeDir(to, from);
}

//
// Sets the color of cubie c on face f to v.
//
void set(Face f, const int c[Axes-1], Face v) {
  int n = phys.cubies;
  for (Axis j = 0; j < Axes - 1; j++) {
    f *= n;
    f += c[j];
  }
  data[f] = v;
}

//
// Returns the color of cubie c on face f.
//
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
// faces a, b, and c and stores them in e.
//
void coords(Face a, Face b, Face c, int e[Axes-1], int i) {
  int n = phys.cubies, f = faceDir(a, b);
  e[f] = edgeDir(a, b);
  e[1-f] = edgeDir(b, c) ? i : n - 1 - i;
}

//
// Swaps the color of the corresponding cubie (see the coords()
// function invocation outlined above) with that of v.
//
void swapCubie(Face a, Face b, Face c, int i, Face *v) {
  int d[Axes-1];
  coords(a, b, c, d, i);
  Face t = get(b, d);
  if (*v < Faces) {
    set(b, d, *v);
  }
  *v = t;
}

//
// Stores in e the sequence of j consecutive faces
// starting at index i in the cyclic ordering of
// faces adjacent to f.
//
void getAdjacent(Face f, int i, int j, Face *e) {
  for (Face k = i; k < i + j; k++) {
    e[k - i] = phys.faces[f].adj[k % (Faces - FACES_PER_AXIS)];
  }
}

//
// Updates cube data corresponding to a face twist.
//
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

//
// Applies rotation matrix for face turn animation.
//
void rotateFace(Face f, float angle) {
  float *v = phys.faces[f].vector;
  glRotatef(angle, v[0], v[1], v[2]);
}

//
// Invokes rotateFace() with parameters corresponding
// to the specified action if it is active.
//
void rotate(Action a) {
  Move *m = &state.actions[a];
  if (m->f < Faces) rotateFace(m->f, m->o ? -1.0 * m->angle : m->angle);
}

//
// Populates array d from c (which has one fewer element)
// by inserting value b at index a and cyclically
// continuing from there.
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

//
// This function serves as a graphics primitive for
// rendering of the cube. By invoking axisInsert()
// with appropriate parameters it translates from
// facial to spatial coordinates, resulting in a
// vertex being drawn at the point corresponding to
// vector a + b on face f.
//
void insert(Face f, float a[Axes-1], int b[Axes-1]) {
  float d[Axes-1], e[Axes];
  for (Axis i = 0; i < Axes-1; i++) {
    d[i] = a[i] + b[i];
  }
  axisInsert(f % Axes, phys.faces[f].offset, d, e);
  glVertex3fv(e);
}

//
// Renders a solid square at cubie c on face f with black border.
//
void square(Face f, const int c[Axes-1]) {
  float p[Axes-1]; // stores coordinates of center of cubie c
  int i, d[Axes-1];
  for (Axis a = 0; a < Axes - 1; a++) {
    p[a] = (1 + phys.spacing) * (2 * c[a] - (int)phys.cubies + 1);
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

//
// Renders cube state after updating animation
// state of action a as necessary.
//
void cube(Action a) {
  int c[Axes-1], n = phys.cubies;
  Move *m = &state.actions[a];
  if (m->f < Faces) {
    if (m->angle > 0) {
      m->f = Faces; // mark animation completed
    } else {
      m->angle++; // progress animation
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

//
// Centers cube perspective on corner if such an
// operation was pending.
//
void centerOnCorner() {
  int c, i, x[4];
  float isom[MATRIX_SIZE];
  c = state.corner.index;
  if (c >= CORNERS) { // no corner operation is pending
    return;
  } else {
    state.corner.index = CORNERS; // mark the operation as completed
  }
  // load the isometric perspective
  memcpy(isom, isometric, sizeof(isom));
  // parse corner coordinates
  for (i = 0; i < Axes + 1; i++) {
    x[i] = c & (1 << i) ? 1 : -1;
  }
  // update isometric perspective matrix with corner coords
  for (i = 0; i < MATRIX_SIZE; i++) {
    isom[i] *= x[i/(Axes + 1)];
  }
  glLoadMatrixf(isom);
  glRotatef(state.corner.orientation * DEGREES_IN_CIRCLE / 3, x[0], x[1], x[2]);
}

//
// Scales the columns of the isometric
// perspective matrix to have unit norm.
//
void normalizeIsometric() {
  float norm[4] = {0};
  for (int i = 0; i < MATRIX_SIZE; i++) {
    norm[i % 4] += isometric[i] * isometric[i];
  }
  for (int i = 0; i < MATRIX_SIZE; i++) {
    isometric[i] /= sqrt(norm[i % 4]);
  }
}

//
// Initialize the global Phys struct, containing
// parameters controlling rendering of physical pixels.
//
void initPhys(const unsigned int n, const float spacing) {
  float s = (float)1/(2 * n), t = (1 + spacing) * (n - 1) + 1;
  Face h;
  phys.cubies = n;
  phys.cubiesPerFace = 1;
  for (Axis a = 0; a < Axes - 1; a++) {
    phys.cubiesPerFace *= n;
  }
  phys.spacing = spacing;
  phys.scale = s;
  glScalef(s, s, s);
  for (Face f = 0; f < Faces; f++) {
    phys.faces[f].offset = (f < Axes) ? t : -1.0 * t;
    for (Axis a = 0; a < Axes; a++) { // populate face vector
      phys.faces[f].vector[a] = (a == f % Axes) * phys.faces[f].offset;
    }
    for (Face g = 0, k = 0; g < Faces; g++) { // populate face adjacency graph
      h = (f % FACES_PER_AXIS) ? Faces - 1 - g : g;
      if (h % Axes == f % Axes) continue;
      phys.faces[f].adj[k++] = h;
    }
  }
}

//
// Set cube to initial state.
//
void reset() {
  for (int i = 0; i < Faces * phys.cubiesPerFace; i++) {
    data[i] = i / phys.cubiesPerFace;
  }
}

//
// Populate global cube structures.
//
bool initCube(const unsigned int n, const float spacing) {
  for (Action a = 0; a < Actions; a++) {
    state.actions[a].f = Faces;
  }
  state.corner.index = state.corner.orientation = 0;
  normalizeIsometric();
  initPhys(n, spacing);
  data = malloc(Faces * phys.cubiesPerFace * sizeof(int));
  return data != NULL;
}

//
// Translate key press code to face.
//
Face keyToFace(const int key) {
  Face f = 0;
  for (; f < Faces; f++) {
    if (key == FaceToKey[f]) break;
  }
  return f;
}

//
// Update specified action to given face
// and orientation. No-op if face is inactive.
// If given parameters match current action,
// cancel the action.
//
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

//
// Input callback. Sets new animation, resets cube,
// or exits the application as appropriate.
//
void input(GLFWwindow* window, int key, int s, int action, int mods) {
  if (action != GLFW_PRESS) return;
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

//
// Called when window gains or loses focus.
//
void focus(GLFWwindow* w, int focused) {
  if (!focused) glfwSetWindowShouldClose(w, GLFW_TRUE);
}

//
// Prepares the openGL framework for use.
//
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
  glfwSetKeyCallback(w, input);
  glfwSetWindowFocusCallback(w, focus);
  glfwMakeContextCurrent(w);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glLineWidth(LINE_WIDTH);
  return w;
Fail:
  glfwTerminate();
  return NULL;
}