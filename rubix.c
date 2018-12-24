#include "rubix.h"

float colors[Faces][3] = CUBE_COLORS;
double x, y;
bool mouseActive;
int corner;
AnimationState state;

int main(int argc, char *argv[]) {
  GLFWwindow* window = prepareGlfw();
  if (window == NULL) {
    return EXIT_FAILURE;
  }

  mouseActive = false;
  unsigned int n;
  corner = 0;
  invalidateAnimationState();

  if (argc < 2 || sscanf(argv[1], "%i", &n) != 1) {
    n = 3;
    printf(USAGE);
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
      glVertex3f((float)(n*(curx - x))/1000, (float)(n*(y - cury))/1000, 0);
    glEnd();
    glPopMatrix();
  }

  glMatrixMode(GL_PROJECTION);
  centerOnCorner(corner);

  glMatrixMode(GL_MODELVIEW);
  cube(n, 1.1);

  if (isFaceActive(state.face)) {
    state.duration++;
  }

  if (!mouseActive && isFaceActive(state.view)) {
    int b[Axes];
    faceVector(state.view, b);
    glRotatef(1, b[0], b[1], b[2]);
  }

  glfwSwapBuffers(window);
}

//
// Insert at location a in array b the value c and populate the rest from d.
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

void insert(Face f, float a[Axes-1], float b, float c) {
  float d[Axes-1];
  float e[Axes];
  d[0] = a[0] + b;
  d[1] = a[1] + c;
  axisInsert(f.axis, e, f.offset, d);
  glVertex3f(e[0], e[1], e[2]);
}

void square(Face f, float *p, RubixCol c) {
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

void cube(unsigned int n, float spacing) {
  float p[Axes-1];
  float maxDim = spacing * (n - 1) + 1;;
  bool shouldRotate;
  int b[Axes];
  int c[Axes-1];
  int d;
  Face f;
  faceVector(state.face, b);

  for (c[0] = 0; c[0] < n; c[0]++) {
    p[0] = spacing * (2*c[0] - (int)n + 1);
    for (c[1] = 0; c[1] < n; c[1]++) {
      p[1] = spacing * (2*c[1] - (int)n + 1);
      for (unsigned int k = 0; k < Faces; k++) {
        f = canonicalFace(k, maxDim);
        glPushMatrix();
        shouldRotate = false;
        if (isFaceActive(state.face)) {
          if (equals(f, state.face)) {
            shouldRotate = true;
          }
          if (f.axis != state.face.axis) {
            d = (int)(f.axis) - (int)(state.face.axis) - 1;
            if (d < 0) {
              d += Axes;
            }
            d = Axes - 2 - d;
            // todo: verify if I worked out the signs correctly here
            if (c[d] == 0 && state.face.offset < 0) {
              shouldRotate = true;
            }
            if (c[d] == n-1 && state.face.offset > 0) {
              shouldRotate = true;
            }
          }
          if (shouldRotate) {
            glRotatef(state.duration, b[0], b[1], b[2]);
          }
        }
        square(f, p, k);
        glPopMatrix();
      }
    }
  }
}

void centerOnCorner(unsigned char c) {
  int i;
  int x[4];
  for (i = 0; i < 4; i++) {
    x[i] = (c % 2) ? -1 : 1;
    c /= 2;
  }

  float isom[4*4] = {
    1/sqrt(2),1/sqrt(6),1/sqrt(3),0,
    -1/sqrt(2),1/sqrt(6),1/sqrt(3),0,
    0,-2/sqrt(6),1/sqrt(3),0,
    0,0,0,1,
  };

  for (i = 0; i < 4*4; i++) {
    isom[i] *= x[i/4];
  }
  glLoadMatrixf(isom);
}

//
// Face operations
//
void faceVector(Face a, int* b) {
  unsigned int i;
  for (i = 0; i < Axes; i++) {
    b[i] = (i == a.axis) * ((a.offset > 0) ? -1 : 1);
  }
}

Face canonicalFace(RubixCol c, float offset) {
  Face f;
  f.axis = c % (Faces/2);
  f.offset = (c/(Faces/2)) ? -offset : offset;
  return f;
}

bool equals(Face a, Face b) {
  if (a.axis != b.axis) {
    return false;
  }
  if (a.offset > 0 && b.offset > 0) {
    return true;
  }
  if (a.offset < 0 && b.offset < 0) {
    return true;
  }
  return false;
}

bool isFaceActive(Face f) {
  return (f.axis < Axes) && (f.offset != 0);
}

void invalidateFace(Face *f) {
  f->axis = Axes;
  f->offset = 0;
}

void invalidateAnimationState() {
  invalidateFace(&state.face);
  invalidateFace(&state.view);
  state.duration = 0;
}

Face keyToFace(int key) {
  Face f;
  f.offset = 1;
  switch (key) {
    case GLFW_KEY_B:
      f.offset = -1;
    case GLFW_KEY_F:
      f.axis = Front;
      break;
    case GLFW_KEY_L:
      f.offset = -1;
    case GLFW_KEY_R:
      f.axis = Right;
      break;
    case GLFW_KEY_D:
      f.offset = -1;
    case GLFW_KEY_U:
    default:
      f.axis = Upper;
      break;
  }
  return f;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  Face new;
  if (action != GLFW_PRESS) {
    return;
  }
  if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) {
    corner = key - GLFW_KEY_1;
    return;
  }
  switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
      glfwSetWindowShouldClose(window, GLFW_TRUE);
      break;
    case GLFW_KEY_A:
      if (mods & GLFW_MOD_SHIFT) {
        printf("deactivating mouse\n");
        mouseActive = false;
      } else {
        mouseActive = true;
        glfwGetCursorPos(window, &x, &y);
        printf("activating mouse: (x = %f, y = %f)\n", x, y);
      }
      break;
    case GLFW_KEY_B:
    case GLFW_KEY_D:
    case GLFW_KEY_F:
    case GLFW_KEY_L:
    case GLFW_KEY_R:
    case GLFW_KEY_U:
      new = keyToFace(key);
      if (mods & GLFW_MOD_SHIFT) {
        if (isFaceActive(state.view) && equals(new, state.view)) {
          invalidateFace(&state.view);
        } else {
          state.view = new;
        }
        break;
      }
      state.duration = 0;
      if (isFaceActive(state.face) && equals(new, state.face)) {
        invalidateFace(&state.face);
      } else {
        state.face = new;
      }
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
  window = glfwCreateWindow(1, 1, WINDOW_TITLE, NULL, NULL);
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