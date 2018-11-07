
// Code for variance shadow mapping
// from https://fabiensanglard.net/shadowmappingVSM/

// STORING MOMENTS
//----------- VS -----------
varying vec4 v_position;

void main()
{
        gl_Position = ftransform();
        v_position = gl_Position;
}

//----------- FS -----------
varying vec4 v_position;

void main()
{
    float depth = v_position.z / v_position.w ;
    depth = depth * 0.5 + 0.5;          //Don't forget to move away from unit cube ([-1,1]) to [0,1] coordinate system

    float moment1 = depth;
    float moment2 = depth * depth;

    // Adjusting moments (this is sort of bias per pixel) using derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    moment2 += 0.25*(dx*dx+dy*dy) ;

    //! Valide pour un FBO 32bits
    //! Pour un FBO 16bits packer dans les 4 composantes pour ne pas
    //! perdre en précision.
    gl_FragColor = vec4( moment1,moment2, 0.0, 0.0 );
}

// APPLYING SHADOW
//----------- VS -----------
// Used for shadow lookup
varying vec4 ShadowCoord;

void main()
{
        ShadowCoord= gl_TextureMatrix[7] * gl_Vertex;
        gl_Position = ftransform();
        gl_FrontColor = gl_Color;
}

//----------- FS -----------
uniform sampler2D ShadowMap;
varying vec4 ShadowCoord;
vec4 ShadowCoordPostW;

float chebyshevUpperBound( float distance)
{
    vec2 moments = texture2D(ShadowMap,ShadowCoordPostW.xy).rg;

    // Surface is fully lit. as the current fragment is before the light occluder
    if (distance <= moments.x)
        return 1.0 ;

    // The fragment is either in shadow or penumbra. We now use chebyshev's upperBound to check
    // How likely this pixel is to be lit (p_max)
    float variance = moments.y - (moments.x*moments.x);
    variance = max(variance,0.00002);

    //! SI C'EST MOCHE, remplacer par :
    // variance = max(variance,0.002);

    float d = distance - moments.x;
    float p_max = variance / (variance + d*d);

    return p_max;
}

void main()
{
    ShadowCoordPostW = ShadowCoord / ShadowCoord.w;
    //ShadowCoordPostW = ShadowCoordPostW * 0.5 + 0.5; This is done via a bias matrix in main.c
    float shadow = chebyshevUpperBound(ShadowCoordPostW.z);
    gl_FragColor = vec4(shadow ) *gl_Color;

    //! Pour le debug
    // ShadowCoordPostW = ShadowCoord / ShadowCoord.w;
    // vec2 moments = texture2D(ShadowMap,ShadowCoordPostW.xy).rg;
    // gl_FragColor = vec4( moments.r, moments.g, ShadowCoordPostW.z, 1.0 );

}
