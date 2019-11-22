#include "../../Include/Common.h"

using namespace glm;
using namespace std;

#define NUM_LIGHTS 64
#define NUM_INSTANCES 225 // 225 = 15 * 15

struct
{
	GLuint vao;
	GLuint position_buffer;
	GLuint normal_buffer;
	GLuint texcoord_buffer;
	GLuint index_buffer;
	int index_count;
	mat4 model_matrix;
	GLuint tex_diffuse;
} shape;

struct
{
	GLuint fbo;
	GLuint position_map;
	GLuint normal_map;
	GLuint diffuse_map;
	GLuint depth_map;
	GLuint vao;
} gbuffer;

struct
{
	struct
	{
		GLint transforms;
		GLint tex_diffuse;
	} geometry;
	struct
	{
		GLint position_map;
		GLint normal_map;
		GLint diffuse_map;
		GLint num_lights;
		GLint light_positions;
		GLint light_colors;
	} deferred;
} uniforms;

GLuint render_program_nm;
GLuint render_transform_ubo;

GLuint light_program;
GLuint light_position_ubo;
GLuint light_color_ubo;

float viewportAspect = 1.0f;

const char *geometry_vs[] = 
{
	"#version 410 core                                                          \n"
	"                                                                           \n"
	"layout (location = 0) in vec3 position;                                    \n"
	"layout (location = 1) in vec3 normal;                                      \n"
	"layout (location = 2) in vec2 texcoord;	                                \n"
	"	                                                                        \n"
	"out VS_OUT	                                                                \n"
	"{	                                                                        \n"
	"    vec3 position;                                           	            \n"
	"    vec3 normal;	                                                        \n"
	"    vec2 texcoord;	                                                        \n"
	"} vs_out;	                                                                \n"
	"	                                                                        \n"
	"layout (std140) uniform transforms	                                        \n"
	"{	                                                                        \n"
	"    mat4 projection_matrix;	                                            \n"
	"    mat4 view_matrix;	                                                    \n"
	"    mat4 model_matrix[256];	                                            \n"
	"};	                                                                        \n"
	"	                                                                        \n"
	"void main(void)	                                                        \n"
	"{	                                                                        \n"
	"    vec4 P = vec4(position, 1.0);                                          \n"
	"    mat4 mv_matrix = view_matrix * model_matrix[gl_InstanceID];	        \n"
	"    gl_Position = projection_matrix * mv_matrix * P;	                    \n"
	"    vs_out.position = (model_matrix[gl_InstanceID] * P).xyz;	            \n"
	"    vs_out.normal = normalize(mat3(model_matrix[gl_InstanceID]) * normal);	\n"
	"    vs_out.texcoord = texcoord;	                                        \n"
	"}	                                                                        \n"
};

const char *geometry_fs[] = 
{
	"#version 410 core                                         \n"
	"                                                          \n"
	"layout (location = 0) out vec4 frag_position;             \n"
	"layout (location = 1) out vec4 frag_normal;               \n"
	"layout (location = 2) out vec4 frag_diffuse;              \n"
	"                                                          \n"
	"in VS_OUT                                                 \n"
	"{                                                         \n"
	"    vec3 position;                                        \n"
	"    vec3 normal;                                          \n"
	"    vec2 texcoord;                                        \n"
	"} fs_in;                                                  \n"
	"                                                          \n"
	"uniform sampler2D tex_diffuse;                            \n"
	"                                                          \n"
	"void main(void)                                           \n"
	"{                                                         \n"
	"    frag_position = vec4(fs_in.position, 1.0);            \n"
	"    frag_normal = vec4(normalize(fs_in.normal), 0.0);     \n"
	"    frag_diffuse = texture(tex_diffuse, fs_in.texcoord);  \n"
	"}                                                         \n"
};

const char* deferred_vs[] =
{
	"#version 410 core                        \n"
	"                                         \n"
	"void main()                              \n"
	"{                                        \n"
	"    const vec4 verts[4] = vec4[4](       \n"
	"        vec4(-1.0, -1.0, 0.5, 1.0),      \n"
	"        vec4( 1.0, -1.0, 0.5, 1.0),      \n"
	"        vec4(-1.0,  1.0, 0.5, 1.0),      \n"
	"        vec4( 1.0,  1.0, 0.5, 1.0));     \n"
	"                                         \n"
	"    gl_Position = verts[gl_VertexID];    \n"
	"}                                        \n"
};

const char* deferred_fs[] =
{
	"#version 410 core                                                                     \n"
	"                                                                                      \n"
	"layout (location = 0) out vec4 frag_color;                                            \n"
	"                                                                                      \n"
	"uniform sampler2D position_map;                                                       \n"
	"uniform sampler2D normal_map;                                                         \n"
	"uniform sampler2D diffuse_map;                                                        \n"
	"                                                                                      \n"
	"uniform int num_lights;                                                               \n"
	"                                                                                      \n"
	"layout (std140) uniform light_position_block                                          \n"
	"{                                                                                     \n"
	"    vec4 light_positions[64];                                                         \n"
	"};                                                                                    \n"
	"                                                                                      \n"
	"layout (std140) uniform light_color_block                                             \n"
	"{                                                                                     \n"
	"    vec4 light_colors[64];                                                            \n"
	"};                                                                                    \n"
	"                                                                                      \n"
	"void main(void)                                                                       \n"
	"{                                                                                     \n"
	"    vec4 result = vec4(0, 0, 0, 1);                                                   \n"
	"    vec3 position = texelFetch(position_map, ivec2(gl_FragCoord.xy), 0).rgb;          \n"
	"    vec3 normal = texelFetch(normal_map, ivec2(gl_FragCoord.xy), 0).rgb;              \n"
	"    vec3 diffuse = texelFetch(diffuse_map, ivec2(gl_FragCoord.xy), 0).rgb;            \n"
	"    for (int i = 0; i < num_lights; ++i)                                              \n"
	"    {                                                                                 \n"
	"        vec3 L = light_positions[i].xyz - position;                                   \n"
	"        float dist = length(L);                                                       \n"
	"        L = normalize(L);                                                             \n"
	"        vec3 R = reflect(-L, normal);                                                 \n"
	"        float NdotL = max(0.0, dot(normal, L));                                       \n"
	"        float NdotR = max(0.0, dot(normal, R));                                       \n"
	"        float attenuation = 50.0 / (pow(dist, 2.0) + 1.0);                            \n"
	"        vec3 diffuse_color  = light_colors[i].rgb * diffuse * NdotL;                  \n"
	"        vec3 specular_color = light_colors[i].rgb * pow(NdotR, 60.0);                 \n"
	"        result += vec4((diffuse_color + specular_color) * attenuation, 0.0);          \n"
	"    }                                                                                 \n"
	"    frag_color = result;                                                              \n"
	"}                                                                                     \n"
};

void My_Init()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// ----- Begin Initialize Geometry Buffer -----
	glGenFramebuffers(1, &gbuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);

	glGenTextures(4, &gbuffer.position_map);

	glBindTexture(GL_TEXTURE_2D, gbuffer.position_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, gbuffer.diffuse_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, 256, 256, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer.position_map, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gbuffer.normal_map, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gbuffer.diffuse_map, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gbuffer.depth_map, 0);

	glGenVertexArrays(1, &gbuffer.vao);
	glBindVertexArray(gbuffer.vao);
	// ----- End Initialize Geometry Buffer -----
	
	// ----- Begin Initialize Input Mesh -----
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;

	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/ladybug.obj");
	
	glGenVertexArrays(1, &shape.vao);
	glBindVertexArray(shape.vao);

	glGenBuffers(1, &shape.position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, shape.position_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.positions.size() * sizeof(float), shapes[0].mesh.positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &shape.normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, shape.normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.normals.size() * sizeof(float), shapes[0].mesh.normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &shape.texcoord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, shape.texcoord_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.texcoords.size() * sizeof(float), shapes[0].mesh.texcoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glGenBuffers(1, &shape.index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[0].mesh.indices.size() * sizeof(unsigned int), shapes[0].mesh.indices.data(), GL_STATIC_DRAW);
	shape.index_count = shapes[0].mesh.indices.size();

	TextureData diffuse_data= loadPNG("../../Media/Textures/ladybug_co.png");
	glGenTextures(1, &shape.tex_diffuse);
	glBindTexture(GL_TEXTURE_2D, shape.tex_diffuse);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, diffuse_data.width, diffuse_data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, diffuse_data.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	delete[] diffuse_data.data;
	// ----- End Initialize Input Mesh -----

	glGenBuffers(1, &light_position_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, light_position_ubo);
	glBufferData(GL_UNIFORM_BUFFER, NUM_LIGHTS * sizeof(vec4), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &light_color_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, light_color_ubo);
	glBufferData(GL_UNIFORM_BUFFER, NUM_LIGHTS * sizeof(vec4), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &render_transform_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, render_transform_ubo);
	glBufferData(GL_UNIFORM_BUFFER, (2 + NUM_INSTANCES) * sizeof(mat4), NULL, GL_DYNAMIC_DRAW);
	
	// ----- Begin Initialize Geometry Program -----
	GLuint vs;
	GLuint fs;
	vs = glCreateShader(GL_VERTEX_SHADER);
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vs, 1, geometry_vs, 0);
	glShaderSource(fs, 1, geometry_fs, 0);
	glCompileShader(vs);
	glCompileShader(fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);
	render_program_nm = glCreateProgram();
	glAttachShader(render_program_nm, vs);
	glAttachShader(render_program_nm, fs);
	glLinkProgram(render_program_nm);
	uniforms.geometry.transforms = glGetUniformBlockIndex(render_program_nm, "transforms");
	uniforms.geometry.tex_diffuse = glGetUniformLocation(render_program_nm, "tex_diffuse");
	// ----- End Initialize Geometry Program -----
	
	// ----- Begin Initialize Deferred Program -----
	GLuint vs2;
	GLuint fs2;
	vs2 = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs2, 1, deferred_vs, 0);
	glCompileShader(vs2);
	printGLShaderLog(vs2);
	fs2 = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs2, 1, deferred_fs, 0);
	glCompileShader(fs2);
	printGLShaderLog(fs2);
	light_program = glCreateProgram();
	glAttachShader(light_program, vs2);
	glAttachShader(light_program, fs2);
	glLinkProgram(light_program);
	uniforms.deferred.diffuse_map = glGetUniformLocation(light_program, "diffuse_map");
	uniforms.deferred.normal_map = glGetUniformLocation(light_program, "normal_map");
	uniforms.deferred.position_map = glGetUniformLocation(light_program, "position_map");
	uniforms.deferred.num_lights = glGetUniformLocation(light_program, "num_lights");
	uniforms.deferred.light_positions = glGetUniformBlockIndex(light_program, "light_position_block");
	glUniformBlockBinding(light_program, uniforms.deferred.light_positions, 0);
	uniforms.deferred.light_colors = glGetUniformBlockIndex(light_program, "light_color_block");
	glUniformBlockBinding(light_program, uniforms.deferred.light_colors, 1);
	// ----- End Initialize Deferred Program -----
}

void My_Display()
{
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

	// ----- End Geometry Pass -----
	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);
	const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, draw_buffers);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(render_program_nm);

	glBindBufferBase(GL_UNIFORM_BUFFER, uniforms.geometry.transforms, render_transform_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, render_transform_ubo);
	mat4 * matrices = reinterpret_cast<mat4 *>(glMapBufferRange(GL_UNIFORM_BUFFER,
		0,
		(2 + NUM_INSTANCES) * sizeof(mat4),
		GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
	matrices[0] = perspective(radians(50.0f), viewportAspect, 0.1f, 1000.0f);
	float d = (sinf(time * 0.131f) + 2.0f) * 0.15f;
	vec3 eye_pos = vec3(d * 120.0f * sinf(time * 0.11f), 5.5f, d * 120.0f * cosf(time * 0.01f));
	matrices[1] = lookAt(eye_pos, vec3(0.0f, -20.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	for (int j = 0; j < 15; ++j)
	{
		for (int i = 0; i < 15; ++i)
		{
			matrices[j * 15 + i + 2] = translate(mat4(),vec3((i - 7.5f) * 7.0f, 0.0f, (j - 7.5f) * 11.0f));
		}
	}
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shape.tex_diffuse);
	glUniform1i(uniforms.geometry.tex_diffuse, 0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glBindVertexArray(shape.vao);
	glDrawArraysInstanced(GL_TRIANGLES, 0, shape.index_count, NUM_INSTANCES);
	// ----- End Geometry Pass -----

	// ----- Begin Deferred Pass -----
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glUseProgram(light_program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbuffer.position_map);
	glUniform1i(uniforms.deferred.position_map, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	glUniform1i(uniforms.deferred.normal_map, 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gbuffer.diffuse_map);
	glUniform1i(uniforms.deferred.diffuse_map, 2);

	glUniform1i(uniforms.deferred.num_lights, NUM_LIGHTS);

	glDisable(GL_DEPTH_TEST);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, light_position_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, light_position_ubo);
	vec4 *light_positions = reinterpret_cast<vec4 *>(glMapBufferRange(GL_UNIFORM_BUFFER,
		0,
		NUM_LIGHTS * sizeof(vec4),
		GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		float i_f = ((float)i - 7.5f) * 0.1f + 0.3f;
		light_positions[i] = vec4(100.0f * sinf(time * 1.1f + (5.0f * i_f)) * cosf(time * 2.3f + (9.0f * i_f)),
			15.0f,
			100.0f * sinf(time * 1.5f + (6.0f * i_f)) * cosf(time * 1.9f + (11.0f * i_f)), 0);
	}
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, light_color_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, light_color_ubo);
	vec4 *light_colors = reinterpret_cast<vec4 *>(glMapBufferRange(GL_UNIFORM_BUFFER,
		0,
		NUM_LIGHTS * sizeof(vec4),
		GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		float i_f = ((float)i - 7.5f) * 0.1f + 0.3f;
		light_colors[i] = vec4(cosf(i_f * 14.0f) * 0.5f + 0.8f,
			sinf(i_f * 17.0f) * 0.5f + 0.8f,
			sinf(i_f * 13.0f) * cosf(i_f * 19.0f) * 0.5f + 0.8f, 0);
	}
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	glBindVertexArray(gbuffer.vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	// ----- End Deferred Pass -----

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	viewportAspect = (float)width / (float)height;

	glBindTexture(GL_TEXTURE_2D, gbuffer.position_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL); 
	glBindTexture(GL_TEXTURE_2D, gbuffer.diffuse_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); 
	glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); 
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
}

int main(int argc, char *argv[])
{
	// Change working directory to source code path
	chdir(__FILEPATH__);
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 600);
	glutCreateWindow(__FILENAME__); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	printGLContextInfo();
	My_Init();
	////////////////////

	// Register GLUT callback functions.
	///////////////////////////////
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutTimerFunc(16, My_Timer, 0);
	///////////////////////////////

	// Enter main event loop.
	//////////////
	glutMainLoop();
	//////////////
	return 0;
}