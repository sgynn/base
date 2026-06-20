
const float PI = 3.14159265f;

struct	LightingData {
	vec3 diffuse;
	vec3 specular;
};

float saturate(float v) { return clamp(v,0.0, 1.0); }
float saturate(vec3  v) { return clamp(v,0.0, 1.0); }

vec3 FresnelDiffuse(vec3 specColor)
{
	return saturate(1.0 - dot(specColor, vec3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f)));
}

vec3 F_Schlick(vec3 f0, float f90, float u)
{
	return f0 + (f90 - f0) * pow(1.0f - u, 5.0f);
}

// Burley Fresnel Diffuse equation
float Fr_DisneyDiffuse(vec3 V, vec3 L, vec3 N, float rough)
{
	vec3 H = normalize(V + L);
	float dotNL = saturate(dot(N, L));
	float dotLH = saturate(dot(L, H));
	float dotNH = saturate(dot(N, H));
	float dotNV = saturate(dot(N, V));

	float energyBias = mix(0.0f, 0.5f, rough);
	float energyFactor = mix(1.0f, 1.0f/1.51f, rough);
	float fd90 = energyBias + 2.0f * dotLH * dotLH * rough;
	vec3 f0 = vec3(1.0f, 1.0f, 1.0f);
	float lightScatter = F_Schlick(f0, fd90, dotNL).r;
	float viewScatter = F_Schlick(f0, fd90, dotNV).r;

	return lightScatter * viewScatter * energyFactor;
}

// do not forget to - F0 = 0.16 * pow(specColor,2.0f)
vec3 LightingFuncGGX_OPT3(vec3 N, vec3 V, vec3 L, float roughness, vec3 F0)
{
	vec3 H = normalize(V+L);

	float dotNL = saturate( dot(N, L) );
	float dotLH = saturate( dot(L, H) );
	float dotNH = saturate( dot(N, H) );


	float alpha = roughness * roughness;
	float alphaSqr = alpha * alpha;
	float denom = dotNH * dotNH * (alphaSqr - 1.0f) + 1.0f;
	denom = max(1e-6f, denom);	// fix div0 error

	float D = alphaSqr/(PI * denom * denom);

	// F
	float F_a, F_b;
	//float dotLH5 = pow( 1.0f - dotLH, 5 ); 					// default
	float dotLH5 = exp2((-5.55473*dotLH - 6.98316)*dotLH);	// Horner expression term for fresnel
	//float dotLH5 = exp2(-8.65617024533378044416*dotLH);			// Schüler resolve for dotLH
	F_a = 1.0f;
	F_b = dotLH5;

	// V
	float vis;
	float k = alpha / 2.0f;
	float k2 = k * k;
	float invK2 = 1.0f - k2;
	vis = rcp( dotLH * dotLH * invK2 + k2);

	vec3 FV = F0 * F_a * vis + ( 1.0f - F0 ) * F_b * vis;
	//vec3 FV = (0.16f * pow(specColor,2.0f)) * F_a * vis + ( 1.0f - F0 ) * F_b * vis; //
	vec3 specular = (dotNL * D * FV);

	return specular;
}

float GlossToRoughness(float gloss)
{
	float roughness = 1.0f - gloss * 0.99f;
	return roughness;
}

LightingData CalcPunctualLight( vec3 normal, vec3 light, vec3 view, float gloss, vec3 specColor, vec3 lightColor, float translucency = 0.0 ) {
	LightingData ld;
	float roughness = GlossToRoughness(gloss); //initial parametrization with invert smoothness to roughness 
	float dotNL = dot(light, normal);
	if(dotNL < 0) dotNL = mix(0.0, -dotNL, translucency);
	else dotNL = saturate(dotNL);

	vec3 lightFactor = PI * dotNL * lightColor;
	ld.diffuse = lightFactor * FresnelDiffuse(specColor);
	//ld.diffuse = lightFactor * Fr_DisneyDiffuse(view, light, normal, roughness ); //Used Burley diffuse term
	ld.specular = lightColor * LightingFuncGGX_OPT3(normal, view, light, roughness, specColor) / PI;
	
   return ld;
}

// function for computing the dominant direction of the specular microfacet GGX-based specular term with lightprobe
// http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
vec3 GetSpecularDominantDir( vec3 N, vec3 R, float gloss )
{
	float roughness = GlossToRoughness( gloss );
	float mixFactor = gloss * ( sqrt( gloss ) + roughness );
	// The result is not normalized as we fetch in a cubemap
	return mix( N, R, mixFactor );
}

// http://blog.selfshadow.com/publications/s2013-shading-course/lazarov/s2013_pbs_black_ops_2_notes.pdf
vec3 LazarovEnvironmentBRDF( float gloss, float NoV, vec3 rf0 )
{
	vec4 t = vec4( 1.0f/0.96f, 0.475f, (0.0275f - 0.25f * 0.04f)/0.96f, 0.25f );
	t *= vec4( gloss, gloss, gloss, gloss );
	t += vec4( 0.0f, 0.0f, (0.015f - 0.75f * 0.04f)/0.96f, 0.75f );
	float a0 = t.x * min( t.y, exp2( -9.28f * NoV ) ) + t.z;
	float a1 = t.w;
	return saturate( a0 + rf0 * ( a1 - a0 ) );
}

//
LightingData CalcEnvironmentLight( samplerCUBE irradianceSampler, samplerCUBE specularitySampler, vec3 normal, vec3 view, float gloss, vec3 specColor ) {
	float NdotV = saturate( dot( view, normal ) );

	vec4 irradianceCube = texCUBElod( irradianceSampler, vec4( normal, 3.0f) );
	vec3 irradiance = irradianceCube.rgb * irradianceCube.a * 4.0f;

	float reflMip = ( 1.0f - gloss ) * 7.0f;
	vec3 specDir = GetSpecularDominantDir( normal, -reflect( view, normal ), gloss );
	//specDir = -reflect( view, normal );

	vec4 reflectCube = texCUBElod( specularitySampler, vec4( specDir, reflMip ) );
	vec3 reflection = reflectCube.rgb * reflectCube.a * 10.0f;

	LightingData ld;
	ld.specular = LazarovEnvironmentBRDF( gloss, NdotV, specColor ) * reflection;
	ld.diffuse = irradiance * FresnelDiffuse( specColor );

	return ld;
}
