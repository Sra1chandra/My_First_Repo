#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
  GLuint VertexArrayID;
  GLuint VertexBuffer;
  GLuint ColorBuffer;

  GLenum PrimitiveMode;
  GLenum FillMode;
  int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
  glm::mat4 projection;
  glm::mat4 model;
  GLuint MatrixID;
  glm::mat4 view;
} Matrices;

struct Rectangle{
  VAO *rect;
  double a;
  double b;
  double x_pos,y_pos,angle;
  int color;
};

struct Circle{
  VAO *circle;
  double a;
  double b;
  double x_pos,y_pos;
  int color;
};

struct Mirror{
  struct Rectangle rect;
};
struct Mirror mirror[4];

struct Bin {
  struct Circle top,bottom;
  struct Rectangle rect;
  double bin_height;
  double bin_width;
  double x_pos;
  double y_pos;
};

struct Gun {
  struct Rectangle rect1,rect2;
  struct Circle circle1,circle2;
  double rot_angle;
  double y_pos;
  double x_pos;
};
struct Bricks{
  struct Rectangle brick[100];
  int pointer;
  int bricks_count;
};
struct Bullets{
  struct Rectangle bullet[10];
  int pointer;
  int count;
};

struct Display
{
  struct Rectangle segment[7];
};

struct Display display;

struct Bin bin[2];
struct Gun gun;
struct Bricks bricks;
struct Bullets bullets;
GLFWwindow* window;
GLuint programID;
void CreateBullet();
/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

  // Create the shaders
  GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
  GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

  // Read the Vertex Shader code from the file
  std::string VertexShaderCode;
  std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
  if(VertexShaderStream.is_open())
  {
    std::string Line = "";
    while(getline(VertexShaderStream, Line))
    VertexShaderCode += "\n" + Line;
    VertexShaderStream.close();
  }

  // Read the Fragment Shader code from the file
  std::string FragmentShaderCode;
  std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
  if(FragmentShaderStream.is_open()){
    std::string Line = "";
    while(getline(FragmentShaderStream, Line))
    FragmentShaderCode += "\n" + Line;
    FragmentShaderStream.close();
  }

  GLint Result = GL_FALSE;
  int InfoLogLength;

  // Compile Vertex Shader
  printf("Compiling shader : %s\n", vertex_file_path);
  char const * VertexSourcePointer = VertexShaderCode.c_str();
  glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
  glCompileShader(VertexShaderID);

  // Check Vertex Shader
  glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  std::vector<char> VertexShaderErrorMessage(InfoLogLength);
  glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
  fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

  // Compile Fragment Shader
  printf("Compiling shader : %s\n", fragment_file_path);
  char const * FragmentSourcePointer = FragmentShaderCode.c_str();
  glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
  glCompileShader(FragmentShaderID);

  // Check Fragment Shader
  glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
  glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
  fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

  // Link the program
  fprintf(stdout, "Linking program\n");
  GLuint ProgramID = glCreateProgram();
  glAttachShader(ProgramID, VertexShaderID);
  glAttachShader(ProgramID, FragmentShaderID);
  glLinkProgram(ProgramID);

  // Check the program
  glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
  glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
  glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
  fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

  glDeleteShader(VertexShaderID);
  glDeleteShader(FragmentShaderID);

  return ProgramID;
}

static void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
  struct VAO* vao = new struct VAO;
  vao->PrimitiveMode = primitive_mode;
  vao->NumVertices = numVertices;
  vao->FillMode = fill_mode;

  // Create Vertex Array Object
  // Should be done after CreateWindow and before any other GL calls
  glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
  glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
  glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

  glBindVertexArray (vao->VertexArrayID); // Bind the VAO
  glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
  glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
  glVertexAttribPointer(
    0,                  // attribute 0. Vertices
    3,                  // size (x,y,z)
    GL_FLOAT,           // type
    GL_FALSE,           // normalized?
    0,                  // stride
    (void*)0            // array buffer offset
  );

  glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
  glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
  glVertexAttribPointer(
    1,                  // attribute 1. Color
    3,                  // size (r,g,b)
    GL_FLOAT,           // type
    GL_FALSE,           // normalized?
    0,                  // stride
    (void*)0            // array buffer offset
  );

  return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
  GLfloat* color_buffer_data = new GLfloat [3*numVertices];
  for (int i=0; i<numVertices; i++) {
    color_buffer_data [3*i] = red;
    color_buffer_data [3*i + 1] = green;
    color_buffer_data [3*i + 2] = blue;
  }

  return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
  // Change the Fill Mode for this object
  glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

  // Bind the VAO to use
  glBindVertexArray (vao->VertexArrayID);

  // Enable Vertex Attribute 0 - 3d Vertices
  glEnableVertexAttribArray(0);
  // Bind the VBO to use
  glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

  // Enable Vertex Attribute 1 - Color
  glEnableVertexAttribArray(1);
  // Bind the VBO to use
  glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

  // Draw the geometry !
  glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
* Customizable functions *
**************************/
double time_to_hit_space= glfwGetTime(), current_time;
double Speed_of_Brick=0.01;
float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
bool RIGHT_control=false,RIGHT=false,LEFT=false,RIGHT_alt=false;
int Score=0;
int fbwidth,fbheight;
double zoom=0;
//float rot_angle=0;
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
  // Function is called first on GLFW_PRESS.

  if (action == GLFW_RELEASE) {
    switch (key) {
      case GLFW_KEY_A:
      if(gun.rot_angle<80)
      gun.rot_angle+=5;
      break;
      case GLFW_KEY_D:
      if(gun.rot_angle>-80)
      gun.rot_angle-=5;
      break;
      case GLFW_KEY_SPACE:
      current_time=glfwGetTime();
      if(current_time - time_to_hit_space>=0.5)
      {
        CreateBullet();
        time_to_hit_space=current_time;
      }
      break;
      case GLFW_KEY_S:
      if(gun.y_pos<3.5)
      gun.y_pos+=0.2;
      break;
      case GLFW_KEY_F:
      if(gun.y_pos>-3.5)
      gun.y_pos-=0.2;
      break;
      case GLFW_KEY_N:
      if(Speed_of_Brick<0.05)
      Speed_of_Brick+=0.005;
      break;
      case GLFW_KEY_M:
      if(Speed_of_Brick>0.01)
      Speed_of_Brick-=0.005;
      break;
      case GLFW_KEY_RIGHT_CONTROL:
      RIGHT_control=false;
      break;
      case GLFW_KEY_RIGHT_ALT:
      RIGHT_alt=false;
      break;
      case GLFW_KEY_RIGHT:
      RIGHT=false;
      break;
      case GLFW_KEY_LEFT:
      LEFT=false;
      break;
      case GLFW_KEY_UP:
        zoom+=0.2;
      break;
      case GLFW_KEY_DOWN:
        zoom-=0.2;
      break;
      default:
      break;
    }
  }
  else if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_ESCAPE:
      quit(window);
      break;
      case GLFW_KEY_RIGHT_CONTROL:
      RIGHT_control=true;
      break;
      case GLFW_KEY_RIGHT_ALT:
      RIGHT_alt=true;
      break;
      case GLFW_KEY_RIGHT:
      RIGHT=true;
      break;
      case GLFW_KEY_LEFT:
      LEFT=true;
      break;
      default:
      break;
    }
  }
  if(RIGHT_control==true &&RIGHT==true)
  bin[0].x_pos+=0.2;
  if(RIGHT_control==true && LEFT==true )
  bin[0].x_pos-=0.2;
  if(RIGHT_alt==true && RIGHT==true)
  bin[1].x_pos+=0.2;
  if(RIGHT_alt==true  && LEFT==true )
  bin[1].x_pos-=0.2;
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
  switch (key) {
    case 'Q':
    case 'q':
    quit(window);
    break;
    default:
    break;
  }
}
bool mouse_left=false;
bool bin0=false,bin1=false,gun0=false;
double x_pos,y_pos;

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
  switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
    if (action == GLFW_PRESS)
    {
      mouse_left=true;
      glfwGetCursorPos(window,&x_pos,&y_pos);
      double temp_x=8*((x_pos-fbwidth/2)/fbwidth*1.0);
      double temp_y=-8*((y_pos-fbheight/2)/fbheight*1.0);
      if(bin[0].x_pos-bin[0].bin_width/2<=temp_x&&bin[0].x_pos+bin[0].bin_width/2>=temp_x)
      if(bin[0].y_pos>=temp_y&&bin[0].y_pos-bin[0].bin_height<=temp_y)
      bin0=true;

      if(bin[1].x_pos-bin[1].bin_width/2<=temp_x&&bin[1].x_pos+bin[1].bin_width/2>=temp_x)
      if(bin[1].y_pos>=temp_y&&bin[1].y_pos-bin[1].bin_height<=temp_y)
      bin1=true;

      if(gun.x_pos-gun.rect1.a/2<=temp_x&&gun.x_pos+gun.rect1.a/2>=temp_x)
      if(gun.y_pos>=temp_y&&gun.y_pos-gun.rect1.b<=temp_y)
      gun0=true;
    }
    if (action == GLFW_RELEASE)
    {
      bin0=false;
      bin1=false;
      gun0=false;
      mouse_left=false;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    if (action == GLFW_RELEASE) {
      //MouseControl();
    }
    break;
    default:
    break;
  }
  if(mouse_left)
  {
  }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
  fbwidth=width; fbheight=height;

  /* With Retina display on Mac OS X, GLFW's FramebufferSize
  is different from WindowSize */
  glfwGetFramebufferSize(window, &fbwidth, &fbheight);

  GLfloat fov = 90.0f;

  // sets the viewport of openGL renderer
  glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

  // set the projection matrix as perspective
  /* glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
  // Store the projection matrix in a variable for future use
  // Perspective projection for 3D views
  // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

  // Ortho projection for 2D views
  Matrices.projection = glm::ortho(-4.0, 4.0, -4.0, 4.0, 0.1, 500.0);
}

VAO *triangle,*rectangle,*rectangle1,*circle;

// Creates the triangle object used in this sample code
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data);
  // create3DObject creates and returns a handle to a VAO that can be used later
}

// Creates the rectangle object used in this sample code
void CreateRectangle (float length,float breadth,int c,VAO **object)
{
  int r=0,g=0,b=0;
  if(c==0) r=1;
  if(c==1) g=1;
  if(c==2) b=1;
  // GL3 accepts only Triangles. Quads are not supported
  GLfloat vertex_buffer_data [] = {
    -length/2,-breadth,0, // vertex 1
    -length/2,0,0, // vertex 2
    length/2, 0,0, // vertex 3

    length/2, 0,0, // vertex 3
    length/2, -breadth,0, // vertex 4
    -length/2,-breadth,0  // vertex 1
  };
  GLfloat color_buffer_data [] = {
    r,g,b, // color 1
    1,1,1, // color 2
    1,1,1, // color 3

    1,1,1, // color 3
    r,g,b, // color 4
    r,g,b  // color 1
  };
  // create3DObject creates and returns a handle to a VAO that can be used later
  *object = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
const float DEG2RAD = 3.14159/180;
// Creates the circle object used in this sample code
void createCircle (float a,float b,int c,VAO **object)
{
  GLfloat vertex_buffer_data [9*360];
  for(int i=0;i<360;i++)
  for(int j=0;j<3;j++)
  {
    for(int k=0;k<3;k++)
    {
      if(k==0)
      vertex_buffer_data[i*9+j*3+k]=floor((j+1)/2)*cos((i+floor(j/2))*DEG2RAD)*(a);
      if(k==1)
      vertex_buffer_data[i*9+j*3+k]=floor((j+1)/2)*sin((i+floor(j/2))*DEG2RAD)*(b);
      if(k==2)
      vertex_buffer_data[i*9+j*3+k]=0;
    }
  }
  GLfloat color_buffer_data [9*360];
  for(int i=0;i<360;i++)
    for(int j=0;j<3;j++)
      for(int k=0;k<3;k++)
      {
        if(j==0)
          color_buffer_data[i*9+j*3+k]=0.8;
        else if(j!=0 && k==c)
          color_buffer_data[i*9+j*3+k]=0.8*floor((j+1)/2);
        else
        color_buffer_data[i*9+j*3+k]=0;
  }
  // create3DObject creates and returns a handle to a VAO that can be used later
  *object = create3DObject(GL_TRIANGLES, 3*360, vertex_buffer_data, color_buffer_data, GL_FILL);
}

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;
double last_update_time = glfwGetTime();
double last_update_time1 = glfwGetTime();

double check(double x1,double y1,double x2,double y2,double x,double y)
{
  return (y1-y2)*(x-x1)-(x1-x2)*(y-y1);
}

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void moveTriangle(glm::mat4 VP,glm::vec3 translate)
{
  glm::mat4 MVP;	// MVP = Projection * View * Model
  // Load identity to model matrix
  Matrices.model = glm::mat4(1.0f);
  /* Render your scene */
  //glm::vec3 translate=glm::vec3(0.0f, 1.0f, 0.0f);
  glm::mat4 translateTriangle =glm::translate (translate) *glm::translate (glm::vec3(0.0f, 1.0f, 0.0f)); // glTranslatef
  glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 rtranslateTriangle = glm::translate (glm::vec3(0.0f, -1.0f, 0.0f)); // glTranslatef
  glm::mat4 triangleTransform = translateTriangle * rotateTriangle * rtranslateTriangle;
  Matrices.model *= triangleTransform;
  MVP = VP * Matrices.model; // MVP = p * V * M
  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(triangle);
}
void CreateBin(struct Bin* bin,int c)
{
  bin->bin_height=1;bin->bin_width=0.8;
  bin->rect.a=bin->bin_width;bin->rect.b=bin->bin_height;bin->rect.color=c;
  bin->top.a=bin->bin_width/2;bin->bottom.a=bin->bin_width/2;
  bin->top.b=bin->bin_width/2;bin->bottom.b=bin->bin_width/2;
  bin->top.color=c;bin->bottom.color=c;
  CreateRectangle (bin->rect.a,bin->rect.b,bin->rect.color,&bin->rect.rect);
  createCircle (bin->top.a,bin->top.b,bin->top.color,&bin->top.circle);
  createCircle (bin->bottom.a,bin->bottom.b,bin->bottom.color,&bin->bottom.circle);
}

void CreateGun(struct Gun* gun,int c)
{
  gun->circle1.a=0.05;gun->circle1.b=0.1;gun->circle1.color=c;
  gun->circle2.a=0.25;gun->circle2.b=0.25;gun->circle2.color=c;
  createCircle(gun->circle1.a,gun->circle1.b,gun->circle1.color,&gun->circle1.circle);
  createCircle(gun->circle2.a,gun->circle2.b,gun->circle2.color,&gun->circle2.circle);
  gun->rect1.a=0.4;gun->rect1.b=0.4;gun->rect1.color=c;
  gun->rect2.a=0.6;gun->rect2.b=0.2;gun->rect2.color=c;
  CreateRectangle(gun->rect1.a,gun->rect1.b,gun->rect1.color,&gun->rect1.rect);
  CreateRectangle(gun->rect2.a,gun->rect2.b,gun->rect2.color,&gun->rect2.rect);
}
void CreateBrick()
{
  int i=(bricks.pointer+bricks.bricks_count)%100;
  bricks.brick[i].x_pos=(rand()%11-5)/2.5;
  bricks.brick[i].y_pos=4.0;
  bricks.brick[i].color=rand()%3;
  if(bricks.brick[i].color==2)
  bricks.brick[i].color++;
  bricks.brick[i].a=0.2;
  bricks.brick[i].b=0.4;
  CreateRectangle(bricks.brick[i].a,bricks.brick[i].b,bricks.brick[i].color,&bricks.brick[i].rect);
  bricks.bricks_count+=1;
}
void CreateBullet()
{
  int i=(bullets.pointer+bullets.count)%10;
  double gun_length = gun.rect1.a+gun.rect2.a;
  bullets.bullet[i].angle=gun.rot_angle;
  bullets.bullet[i].x_pos=-gun.rect1.a/2+gun.x_pos+gun_length*cos((bullets.bullet[i].angle)*M_PI/180.0f);
  bullets.bullet[i].y_pos=-gun.rect1.b/2+gun.y_pos+gun_length*sin((bullets.bullet[i].angle)*M_PI/180.0f);
  bullets.bullet[i].a=gun.rect2.b/3;bullets.bullet[i].b=0.8;
  bullets.bullet[i].color=3;
  CreateRectangle(bullets.bullet[i].a,bullets.bullet[i].b,bullets.bullet[i].color,&bullets.bullet[i].rect);
  bullets.count+=1;
}
void CreateMirror()
{
  mirror[0].rect.angle=-135;mirror[0].rect.a=1.2;mirror[0].rect.b=0.05;
  mirror[0].rect.x_pos=3;mirror[0].rect.y_pos=-1.5;
  CreateRectangle(mirror[0].rect.a,mirror[0].rect.b,3,&mirror[0].rect.rect);
  mirror[1].rect.angle=135;mirror[1].rect.a=1.2;mirror[1].rect.b=0.05;
  mirror[1].rect.x_pos=3;mirror[1].rect.y_pos=1.5;
  CreateRectangle(mirror[1].rect.a,mirror[1].rect.b,3,&mirror[1].rect.rect);
  mirror[2].rect.angle=90;mirror[2].rect.a=1.2;mirror[2].rect.b=0.05;
  mirror[2].rect.x_pos=3.5;mirror[2].rect.y_pos=0;
  CreateRectangle(mirror[2].rect.a,mirror[2].rect.b,3,&mirror[2].rect.rect);

}
void drawBin(glm::mat4 VP,glm::vec3 translate,struct Bin *bin,glm::vec3 rotate,double angle)
{
  glm::mat4 MVP;	// MVP = Projection * View * Model
  glm::mat4 translateRectangle = glm::translate(translate);
  Matrices.model = glm::mat4(1.0f);
  Matrices.model *= (translateRectangle);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(bin->rect.rect);
  Matrices.model = glm::mat4(1.0f);
  /* Render your scene */
  glm::mat4 translateCircle = glm::translate (translate); // glTranslatef
  glm::mat4 rotateCircle = glm::rotate((float)(angle*M_PI/180.0f),rotate);  // rotate about vector (1,0,0)
  Matrices.model *= translateCircle*rotateCircle;
  MVP = VP * Matrices.model; // MVP = p * V * M
  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(bin->top.circle);
  Matrices.model = glm::mat4(1.0f);
  translateCircle*=glm::translate(glm::vec3(0,-bin->bin_height,0));
  rotateCircle = glm::rotate((float)(-angle*M_PI/180.0f), rotate);
  Matrices.model*=translateCircle*rotateCircle;
  MVP = VP * Matrices.model; // MVP = p * V * M
  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(bin->bottom.circle);
}

void drawGun(glm::mat4 VP,glm::vec3 translate,struct Gun *gun)
{
  glm::mat4 MVP;	// MVP = Projection * View * Model
  glm::mat4 move=glm::translate(glm::vec3(-gun->rect1.a/2,-gun->rect1.b/2,0));
  glm::mat4 rotate=glm::rotate((float)(gun->rot_angle*M_PI/180.0f),glm::vec3(0,0,1));
  move=move*rotate*glm::translate(glm::vec3(gun->rect1.a/2,gun->rect1.b/2,0));
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangle = glm::translate(translate);
  Matrices.model *= (translateRectangle)*move;
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(gun->rect1.rect);

  Matrices.model = glm::mat4(1.0f);
  move = glm::translate(glm::vec3(gun->rect1.a/2+gun->rect2.a/2,-gun->rect1.b/2+gun->rect2.b/2, 0));
  move = move * glm::translate(glm::vec3(-gun->rect1.a-gun->rect2.a/2,-gun->rect2.b/2,0));
  move = move * rotate * glm::translate(glm::vec3(gun->rect1.a+gun->rect2.a/2,gun->rect2.b/2,0));
  Matrices.model *= (translateRectangle)*move;
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(gun->rect2.rect);

  Matrices.model = glm::mat4(1.0f);
  move = glm::translate(glm::vec3(gun->rect1.a/2+gun->rect2.a,-gun->rect1.a/2, 0));
  move = move * glm::translate(glm::vec3(-gun->rect1.a-gun->rect2.a,0,0));
  move = move * rotate * glm::translate(glm::vec3(gun->rect1.a+gun->rect2.a,0,0));
  Matrices.model *= translateRectangle*move;
  MVP = VP * Matrices.model; // MVP = p * V * M
  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(gun->circle1.circle);

  Matrices.model = glm::mat4(1.0f);
  Matrices.model *= (translateRectangle)*glm::translate(glm::vec3(-gun->rect1.a/2,-gun->rect1.b/2,0));
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(gun->circle2.circle);

}
void drawRectangle(glm::mat4 VP,glm::vec3 translate,VAO ** rectangle,double angle)
{
  glm::mat4 MVP;	// MVP = Projection * View * Model
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangle = glm::translate(translate);
  glm::mat4 rotateRectangle = glm::rotate((float)((angle)*M_PI/180.0f), glm::vec3(0,0,1) );
  Matrices.model *= translateRectangle*rotateRectangle;
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(*rectangle);
}
void drawCircle(glm::mat4 VP,glm::vec3 translate,VAO **circle,glm::vec3 rotate,double angle)
{
  glm::mat4 MVP;	// MVP = Projection * View * Model
  Matrices.model = glm::mat4(1.0f);
  /* Render your scene */
  glm::mat4 translateCircle = glm::translate (translate); // glTranslatef
  glm::mat4 rotateCircle = glm::rotate((float)(angle*M_PI/180.0f), rotate );  // rotate about vector (1,0,0)
  glm::mat4 CircleTransform = translateCircle * rotateCircle;
  //glm::mat4 CircleTransform = translateCircle;
  Matrices.model *= CircleTransform;
  MVP = VP * Matrices.model; // MVP = p * V * M
  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(*circle);
}
void drawBricks(glm::mat4 VP)
{
  int count=bricks.bricks_count,pointer=bricks.pointer,j;
  for(int i=0;i<count;i++)
  {
    j=(pointer+i)%100;
    if(bricks.brick[j].y_pos>4)
    {
      bricks.pointer=(bricks.pointer+1)%100;
      bricks.bricks_count--;
    }
    if(bricks.brick[j].color!=-1)
    drawRectangle(VP,glm::vec3(bricks.brick[j].x_pos, bricks.brick[j].y_pos, 0.0f),&bricks.brick[j].rect,0);
  }
}
void drawBullets(glm::mat4 VP)
{
  int count=bullets.count,pointer=bullets.pointer,j;
  glm::mat4 MVP;	// MVP = Projection * View * Model
  for(int i=0;i<count;i++)
  {
    j=(pointer+i)%10;
    if(bullets.bullet[j].y_pos<-4||bullets.bullet[j].y_pos>4||bullets.bullet[j].x_pos>4||bullets.bullet[j].x_pos<-4)
    {
      bullets.pointer=(bullets.pointer+1)%10;
      bullets.count--;
    }
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 rotateRectangle = glm::rotate((float)((90+bullets.bullet[j].angle)*M_PI/180.0f), glm::vec3(0,0,1) );
    glm::mat4 translateRectangle = glm::translate(glm::vec3(bullets.bullet[j].x_pos,bullets.bullet[j].y_pos, 0.0f));
    Matrices.model *= translateRectangle*rotateRectangle;//translateRectangle*translateRectangle1*rotateRectangle;
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(bullets.bullet[j].rect);
  }
}
void drawMirror(glm::mat4 VP)
{
  for(int i=0;i<3;i++)
    drawRectangle(VP,glm::vec3(mirror[i].rect.x_pos, mirror[i].rect.y_pos, 0.0f),&mirror[i].rect.rect,mirror[i].rect.angle);
}
void bin_collection(struct Bin *bin)
{
  int j;
  for(int i=0;i<bricks.bricks_count;i++)
  {
    j=(i+bricks.pointer)%100;
    if(bricks.brick[j].color!=-1)
    {
      if(bricks.brick[j].y_pos<= bin->y_pos&&bricks.brick[j].y_pos-bricks.brick[j].b>= bin->y_pos-bin->bin_height)
      if(bricks.brick[j].x_pos-bricks.brick[j].a/2>=bin->x_pos-bin->bin_width/2
        && bricks.brick[j].x_pos+bricks.brick[j].a/2<=bin->x_pos+bin->bin_width/2)
        {
          if(bricks.brick[j].color==bin->rect.color)
          {
            Score+=1;
          }
          if(bricks.brick[j].color==3)
          {
            cout<<"GAME OVER"<<endl;
            cout << "Final Score is "<<Score<<endl;
            keyboardChar (window,'Q');
          }
        bricks.brick[j].color=-1;
      }
      }
    }
  }
  void reflection(struct Mirror *mirror)
  {
    double x1=mirror->rect.x_pos-mirror->rect.a/2*cos((mirror->rect.angle)*M_PI/180.0f);
    double y1=mirror->rect.y_pos-mirror->rect.a/2*sin((mirror->rect.angle)*M_PI/180.0f);
    double x2=mirror->rect.x_pos+mirror->rect.a/2*cos((mirror->rect.angle)*M_PI/180.0f);
    double y2=mirror->rect.y_pos+mirror->rect.a/2*sin((mirror->rect.angle)*M_PI/180.0f);
    for(int i=0;i<bullets.count;i++)
    {
      int j=(i+bullets.pointer)%10;
      double x3=bullets.bullet[j].x_pos,y3=bullets.bullet[j].y_pos;
      double x4=x3+bullets.bullet[j].b*cos((bullets.bullet[j].angle)*M_PI/180.0f);
      double y4=y3+bullets.bullet[j].b*sin((bullets.bullet[j].angle)*M_PI/180.0f);
      double check1=check(x1,y1,x2,y2,x3,y3);
      double check2=check(x1,y1,x2,y2,x4,y4);
      double check3=check(x3,y3,x4,y4,x1,y1);
      double check4=check(x3,y3,x4,y4,x2,y2);
      if(check1*check2<=0 && check3*check4<=0)
      {
        bullets.bullet[j].angle=2*mirror->rect.angle-bullets.bullet[j].angle;
        bullets.bullet[j].x_pos=x4;//bullets.bullet[j].b*cos((bullets.bullet[j].angle)*M_PI/180.0f);
        bullets.bullet[j].y_pos=y4;//bullets.bullet[j].b*sin((bullets.bullet[j].angle)*M_PI/180.0f);
      }
    }
  }
  void collision()
  {
    bin_collection(&bin[0]);
    bin_collection(&bin[1]);
    for(int i=0;i<bullets.count;i++)
    {
      for(int j=0;j<bricks.bricks_count;j++)
      {
        int t1=(bricks.pointer+j)%100;
        int t2=(bullets.pointer+i)%10;
        double x1=bricks.brick[t1].x_pos,y1=bricks.brick[t1].y_pos;//+0.1*bricks.brick[t1].b;
        double x2=bricks.brick[t1].x_pos,y2=bricks.brick[t1].y_pos-1*bricks.brick[t1].b;
        double x3=bullets.bullet[t2].x_pos-0.1*bullets.bullet[t2].b*cos((bullets.bullet[j].angle)*M_PI/180.0f);
        double y3=bullets.bullet[t2].y_pos-0.1*bullets.bullet[t2].b*sin((bullets.bullet[j].angle)*M_PI/180.0f);
        double x4=bullets.bullet[t2].x_pos+1.1*bullets.bullet[t2].b*cos((bullets.bullet[j].angle)*M_PI/180.0f);
        double y4=bullets.bullet[t2].y_pos+1.1*bullets.bullet[t2].b*sin((bullets.bullet[j].angle)*M_PI/180.0f);
        double check4=check(x3,y3,x4,y4,x2,y2);
        double check1=check(x1,y1,x2,y2,x3,y3);
        double check2=check(x1,y1,x2,y2,x4,y4);
        double check3=check(x3,y3,x4,y4,x1,y1);

        if(check1*check2<=0 && check3*check4<=0)
        {
          if(bricks.brick[t1].color==3)
            Score++;
          bricks.brick[t1].color=-1;
        }
      }
      bullets.bullet[(bullets.pointer+i)%10].x_pos+=0.15*cos(bullets.bullet[(bullets.pointer+i)%10].angle*M_PI/180.0f);
      bullets.bullet[(bullets.pointer+i)%10].y_pos+=0.15*sin(bullets.bullet[(bullets.pointer+i)%10].angle*M_PI/180.0f);
    }
    reflection(&mirror[0]);
    reflection(&mirror[1]);
    reflection(&mirror[2]);

    current_time = glfwGetTime(); // Time in seconds
    if ((current_time - last_update_time) >= 0.01) { // atleast 0.01s elapsed since last frame

      for(int i=0;i<bricks.bricks_count;i++)
      bricks.brick[(bricks.pointer+i)%100].y_pos-=Speed_of_Brick;
      // do something every 0.5 seconds ..
      last_update_time = current_time;
    }

    if ((current_time - last_update_time1) >= 1) {
      CreateBrick();
      last_update_time1 = current_time;
    }
  }
  void Create_Seven_Segment()
  {
    for(int i=0;i<3;i++){
      display.segment[i].a=0.4;
      display.segment[i].b=0.1;
    }
    for(int i=3;i<7;i++){
      display.segment[i].b=0.4;
      display.segment[i].a=0.1;
    }
    display.segment[0].x_pos=0;display.segment[0].y_pos=0;
    display.segment[1].x_pos=0;display.segment[1].y_pos=-0.4;
    display.segment[2].x_pos=0;display.segment[2].y_pos=-0.8;
    display.segment[3].x_pos=-0.2;display.segment[3].y_pos=0;
    display.segment[4].x_pos=0.2;display.segment[4].y_pos=0;
    display.segment[5].x_pos=-0.2;display.segment[5].y_pos=-0.4;
    display.segment[6].x_pos=0.2;display.segment[6].y_pos=-0.4;
    for(int i=0;i<7;i++)
    {
      display.segment[i].x_pos+=3.5;
      display.segment[i].y_pos+=3.8;
    }
    for(int i=0;i<7;i++)
      CreateRectangle(display.segment[i].a,display.segment[i].b,2,&display.segment[i].rect);
  }
  void drawSevenSegment(glm::mat4 VP)
  {
    int temp=Score,digit;
  while(temp!=0)
    {
      for(int i=0;i<7;i++)
      {
        display.segment[i].x_pos-=0.6;
        //display.segment[i].y_pos+=3.8;
      }
    digit=temp%10;
     temp=temp/10;
    if(digit!=1&&digit!=4)
      drawRectangle(VP,glm::vec3(display.segment[0].x_pos,display.segment[0].y_pos, 0),&display.segment[0].rect,0);
    if(digit!=1&&digit!=7&&digit!=0)
      drawRectangle(VP,glm::vec3(display.segment[1].x_pos,display.segment[1].y_pos, 0),&display.segment[1].rect,0);
    if(digit!=1&&digit!=7&&digit!=4)
      drawRectangle(VP,glm::vec3(display.segment[2].x_pos,display.segment[2].y_pos, 0),&display.segment[2].rect,0);
    if(digit!=1&&digit!=2&&digit!=3&&digit!=7)
      drawRectangle(VP,glm::vec3(display.segment[3].x_pos,display.segment[3].y_pos, 0),&display.segment[3].rect,0);
    if(digit!=5&&digit!=6)
      drawRectangle(VP,glm::vec3(display.segment[4].x_pos,display.segment[4].y_pos, 0),&display.segment[4].rect,0);
    if(digit==0||digit==2||digit==6||digit==8)
      drawRectangle(VP,glm::vec3(display.segment[5].x_pos,display.segment[5].y_pos, 0),&display.segment[5].rect,0);
    if(digit!=2)
      drawRectangle(VP,glm::vec3(display.segment[6].x_pos,display.segment[6].y_pos, 0),&display.segment[6].rect,0);
    }

  }
  void mouse_func(GLFWwindow* window)
  {
    if(mouse_left)
    {
      glfwGetCursorPos(window,&x_pos,&y_pos);
      x_pos=8*((x_pos-fbwidth/2)/fbwidth*1.0);
      y_pos=-8*((y_pos-fbheight/2)/fbheight*1.0);
      current_time=glfwGetTime();

      if(bin0)
        bin[0].x_pos=x_pos;
      else if(bin1)
        bin[1].x_pos=x_pos;
      else if(gun0)
      gun.y_pos=y_pos;
      else if(current_time- time_to_hit_space>=0.5)
        {
        //cout << atan((y_pos-gun.y_pos)/(x_pos-gun.x_pos))<< endl;
        gun.rot_angle=atan((y_pos-gun.y_pos)/(x_pos-gun.x_pos))/DEG2RAD;
        CreateBullet();
        time_to_hit_space=current_time;
      }

    }
  }
  void draw ()
  {
    collision();
    // clear the color and depth in the frame buffer
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram (programID);

    // Eye - Location of camera. Don't change unless you are sure!!
    glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );


    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (0, 0, 0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 1, 0);

    Matrices.projection = glm::ortho(-4.0+ zoom, 4.0-zoom, -4.0+zoom, 4.0-zoom, 0.1, 500.0);

    // Compute Camera matrix (view)
    //Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
    //  Don't change unless you are sure!!
    Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
    glm::mat4 MVP;	// MVP = Projection * View * Model

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = Matrices.projection * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    //moveTriangle(VP,glm::vec3(0.0f, 0.0f, 0.0f));
    //drawRectangle(VP,glm::vec3(0.0f, 0.0f, 0.0f),&bin[0].rect,0);
    //drawCircle(VP,glm::vec3(0.8f, -0.2f, 0.0f),&circle,glm::vec3(0,1,0),70);
    //drawRectangle(VP,glm::vec3(mirror[0].rect.x_pos, mirror[0].rect.y_pos, 0.0f),&mirror[0].rect.rect,mirror[0].rect.angle);
    drawGun(VP,glm::vec3(gun.x_pos,gun.y_pos, 0.0f),&gun);
    //drawRectangle(VP,glm::vec3(0.5f,-0.1f, 0.0f),&rectangle1,0);
    Create_Seven_Segment();
    drawSevenSegment(VP);
    //drawCircle(VP,glm::vec3(0.0f, -1.0f, 0.0f),&bin[0].bottom,70);
    drawBricks(VP);
    drawBin(VP,glm::vec3(bin[0].x_pos, bin[0].y_pos, 0.0f),&bin[0],glm::vec3(1,0,0),-70);
    drawBin(VP,glm::vec3(bin[1].x_pos, bin[1].y_pos, 0.0f),&bin[1],glm::vec3(1,0,0),-70);
    drawBullets(VP);
    drawMirror(VP);


    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();

    // Load identity to model matrix

    // Increment angles
    float increments = 1;
    //if(camera_rotation_angle<180)
    camera_rotation_angle++; // Simulating camera rotation

    //triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
    //rectangl e_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  }

  /* Initialise glfw window, I/O callbacks and the renderer to use */
  /* Nothing to Edit here */
  GLFWwindow* initGLFW (int width, int height)
  {
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
      //        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
      glfwTerminate();
      //        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);
    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
    is different from WindowSize */
    glfwSetFramebufferSizeCallback(window,reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);
    /* Register function to handle window close */

    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    //glfwSetMouseButtonCallback(window, MouseControl);
    return window;
  }

  /* Initialize the OpenGL rendering properties */
  /* Add all the models to be created here */
  void initGL (GLFWwindow* window, int width, int height)
  {
    /* Objects should be created before any other gl function and shaders */
    // Create the models
    //createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
    CreateBin(&bin[0],1);
    CreateBin(&bin[1],0);
    CreateGun(&gun,3);
    CreateMirror();
    Create_Seven_Segment();
    //CreateRectangle(0.2,0.4,3,&rectangle);
    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

    reshapeWindow (window, width, height);

    // Background color of the scene
    glClearColor (0.8f, 0.8f, 0.8f, 0.0f); // R, G, B, A
    glClearDepth (1.0f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
  }

  int main (int argc, char** argv)
  {
    int width = 600;
    int height = 600;
    fbwidth=width;
    fbheight=height;
    srand(time(NULL));
    window = initGLFW(width, height);
    bricks.bricks_count=0;
    bricks.pointer=0;
    gun.x_pos=-3.5;
    gun.y_pos=0;
    bin[0].x_pos=0;
    bin[0].y_pos=-2.5;
    bin[1].x_pos=1;
    bin[1].y_pos=-2.5;
    initGL (window, width, height);
    /* Draw in loop */

    while (!glfwWindowShouldClose(window)) {

      // OpenGL Draw commands
      mouse_func(window);
      draw();

      // Swap Frame Buffer in double buffering
      glfwSwapBuffers(window);

      // Poll for Keyboard and mouse events
      glfwPollEvents();

      // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)

    }

    glfwTerminate();
    //    exit(EXIT_SUCCESS);
  }
