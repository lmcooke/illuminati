#version 410

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 diff1[];
in vec3 diff2[];

uniform mat4 MVP;

out vec2 uv;

void main() {

    // Extrusion distance
    const float d = 0.2;

    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;

    vec4 off_l = vec4(-d, 0, 0, 1);
    vec4 off_r = vec4(d, 0, 0, 1);
    //vec3 offset_r = diff1[0];
    //vec3 offset_l = diff2[0];

    mat4 mat = g3d_ObjectToScreenMatrixTranspose;
    // First strip
    {
    //  Top triangle
        gl_Position = vec4(start, 1.0) * mat + off_l; uv = vec2(0.0, 0.0); EmitVertex();
        gl_Position = vec4(start, 1.0) * mat + off_r; uv = vec2(0.0, 1.0); EmitVertex();
        gl_Position = vec4(end, 1.0) * mat + off_l; uv = vec2(1.0, 0.0); EmitVertex();

    //  Bottom triangle
        gl_Position = vec4(end, 1.0) * mat + off_r; uv = vec2(1.0, 1.0); EmitVertex();

    }
    EndPrimitive();


}
