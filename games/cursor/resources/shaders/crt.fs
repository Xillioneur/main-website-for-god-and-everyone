#version 300 es
precision mediump float;

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// Custom uniforms
uniform vec2 resolution;
uniform float time;
uniform float aberration;

// Constants
const float CURVATURE = 3.0;
const float VIGNETTE_OPACITY = 1.0;

vec2 uv_curve(vec2 uv)
{
    uv = (uv - 0.5) * 2.0;
    uv.x *= 1.0 + pow(abs(uv.y) / CURVATURE, 2.0);
    uv.y *= 1.0 + pow(abs(uv.x) / CURVATURE, 2.0);
    uv /= 2.0;
    return uv + 0.5;
}

void main()
{
    vec2 uv = uv_curve(fragTexCoord);
    
    // Discard outside pixels
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) discard;
    
    // Chromatic Aberration
    float r = texture(texture0, uv + vec2(aberration, 0.0)).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uv - vec2(aberration, 0.0)).b;
    vec3 color = vec3(r, g, b);
    
    // Scanlines
    float scanline = sin(uv.y * resolution.y * 3.0) * 0.1;
    color -= scanline;
    
    // Vignette
    float vignette = uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
    vignette = pow(vignette * 15.0, 0.25); // strength
    color *= vignette;

    finalColor = vec4(color, 1.0);
}