#version 410

uniform mat4 MVP;
uniform mat4 MVP2;
uniform vec3 Look;

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 major[];
in vec3 minor[];
in vec3 power_geo[];

out vec2 uv;
out vec2 start_pt;
out vec2 end_pt;
out vec2 bstart;
out vec2 bend;
out vec3 power;


void main() {

    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;
    vec3 look = normalize(Look);
    vec3 vbeam = end.xyz - start.xyz;
    vec3 nbeam = normalize(vbeam);

    vec3 maj0 = major[0];
    vec3 maj1 = major[1];
    vec3 min0 = minor[0];
    vec3 min1 = minor[1];

    vec3 vstart = normalize(cross(maj0, minor[0]));
    vec3 vend = normalize(cross(maj1, minor[1]));

    vec3 start_perp = normalize(cross(look, vstart)) * length(minor[0]);
    vec3 end_perp = normalize(cross(look, vend)) * length(minor[1]);

    if (dot(cross(look, start_perp), nbeam) > 0) {
        start_perp = -start_perp;
    }
    if (dot(cross(look, end_perp), nbeam) > 0) {
        end_perp = -end_perp;
    }

    mat4 mat = g3d_ObjectToScreenMatrixTranspose;
    mat4 mvp = mat4(1.0);

    vec4 tmp = (vec4(start.xyz, 1.0)) * mat;
    vec2 s = (vec4(start.x * 1.0, start.y * 1.0, start.z * 1.0, 1.0) * mat).xy;

    vec2 e = (vec4(end, 1.0) * mat).xy;
    vec2 bs = (vec4(start_perp, 1.0) * mat).xy;
    vec2 be = (vec4(end_perp, 1.0) * mat).xy;

/*
    if (s.x <= 1.0 && s.x >= -1.0 &&
        tmp.y <= 3.0 &&
        tmp.y >= 0.0 &&
        tmp.z <= 1.0 &&
        tmp.z >= -1.0) {
        */
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


            //mat = mat4(1.0);


            gl_Position = vec4(start - start_perp, 1.0) * mat;
            //gl_Position.z = 1.0;
            uv = vec2(0, 0);
            start_pt = s;
            end_pt = e;
            bstart = bs;
            bend = be;
            power = power_geo[0];
            EmitVertex();

            gl_Position = vec4(start + start_perp, 1.0) * mat;
            //gl_Position.z = 1.0;
            uv = vec2(0, 1);
            start_pt = s;
            end_pt = e;
            bstart = bs;
            bend = be;
            power = power_geo[0];
            EmitVertex();

            gl_Position = vec4(end - end_perp, 1.0) * mat;
            //gl_Position.z = 1.0;
            uv = vec2(1, 0.0);
            start_pt = s;
            end_pt = e;
            bstart = bs;
            bend = be;
            power = power_geo[1];
            EmitVertex();

            gl_Position = vec4(end + end_perp, 1.0) * mat;
            //gl_Position.z = 1.0;
            uv = vec2(1, 1);
            start_pt = s;
            end_pt = e;
            bstart = bs;
            bend = be;
            power = power_geo[1];
            EmitVertex();

        }
        EndPrimitive();
//    }

}
