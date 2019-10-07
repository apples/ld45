attribute vec3 position;
attribute vec2 texcoord;
attribute vec3 normal;

varying vec2 v_texcoord;
varying vec3 v_normal;

uniform mat4 MVP;
uniform mat4 normal_mat;
uniform mat3 texcoord_mat;

void main()
{
    gl_Position = MVP * vec4(position, 1.0);
    v_texcoord = vec2(texcoord_mat * vec3(texcoord, 1.0));
    v_normal = vec3(normal_mat * vec4(normal, 0.0));
}
