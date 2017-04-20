#version 410

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 MVP;

void main() {

    // Extrusion distance
    const float d = 0.1;

    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;

    vec3 offset_l = vec3(-d, 0, 0);
    vec3 offset_r = vec3(d, 0, 0);

    // First strip
    {
        //  Top triangle
            gl_Position = vec4(start + offset_l, 1.0) * g3d_ObjectToScreenMatrixTranspose;    	EmitVertex();
            gl_Position = vec4(start + offset_r, 1.0) * g3d_ObjectToScreenMatrixTranspose;    	EmitVertex();
            gl_Position = vec4(end + offset_l, 1.0) * g3d_ObjectToScreenMatrixTranspose;        EmitVertex();

        //  Bottom triangle
            gl_Position = vec4(end + offset_r, 1.0) * g3d_ObjectToScreenMatrixTranspose;        EmitVertex();
    }
    EndPrimitive();


}
