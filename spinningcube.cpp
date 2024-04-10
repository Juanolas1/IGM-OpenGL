// Copyright (C) 2021 Emilio J. Padrón
// Released as Free Software under the X11 License
// https://spdx.org/licenses/X11.html
//
// Strongly inspired by spinnycube.cpp in OpenGL Superbible
// https://github.com/openglsuperbible

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

// GLM library to deal with matrix operations
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::perspective
#include <glm/gtc/type_ptr.hpp>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


int gl_width = 640;
int gl_height = 480;

void glfw_window_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void render(double);

GLuint shader_program_texture = 0; // Shader para la textura
GLuint shader_program_color = 0; // Shader para los colores
GLuint vao = 0; // Vertext Array Object to set input data
GLuint texture;
GLint mv_location, proj_location, tex_location; // Uniforms for transformation matrices
GLint mv_location_color, proj_location_color; 

int main() {
  // start GL context and O/S window using the GLFW helper library
  if (!glfwInit()) {
    fprintf(stderr, "ERROR: could not start GLFW3\n");
    return 1;
  }

    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(gl_width, gl_height, "My spinning cube", NULL, NULL);
  if (!window) {
    fprintf(stderr, "ERROR: could not open window with GLFW3\n");
    glfwTerminate();
    return 1;
  }
  glfwSetWindowSizeCallback(window, glfw_window_size_callback);
  glfwMakeContextCurrent(window);

  // start GLEW extension handler
  // glewExperimental = GL_TRUE;
  glewInit();

  // get version info
  const GLubyte* vendor = glGetString(GL_VENDOR); // get vendor string
  const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
  const GLubyte* glversion = glGetString(GL_VERSION); // version as a string
  const GLubyte* glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION); // version as a string
  printf("Vendor: %s\n", vendor);
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", glversion);
  printf("GLSL version supported %s\n", glslversion);
  printf("Starting viewport: (width: %d, height: %d)\n", gl_width, gl_height);

  // Enable Depth test: only draw onto a pixel if fragment closer to viewer
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS); // set a smaller value as "closer"

  // Vertex Shader
  const char* vertex_shader_texture =
    "#version 130\n"

    "in vec4 v_pos;"
    "in vec2 v_texCoord;"
    
    "out vec2 texCoord;"
    "out vec4 pos;"
    
    "uniform mat4 mv_matrix;"
    "uniform mat4 proj_matrix;"

    "void main() {"
    "  gl_Position = proj_matrix * mv_matrix * v_pos;"
    "  texCoord = v_texCoord;\n"
    "}";

  // Fragment Shader
  const char* fragment_shader_texture =
    "#version 130\n"
    "in vec2 texCoord;"
    
    "out vec4 frag_col;"

    "uniform sampler2D tex;"

    "void main() {"
    "  frag_col = texture(tex, texCoord);"
    "}";

  // Shaders compilation
  GLuint vs_texture = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs_texture, 1, &vertex_shader_texture, NULL);
  glCompileShader(vs_texture);
  GLuint fs_texture = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs_texture, 1, &fragment_shader_texture, NULL);
  glCompileShader(fs_texture);

  // Create program, attach shaders to it and link it
  shader_program_texture = glCreateProgram();
  glAttachShader(shader_program_texture, vs_texture);
  glAttachShader(shader_program_texture, fs_texture);
  glLinkProgram(shader_program_texture);

  // Release shader objects
  glDeleteShader(vs_texture);
  glDeleteShader(vs_texture);

  const char* vertex_shader_color =
    "#version 130\n"

    "in vec4 v_pos;"

    "out vec4 vs_color;"

    "uniform mat4 mv_matrix;"
    "uniform mat4 proj_matrix;"

    "void main() {"
    "  gl_Position = proj_matrix * mv_matrix * v_pos;"
    "  vs_color = v_pos * 2.0 + vec4(0.4, 0.4, 0.4, 0.0);"
    "}";

  
  const char* fragment_shader_color =
    "#version 130\n"

    "out vec4 frag_col;"

    "in vec4 vs_color;"

    "void main() {"
    "  frag_col = vs_color;"
    "}";

  // Compila y enlaza los shaders de color
  GLuint vs_color = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs_color, 1, &vertex_shader_color, NULL);
  glCompileShader(vs_color);

  GLuint fs_color = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs_color, 1, &fragment_shader_color, NULL);
  glCompileShader(fs_color);

  shader_program_color = glCreateProgram();
  glAttachShader(shader_program_color, vs_color);
  glAttachShader(shader_program_color, fs_color);
  glLinkProgram(shader_program_color);

  glDeleteShader(vs_color);
  glDeleteShader(fs_color);


  // Vertex Array Object
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Cube to be rendered
  //
  //          0        3
  //       7        4 <-- top-right-near
  // bottom
  // left
  // far ---> 1        2
  //       6        5
  //
  const GLfloat vertex_positions[] = {
    -0.25f, -0.25f, -0.25f, 0.0f, 0.0f, // 1
    -0.25f,  0.25f, -0.25f, 0.0f, 1.0f, // 0
     0.25f, -0.25f, -0.25f, 1.0f, 0.0f, // 2

     0.25f,  0.25f, -0.25f, 1.0f, 1.0f, // 3
     0.25f, -0.25f, -0.25f, 1.0f, 0.0f, // 2
    -0.25f,  0.25f, -0.25f, 0.0f, 1.0f, // 0

     0.25f, -0.25f, -0.25f, 0.0f, 0.0f, // 2
     0.25f,  0.25f, -0.25f, 0.0f, 0.0f, // 3
     0.25f, -0.25f,  0.25f, 0.0f, 0.0f, // 5

     0.25f,  0.25f,  0.25f, 0.0f, 0.0f, // 4
     0.25f, -0.25f,  0.25f, 0.0f, 0.0f, // 5
     0.25f,  0.25f, -0.25f, 0.0f, 0.0f, // 3

     0.25f, -0.25f,  0.25f, 0.0f, 0.0f, // 5
     0.25f,  0.25f,  0.25f, 0.0f, 0.0f, // 4
    -0.25f, -0.25f,  0.25f, 0.0f, 0.0f, // 6

    -0.25f,  0.25f,  0.25f, 0.0f, 0.0f, // 7
    -0.25f, -0.25f,  0.25f, 0.0f, 0.0f, // 6
     0.25f,  0.25f,  0.25f, 0.0f, 0.0f, // 4

    -0.25f, -0.25f,  0.25f, 0.0f, 0.0f, // 6
    -0.25f,  0.25f,  0.25f, 0.0f, 0.0f, // 7
    -0.25f, -0.25f, -0.25f, 0.0f, 0.0f, // 1

    -0.25f,  0.25f, -0.25f, 0.0f, 0.0f, // 0
    -0.25f, -0.25f, -0.25f, 0.0f, 0.0f, // 1
    -0.25f,  0.25f,  0.25f, 0.0f, 0.0f, // 7

     0.25f, -0.25f, -0.25f, 0.0f, 0.0f, // 2
     0.25f, -0.25f,  0.25f, 0.0f, 0.0f, // 5
    -0.25f, -0.25f, -0.25f, 0.0f, 0.0f, // 1

    -0.25f, -0.25f,  0.25f, 0.0f, 0.0f, // 6
    -0.25f, -0.25f, -0.25f, 0.0f, 0.0f, // 1
     0.25f, -0.25f,  0.25f, 0.0f, 0.0f, // 5

     0.25f,  0.25f,  0.25f, 0.0f, 0.0f, // 4
     0.25f,  0.25f, -0.25f, 0.0f, 0.0f, // 3
    -0.25f,  0.25f,  0.25f, 0.0f, 0.0f, // 7

    -0.25f,  0.25f, -0.25f, 0.0f, 0.0f, // 0
    -0.25f,  0.25f,  0.25f, 0.0f, 0.0f, // 7
     0.25f,  0.25f, -0.25f, 0.0f, 0.0f,  // 3
  };

  // Vertex Buffer Object (for vertex coordinates)
  GLuint vbo = 0;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_positions), vertex_positions, GL_STATIC_DRAW);

  // Vertex attributes
  // 0: vertex position (x, y, z)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(0);

  // 1: texture coordinates (u, v)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  // Unbind vbo (it was conveniently registered by VertexAttribPointer)
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Unbind vao
  glBindVertexArray(0);

  // Carga y creacion
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  // Seleccionamos la textura
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Filtramos por parametros
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Cargamos y creamos la textura utilizando la ruta del archivo y generamos los mipmaps
  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(true); // Tell stb_image.h to flip loaded texture's on the y-axis.
  unsigned char *data = stbi_load("/home/user/Desktop/IGM/ENTREGA 1/igm-opengl-day3/texture.jpg", &width, &height, &nrChannels, 0);
  if (data)
  {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
  }
  else
  {
      fprintf(stderr, "Failed to load texture\n");
  }
  stbi_image_free(data);

  // Uniforms
  mv_location = glGetUniformLocation(shader_program_texture, "mv_matrix");
  proj_location = glGetUniformLocation(shader_program_texture, "proj_matrix");
  tex_location = glGetUniformLocation(shader_program_texture, "tex");


  mv_location_color = glGetUniformLocation(shader_program_color, "mv_matrix");
  proj_location_color = glGetUniformLocation(shader_program_color, "proj_matrix");

  // Render loop
  while (!glfwWindowShouldClose(window))
  {
      processInput(window);

      render(glfwGetTime());

      glfwSwapBuffers(window);

      glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}

void render(double currentTime) {
    float f = (float)currentTime * 0.3f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, gl_width, gl_height);

    glm::mat4 mv_matrix, proj_matrix;
    mv_matrix = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, -4.0f));
    mv_matrix = glm::translate(mv_matrix,
                               glm::vec3(sinf(2.1f * f) * 0.5f,
                                         cosf(1.7f * f) * 0.5f,
                                         sinf(1.3f * f) * cosf(1.5f * f) * 2.0f));
    mv_matrix = glm::rotate(mv_matrix, glm::radians((float)currentTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    mv_matrix = glm::rotate(mv_matrix, glm::radians((float)currentTime * 81.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    proj_matrix = glm::perspective(glm::radians(50.0f), (float)gl_width / (float)gl_height, 0.1f, 1000.0f);

    // Seleccionamos el shader de la primera cara con la textura
    glUseProgram(shader_program_texture);
    glUniformMatrix4fv(mv_location, 1, GL_FALSE, glm::value_ptr(mv_matrix));
    glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(proj_matrix));
    // Verificamos y dibujamos
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6); // Se dibuja

    // Seleccionamos el shder por defecto para el resto del cubo con color
    glUseProgram(shader_program_color);
    
    glUniformMatrix4fv(mv_location_color, 1, GL_FALSE, glm::value_ptr(mv_matrix));
    glUniformMatrix4fv(proj_location_color, 1, GL_FALSE, glm::value_ptr(proj_matrix));
    glDrawArrays(GL_TRIANGLES, 6, 30); // Se dibuja el resto
}


void processInput(GLFWwindow *window) {
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
}

// Callback function to track window size and update viewport
void glfw_window_size_callback(GLFWwindow* window, int width, int height) {
  gl_width = width;
  gl_height = height;
  printf("New viewport: (width: %d, height: %d)\n", width, height);
}
