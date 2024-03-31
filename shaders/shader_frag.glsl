#version 450 core

layout(location = 0) out vec4 outColor;

// Lewis noise
// https://dl.acm.org/doi/pdf/10.1145/74334.74360

// Gabor noise
// https://inria.hal.science/inria-00606821v1/document

// Phasor noise
// https://hal.science/hal-02118508/file/ProceduralPhasorNoise.pdf


#define PI 3.1415926538
#define UINT_MAX 4294967295u
#define EPS 1e-6f


// CONFIGURATION
layout(location = 0) uniform float Kernel_Gaussian_Magnitude;
layout(location = 1) uniform float Kernel_Gaussian_Bandwidth;
layout(location = 2) uniform float Kernel_Harmonic_Frequency_Magnitude;
layout(location = 3) uniform float Kernel_Harmonic_Frequency_Magnitude_Spread;
layout(location = 4) uniform float Kernel_Harmonic_Frequency_Orientation;
layout(location = 5) uniform float Kernel_Harmonic_Frequency_Orientation_Spread;

layout(location = 6) uniform bool Gabor_Phasor_Toggle;

layout(location = 7) uniform int Kernels_Per_Cell;
layout(location = 8) uniform bool Cell_Boundaries;
layout(location = 9) uniform float Cell_Size_Factor;

layout(location = 10) uniform bool Show_Kernel_Centers;
layout(location = 11) uniform bool Gabor_Show_Harmonic;

layout(location = 12) uniform int Oscillation_Profile;

uint RANDOM_OFFSET = 25565u;

// computed constants
float Kernel_Gaussian_Radius;
float Cell_Size;

void init() {
    Kernel_Gaussian_Radius = 1.0 / Kernel_Gaussian_Bandwidth;
    Cell_Size = Kernel_Gaussian_Radius * Cell_Size_Factor;
}



// PRNG
// multiplier from doi:10.1007/bf01937326
uint _prng_x;
void prng_seed(uint s) {
    _prng_x = s;
}
uint prng_next() {
    _prng_x *= 3039177861u;
    // we get mod 2^32 for free since
    // "Operations resulting in overflow ... yield the low-order 32 bits of the result"
    return _prng_x;
}
float prng_01() {
    return float(prng_next()) / float(UINT_MAX);
}
float prng_ab(float a, float b) {
    return a + (b - a) * prng_01();
}


// 2d coordinate to integer (in Morton order) + offset =  non-periodic seed
uint morton(ivec2 xy) {
    int x = xy.x; int y = xy.y;

    // http://www-graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
    int B[4] = int[](0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF);
    int S[4] = int[](1, 2, 4, 8);

    int z; // z gets the resulting 32-bit Morton Number.  
            // x and y must initially be less than 65536.

    x = (x | (x << S[3])) & B[3];
    x = (x | (x << S[2])) & B[2];
    x = (x | (x << S[1])) & B[1];
    x = (x | (x << S[0])) & B[0];

    y = (y | (y << S[3])) & B[3];
    y = (y | (y << S[2])) & B[2];
    y = (y | (y << S[1])) & B[1];
    y = (y | (y << S[0])) & B[0];

    z = x | (y << 1);
    return uint(z);
}


// gabor noise
//
// the viewport is split into a grid of cells
// each cell has impulses (Gabor kernels) in it

float gabor_kernel(
    float Gaussian_Magnitude,
    float Gaussian_Bandwidth,
    float Harmonic_Frequency_Magnitude,
    float Harmonic_Frequency_Orientation,
    vec2 d
) {
    // same notation as in paper
    float K = Gaussian_Magnitude;
    float a = Gaussian_Bandwidth;
    float F_0 = Harmonic_Frequency_Magnitude;
    float w_0 = Harmonic_Frequency_Orientation;

    float sq_a = a * a;
    float sq_x = d.x * d.x;
    float sq_y = d.y * d.y;
    
    float gaussian = K * exp(-PI * sq_a * (sq_x + sq_y));
    float harmonic = cos(2.0 * PI * F_0 * (d.x * cos(w_0) + d.y * sin(w_0)));

    float noise = gaussian;
    if (Gabor_Show_Harmonic)
        noise *= harmonic;

    return noise;
}

float gabor_cell(ivec2 ci, vec2 cp) {
    uint seed = morton(ci) + RANDOM_OFFSET;
    prng_seed(seed > 0u ? seed : 1u);
    
    float noise = 0.0;
    for (int k = 0; k < Kernels_Per_Cell; k++) {
        // random position within cell
        vec2 kernel_pos = vec2(prng_01(), prng_01());
        vec2 d = (cp - kernel_pos) * Cell_Size;
        
        // use random omega for isotropic noise
        noise += gabor_kernel(
            Kernel_Gaussian_Magnitude,
            Kernel_Gaussian_Bandwidth,
            Kernel_Harmonic_Frequency_Magnitude + (prng_01() - 0.5) * Kernel_Harmonic_Frequency_Magnitude_Spread,
            Kernel_Harmonic_Frequency_Orientation + (prng_01() - 0.5) * Kernel_Harmonic_Frequency_Orientation_Spread,
            d
        );
    }
    
    return noise;
}

float gabor(ivec2 cell_idx, vec2 cell_pos) {
    float noise = 0.0;
    // cell size is equal to kernel radius so we only need to compute
    // noise for the current cell and direct neighbours
    for (int di = -1; di <= 1; di++)
        for (int dj = -1; dj <= 1; dj++)
            noise += gabor_cell(cell_idx+ivec2(di,dj), cell_pos-vec2(di,dj));
    
    return noise;
}

float normalized_gabor(ivec2 cell_idx, vec2 cell_pos) {
    float noise = gabor(cell_idx, cell_pos);
    
    float F_0 = Kernel_Harmonic_Frequency_Magnitude;
    float variance =
        2.0 * sqrt(float(Kernels_Per_Cell) * 0.25 * exp(-2.0 * PI * F_0 * F_0 / (Cell_Size * Cell_Size)));

    if (!Gabor_Show_Harmonic)
        variance = 2.0 * sqrt(float(Kernels_Per_Cell) * exp(1.0 / (Cell_Size * Cell_Size)));

    return noise / variance;
}


// phasor noise
vec2 phasor_kernel(
    float F_0,
    float a,
    float w_0,
    float phi,
    vec2 d
) {
    float sq_a = a * a;
    float sq_x = d.x * d.x;
    float sq_y = d.y * d.y;
    
    float gaussian = exp(-PI * sq_a * (sq_x + sq_y));
    
    float s = sin(2.0 * PI * F_0  * (d.x * cos(w_0) + d.y * sin(w_0)) + phi);
    float c = cos(2.0 * PI * F_0  * (d.x * cos(w_0) + d.y * sin(w_0)) + phi);
    return gaussian * vec2(s, c);
}

vec2 phasor_cell(ivec2 ci, vec2 cp) {
    uint seed = morton(ci) + RANDOM_OFFSET;
    prng_seed(seed > 0u ? seed : 1u);
    
    vec2 noise = vec2(0.0);
    for (int k = 0; k < Kernels_Per_Cell; k++) {
        vec2 kernel_pos = vec2(prng_01(), prng_01());
        vec2 d = (cp - kernel_pos) * Cell_Size;
        float phi = 0.0;
        noise += phasor_kernel(
            Kernel_Harmonic_Frequency_Magnitude + (prng_01() - 0.5) * Kernel_Harmonic_Frequency_Magnitude_Spread,
            Kernel_Gaussian_Bandwidth,
            Kernel_Harmonic_Frequency_Orientation + (prng_01() - 0.5) * Kernel_Harmonic_Frequency_Orientation_Spread,
            phi,
            d
        );
    }
    
    return noise;
}

vec2 phasor(ivec2 cell_idx, vec2 cell_pos) {
    vec2 noise = vec2(0.0);
    for (int di = -1; di <= 1; di++)
        for (int dj = -1; dj <= 1; dj++)
            noise += phasor_cell(cell_idx+ivec2(di,dj), cell_pos-vec2(di,dj));
    
    return noise;
}

void main() {
    init();
    
    vec2 cell_uv = gl_FragCoord.xy / Cell_Size;
    ivec2 cell_idx = ivec2(cell_uv);
    vec2 cell_pos = cell_uv - vec2(cell_idx);

    bool kernel_center = false;
    if (Show_Kernel_Centers) {
        uint seed = morton(cell_idx) + RANDOM_OFFSET;
        prng_seed(seed > 0u ? seed : 1u);
        for (int k = 0; k < Kernels_Per_Cell; k++) {
            vec2 kernel_pos = vec2(prng_01(), prng_01());
            if (distance(kernel_pos, cell_pos) * Cell_Size <= 2)
                kernel_center = true;
        }
    }
   
    if (kernel_center) {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
    else if (Cell_Boundaries && min(cell_pos.x, cell_pos.y) * Cell_Size <= 1) {
        outColor = vec4(0.0);
    }
    else {
        if (!Gabor_Phasor_Toggle) {
            float noise = normalized_gabor(cell_idx, cell_pos);
            outColor = vec4(0.5 + 0.5 * noise);
        }
        else {
            vec2 noise = phasor(cell_idx, cell_pos);
            float phi = atan(noise.y, noise.x);

            if (Oscillation_Profile == 0)
                outColor = vec4(phi + PI < PI / 2.0 ? 0.0 : 1.0);
            else if (Oscillation_Profile == 1)
                outColor = vec4(phi + PI < PI ? 0.0 : 1.0);
            else if (Oscillation_Profile == 2)
                outColor = vec4(0.5 + phi / (2.0 * PI));
            else
                outColor = vec4(0.5 + 0.5 * sin(phi));
        }
    }
}