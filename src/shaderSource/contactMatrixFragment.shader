#version 330
in vec3 Texcoord;
out vec4 outColor;
uniform sampler2DArray tex;                // 0, which texture to use
uniform samplerBuffer colormap;            // 1, returns float values.
uniform usamplerBuffer pixstartlookup;     // 2, returns unsigned integer values.
uniform usamplerBuffer pixrearrangelookup; // 3 
uniform uint pixpertex;        // 1024
uniform uint ntex1dm1;         // number of textures in 1D minus 1 = 31
uniform float oopixpertex;     // 1.0 / pixpertex
uniform vec3 controlpoints;    // 0.0, 0.5, 1.0 

float bezier(float t)  // bezier function to adjust the color map
{
    float tsq = t * t;
    float omt = 1.0 - t;
    float omtsq = omt * omt; // (1-t)^2
    // (1-t)^2 * x + 2*t* (1-t) * y + t^2 * z
    return((omtsq * controlpoints.x) + (2.0 * t * omt * controlpoints.y) + (tsq * controlpoints.z));
}

float linearTextureID(vec2 coords)
{   // cal the linear texture id according to the coords
    float min;
    float max;
    
    if (coords.x > coords.y)
    {
        min = coords.y;
        max = coords.x;
    }
    else
    {
        min = coords.x;
        max = coords.y;
    }

    int i = int(min); // row number
    return ( 
        max +  
        (min * ntex1dm1) -
        ( (i & 1) == 1 ? (((i-1)>>1) * min) : ((i>>1)*(min-1)) )
    );
}

// https://community.khronos.org/t/mipmap-level-calculation-using-dfdx-dfdy/67480
float mip_map_level(in vec2 texture_coordinate)
{
    // The OpenGL Graphics System: A Specification 4.2
    //  - chapter 3.9.11, equation 3.21
    vec2  dx_vtc        = dFdx(texture_coordinate);
    vec2  dy_vtc        = dFdy(texture_coordinate);
    float delta_max_sqr = max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc));
    return 0.5 * log2(delta_max_sqr);
}

vec3 pixLookup(vec3 inCoords)
{
    vec2 pix = floor(inCoords.xy * pixpertex);  // floor(xy * 1024)
    
    vec2 over = inCoords.xy - (pix * oopixpertex); // giving the offset within the texture
    vec2 pixStart = texelFetch(pixstartlookup, int(inCoords.z)).xy;
    
    pix += pixStart;
    
    pix = vec2(
        texelFetch(pixrearrangelookup, int(pix.x)).x, 
        texelFetch(pixrearrangelookup, int(pix.y)).x
        );
    
    if (pix.y > pix.x)
    {
        pix = vec2(pix.y, pix.x);
        over = vec2(over.y, over.x);
    }
    
    vec2 tileCoord = pix * oopixpertex; //  pix / 1024
    vec2 tileCoordFloor = floor(tileCoord);
    
    float z = linearTextureID(tileCoordFloor);
    
    pix = tileCoord - tileCoordFloor;
    pix = clamp(pix + over, vec2(0, 0), vec2(1, 1));  // clamp(x, minVal, maxVal)
    
    return(vec3(pix, z));
}

// https://www.codeproject.com/Articles/236394/Bi-Cubic-and-Bi-Linear-Interpolation-with-GLSL/
float BiLinear( vec3 inCoord, vec2 texSize, float lod )  // size = [1024, 1024]
{
    vec2 texelSize = 1.0 / texSize;

    // sample the texture at the given coordinate. (r g b a), select red channel.
    float p0q0 = textureLod(tex, pixLookup(inCoord                                    ), lod).r;
    float p1q0 = textureLod(tex, pixLookup(inCoord + vec3(texelSize.x, 0,           0)), lod).r;
    float p0q1 = textureLod(tex, pixLookup(inCoord + vec3(0,           texelSize.y, 0)), lod).r;
    float p1q1 = textureLod(tex, pixLookup(inCoord + vec3(texelSize.x, texelSize.y, 0)), lod).r;

    // Fraction near to valid data.
    float a = fract( inCoord.x * texSize.x ); // Get Interpolation factor for X direction.
    float b = fract( inCoord.y * texSize.y );// Get Interpolation factor for Y direction.
    
    float pInterp_q0 = mix( p0q0, p1q0, a ); // Interpolates top row in X direction.
    float pInterp_q1 = mix( p0q1, p1q1, a ); // Interpolates bottom row in X direction.

    return mix( pInterp_q0, pInterp_q1, b ); // Interpolate in Y direction.
}

void main()
{
    float mml = mip_map_level(Texcoord.xy * textureSize(tex, 0).xy);
    
    float floormml = floor(mml);

    float f1 = BiLinear(Texcoord, textureSize(tex, 0).xy, floormml); // textureSize.xy = [1024, 1024]
    float f2 = BiLinear(Texcoord, textureSize(tex, 0).xy, floormml + 1);

    float value = bezier(mix(f1, f2, fract(mml))); // mix(x, y, a) = x * (1 - a) + y * a
    int idx = int(round(value * 255)); // covert to from [0 - 1] to [0 - 255]
    outColor = vec4(texelFetch(colormap, idx).rgb, 1.0);
    // outColor = texelFetch(colormap, idx);
}