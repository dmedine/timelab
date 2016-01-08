#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include "shader_utils.h"
#include "gsl_utils.h"
#include "ui_main.h"
#include "g_api.h"
#include "pthread.h"

GLuint scope_shader;

int screen_width = 1200;
int screen_height = 400;
float offsetx = 0;
float scaley=1.0;

float rot_x, rot_y, rot_z;
float trans_x, trans_y, trans_z;
float small_angle;
#define N_VBOS 5
GLuint vbos[N_VBOS];

int buff_len = 1000;
int half_buff_len = 500;
const int margin = 40;

GLint attribute_coord3d;
GLint uniform_color;
GLint uniform_transform;
GLint uniform_scale;
GLint attribute_f_color;

GLfloat red[4] = { 1, 0, 0, 1 };
GLfloat green[4] = { 0, 1, 0, .5 };
GLfloat black[4] = { 1, 1, 1, 1 };

GLfloat scope_colors[4000];

gsl_matrix *trans_scale;

gsl_matrix *mx_translate;//position
gsl_matrix *mx_rotate;//rotation
gsl_matrix *mx_view;//view
gsl_matrix *mx_projection;//projection

gsl_matrix *dummy1, *dummy2;//needed for the view matrix

gsl_matrix *prod1;//
gsl_matrix *prod2;//
gsl_matrix *prod3;//



float zoom = 1.0;

typedef struct _point{
  GLfloat x;
  GLfloat y;
  GLfloat z;
}t_point;
t_point graph[1000];

pthread_mutex_t buff_lock = PTHREAD_MUTEX_INITIALIZER;

int gl_scope_init(void){

  //setup the shader programs
  GLuint vs, fs;
  GLint link_ok = GL_FALSE;
 
  if ((vs = tut_create_shader("build/shaders/scope.v.glsl", GL_VERTEX_SHADER))   == 0) return 0;
  if ((fs = tut_create_shader("build/shaders/scope.f.glsl", GL_FRAGMENT_SHADER)) == 0) return 0;

  scope_shader = glCreateProgram();
  glAttachShader(scope_shader, vs);
  glAttachShader(scope_shader, fs);
  glLinkProgram(scope_shader);
  glGetProgramiv(scope_shader, GL_LINK_STATUS, &link_ok);

  if (!link_ok) {
    fprintf(stderr, "glLinkProgram:");
    print_log(scope_shader);
    return(0);
  }
    
  //attributes and uniforms
  const char *attribute_name = "coord3d";
  attribute_coord3d = glGetAttribLocation(scope_shader, attribute_name);
  if(attribute_coord3d == -1)
    {
      printf("Error, could not bind attribute %s\n", attribute_name);
      return 0;
    }

  attribute_name = "transform";
  uniform_transform = glGetUniformLocation(scope_shader, attribute_name);
  if(uniform_transform == -1)
    {
      printf("Error, could not bind attribute %s\n", attribute_name);
      return 0;
    }

  attribute_name = "scale";
  uniform_scale = glGetUniformLocation(scope_shader, attribute_name);
  if(uniform_scale == -1)
    {
      printf("Error, could not bind attribute %s\n", attribute_name);
      return 0;
    }

  /* attribute_name = "color"; */
  /* attribute_f_color = glGetAttribLocation(scope_shader, attribute_name); */
  /* if(attribute_f_color == -1) */
  /*   { */
  /*     fprintf(stderr, "Error, could not bind attribute %s\n", attribute_name); */
  /*     return 0; */
  /*   } */

  attribute_name = "color";
  uniform_color = glGetUniformLocation(scope_shader, attribute_name);
  if(uniform_color == -1)
    {
      printf("Error, could not bind attribute %s\n", attribute_name);
      return 0;
    }
  
  //originate the buffers we need
  glGenBuffers(N_VBOS, vbos);

  t_point y_axis[2] = {{-1,-1, 0}, {-1, 1, 0}};
  glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(y_axis), y_axis, GL_STATIC_DRAW);

  t_point x_axis[2] = {{-1, 0, 0}, {1, 0, 0}};
  glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(x_axis), x_axis, GL_STATIC_DRAW);

  t_point z_axis[2] = {{-1, 0, -1}, {-1, 0, 1}};
  glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(z_axis), z_axis, GL_STATIC_DRAW);
  
  int i;
  float x;

  for(i=0; i<buff_len; i++)
    {
      x = (i-(float)half_buff_len)/(float)half_buff_len;
      graph[i].x = x;

      graph[i].y = 0.0;
      graph[i].z = 0.0;
      //printf("%f %f\n", x, graph[i].y);
    }

  glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(t_point) * buff_len, graph, GL_DYNAMIC_DRAW);

  /* int j; */
  /* for(i=0; j<4000; i+=4) */
  /*   { */
  /*     scope_colors[i+0] = .5 + (.5 * i); */
  /*     scope_colors[i+1] = 0.0; */
  /*     scope_colors[i+2] = .5 - (.5*i); */
  /*     scope_colors[i+3] = 1.0; */
  /*   } */
      
    
  /* glBindBuffer(GL_ARRAY_BUFFER, vbos[4]); */
  /* glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4000, scope_colors, GL_DYNAMIC_DRAW); */

  //set up trans_scale matrix
  trans_scale = gslu_gen_id_matrix(4,4);


  //setup matrices
  mx_translate       = gslu_gen_id_matrix(4,4);
  mx_rotate          = gslu_gen_id_matrix(4,4);
  mx_view            = gslu_gen_id_matrix(4,4);
  mx_projection      = gslu_gen_id_matrix(4,4); 
  dummy1             = gslu_gen_id_matrix(4,4);
  dummy2             = gslu_gen_id_matrix(4,4);
  prod1              = gslu_gen_zero_matrix(4,4);
  prod2              = gslu_gen_zero_matrix(4,4);
  prod3              = gslu_gen_zero_matrix(4,4);
 
  rot_x = rot_y = rot_z = 0.0;
  trans_x = trans_y = trans_z = 0.0;
  small_angle = M_PI/100;
  return(1);

}

void gl_scope_draw(void){

  //TODO: make all these aarrays in global memory
  float mx[16];
  int i;

  int window_width = screen_width; 
  int window_height = screen_height;

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LINE_SMOOTH);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glUseProgram(scope_shader);

  /* glViewport(margin, 0, window_width - margin, window_height); */
  /* glScissor(margin, 0, window_width - margin, window_height); */

  /* glEnable(GL_SCISSOR_TEST); */

  double l_eye[]    = {0.0, 0.0, 1.0};
  double l_center[] = {0.0, 0.0, -4.0};
  double l_up[]     = {0.0, 1.0, 0.0};

  gslu_look_at(mx_view, dummy1, dummy2,
  	       l_eye, l_center, l_up);

  gslu_perspective(mx_projection,
  		   M_PI/4.0,
  		   4.0/3.0,
  		   0.1,
  		   10.0);

  //?do we need this?

  gslu_translate(mx_translate, trans_x, trans_y, trans_z);
  gslu_rotate(mx_rotate, rot_y, 'y');
  gslu_rotate(mx_rotate, rot_x, 'x');

  gslu_mult_matrices(prod1,
  		     mx_translate,
  		     mx_rotate);

  gslu_mult_matrices(prod2,
  		     mx_view,
  		     prod1);

  gslu_mult_matrices(prod3,
  		     mx_projection,
  		     prod2);

  gslu_gsl_matrix_2_float_array(mx, prod3);
  glUniformMatrix4fv(uniform_transform,
  		     1,
  		     GL_FALSE,
  		     mx);


  double trans[] = {0, 0, 0};
  double scale[] = {1, 1, 1};

  gslu_trans_scale(trans_scale, trans, scale);
  gslu_gsl_matrix_2_float_array(mx, trans_scale);


  glUniformMatrix4fv(uniform_scale,
  		     1,
  		     GL_FALSE,
  		     mx);


  glUniform4fv(uniform_color, 1, red);

  glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
  glEnableVertexAttribArray(attribute_coord3d);
 
  glVertexAttribPointer(
    attribute_coord3d, // attribute
    3,                 // number of elements per vertex, here (x,y,z)
    GL_FLOAT,          // the type of each element
    GL_FALSE,          // take our values as-is
    0,                 // no extra data between each position
    0                  // offset
  );  

  /* glEnableVertexAttribArray(uniform_color); */
  /* glBindBuffer(GL_ARRAY_BUFFER, vbos[4]); */
  /* glVertexAttribPointer( */
  /*   uniform_color, // attribute */
  /*   4,                 // number of elements per vertex, here (r,g,b) */
  /*   GL_FLOAT,          // the type of each element */
  /*   GL_FALSE,          // take our values as-is */
  /*   0,                 // no extra data between each position */
  /*   0                  // offset of first element */
  /* ); */

  /* glDisableVertexAttribArray(uniform_color); */

  glLineWidth(5.0);
  glDrawArrays(GL_LINE_STRIP, 0, buff_len);

  glViewport(0, 0, window_width, window_height);
  /* glDisable(GL_SCISSOR_TEST); */

  //---------------------------------

  /* gslu_gsl_matrix_2_float_array(mx, dummy1); */
  /* glUniformMatrix4fv(uniform_transform, */
  /* 		     1, */
  /* 		     GL_FALSE, */
  /* 		     mx); */

  gslu_viewport_transform(trans_scale,
			  0, 0, 
			  window_width, window_height,
			  window_width, window_height);

  gslu_gsl_matrix_2_float_array(mx, trans_scale);
  
  glUniformMatrix4fv(uniform_scale,
  		     1,
  		     GL_FALSE,
  		     mx);


  glUniform4fv(uniform_color, 1, green);
  glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
  glEnableVertexAttribArray(attribute_coord3d);
  glVertexAttribPointer(attribute_coord3d,
  			3,
  			GL_FLOAT,
  			GL_FALSE,
  			0,
  			0);

  glDrawArrays(GL_LINE_STRIP, 0, 2);

  glUniformMatrix4fv(uniform_scale, 
		     1, 
		     GL_FALSE, 
		     mx);


  glUniform4fv(uniform_color, 1, green);
  glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
  glVertexAttribPointer(attribute_coord3d,
  			3,
  			GL_FLOAT,
  			GL_FALSE,
  			0,
  			0);
  glLineWidth(5.0);
  glDrawArrays(GL_LINE_STRIP, 0, 2);

  glUniformMatrix4fv(uniform_scale, 
		     1, 
		     GL_FALSE, 
		     mx);

  glUniform4fv(uniform_color, 1, green);
  glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
  glVertexAttribPointer(attribute_coord3d,
  			3,
  			GL_FLOAT,
  			GL_FALSE,
  			0,
  			0);
  glLineWidth(5.0);
  glDrawArrays(GL_LINE_STRIP, 0, 2);

  glDisableVertexAttribArray(attribute_coord3d);

}

void gl_scope_animate(){
  int i;

  double mx[16];

  double l_eye[]    = {0.0, 0.0, 1.0};
  double l_center[] = {0.0, 0.0, -4.0};
  double l_up[]     = {0.0, 1.0, 0.0};

  gslu_look_at(mx_view, dummy1, dummy2,
  	       l_eye, l_center, l_up);

  gslu_perspective(mx_projection,
  		   M_PI/4.0,
  		   screen_width/screen_height,
  		   .01,
  		   10.0);

  //?do we need this?

  gslu_translate(mx_translate, trans_x, trans_y, trans_z);
  gslu_rotate(mx_rotate, rot_y, 'y');
  gslu_rotate(mx_rotate, rot_x, 'x');

  gslu_mult_matrices(prod1,
  		     mx_translate,
  		     mx_rotate);

  gslu_mult_matrices(prod2,
  		     mx_view,
  		     prod1);

  gslu_mult_matrices(prod3,
  		     mx_projection,
  		     prod2);


  /* printf("prod3:\n"); */
  /* gslu_show_matrix(prod3); */

  glUseProgram(scope_shader);

  /* gslu_gsl_matrix_2_float_array(mx, prod3); */

  double trans[] = {rot_x, 0, 0};
  double scale[] = {1, rot_y, 1};
  
  gslu_trans_scale(trans_scale, trans, scale);
  gslu_gsl_matrix_2_float_array(mx, trans_scale);
  
  glUniformMatrix4fv(uniform_scale, 
		     1, 
		     GL_FALSE, 
		     mx);
      
  for(i=0; i<buff_len; i++)
    {
      graph[i].y = g_circ_buff_scope_y->blck[i];
      graph[i].z = g_circ_buff_scope_z->blck[i];
      //printf("scope: %f %f\n", graph[i].z, graph[i].y);
    }
  
  glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(graph), graph, GL_DYNAMIC_DRAW);
 

      
}

void gl_scope_reshape(GLFWwindow *window, int width, int height) {
  
  glfwGetFramebufferSize(window, &screen_width, &screen_height);
  glViewport(0, 0, screen_width, screen_height);
}


void gl_scope_key( GLFWwindow* window, int k, int s, int action, int mods){

  int buff_len_changed = 0;
  int i;
  switch(k) 
    {
    case GLFW_KEY_LEFT:
      rot_y -= small_angle;
      //printf("%f\n",rot_y);
      break;
    case GLFW_KEY_RIGHT:
      rot_y += small_angle;
      //printf("%f\n",rot_y);
      break;
    case GLFW_KEY_UP:
      rot_x += small_angle;
      //printf("%f\n",rot_x);
      break;
    case GLFW_KEY_DOWN:
      rot_x -= small_angle;
      //printf("%f\n",rot_x);
      break;
    case GLFW_KEY_HOME:
      break;
    case GLFW_KEY_ESCAPE:
      glfwSetWindowShouldClose(window, GL_TRUE);
      break;
    case GLFW_KEY_A:
      //printf("a1: %d\n", trans_x);
      trans_x += .01;
      //printf("a2: %d\n", trans_x);
      break;
    case GLFW_KEY_D:
      //printf("s1: %d\n", trans_x);
      trans_x -= .01;
      //printf("s2: %d\n", trans_x);
      break;
    case GLFW_KEY_W:
      //printf("s1: %d\n", trans_y);
      trans_z += .01;
      //printf("s2: %d\n", trans_y);
      break;
    case GLFW_KEY_S:
      //printf("s1: %d\n", trans_z);
      trans_z -= .01;
      //printf("s2: %d\n", trans_z);
      break;


    }
  if(buff_len_changed == 1)
    {
      if(buff_len<0)buff_len=0;
      if(buff_len>1000)buff_len=1000;

      half_buff_len = buff_len/2;
      for(i=0; i<buff_len; i++)
	graph[i].x = (i-(float)half_buff_len)/(float)half_buff_len;
      
    }
}


void gl_scope_free(){
 
  glDeleteBuffers(3, vbos);
  glDeleteProgram(scope_shader); 
  gsl_matrix_free(trans_scale);

  gsl_matrix_free(mx_translate);
  gsl_matrix_free(mx_rotate);
  gsl_matrix_free(mx_projection);
  gsl_matrix_free(mx_view);
  gsl_matrix_free(dummy1);
  gsl_matrix_free(dummy2);
  gsl_matrix_free(prod1);
  gsl_matrix_free(prod2);
  gsl_matrix_free(prod3);
 
}

void gl_scope_close(void){
  gl_scope_free();
  gl_scope_on_flag = 0;
  
}


int gl_scope_go(void){

  GLFWwindow* window;
  int width, height;
  int monitor_cnt;
  /* GLFWmonitor **monitors = glfwGetMonitors(&monitor_cnt);  */
  /* printf("monitor count: %d\n", monitor_cnt); */
  if (!glfwInit())
    exit(EXIT_FAILURE);
   
   window = glfwCreateWindow(screen_width, 
			     screen_height, 
			     "Scope", 
			     NULL, 
			     NULL);
   
   glfwMakeContextCurrent(window);
   glfwSwapInterval(1);
   
   glfwSetFramebufferSizeCallback(window, gl_scope_reshape);
   glfwSetKeyCallback(window, gl_scope_key);

  
   GLenum glew_status = glewInit();
   if(glew_status != GLEW_OK)
     {
       fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(glew_status));
       return(EXIT_FAILURE);
     }
   
  if (!window)
     {
       glfwTerminate();
       exit( EXIT_FAILURE );
     }

   gl_scope_on_flag = 1;


  if(1 == gl_scope_init())
    {
      printf("gl_scope_initialized\n");
      while(!glfwWindowShouldClose(window))
	{
	  glEnable(GL_BLEND);
	  glEnable(GL_DEPTH_TEST);
	  glDepthFunc(GL_LESS);
	  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	  
	  gl_scope_draw();
	  gl_scope_animate();
	  //printf("about to swap buffers\n");
	  glfwSwapBuffers(window);
	  glfwPollEvents();
	}
      glfwDestroyWindow(window);
      glfwTerminate();
      gl_scope_close();
    }
  return(EXIT_SUCCESS);
}
