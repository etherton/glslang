#version 420 core

varying vec2 v2;               // ERROR, varying reserved
in vec4 bad[10];
highp in vec4 badorder;
out invariant vec4 badorder2;
in centroid vec4 badorder4;    // ERROR, no centroid input to vertex stage
out flat vec4 badorder3;
void bar(in const float a);
void bar2(highp in float b);
smooth flat out vec4 rep;      // ERROR, replicating interpolation qualification
centroid sample out vec4 rep2; // ERROR, replicating auxiliary qualification
in uniform vec4 rep3;          // ERROR, replicating storage qualification

int anonconst;
const int aconst = 5;
const int a = aconst;
const int b = anonconst;       // ERROR at global scope

const int foo()                // ERROR, no const functions
{
    const int a = aconst;
    const int b = anonconst;
    const int c = a;          // still compile-time const
    const int d = b;          // not a compile-time const
    float x[c];               // okay
    float y[d];               // ERROR

    return b;
}

void main()
{
    int i;
    if (i == 3)
        int j = i;
    else
        int k = j;              // ERROR, j is undeclared
    int m = k;                  // ERROR, k is undeclared
    int n = j;                  // ERROR, j is undeclared

    while (true)
        int jj;
    int kk = jj;                // ERROR, jj is undeclared
}

const float cx = 4.20;
const float dx = 4.20;

void bar(in highp volatile vec4 v)
{
    int s;
    s.x;       // okay
    s.y;       // ERROR
    if (bad[0].x == cx.x)
        ;
    if (cx.x == dx.x)
        badorder3 = bad[0];

    float f;
    vec3 smeared = f.xxx;
    f.xxxxx;   // ERROR
    f.xxy;     // ERROR
}

layout(binding = 3) uniform;  // ERROR
layout(binding = 3) uniform boundblock { int aoeu; } boundInst;
layout(binding = 7) uniform anonblock { int aoeu; } ;
layout(location = 1) in;      // ERROR
layout(binding = 1) in inblock { int aoeua; };       // ERROR
layout(binding = 100000) uniform anonblock2 { int aooeu; } ;
layout(binding = 4) uniform sampler2D sampb1;
layout(binding = 5) uniform sampler2D sampb2[10];