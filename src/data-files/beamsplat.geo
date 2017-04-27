#version 410

uniform mat4 MVP;
uniform vec3 Camera;

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 major[];
in vec3 minor[];
in vec3 power_geo[];

out vec2 uv;
out vec3 power;

void main() {

    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;
    vec3 look = normalize(start.xyz - Camera);
    vec3 beam = normalize(end.xyz - start.xyz);

    vec3 maj0 = major[0];
    vec3 maj1 = major[1];
    vec3 min0 = minor[0];
    vec3 min1 = minor[1];

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
        power = power_geo[0];
        EmitVertex();

        gl_Position = vec4(start + start_perp, 1) * mat;
        uv = vec2(0, 1);
        power = power_geo[0];
        EmitVertex();

        gl_Position = vec4(end - end_perp, 1) * mat;
        uv = vec2(1, 0.0);
        power = power_geo[1];
        EmitVertex();

        gl_Position = vec4(end + end_perp, 1) * mat;
        uv = vec2(1, 1);
        power = power_geo[1];
        EmitVertex();

    }
    EndPrimitive();

}
