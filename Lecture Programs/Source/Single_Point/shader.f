R"(

#version 410 core                
                                 
uniform sampler2D s_texture;     
out vec4 frag_color;             
in vec2 v_texcoord;              
                                 
void main(void)                  
{                                
    frag_color = texture(s_texture, v_texcoord); 
}    

)"