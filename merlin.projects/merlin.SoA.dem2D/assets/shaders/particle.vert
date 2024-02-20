#version 430
#include "common/uniforms.comp"
#include "common/constants.comp"
#include "common/buffers.comp"
#include "common/nns.comp"
#include "common/colors.comp"

layout(location = 0) in vec3 _position;
layout(location = 1) in vec3 _normal;

out vec4 position;
out vec4 screen_position;
out vec4 normal;
out vec4 color;
out vec2 texCoord;
out mat4 mv;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

uniform int colorCycle = 0;
uniform uint particleTest = 50;
uniform int showBoundary = 0;
uniform vec2 WindowSize;
uniform float zoomLevel = 20;

void main() {
	uint i = gl_InstanceID;
	vec2 offset = B_x(i);
	position = model * (vec4(_position + vec3(offset,0),1));

	uint testsortedID = B_SortedID(particleTest);
	
	bool binTest = true;
	bool nnTest = false;
	bool hTest = false;
	uint binindex = B_BinIndex(gl_InstanceID);//getBinIndex(particles[sortedID].position);

	bool test = B_Phase(gl_InstanceID) == UNUSED || (B_Phase(gl_InstanceID) == BOUNDARY && showBoundary == 0);
	color = vec4(1);
	if(colorCycle == 0){ 
		color = vec4(vec3(0.8), 1.0);
	}else if(colorCycle == 1){ 
		color = vec4(randomColor(binindex), 1);
		//color = vec4(randomColor(particles[gl_InstanceID].meta.z), 1);
	}else if(colorCycle == 2){ 
		color = vec4(randomColor(binindex), 1);
	}else if(colorCycle == 3){ 
		color = colorMap(map(Ti,ambientTemperature, nozzleTemperature), inferno);
	}else if(colorCycle == 4){ 
		color = colorMap(map(length(B_V(gl_InstanceID)),0,1000.0), parula);
	}else if(colorCycle == 5){ 
		color = vec4(randomColor(binindex), 1);
	}else{ //NNS Test
		if(gl_InstanceID == testsortedID){
			color = vec4(1,0,0, 1);
			binTest = true;
		}else{

			binTest = false;
			uvec2 binIndexVec2 = getBinCoord(B_X(testsortedID));
			for (int y = int(binIndexVec2.y) - 1; y <= int(binIndexVec2.y) + 1; y++) {
				for (int x = int(binIndexVec2.x) - 1; x <= int(binIndexVec2.x) + 1; x++) {
					if (x < 0 || y < 0) continue;
					if (x >= binMax.x || y >= binMax.y) continue; 
					if (getBinIndexFromCoord(uvec2(x,y)) == binindex) binTest = true;
				}
			}
		

			vec2 position = B_X(testsortedID);
			OVERNNS
				if(gl_InstanceID == j){
					nnTest = true;
					if(length(B_X(testsortedID) - B_X(j)) <= smoothingRadius) hTest = true;
				}
			OVERNNS_END

			color = vec4(0,0,0, 1);
			if(nnTest) color = vec4(0,0,1, 1);
			if(hTest) color = vec4(0,1,0, 1);
		}
		
	}

	if( test || !binTest){
		screen_position = projection * view * vec4(0,0,0,1);
		gl_Position = screen_position;
		gl_PointSize = 0;
	}else{
		screen_position = projection * view * position;
		mv = projection * view;
		
		gl_Position = screen_position;
		gl_PointSize = particleRadius * WindowSize.y * 1.0 / zoomLevel;
		if(colorCycle == 6 && !hTest && !(gl_InstanceID == testsortedID)) gl_PointSize = particleRadius * WindowSize.y * 0.3 / zoomLevel;
		
	}
}