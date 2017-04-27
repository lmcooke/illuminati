#version 410

uniform mat4 MVP;
uniform vec3 Camera;
uniform sampler2D axis;

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 major[];
in vec3 minor[];
in int index[];

out vec2 uv;
out vec3 start_major;
out vec3 start_minor;
out vec3 end_major;
out vec3 end_minor;
flat out int id;

void main() {


    // scale constants for debugging
    const float d = 1.0;
    const float w = 1.0;

    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;
    vec3 look = normalize(start.xyz - Camera);
    vec3 beam = normalize(end.xyz - start.xyz);
    vec3 b_perp = normalize(cross(beam, look));

    vec3 maj0 = major[0];
    //maj0 = texture(axis, vec2(index[0], 0)).xyz;
    vec3 maj1 = major[1];
    //maj1 = texture(axis, vec2(index[1], 1)).xyz;
    vec3 min0 = minor[0];
    vec3 min1 = minor[1];
    vec3 beam_major0 = dot(maj0, beam) * beam * d;
    vec3 beam_major1 = dot(maj1, beam) * beam * d;

    vec3 vstart = normalize(cross(maj0, minor[0]));
    vec3 vend = normalize(cross(maj1, minor[1]));

    vec3 start_perp = normalize(cross(look, vstart)) * length(minor[0]);
    vec3 end_perp = normalize(cross(look, vend)) * length(minor[1]);

    if (dot(cross(look, start_perp), beam) > 0) {
        start_perp = -start_perp;
    }
    if (dot(cross(look, end_perp), beam) > 0) {
        end_perp = -end_perp;
    }

    float b_start_w = length(minor[0]) * w;
    //float b_start_w = length(texture(axis, vec2(1, in dex[0])).xyz) * w;
    //float b_start_w = 0.05;
    float beam_end_w = length(minor[1]) * w;

    mat4 mat = g3d_ObjectToScreenMatrixTranspose;

    // First strip
    {

/*

    end_perp----->

    3----4
    |\   |
    | \  |
    |  \ |
    |   \|
    1----2

    start_perp---->
*/

        gl_Position = vec4(start - start_perp, 1) * mat;
        uv = vec2(0, 0);
        id = index[0];
        EmitVertex();

        gl_Position = vec4(start + start_perp, 1) * mat;
        uv = vec2(0, 1);
        id = index[0];
        EmitVertex();

        gl_Position = vec4(end - end_perp, 1) * mat;
        uv = vec2(1, 0.0);
        id = index[1];
        EmitVertex();

        gl_Position = vec4(end + end_perp, 1) * mat;
        uv = vec2(1, 1);
        id = index[1];
        EmitVertex();


/*
    //  Top triangle
        gl_Position = vec4(start + b_perp * b_start_w - beam_major0, 1) * mat;
        uv = vec2(0, 0);
        id = index[0];
        EmitVertex();

        gl_Position = vec4(start - b_perp * b_start_w - beam_major0, 1) * mat;
        uv = vec2(0, 1);
        id = index[0];
        EmitVertex();

        gl_Position = vec4(end + b_perp * beam_end_w + beam_major1, 1) * mat;
        uv = vec2(1, 0.0);
        id = index[1];
        EmitVertex();

    //  Bottom triangle
        gl_Position = vec4(end - b_perp * beam_end_w + beam_major1, 1) * mat;
        uv = vec2(1, 1);
        id = index[1];
        EmitVertex();
*/
    }
    EndPrimitive();

}
