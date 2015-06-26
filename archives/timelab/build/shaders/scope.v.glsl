attribute vec3 coord3d;
uniform mat4 scale;
uniform mat4 transform;


void main(void) {
  gl_Position = transform * scale * vec4(coord3d,  1.0);


}
