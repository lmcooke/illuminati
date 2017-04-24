#version 410

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 major[];
in vec3 minor[];

uniform mat4 MVP;
uniform vec3 Camera;

out vec2 uv;
out vec3 start_major;
out vec3 start_minor;
out vec3 end_major;
out vec3 end_minor;

void main() {


    // scale constants for debugging
    const float d = 1.0;
    const float w = 1.0;

    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;
    vec3 look = normalize(start.xyz - Camera);
    vec3 beam = normalize(end.xyz - start.xyz);
    vec3 beam_perp = normalize(cross(beam, look));

    vec3 maj0 = major[0];
    vec3 maj1 = major[1];
    vec3 beam_major0 = dot(maj0, beam) * beam * d;
    vec3 beam_major1 = dot(maj1, beam) * beam * d;

    float beam_start_w = length(minor[0]) * w;
    float beam_end_w = length(minor[1]) * w;

    mat4 mat = g3d_ObjectToScreenMatrixTranspose;

    // First strip
    {
    //  Top triangle
        gl_Position = vec4(start + beam_perp * beam_start_w - beam_major0, 1) * mat; uv = vec2(0, 0); EmitVertex();
        gl_Position = vec4(start - beam_perp * beam_start_w - beam_major0, 1) * mat; uv = vec2(0, 1); EmitVertex();
        gl_Position = vec4(end + beam_perp * beam_end_w + beam_major1, 1) * mat; uv = vec2(1, 0.0); EmitVertex();

    //  Bottom triangle
        gl_Position = vec4(end - beam_perp * beam_end_w + beam_major1, 1) * mat; uv = vec2(1, 1); EmitVertex();

    }
    EndPrimitive();

}
