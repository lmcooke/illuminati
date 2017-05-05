#version 410
#define M_PI 3.1415926535897932384626433832795
#define EPS 0.0001 // to prevent division by zero
#define PWR_CLAMP 150.0 // clamp bound on beam power, esp. for beams with radius 0

uniform mat4 MVP;
uniform vec3 Look;

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 major[];
in vec3 minor[];
in vec3 power_geo[];

out vec2 start_pt;
out vec2 end_pt;
out vec2 start_v;
out vec2 end_v;
out vec3 power;
out vec2 depth;

void main() {

    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;
    vec3 nlook = normalize(Look);
    vec3 vbeam = end.xyz - start.xyz;
    vec3 nbeam = normalize(vbeam);

    vec3 maj0 = major[0];
    vec3 maj1 = major[1];
    vec3 min0 = minor[0];
    vec3 min1 = minor[1];

    vec3 vstart = normalize(cross(normalize(maj0), normalize(min0)));
    vec3 vend = normalize(cross(normalize(maj1), normalize(min1)));

    vec3 start_perp = normalize(cross(nlook, vstart)) * length(min0);
    vec3 end_perp = normalize(cross(nlook, vend)) * length(min1);

    if (dot(cross(nlook, start_perp), nbeam) > 0) {
        start_perp = -start_perp;
    }
    if (dot(cross(nlook, end_perp), nbeam) > 0) {
        end_perp = -end_perp;
    }

    vec4 s4 = (MVP * vec4(start.xyz, 1.0));
    vec3 s = s4.xyz / s4.w;
    s.xy = s.xy * 0.5 + vec2(0.5);
    vec4 e4 = (MVP * vec4(end.xyz, 1.0));
    vec3 e = e4.xyz / e4.w;
    e.xy = e.xy * 0.5 + vec2(0.5);

    vec4 p1 = MVP * vec4(start + start_perp, 1.0);
    vec4 p2 = MVP * vec4(start + end_perp, 1.0);

    vec2 bs = (((p1.xy / p1.w) * 0.5 + vec2(0.5)) - s.xy);
    vec2 be = (((p2.xy / p2.w) * 0.5 + vec2(0.5)) - e.xy);

    vec3 pwr0 = power_geo[0] / (M_PI * pow(max(EPS, length(min0)), 2));
    vec3 pwr1 = power_geo[1] / (M_PI * pow(max(EPS, length(min1)), 2));

    if (length(pwr0) > PWR_CLAMP) {
        pwr0 = pwr0 * (PWR_CLAMP / length(pwr0));
    }
    if (length(pwr1) > PWR_CLAMP) {
        pwr1 = pwr1 * (PWR_CLAMP / length(pwr1));
    }

    vec2 d = vec2(s.z, e.z);


    {
            gl_Position = MVP * vec4(start - start_perp, 1.0);

            depth = d;
            start_pt = s.xy;
            end_pt = e.xy;
            start_v = bs;
            end_v = be;
            power = pwr0;
            EmitVertex();


            gl_Position = MVP * vec4(start + start_perp, 1.0);

            depth = d;
            start_pt = s.xy;
            end_pt = e.xy;
            start_v = bs;
            end_v = be;
            power = pwr0;
            EmitVertex();


            gl_Position = MVP * vec4(end - end_perp, 1.0);

            depth = d;
            start_pt = s.xy;
            end_pt = e.xy;
            start_v = bs;
            end_v = be;
            power = pwr1;
            EmitVertex();


            gl_Position = MVP * vec4(end + end_perp, 1.0);

            depth = d;
            start_pt = s.xy;
            end_pt = e.xy;
            start_v = bs;
            end_v = be;
            power = pwr1;
            EmitVertex();

        }
        EndPrimitive();
}

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
