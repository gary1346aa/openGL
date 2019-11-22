#include "../../Include/Common.h"

using namespace glm;

static inline float random_float()
{
	float res;
	unsigned int tmp;

	static unsigned int seed = 0x13371337;
	seed *= 16807;

	tmp = seed ^ (seed >> 4) ^ (seed << 15);

	*((unsigned int *) &res) = (tmp >> 9) | 0x3F800000;

	return (res - 1.0f);
}

static const char * vs_source[] =
{
	"#version 410 core                                                      \n"
	"                                                                       \n"
	"layout (location = 1) in int alien_index;                              \n"
	"                                                                       \n"
	"out VS_OUT                                                             \n"
	"{                                                                      \n"
	"    flat int alien;                                                    \n"
	"    vec2 tc;                                                           \n"
	"} vs_out;                                                              \n"
	"                                                                       \n"
	"layout(std140) uniform droplets                                        \n"
	"{                                                                      \n"
	"    vec4 droplet[256];                                                 \n"
	"};                                                                     \n"
	"                                                                       \n"
	"void main(void)                                                        \n"
	"{                                                                      \n"
	"    const vec2[4] position = vec2[4](vec2(-0.5, -0.5),                 \n"
	"                                     vec2( 0.5, -0.5),                 \n"
	"                                     vec2(-0.5,  0.5),                 \n"
	"                                     vec2( 0.5,  0.5));                \n"
    "    const vec2[4] texcoord = vec2[4](vec2(0, 0),                       \n"
    "                                     vec2(1, 0),                       \n"
    "                                     vec2(0, 1),                       \n"
    "                                     vec2(1, 1));                      \n"
    "    vs_out.tc = texcoord[gl_VertexID];                                 \n"
	"    float co = cos(droplet[alien_index].z);                            \n"
	"    float so = sin(droplet[alien_index].z);                            \n"
	"    mat2 rot = mat2(vec2(co, so),                                      \n"
	"                    vec2(-so, co));                                    \n"
	"    vec2 pos = 0.25 * rot * position[gl_VertexID];                     \n"
	"    gl_Position = vec4(pos + droplet[alien_index].xy, 0.5, 1.0);       \n"
	"    vs_out.alien = int(mod(float(alien_index), 64.0));          		\n"
	"}                                                                      \n"
};

static const char * fs_source[] =
{
	"#version 410 core                                                      \n"
	"                                                                       \n"
	"layout (location = 0) out vec4 color;                                  \n"
	"                                                                       \n"
	"in VS_OUT                                                              \n"
	"{                                                                      \n"
	"    flat int alien;                                                    \n"
	"    vec2 tc;                                                           \n"
	"} fs_in;                                                               \n"
	"                                                                       \n"
	"uniform sampler2DArray tex_aliens;                                     \n"
	"                                                                       \n"
	"void main(void)                                                        \n"
	"{                                                                      \n"
	"    color = texture(tex_aliens, vec3(fs_in.tc, float(fs_in.alien)));   \n"
	"}                                                                      \n"
};

GLuint          program;
GLuint          vao;
GLuint          tex_alien_array;
GLuint          rain_buffer;

float           droplet_x_offset[256];
float           droplet_rot_speed[256];
float           droplet_fall_speed[256];

void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	program = glCreateProgram();
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, fs_source, NULL);
	glCompileShader(fs);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vs_source, NULL);
	glCompileShader(vs);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
    printGLShaderLog(vs);
    printGLShaderLog(fs);

	glLinkProgram(program);
	glUseProgram(program);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	TextureData tex = loadPNG("../../Media/Textures/aliens.png");
	glGenTextures(1, &tex_alien_array);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex_alien_array);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 256, 256, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	delete[] tex.data;

	glGenBuffers(1, &rain_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, rain_buffer);
	glBufferData(GL_UNIFORM_BUFFER, 256 * sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, rain_buffer);

	for (int i = 0; i < 256; i++)
	{
		droplet_x_offset[i] = random_float() * 2.0f - 1.0f;
		droplet_rot_speed[i] = (random_float() + 0.5f) * ((i & 1) ? -3.0f : 3.0f);
		droplet_fall_speed[i] = random_float() + 0.2f;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

	vec4 * droplet = (vec4 *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, 256 * sizeof(vec4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (int i = 0; i < 256; i++)
	{
		droplet[i][0] = droplet_x_offset[i];
        droplet[i][1] = 2.0f - fmodf((currentTime + float(i)) * droplet_fall_speed[i], 4.31f);
        droplet[i][2] = currentTime * droplet_rot_speed[i];
		droplet[i][3] = 0.0f;
	}
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	int alien_index;
	for (alien_index = 0; alien_index < 256; alien_index++)
	{
		glVertexAttribI1i(1, alien_index);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
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