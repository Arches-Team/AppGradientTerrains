#version 430 core

#ifdef COMPUTE_SHADER

const float PI = 3.14159265359;

// Buffers
layout(std430, binding = 0) readonly buffer HeightfieldBuffer {
    float hf[];
};

layout(std430, binding = 1) writeonly buffer HeightfieldOutBuffer {
    float hfOut[];
};

// Uniforms
uniform int WIDTH;
uniform int HEIGHT;
uniform float WORLD_SIZE;

// In meters - how wide gullies are
uniform float EROSION_SCALE_METER = 200;
uniform float EROSION_STRENGTH = 5.;

uniform float EROSION_SLOPE_POWER = 0.6;
uniform float EROSION_CELL_SCALE = 1.;
uniform int   EROSION_OCTAVES = 3;
uniform float EROSION_GAIN = 0.5;
uniform float EROSION_LACUNARITY = 2.0;
uniform float EROSION_HEIGHT_OFFSET = 0.;

int index(ivec2 p) {
    return clamp(p.x, 0, WIDTH-1) + clamp(p.y, 0, HEIGHT-1) * WIDTH;
}

// -- Adapted from https://www.shadertoy.com/view/33cXW8

vec2 hash( in vec2 x )
{
    const vec2 k = vec2( 0.3183099, 0.3678794 );
    x = x * k + k.yx;
    return -1.0 + 2.0 * fract(16.0 * k * fract( x.x * x.y * (x.x + x.y)));
}

// Produce gullies (height offset and slope/derivatives) orthogonal to the input slope.
// Steeper slopes produce more high frequency gullies. The amplitude is always one.
// The function is a type of noise with strong directionality, produced by blending between
// sine wave stripes contributed by each cell in a Voronoi-like pattern.
// The sine waves of all cells are along the same direction: Orthogonal to the slope direction,
// meaning the wave stripes run along the slope. Both frequency and phase are relative to the
// cell center, so that small changes in slope only produce small changes in output.
//
// Gullies function by Rune Skovbo Johansen, derived from erosion function by Clay John,
// with the following changes:
// - Stripes are parallel to the input direction rather than orthogonal to it.
// - Derivatives have correct magnitude.
// - Code is slightly more optimized, front-loading calculations outside the for-loops.
// - Variable names are descriptive and comments are verbose, for easier understanding.
vec3 Gullies(in vec2 p, vec2 slope) {
    // Get a vector orthogonal to the slope, with a proportional magnitude. Front-load the
    // multiplication by 2 pi, since every usage of sideDir multiplied with that anyway.
    vec2 sideDir = slope.yx * vec2(-1.0, 1.0) * 2.0 * PI;

    // Iterate over 4x4 cells, calculating a stripe pattern for each and blending between them.
    // pInt is the integer part of the current coordinate p, pFrac is the remainder.
    //
    // o   o   o   o
    //
    // o   o   o   o
    //       p
    // o   i   o   o
    //
    // o   o   o   o
    //
    // p: current coordinate    i: integer part of p    o: grid points for 4x4 cells
    //
    vec2 pInt = floor(p);
    vec2 pFrac = fract(p);
    vec3 heightAndSlope = vec3(0.0);
    float weightSum = 0.0;
    for (int i = -1; i <= 2; i++) {
        for (int j = -1; j <= 2; j++) {
            vec2 gridOffset = vec2(i, j);

            // Calculate a cell point by starting off with a point in an integer grid.
            vec2 gridPoint = pInt + gridOffset;

            // Calculate a random offset for the cell point between -0.5 and 0.5 on each axis.
            vec2 randomOffset = hash(gridPoint) * 0.5;

            // The final cell point (we don't store it) is the gridPoint plus the randomOffset.
            // Calculate a vector representing the input point relative to this cell point:
            // p - (gridPoint + randomOffset)
            // = (pFrac + pInt) - ((pInt + gridOffset) + randomOffset)
            // = pFrac + pInt - pInt - gridOffset - randomOffset
            // = pFrac - gridOffset - randomOffset
            vec2 vectorFromCellPoint = pFrac - gridOffset - randomOffset;

            // Bell-shaped weight function which is 1 at dist 0 and 0 at dist 1.5.
            // Due to the random offsets of up to 0.5, the closest a cell point not in the 4x4
            // grid can be to the current point is 1.5 units away.
            // The subtraction of 0.01111 is to make the function actually 0 at distance 1.5,
            // which avoids some (very subltle) grid line artefacts.
            float sqrDist = dot(vectorFromCellPoint, vectorFromCellPoint);
            float weight = max(0.0, exp(-sqrDist * 2.0) - 0.01111);

            // Keep track of total sum of weights.
            weightSum += weight;

            // The waveInput is a gradient which increases in value along sideDir (sideways
            // along the terrain slope). Its rate of change is the slope times 2 pi, due to the
            // 2 pi multiplier pre-applied to sideDir.
            float waveInput = dot(vectorFromCellPoint, sideDir);

            // Add a wave along sideDir. The longer sideDir is, the higher frequency the wave.
            // This means we get more high-frequency creases on steeper slopes.
            // Note that the fact we use cos() for the height (and not sin()) has a significant
            // effect on mountain peaks. Since the slope and hence the frequency is zero there,
            // cos() ensures this evaluates to the maximum value in the wave, meaning peaks
            // "get no erosion".
            heightAndSlope += vec3(
            cos(waveInput), // Amplitude of the wave
            -sin(waveInput) * sideDir // Analytical derivative of the wave
            // Correct derivatives would also involve applying the chain rule here,
            // but as it seems to look worse, it's disabled here:
            //+ cos(waveInput) * 4.0 * vectorFromCellPoint
            ) * weight;
        }
    }
    return heightAndSlope / weightSum;
}


// Apply an erosion filter inspired by Fractal Brownian Motion type noise.
// Over several iterations, gullies are added as sine wave stripes that run parallel to the
// terrain slope. The gullies modify the slope, causing the next iteration of gullies to branch
// out at an angle to the previous ones.
vec3 Erosion(
in vec2 p, vec3 heightAndSlope, float scale, float strength, float slopePower,
   float cellScale, int octaves, float gain, float lacunarity
) {
    vec3 inputHeightAndSlope = heightAndSlope;
    float freq = 1.0 / (scale * cellScale);
//    strength *= scale;
    for (int i = 0; i < octaves; i++) {
        // Adjust the slope given to the Gullies function, without influencing the final slope.
        // The calculation is equivalent to raising the length of the slope to slopePower.
        float sqrLen = dot(heightAndSlope.yz, heightAndSlope.yz);
        vec2 inputSlope = heightAndSlope.yz * pow(sqrLen, 0.5 * (slopePower - 1.0));

        // Calculate and add gullies to the height and slope.
        heightAndSlope += Gullies(p * freq, inputSlope * cellScale)
        * strength * vec3(1, freq, freq);

        // Prepare the next octave.
        strength *= gain;
        freq *= lacunarity;
    }
    return heightAndSlope - inputHeightAndSlope;
}

// Use the geometric series to calculate the extents of layered noise where the first layer has
// magnitude 1 and each successive layer is multiplied with gain.
float MagnitudeSum(int octaves, float gain) {
    return (1.0 - pow(gain, float(octaves))) / (1.0 - gain);
}

// --

float Heightmap(vec2 p, vec3 n)
{
    float metersPerPixel = WORLD_SIZE / float(WIDTH);
    float strength = EROSION_STRENGTH * metersPerPixel;

    int octaves = EROSION_OCTAVES;

    vec3 h = Erosion(p, n, EROSION_SCALE_METER, strength,
                     EROSION_SLOPE_POWER, EROSION_CELL_SCALE,
                     octaves, EROSION_GAIN, EROSION_LACUNARITY);

    float erosionMagnitude = EROSION_SCALE_METER * strength * MagnitudeSum(octaves, EROSION_GAIN);
    float offset = erosionMagnitude * EROSION_HEIGHT_OFFSET;

    return (n.x + h.x + offset)*1.;
}

float getHeight(ivec2 p)
{
    return hf[index(p)];
}

layout(local_size_x = 8, local_size_y = 8) in;
void main() {
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);

    // Invocations outside the grid would overwrite border cells through the clamped index()
    if (id.x >= WIDTH || id.y >= HEIGHT) return;

    vec2 p = (vec2(id)/vec2(WIDTH)) * WORLD_SIZE;

    float h = getHeight(id);

    float dx = WORLD_SIZE / float(WIDTH);
    float dy = WORLD_SIZE / float(HEIGHT);

    vec2 s = vec2((getHeight(id + ivec2(1, 0)) - getHeight(id + ivec2(-1, 0))) / (2.0 * dx),
                  (getHeight(id + ivec2(0, 1)) - getHeight(id + ivec2(0, -1))) / (2.0 * dy));

    vec3 hs = vec3(h, s);

    float erosion = Heightmap(p, hs);

    float newH = erosion;

    hfOut[index(id)] = (newH);
}

#endif