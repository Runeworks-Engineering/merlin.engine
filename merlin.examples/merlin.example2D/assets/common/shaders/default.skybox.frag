//This file was automatically generated 
//DO NOT CHANGE !
#version 330 core
out vec4 FragColor;

in vec3 texCoords;

struct Environment{
	vec3 ambient;
	vec3 irradiance;
	vec3 specular;

	bool use_skybox_tex;
	bool use_ambient_tex;
	bool use_specular_tex;
	bool use_specularBRDF_tex;

	samplerCube skybox_tex;
	samplerCube specular_tex;
	samplerCube irradiance_tex;
	sampler2D specularBRDF_tex;
};

uniform Environment environment;

uniform vec3 gradientColor = vec3(1);

vec4 grayBox(){
    float r = length(texCoords);
    return vec4( vec3((acos(-texCoords.y / r)/ 3.0)) * gradientColor,1.0f);
}


void main()
{    
    if(environment.use_skybox_tex) FragColor = texture(environment.skybox_tex, texCoords); //Cubemap texture
    else FragColor = grayBox(); //texcoord as color

}






/*
licence.txt from Unity built-in shader source:

Copyright (c) 2016 Unity Technologies

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
Original code was translated and adapted for ShaderToy by P.Z.
*/
/*
const vec4 _LightColor0 = vec4(0.9,0.9,0.9,1.0);
const  float _Exposure = 1.0;
const vec3 _GroundColor = vec3(.2);
const float _SunSize = 0.04;
const float _SunSizeConvergence = 5.0;
const vec3 _SkyTint = vec3(.5, .5, .5);
const float _AtmosphereThickness = 1.0; 
#define OUTER_RADIUS 1.025 
#define kRAYLEIGH (mix(0.0, 0.0025, pow(_AtmosphereThickness,2.5))) 
#define kMIE 0.0010 
#define kSUN_BRIGHTNESS 20.0 
#define kMAX_SCATTER 50.0 
#define MIE_G (-0.990) 
#define MIE_G2 0.9801 
#define SKY_GROUND_THRESHOLD 0.02 
#define SKYBOX_COLOR_IN_TARGET_COLOR_SPACE 0 
const vec3 ScatteringWavelength = vec3(.65, .57, .475);
const vec3 ScatteringWavelengthRange = vec3(.15, .15, .15);    
const float kOuterRadius = OUTER_RADIUS; 
const float kOuterRadius2 = OUTER_RADIUS*OUTER_RADIUS;
const float kInnerRadius = 1.0;
const float kInnerRadius2 = 1.0;
const float kCameraHeight = 0.0001;
const float kHDSundiskIntensityFactor = 15.0;
const float kSunScale = 400.0 * kSUN_BRIGHTNESS;
const float kKmESun = kMIE * kSUN_BRIGHTNESS;
const float kKm4PI = kMIE * 4.0 * 3.14159265;
const float kScale = 1.0 / (OUTER_RADIUS - 1.0);
const float kScaleDepth = 0.25;
const float kScaleOverScaleDepth = (1.0 / (OUTER_RADIUS - 1.0)) / 0.25;
const float kSamples = 2.0;

float Scale(float inCos)
{
	float x = 1.0 - inCos;
	return 0.25 * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

float SunAttenuation(vec3 lightPos, vec3 ray)
{
	float EyeCos = pow(clamp(dot(lightPos, ray),0.0,1.0), _SunSizeConvergence);		
	float temp = pow(1.0 + MIE_G2 - 2.0 * MIE_G * (-EyeCos), pow(_SunSize,0.65) * 10.);
	return (1.5 * ((1.0 - MIE_G2) / (2.0 + MIE_G2)) * (1.0 + EyeCos * EyeCos) / max(temp,1.0e-4));	
}

vec4 ProceduralSkybox(vec3 ro, vec3 rd)
{
    vec3 _WorldSpaceLightPos0 = vec3(2,sin(1.2*0.5)*10.0+1.0,20);
    vec3 kSkyTintInGammaSpace = _SkyTint;
    vec3 kScatteringWavelength = mix(ScatteringWavelength-ScatteringWavelengthRange,ScatteringWavelength+ScatteringWavelengthRange,vec3(1,1,1) - kSkyTintInGammaSpace);
    vec3 kInvWavelength = 1.0 / (pow(kScatteringWavelength, vec3(4.0)));
    float kKrESun = kRAYLEIGH * kSUN_BRIGHTNESS;
    float kKr4PI = kRAYLEIGH * 4.0 * 3.14159265;
    vec3 cameraPos = vec3(0,kInnerRadius + kCameraHeight,0);
    vec3 eyeRay = rd;
    float far = 0.0;
    vec3 cIn, cOut;
    if(eyeRay.y >= 0.0)
    {
        far = sqrt(kOuterRadius2 + kInnerRadius2 * eyeRay.y * eyeRay.y - kInnerRadius2) - kInnerRadius * eyeRay.y;
        vec3 pos = cameraPos + far * eyeRay;
        float height = kInnerRadius + kCameraHeight;
        float depth = exp(kScaleOverScaleDepth * (-kCameraHeight));
        float startAngle = dot(eyeRay, cameraPos) / height;
        float startOffset = depth*Scale(startAngle);
        float sampleLength = far / kSamples;
        float scaledLength = sampleLength * kScale;
        vec3 sampleRay = eyeRay * sampleLength;
        vec3 samplePoint = cameraPos + sampleRay * 0.5;
        vec3 frontColor = vec3(0.0, 0.0, 0.0);
        for (int i=0; i<2; i++)
        {
            float height = length(samplePoint);
            float depth = exp(kScaleOverScaleDepth * (kInnerRadius - height));
            float lightAngle = dot(normalize(_WorldSpaceLightPos0.xyz), samplePoint) / height;
            float cameraAngle = dot(eyeRay, samplePoint) / height;
            float scatter = (startOffset + depth*(Scale(lightAngle) - Scale(cameraAngle)));
            vec3 attenuate = exp(-clamp(scatter, 0.0, kMAX_SCATTER) * (kInvWavelength * kKr4PI + kKm4PI));
            frontColor += attenuate * (depth * scaledLength);
            samplePoint += sampleRay;
        }
        cIn = frontColor * (kInvWavelength * kKrESun);
        cOut = frontColor * kKmESun;
    }
    else
    {
        far = (-kCameraHeight) / (min(-0.001, eyeRay.y));
        vec3 pos = cameraPos + far * eyeRay;
        float cameraScale = Scale(dot(-eyeRay, pos));
        float lightScale = Scale(dot(normalize(_WorldSpaceLightPos0.xyz), pos));
        float sampleLength = far / kSamples;
        float scaledLength = sampleLength * kScale;
        vec3 sampleRay = eyeRay * sampleLength;
        vec3 samplePoint = cameraPos + sampleRay * 0.5;
        vec3 frontColor = vec3(0.0, 0.0, 0.0);            
        float height = length(samplePoint);
        float d = exp(kScaleOverScaleDepth * (kInnerRadius - height));
        float scatter = d*(lightScale + cameraScale) - exp((-kCameraHeight) * (1.0/kScaleDepth))*cameraScale;
        vec3 attenuate = exp(-clamp(scatter, 0.0, kMAX_SCATTER) * (kInvWavelength * kKr4PI + kKm4PI));
        frontColor += attenuate * (d * scaledLength);
        samplePoint += sampleRay;
        cIn = frontColor * (kInvWavelength * kKrESun + kKmESun);
        cOut = clamp(attenuate, 0.0, 1.0);
    }
    vec3 groundColor = _Exposure * (cIn + _GroundColor*_GroundColor * cOut);
    vec3 skyColor = _Exposure * (cIn * (0.75 + 0.75 * dot(normalize(_WorldSpaceLightPos0.xyz), -eyeRay) * dot(normalize(_WorldSpaceLightPos0.xyz), -eyeRay))); 
    float lightColorIntensity = clamp(length(_LightColor0.xyz), 0.25, 1.0);
    vec3 sunColor = kHDSundiskIntensityFactor * clamp(cOut,0.0,1.0) * _LightColor0.xyz / lightColorIntensity;	    
    vec3 ray = -rd;
    float y = ray.y / SKY_GROUND_THRESHOLD;
    vec3 color = mix(skyColor, groundColor, clamp(y,0.0,1.0));
    if(y < 0.0) color += sunColor * SunAttenuation(normalize(_WorldSpaceLightPos0.xyz), -ray);
    return vec4(sqrt(color),1.0);      
}

//////////////////////////////////////////////////////////////////////////////////////////////

float sphere( vec3 p, vec3 c,float s )
{
	return length(p-c)-s;
}

float box (vec3 p, vec3 c, vec3 s)
{
	float x = max(p.x - c.x - s.x, c.x - p.x - s.x);
	float y = max(p.y - c.y - s.y, c.y - p.y - s.y);   
	float z = max(p.z - c.z - s.z, c.z - p.z - s.z);
	return max(max(x,y),z);
}

float map (vec3 p)
{
	float a = box(p,vec3(0.0),vec3(1000));
	float b = sphere(p,vec3(0.0),300.0);
	return max(-b,a);
}

vec4 raymarch (vec3 ro, vec3 rd)
{
	for (int i=0;i<16;i++)
	{
		float t = map(ro);
		if (t<0.001) return ProceduralSkybox(ro,rd);     
		ro+=t*rd;
	}
	return vec4(0,0,0,1);
}

vec4 sun(){
    vec3 sunVector = vec3(1,-1,-1);
    gl_FragDepth   = gl_DepthRange.far;
    float gradient = dot(normalize(sunVector), normalize(texCoords)) / 2.0 + 0.5;
    return vec4(pow(gradient, 32), pow(gradient, 48) / 2.0 + 0.5, gradient / 4.0 + 0.75, 1.0);
}


//////////////////////////////////////////////////////////////////////////////////////////////
*/
/*
void main(){
    if(isSkyboxBound) FragColor = texture(skybox, texCoords); //Cubemap texture
    else{
    	vec2 uv = texCoords.xy;
	    vec3 ro = vec3 (0.,0.,0.);
	    vec3 rd = normalize(vec3(uv.x,uv.y, 2.0)).xzy;
	    FragColor = raymarch(ro,rd);
    }
}
*/