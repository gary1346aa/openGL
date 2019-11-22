#include "../../Include/Common.h"

using namespace glm;
using namespace std;

#define IMAGE_WIDTH 1000
#define IMAGE_HEIGHT 750

GLuint texture_in;
GLuint texture_mid;
GLuint texture_out;

GLuint program_blurh;
GLuint program_blurv;
GLuint program_render_image;

const char* blurh_cs[] =
{
	"#version 430 core                                                                       \n"
	"                                                                                        \n"
	"#define IMAGE_WIDTH 1000                                                                \n"
	"                                                                                        \n"
	"layout(local_size_x = IMAGE_WIDTH, local_size_y = 1, local_size_z = 1) in;              \n"
	"                                                                                        \n"
	"layout(rgba8, binding = 0) uniform image2D input_image;                                 \n"
	"layout(rgba8, binding = 1) uniform image2D output_image;                                \n"
	"                                                                                        \n"
	"const float kernal[7] = float[7]( 0.030078323, 0.104983664, 0.222250419,                \n"
	"                                  0.285375187, 0.222250419, 0.104983664, 0.030078323 ); \n"
	"                                                                                        \n"
	"void main(void)                                                                         \n"
	"{                                                                                       \n"
	"	int loc = int(gl_GlobalInvocationID.x) - 3;                                          \n"
	"                                                                                        \n"
	"	vec4 sum = vec4(0);                                                                  \n"
	"	for(int i = 0; i < 7; ++i)                                                           \n"
	"	{                                                                                    \n"
	"		int loc2 = clamp(loc + i, 0, IMAGE_WIDTH - 1);                                   \n"
	"		sum += imageLoad(input_image, ivec2(loc2, gl_GlobalInvocationID.y)) * kernal[i]; \n"
	"	}                                                                                    \n"
	"                                                                                        \n"
	"	imageStore(output_image, ivec2(gl_GlobalInvocationID.xy), sum);                      \n"
	"}                                                                                       \n"
};

const char* blurv_cs[] =
{
	"#version 430 core                                                                       \n"
	"                                                                                        \n"
	"#define IMAGE_HEIGHT 750                                                                \n"
	"                                                                                        \n"
	"layout(local_size_x = 1, local_size_y = IMAGE_HEIGHT, local_size_z = 1) in;             \n"
	"                                                                                        \n"
	"layout(rgba8, binding = 0) uniform image2D input_image;                                 \n"
	"layout(rgba8, binding = 1) uniform image2D output_image;                                \n"
	"                                                                                        \n"
	"const float kernal[7] = float[7]( 0.030078323, 0.104983664, 0.222250419,                \n"
	"                                  0.285375187, 0.222250419, 0.104983664, 0.030078323 ); \n"
	"                                                                                        \n"
	"void main(void)                                                                         \n"
	"{                                                                                       \n"
	"	int loc = int(gl_GlobalInvocationID.y) - 3;                                          \n"
	"                                                                                        \n"
	"	vec4 sum = vec4(0);                                                                  \n"
	"	for(int i = 0; i < 7; ++i)                                                           \n"
	"	{                                                                                    \n"
	"		int loc2 = clamp(loc + i, 0, IMAGE_HEIGHT - 1);                                  \n"
	"		sum += imageLoad(input_image, ivec2(gl_GlobalInvocationID.x, loc2)) * kernal[i]; \n"
	"	}                                                                                    \n"
	"                                                                                        \n"
	"	imageStore(output_image, ivec2(gl_GlobalInvocationID.xy), sum);                      \n"
	"}                                                                                       \n"
};

const char* simple_vs[] = 
{
	"#version 430 core                                        \n"
	"                                                         \n"
	"out vec2 texcoord;                                       \n"
	"                                                         \n"
	"void main(void)                                          \n"
	"{                                                        \n"
	"    const vec4 vertices[] = vec4[](vec4(-1, -1, 0, 1),   \n"
	"                                   vec4( 1, -1, 0, 1),   \n"
	"                                   vec4( 1,  1, 0, 1),   \n"
	"                                   vec4(-1, -1, 0, 1),   \n"
	"                                   vec4( 1,  1, 0, 1),   \n"
	"                                   vec4(-1,  1, 0, 1));  \n"
	"                                                         \n"
	"    gl_Position = vertices[gl_VertexID];                 \n"
	"    texcoord = vertices[gl_VertexID].xy * 0.5 + 0.5;     \n"
	"}                                                        \n"
};

const char* simple_fs[] =
{
	"#version 430 core                           \n"
	"                                            \n"
	"layout(binding = 0) uniform sampler2D tex;  \n"
	"                                            \n"
	"in vec2 texcoord;                           \n"
	"                                            \n"
	"out vec4 color;                             \n"
	"                                            \n"
	"void main(void)                             \n"
	"{                                           \n"
	"    color = texture(tex, texcoord);         \n"
	"}                                           \n"
};

void My_Init()
{
	program_blurh = glCreateProgram();
	GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(cs, 1, blurh_cs, NULL);
	glCompileShader(cs);
	glAttachShader(program_blurh, cs);
	glLinkProgram(program_blurh);

	program_blurv = glCreateProgram();
	cs = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(cs, 1, blurv_cs, NULL);
	glCompileShader(cs);
	glAttachShader(program_blurv, cs);
	glLinkProgram(program_blurv);

	program_render_image = glCreateProgram();
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, simple_vs, NULL);
	glCompileShader(vs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, simple_fs, NULL);
	glCompileShader(fs);
	glAttachShader(program_render_image, vs);
	glAttachShader(program_render_image, fs);
	glLinkProgram(program_render_image);

	TextureData textureData = loadPNG("../../Media/Textures/plant.jpg");
	glGenTextures(1, &texture_in);
	glBindTexture(GL_TEXTURE_2D, texture_in);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, textureData.width, textureData.height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureData.width, textureData.height, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	delete[] textureData.data;

	glGenTextures(1, &texture_mid);
	glBindTexture(GL_TEXTURE_2D, texture_mid);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, textureData.width, textureData.height);

	glGenTextures(1, &texture_out);
	glBindTexture(GL_TEXTURE_2D, texture_out);
	glTexStorage2D(GL_TEXTURE_2D, 10, GL_RGBA8, textureData.width, textureData.height);
}

void My_Display()
{
	glUseProgram(program_blurh);
    glBindImageTexture(0, texture_in, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
    glBindImageTexture(1, texture_mid, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glDispatchCompute(1, IMAGE_HEIGHT, 1);

	glUseProgram(program_blurv);
    glBindImageTexture(0, texture_mid, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
    glBindImageTexture(1, texture_out, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glDispatchCompute(IMAGE_WIDTH, 1, 1);
	glBindTexture(GL_TEXTURE_2D, texture_out);
    glGenerateMipmap(GL_TEXTURE_2D);

	glUseProgram(program_render_image);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_out);
    glDrawArrays(GL_TRIANGLES, 0, 6);

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
	glutInitWindowSize(IMAGE_WIDTH, IMAGE_HEIGHT);
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