#include "../../Include/Common.h"

#include <cstdlib>
#include <ctime>

using namespace glm;
using namespace std;

GLuint render_prog;
GLuint ssao_prog;
GLuint ssao_vao;
GLuint dragon_vao;
GLuint plane_vao;
size_t index_count;
mat4 proj_matrix;
GLuint kernal_ubo;
GLuint noise_map;

struct
{
    struct
    {
        GLint mv_matrix;
        GLint proj_matrix;
    } render;
	struct
	{
		GLint normal_map;
		GLint depth_map;
		GLint noise_map;
		GLint noise_scale;
		GLint proj;
	} ssao;
} uniforms;

struct
{
	GLuint fbo;
	GLuint normal_map;
	GLuint depth_map;
} gbuffer;

struct
{
	int width;
	int height;
} viewport_size;

const char *depth_normal_fs[] = 
{
    "#version 410 core                                                 \n"
    "                                                                  \n"
    "in VS_OUT                                                         \n"
    "{                                                                 \n"
    "    vec3 normal;                                                  \n"
    "} fs_in;                                                          \n"
    "                                                                  \n"
	"out vec4 frag_normal;                                             \n"
    "                                                                  \n"
    "void main(void)                                                   \n"
    "{                                                                 \n"
	"    frag_normal = vec4(normalize(fs_in.normal), 0.0);             \n"
    "}                                                                 \n"
};

const char *depth_normal_vs[] = 
{
    "#version 410 core                                 \n"
    "                                                  \n"
    "uniform mat4 mv_matrix;                           \n"
    "uniform mat4 proj_matrix;                         \n"
    "                                                  \n"
    "layout (location = 0) in vec3 position;           \n"
    "layout (location = 1) in vec3 normal;             \n"
	"                                                  \n"
	"out VS_OUT                                        \n"
    "{                                                 \n"
    "    vec3 normal;                                  \n"
    "} vs_out;                                         \n"
    "                                                  \n"
    "void main(void)                                   \n"
    "{                                                 \n"
	"    vs_out.normal = mat3(mv_matrix) * normal;     \n"
	"    vec4 pos_vs = mv_matrix * vec4(position, 1.0);\n"
    "    gl_Position = proj_matrix * pos_vs;           \n"
    "}                                                 \n"
};

const char* ssao_vs[] = 
{
	"#version 410 core                                            \n"
	"                                                             \n"
	"out VS_OUT                                                   \n"
	"{                                                            \n"
	"    vec2 texcoord;                                           \n"
	"} vs_out;                                                    \n"
	"                                                             \n"
	"void main()                                                  \n"
	"{                                                            \n"
	"    vec4 positions[4] = vec4[](                              \n"
	"         vec4(-1.0, -1.0, 0.0, 1.0),                         \n"
	"         vec4( 1.0, -1.0, 0.0, 1.0),                         \n"
	"         vec4(-1.0,  1.0, 0.0, 1.0),                         \n"
	"         vec4( 1.0,  1.0, 0.0, 1.0));                        \n"
	"    gl_Position = positions[gl_VertexID];                    \n"
	"    vs_out.texcoord = positions[gl_VertexID].xy * 0.5 + 0.5; \n"
	"}                                                            \n"
};

const char* ssao_fs[] =
{
	"#version 410 core                                                                               \n"
	"                                                                                                \n"
	"uniform sampler2D normal_map;                                                                   \n"
	"uniform sampler2D depth_map;                                                                    \n"
	"uniform sampler2D noise_map;                                                                    \n"
	"uniform vec2 noise_scale;                                                                       \n"
	"uniform mat4 proj;                                                                              \n"
	"                                                                                                \n"
	"in VS_OUT                                                                                       \n"
	"{                                                                                               \n"
	"    vec2 texcoord;                                                                              \n"
	"} fs_in;                                                                                        \n"
	"                                                                                                \n"
	"layout(std140) uniform Kernals                                                                  \n"
	"{                                                                                               \n"
	"    vec4 kernals[32];                                                                           \n"
	"};                                                                                              \n"
	"                                                                                                \n"
	"out vec4 fragAO;                                                                                \n"
	"                                                                                                \n"
	"void main()                                                                                     \n"
	"{                                                                                               \n"
	"    float depth = texture(depth_map, fs_in.texcoord).r;                                         \n"
	"    if(depth == 1.0) { discard; }                                                               \n"
	"    mat4 invproj = inverse(proj);                                                               \n"
	"    vec4 position = invproj * vec4(vec3(fs_in.texcoord, depth) * 2.0 - 1.0, 1.0);               \n"
	"    position /= position.w;                                                                     \n"
	"    vec3 N = texture(normal_map, fs_in.texcoord).xyz;                                           \n"
	"    vec3 randvec = normalize(texture(noise_map, fs_in.texcoord * noise_scale).xyz * 2.0 - 1.0); \n"
	"    vec3 T = normalize(randvec - N * dot(randvec, N));                                          \n"
	"    vec3 B = cross(N, T);                                                                       \n"
	"    mat3 tbn = mat3(T, B, N); // tangent to eye matrix                                          \n"
	"    const float radius = 2.0;                                                                   \n"
	"    float ao = 0.0;                                                                             \n"
	"    for(int i = 0; i < 32; ++i)                                                                 \n"
	"    {                                                                                           \n"
	"        vec4 sampleEye = position + vec4(tbn * kernals[i].xyz * radius, 0.0);                   \n"
	"        vec4 sampleP = proj * sampleEye;                                                        \n"
	"        sampleP /= sampleP.w;                                                                   \n"
	"        sampleP = sampleP * 0.5 + 0.5;                                                          \n"
	"        float sampleDepth = texture(depth_map, sampleP.xy).r;                                   \n"
	"        vec4 invP = invproj * vec4(vec3(sampleP.xy, sampleDepth) * 2.0 - 1.0, 1.0);             \n"
	"        invP /= invP.w;                                                                         \n"
	"        if(sampleDepth > sampleP.z || length(invP - position) > radius)                         \n"
	"        {                                                                                       \n"
	"            ao += 1.0;                                                                          \n"
	"        }                                                                                       \n"
	"    }                                                                                           \n"
	"    fragAO = vec4(vec3(ao / 32.0), 1.0);                                                        \n"
	"}                                                                                               \n"
};

void My_Init()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

	// ----- Begin Initialize Depth-Normal Only Program -----
	GLuint fs;
	GLuint vs;
	render_prog = glCreateProgram();
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, depth_normal_fs, NULL);
	glCompileShader(fs);
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, depth_normal_vs, NULL);
	glCompileShader(vs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);
	glAttachShader(render_prog, vs);
	glAttachShader(render_prog, fs);

	glLinkProgram(render_prog);
	glUseProgram(render_prog);
    uniforms.render.mv_matrix = glGetUniformLocation(render_prog, "mv_matrix");
    uniforms.render.proj_matrix = glGetUniformLocation(render_prog, "proj_matrix");
	// ----- End Initialize Depth-Normal Only Program -----

	// ----- Begin Initialize SSAO Program -----
	ssao_prog = glCreateProgram();
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, ssao_fs, NULL);
	glCompileShader(fs);
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, ssao_vs, NULL);
	glCompileShader(vs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);
	glAttachShader(ssao_prog, vs);
	glAttachShader(ssao_prog, fs);
	glLinkProgram(ssao_prog);
	glUseProgram(ssao_prog);
	uniforms.ssao.normal_map = glGetUniformLocation(ssao_prog, "normal_map");
	uniforms.ssao.depth_map = glGetUniformLocation(ssao_prog, "depth_map");
	uniforms.ssao.proj = glGetUniformLocation(ssao_prog, "proj");
	uniforms.ssao.noise_map = glGetUniformLocation(ssao_prog, "noise_map");
	uniforms.ssao.noise_scale = glGetUniformLocation(ssao_prog, "noise_scale");
	GLint blockIdx = glGetUniformBlockIndex(ssao_prog, "Kernals");
	glUniformBlockBinding(ssao_prog, blockIdx, 0);
	glGenVertexArrays(1, &ssao_vao);
	glBindVertexArray(ssao_vao);
	// ----- End Initialize SSAO Program -----

	// ----- Begin Initialize Kernal UBO
	glGenBuffers(1, &kernal_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, kernal_ubo);
	vec4 kernals[32];
	srand(time(NULL));
	for(int i = 0; i < 32; ++i)
	{
		float scale = i / 32.0;
		scale = 0.1f + 0.9f * scale * scale;
		kernals[i] = vec4(normalize(vec3(
			rand() / (float)RAND_MAX * 2.0f - 1.0f,
			rand() / (float)RAND_MAX * 2.0f - 1.0f,
			rand() / (float)RAND_MAX * 0.85f + 0.15f)) * scale,
			0.0f
		);
	}
	glBufferData(GL_UNIFORM_BUFFER, 32 * sizeof(vec4), &kernals[0][0], GL_STATIC_DRAW);
	// ----- End Initialize Kernal UBO

	// ----- Begin Initialize Random Noise Map -----
	glGenTextures(1, &noise_map);
	glBindTexture(GL_TEXTURE_2D, noise_map);
	vec3 noiseData[16];
	for (int i = 0; i < 16; ++i)
	{
		noiseData[i] = normalize(vec3(
			rand() / (float)RAND_MAX, // 0.0 ~ 1.0
			rand() / (float)RAND_MAX, // 0.0 ~ 1.0
			0.0f
		));
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 4, 4, 0, GL_RGB, GL_FLOAT, &noiseData[0][0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// ----- End Initialize Random Noise Map -----

	// ----- Begin Initialize Input Mesh -----
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/dragon.obj");

	glGenVertexArrays(1, &dragon_vao);
	glBindVertexArray(dragon_vao);

	GLuint position_buffer;
	GLuint normal_buffer;
	GLuint index_buffer;

	glGenBuffers(1, &position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.positions.size() * sizeof(float), shapes[0].mesh.positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.normals.size() * sizeof(float), shapes[0].mesh.normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[0].mesh.indices.size() * sizeof(unsigned int), shapes[0].mesh.indices.data(), GL_STATIC_DRAW);
	index_count = shapes[0].mesh.indices.size();
	// ----- End Initialize Input Mesh -----

	// ----- Begin Initialize Plane Mesh -----
	glGenVertexArrays(1, &plane_vao);
	glBindVertexArray(plane_vao);

	float position_data[] = { -100, 0, -100, 100, 0, -100, -100, 0, 100, 100, 0, 100 };
	float normal_data[] = { 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0 };

	glGenBuffers(1, &position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), position_data, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), normal_data, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	// ----- End Initialize Plane Mesh -----

	// ----- Begin Initialize G Buffer -----
	glGenFramebuffers(1, &gbuffer.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gbuffer.fbo);

	glGenTextures(2, &gbuffer.normal_map);
	glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, 256, 256, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gbuffer.normal_map, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbuffer.depth_map, 0);
	// ----- End Initialize G Buffer -----
}

void My_Display()
{
	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const GLfloat ones[] = { 1.0f };
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.0005f;

	// ----- Begin Depth-Normal Pass -----
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gbuffer.fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClearBufferfv(GL_COLOR, 0, black);
	glClearBufferfv(GL_DEPTH, 0, ones);
	glUseProgram(render_prog);

	mat4 view_matrix = lookAt(vec3(25.0f * sinf(currentTime), 5.0f, 25.0f * cosf(currentTime)), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 mv_matrix = view_matrix * translate(mat4(), vec3(0.0f, -4.0f, 0.0f));
	glUniformMatrix4fv(uniforms.render.mv_matrix, 1, GL_FALSE, &mv_matrix[0][0]);
	glUniformMatrix4fv(uniforms.render.proj_matrix, 1, GL_FALSE, &proj_matrix[0][0]);

	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(dragon_vao);
	glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
	glBindVertexArray(plane_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	// ----- End Depth-Normal Pass -----

	// ----- Begin SSAO Pass -----
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glClearBufferfv(GL_COLOR, 0, white);
	glUseProgram(ssao_prog);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	glUniform1i(uniforms.ssao.normal_map, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gbuffer.depth_map);
	glUniform1i(uniforms.ssao.depth_map, 1);
	glUniformMatrix4fv(uniforms.ssao.proj, 1, GL_FALSE, &proj_matrix[0][0]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noise_map);
	glUniform1i(uniforms.ssao.noise_map, 2);
	glUniform2f(uniforms.ssao.noise_scale, viewport_size.width / 4.0f, viewport_size.height / 4.0f);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, kernal_ubo);

	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(ssao_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	// ----- End SSAO Pass -----

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	viewport_size.width = width;
	viewport_size.height = height;
	proj_matrix = perspective(deg2rad(60.0f), viewportAspect, 0.1f, 1000.0f);

	glBindTexture(GL_TEXTURE_2D, gbuffer.normal_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
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