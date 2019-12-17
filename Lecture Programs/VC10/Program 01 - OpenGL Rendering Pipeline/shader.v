R"(

#version 410 core                                 
	                                                  
uniform mat4 mvp_matrix;                          
layout (location = 0) in vec3 position;           
layout (location = 1) in vec2 texcoord;           
out vec2 v_texcoord;                              
	                                                  
void main(void)                                   
{                                                 
	v_texcoord = texcoord;                        
	gl_Position = mvp_matrix*vec4(position, 1.0); 
}

)"