#version 450 core

//
// Note(Axel): this is the OpenGL shader version of Sebastian Lague Hydraulic erosion implementation in Unity.
// It definitely has a race condition, because multiple threads can be writing to the elevation buffer at the same time,
// Thus making operations not taken into account.
// The unity shader implementation also has this problem.
//

#ifdef COMPUTE_SHADER

layout(binding = 0, std430) coherent buffer Elevation
{
	float bedrock[];
};
layout(binding = 1, std430) coherent buffer Sediments
{
	float sediments[];
};
layout(binding = 2, std430) readonly buffer BrushIndices
{
	int brushIndices[];
};
layout(binding = 3, std430) readonly buffer BrushWeights
{
	float brushWeights[];
};
layout(binding = 4, std430) readonly buffer RandomIndices
{
	int randomIndices[];
};
layout(binding = 5, std430) readonly buffer HardnessBuffer
{
	float hardness[];
};
layout(binding = 6, std430) readonly buffer PrecipitationBuffer
{
	float precipitation[];
};
layout(binding = 7, std430) writeonly buffer OutHardnessBuffer
{
	float outHardness[];
};

uniform int nx;
uniform int ny;
uniform int brushLength;
uniform int borderSize;
uniform int useHardness;
uniform int usePrecipitation;
uniform int hardnessFunctionIndex = 0;
uniform int startIndex;

// Almost fixed parameters
uniform float startSpeed                = 1;
uniform float startWater                = 2;
uniform int maxLifetime                 = 200;
uniform float inertia                   = 0.3f;
uniform float sedimentCapacityFactor    = 10.0f;
uniform float minSedimentCapacity       = 0.01f;
uniform float gravity                   = 4;

// Parameters from interface
uniform float depositSpeed              = 0.01f;
uniform float erodeSpeed                = 0.05f;
uniform float evaporateSpeed            = 0.01f;
uniform float randomSedimentation       = 0.0f;
uniform float randomSedimentationFreq   = 0.0002f;
uniform float randomSedimentationAmp    = 1.0f;
uniform float globalSpeed               = 1.0f;


float random (vec2 st) { return fract(sin(dot(st.xy, vec2(12.9898,78.233)))* 43758.5453123); }
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) {  return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 permute(vec4 x) {     return mod289(((x*34.0)+1.0)*x);}
vec4 taylorInvSqrt(vec4 r){ return 1.79284291400159 - 0.85373472095314 * r;}
float snoise(vec3 v) { 
	const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);
	vec3 i  = floor(v + dot(v, C.yyy) );  vec3 x0 =   v - i + dot(i, C.xxx) ;  vec3 g = step(x0.yzx, x0.xyz);  vec3 l = 1.0 - g;  vec3 i1 = min( g.xyz, l.zxy );  vec3 i2 = max( g.xyz, l.zxy );
	vec3 x1 = x0 - i1 + C.xxx;  vec3 x2 = x0 - i2 + C.yyy;   vec3 x3 = x0 - D.yyy;    
	i = mod289(i);  vec4 p = permute( permute( permute(  i.z + vec4(0.0, i1.z, i2.z, 1.0 ))   + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));
	float n_ = 0.142857142857;   vec3  ns = n_ * D.wyz - D.xzx;  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)
	vec4 x_ = floor(j * ns.z);  vec4 y_ = floor(j - 7.0 * x_ );      vec4 x = x_ *ns.x + ns.yyyy;  vec4 y = y_ *ns.x + ns.yyyy;  vec4 h = 1.0 - abs(x) - abs(y);
	vec4 b0 = vec4( x.xy, y.xy );  vec4 b1 = vec4( x.zw, y.zw );  vec4 s0 = floor(b0)*2.0 + 1.0;  vec4 s1 = floor(b1)*2.0 + 1.0;  vec4 sh = -step(h, vec4(0.0));
	vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;  vec3 p0 = vec3(a0.xy,h.x);  vec3 p1 = vec3(a0.zw,h.y);  vec3 p2 = vec3(a1.xy,h.z);  vec3 p3 = vec3(a1.zw,h.w);
	vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));   p0 *= norm.x;  p1 *= norm.y;  p2 *= norm.z;  p3 *= norm.w;
	vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);  m = m * m;
	return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3) ) );
}

float ridge(vec3 v) {
    return 1.0f - abs(snoise(v) * 0.5 + 0.5);
}

float Height(int index) {
    return bedrock[index] + sediments[index];
}

int Side(vec3 p, vec3 pp, vec3 n) {
    float c = dot(pp, n);
    float r = dot(n, p) - c;
    if (r > 0.0000001f)
        return 1;
    else if (r < -0.0000001f)
        return -1;
    else
        return 0;
}

float GetHardness(in vec3 p, int index){
    float h = 1.0f;

    // Strata function
    //if (hardnessFunctionIndex == 0)
   // {
    	p       *= 50.0f;
        h        = sin(p.z) * 0.5f + 0.5f;
    //}

    // Warped strata function
    if (hardnessFunctionIndex == 1)
    {    
        float wa = 15.0 * snoise(p * 0.005f);
        p       *= 80.0f;
        h        = sin(p.z + wa) * 0.5f + 0.5f;
    }

    // 3D Noise
    if (hardnessFunctionIndex == 2)
    {
        h = (snoise(p * 0.005) * 0.5f + 0.5f) + 0.5f * (snoise(p * 0.01) * 0.5f + 0.5f) + 0.25f * (snoise(p * 0.025) * 0.5f + 0.5f);
        h *= 2.0;
    }
      
    // Faulting: strata on one side, 3D noise on the other
    if (hardnessFunctionIndex == 3)
    {
        // Horizontal fault
        /*h = snoise(p * 0.005) * 0.5 + 0.5;
        if (p.z > 0.65)
            h = sin(p.z * 50.0f) * 0.5f + 0.5f;*/

        // Vertical fault
        h = sin(p.z * 50.0f) * 0.5f + 0.5f;
        if (Side(p, vec3(550.0f, 550.0f, 0), vec3(1, 1, 0)) == 1)
            h = (snoise(p * 0.005) * 0.5f + 0.5f) + 0.5f * (snoise(p * 0.01) * 0.5f + 0.5f) + 0.25f * (snoise(p * 0.025) * 0.5f + 0.5f);
    }

    // Then modulate the effect with a user map, if enabled
    if (useHardness == 1)
        return clamp(h * hardness[index], 0.6f, h);
    else
        return 1.0f;
}

int ToIndex1D(int x, int y) {
    return y * nx + x;
}

// Returns vec3(gradientX, gradientY, height)
vec3 CalculateHeightAndGradient (float posX, float posY) {
    int coordX = int(posX);
    int coordY = int(posY);

    // Calculate droplet's offset inside the cell (0,0) = at NW node, (1,1) = at SE node
    float x = posX - coordX;
    float y = posY - coordY;

    // Calculate heights of the four nodes of the droplet's cell
    int nodeIndexNW = coordY * nx + coordX;
    float heightNW = Height(nodeIndexNW);
    float heightNE = Height(nodeIndexNW + 1);
    float heightSW = Height(nodeIndexNW + nx);
    float heightSE = Height(nodeIndexNW + nx + 1);

    // Calculate droplet's direction of flow with bilinear interpolation of height difference along the edges
    float gradientX = (heightNE - heightNW) * (1 - y) + (heightSE - heightSW) * y;
    float gradientY = (heightSW - heightNW) * (1 - x) + (heightSE - heightNE) * x;

    // Calculate height with bilinear interpolation of the heights of the nodes of the cell
    float height = heightNW * (1 - x) * (1 - y) + heightNE * x * (1 - y) + heightSW * (1 - x) * y + heightSE * x * y;

    return vec3(gradientX, gradientY, height);
}

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main()
{
    int index       = randomIndices[startIndex + gl_GlobalInvocationID.x];
    float posX      = float(index % nx);
    float posY      = float(index / nx);
	if (posX < 0) 	return;
	if (posY < 0) 	return;
	if (posX >= nx) return;
	if (posY >= ny) return;

    // Init particle data
    float dirX = 0;
    float dirY = 0;
    float sediment = 0;
    float speed = startSpeed;

    // Water can come from the precipitation map
    float water = usePrecipitation == 1 ? precipitation[index] * startWater : startWater;
    for (int lifetime = 0; lifetime < maxLifetime; lifetime++)
    {
        int nodeX = int(posX);
        int nodeY = int(posY);
        int dropletIndex = ToIndex1D(nodeX, nodeY);

        // Calculate droplet's offset inside the cell (0,0) = at NW node, (1,1) = at SE node
        float cellOffsetX = posX - nodeX;
        float cellOffsetY = posY - nodeY;   

        vec3 heightAndGradient = CalculateHeightAndGradient(posX, posY);

        // Try: slight noisy pertubation of particle trajectory
        vec2 noisePertub = vec2(0, 0);
        //noisePertub = vec2(snoise(vec3(posX, posY, 0.0f) * 0.0025f), snoise(vec3(posX + 987.47f, posY + 256.1796f, 0.0f) * 0.0025f));
        //noisePertub = vec2(random(vec2(posX, posY)), random(vec2(posX + 256.23f, posY + 256.23f))) * 2.0f - 1.0f;
        //noisePertub *= randomSedimentationAmp;

        // Update the droplet's direction and position (move position 1 unit regardless of speed)
        dirX = (dirX * inertia - heightAndGradient.x * (1 - inertia)) + (noisePertub.x);
        dirY = (dirY * inertia - heightAndGradient.y * (1 - inertia)) + (noisePertub.y);

        // Normalize direction
        float len = max(0.01f, sqrt(dirX * dirX + dirY * dirY));
        dirX /= len;
        dirY /= len;
        
        posX += dirX;
        posY += dirY;

        // Stop simulating droplet if the particle is not moving
        if ((dirX == 0 && dirY == 0) || posX < borderSize || posX > nx - borderSize || posY < borderSize || posY > ny - borderSize)
            break;
        
        // Find the droplet's new height and calculate the deltaHeight
        float newHeight = CalculateHeightAndGradient(posX, posY).z;
        float deltaHeight = newHeight - heightAndGradient.z;

        // Calculate the droplet's sediment capacity (higher when moving fast down a slope and contains lots of water)
        float sedimentCapacity = max(-deltaHeight, minSedimentCapacity) * speed * water * sedimentCapacityFactor;

        // If carrying more sediment than capacity, or if flowing uphill:
        if (sediment > sedimentCapacity || deltaHeight > 0) 
        {
            // If moving uphill (deltaHeight > 0) try fill up to the current height, otherwise deposit a fraction of the excess sediment
            float amountToDeposit = (deltaHeight > 0) ? min(deltaHeight, sediment) : (sediment - sedimentCapacity) * depositSpeed;

            // Random sedimentation
            {
                // With a noise, gives more control to the user, but can show repetitions.
                //float r = snoise(vec3(posX, posY, 1.0f) * randomSedimentationFreq) * 0.5 + 0.5f;

                // Random sedimentation with a white noise doesn't allow the frequency parameter for the user.
                // White noise is white noise at any and every scale.
                float r = random(vec2(posX, posY));
            
                if (r < randomSedimentation)
                {
                    amountToDeposit -= r * randomSedimentationAmp;
                    amountToDeposit = clamp(amountToDeposit, 0.0, amountToDeposit);
                }
            }

            // Global speed term
            amountToDeposit *= globalSpeed;
            
            // Remove sediment from particle
            sediment -= amountToDeposit;

            // Deposit the sediment to the four nodes of the current cell using bilinear interpolation
            // Deposition is not distributed over a radius (like erosion) so that it can fill small pits
            {
                // Deposit
                sediments[dropletIndex]            += amountToDeposit * (1 - cellOffsetX) * (1 - cellOffsetY);
                sediments[dropletIndex + 1]        += amountToDeposit * cellOffsetX * (1 - cellOffsetY);
                sediments[dropletIndex + nx]       += amountToDeposit * (1 - cellOffsetX) * cellOffsetY;
                sediments[dropletIndex + nx + 1]   += amountToDeposit * cellOffsetX * cellOffsetY;
            }     
        }
        else
        {
            // Erode a fraction of the droplet's current carry capacity.
            // Clamp the erosion to the change in height so that it doesn't dig a hole in the terrain behind the droplet
            float amountToErode = min((sedimentCapacity - sediment) * erodeSpeed, -deltaHeight);
            
            // Modulate with hardness function
            float hardnessFactor = GetHardness(vec3(posX, posY, bedrock[dropletIndex]), dropletIndex);
            outHardness[dropletIndex] = hardnessFactor;
            amountToErode *= hardnessFactor;

            // Global speed term
            amountToErode *= globalSpeed;

            // Erode
            for (int i = 0; i < brushLength; i ++)
            {
                int erodeIndex              = dropletIndex + brushIndices[i];
                float weightedErodeAmount   = amountToErode * brushWeights[i];
                float deltaSediment         = (bedrock[erodeIndex] < weightedErodeAmount) ? bedrock[erodeIndex] : weightedErodeAmount;
                bedrock[erodeIndex]        -= deltaSediment;
                sediment                   += deltaSediment;
            }
        }

        // Update droplet's speed and water content
        speed = sqrt(max(0, speed * speed + abs(deltaHeight) * gravity));
        water *= (1.0f - evaporateSpeed);
    }
}

#endif
