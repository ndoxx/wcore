vec3 blinn_phong(vec3 lightColor,
                 vec3 lightDir,
                 vec3 normal,
                 vec3 viewDir,
                 vec3 albedo,
                 float shininess,
                 float specInt,
                 float lumaCorrect)
{
    lightDir = normalize(lightDir);
    // Halfway vector
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // Diffuse shading factor
    float diff = max(dot(normal, lightDir), 0.0);
    // Modified energy conservative Blinn-Phong specular factor
    float spec = (1.0-diff)*pow(max(dot(normal, halfwayDir), 0.0), shininess);
    spec = clamp(spec,0.0,1.0);

    // combine results
    vec3 ambient  = 0.05*lightColor * albedo;
    vec3 diffuse  = lightColor * diff * albedo * lumaCorrect;
    vec3 specular = lightColor * spec * specInt * lumaCorrect;

    return ambient + diffuse + specular;
}
