
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <string.h>

char *tut_file_read(const char *filename){

  FILE *shader_fp = fopen(filename, "r");
  char c;
  int i = 0;
  if(shader_fp == NULL)
    {
      printf("shader_fp == NULL\n");
      return(NULL);
    }

  while((c=getc(shader_fp))!=EOF)
      i++;
  //crude, eh?

  int size = i;
  fclose(shader_fp);
  shader_fp = fopen(filename, "r");
  i = 0;

  char *content = malloc(sizeof(char) * (size+1));
  if(content==NULL)
    {
      printf("content == NULL\n");
      return(NULL);
    }

  while((c=getc(shader_fp))!=EOF)
      content[i++] = c;

  if(ferror(shader_fp))
    {
      printf("ferror\n");
      free(content);
      return(NULL);
    }

  fclose(shader_fp);
  content[size] = '\0';
  //printf("content\n\n%s\n\n", content);
  return(content);


}

/**
* Display compilation errors from the OpenGL shader compiler
*/
void print_log(GLuint object)
{
  GLint log_length = 0;
  if (glIsShader(object))
    glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
  else if (glIsProgram(object))
    glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
  else {
    fprintf(stderr, "printlog: Not a shader or a program\n");
    return;
  }
 
  char* log = malloc(log_length);
 
  if (glIsShader(object))
    glGetShaderInfoLog(object, log_length, NULL, log);
  else if (glIsProgram(object))
    glGetProgramInfoLog(object, log_length, NULL, log);
 
  fprintf(stderr, "Shader File Error: %s", log);
  free(log);
}

GLuint tut_create_shader(const char *filename, GLenum type){

  char *src = tut_file_read(filename);
  //printf("src\n\n%s\n\n", src);
  if(src == NULL)
    {
      fprintf(stderr, "Error opening %s\n", filename);
      FILE *foo;
      foo = fopen("sucks", "w");
      return(0);
    }

  GLuint res = glCreateShader(type);
  char src_header[] = {
#ifdef GL_ES_VERSION_2_0
    "#version 100\n"
#else
    "#version 120\n"
#endif
  };
  
  const GLchar *src_full = (GLchar *)malloc(sizeof(char) * (strlen(src) + strlen(src_header) + 2));
  strcpy(src_full, src_header);
  strcat(src_full, src);

  //fprintf(stderr,"\n\n%s\n\n", src_full);
  glShaderSource(res,
   1,
   &src_full,//to avoid c-compiler warnings
   NULL);

  glCompileShader(res);
  GLint compile_ok = GL_FALSE;
  glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
  if(compile_ok == GL_FALSE)
    {
      fprintf(stderr, "%s:\n\t", filename);
      print_log(res);
      glDeleteShader(res);
      return(0);
    }

  return(res);
}
