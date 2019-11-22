#include "../../Include/Common.h"

using namespace glm;
using namespace std;

#define MAX_PARTICLE_COUNT 1000

struct DrawArraysIndirectCommand
{
    uint count;
    uint primCount;
    uint first;
    uint baseInstance;
};
DrawArraysIndirectCommand defalutDrawArraysCommand = { 0, 1, 0, 0 };

struct Particle
{
    vec3 position;
    float _padding;
    vec3 velocity;
    float lifeTime;
};

struct ParticleBuffer
{
	GLuint shaderStorageBuffer;
	GLuint indirectBuffer;
};

ParticleBuffer particleIn;
ParticleBuffer particleOut;
GLuint particleTexture;
GLuint updateProgram;
GLuint addProgram;
GLuint renderProgram;
GLuint vao;
mat4 mv;
mat4 p;

const char* add_cs_source[] =
{
	"#version 430 core                                                                                               \n"
	"                                                                                                                \n"
	"layout(local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;                                             \n"
	"                                                                                                                \n"
	"struct Particle                                                                                                 \n"
	"{                                                                                                               \n"
	"    vec3 position;                                                                                              \n"
	"    vec3 velocity;                                                                                              \n"
	"    float lifeTime;                                                                                             \n"
	"};                                                                                                              \n"
	"                                                                                                                \n"
	"layout(std140, binding=0) buffer Particles                                                                      \n"
	"{                                                                                                               \n"
	"     Particle particles[1000];                                                                                  \n"
	"};                                                                                                              \n"
	"                                                                                                                \n"
	"layout(binding = 0, offset = 0) uniform atomic_uint count;                                                      \n"
	"layout(location = 0) uniform uint addCount;                                                                     \n"
	"layout(location = 1) uniform vec2 randomSeed;                                                                   \n"
	"                                                                                                                \n"
	"float rand(vec2 n)                                                                                              \n"
	"{                                                                                                               \n"
	"    return fract(sin(dot(n.xy, vec2(12.9898, 78.233))) * 43758.5453);                                           \n"
	"}                                                                                                               \n"
	"                                                                                                                \n"
	"const float PI2 = 6.28318530718;                                                                                \n"
	"                                                                                                                \n"
	"void main(void)                                                                                                 \n"
	"{                                                                                                               \n"
	"    if(gl_GlobalInvocationID.x < addCount)                                                                      \n"
	"    {                                                                                                           \n"
	"        uint idx = atomicCounterIncrement(count);                                                               \n"
	"        float rand1 = rand(randomSeed + vec2(float(gl_GlobalInvocationID.x * 2)));                              \n"
	"        float rand2 = rand(randomSeed + vec2(float(gl_GlobalInvocationID.x * 2 + 1)));                          \n"
	"        particles[idx].position = vec3(0, 0, 0);                                                                \n"
	"        particles[idx].velocity = normalize(vec3(cos(rand1 * PI2), 5.0 + rand2 * 5.0, sin(rand1 * PI2))) * 0.15;\n"
	"        particles[idx].lifeTime = 0;                                                                            \n"
	"    }                                                                                                           \n"
	"}                                                                                                               \n"
};

const char* update_cs_source[] =
{
	"#version 430 core                                                                                               \n"
	"                                                                                                                \n"
	"layout(local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;                                             \n"
	"                                                                                                                \n"
	"struct Particle                                                                                                 \n"
	"{                                                                                                               \n"
	"    vec3 position;                                                                                              \n"
	"    vec3 velocity;                                                                                              \n"
	"    float lifeTime;                                                                                             \n"
	"};                                                                                                              \n"
	"                                                                                                                \n"
	"layout(std140, binding=0) buffer InParticles                                                                    \n"
	"{                                                                                                               \n"
	"     Particle inParticles[1000];                                                                                \n"
	"};                                                                                                              \n"
	"                                                                                                                \n"
	"layout(std140, binding=1) buffer OutParticles                                                                   \n"
	"{                                                                                                               \n"
	"     Particle outParticles[1000];                                                                               \n"
	"};                                                                                                              \n"
	"                                                                                                                \n"
	"layout(binding = 0, offset = 0) uniform atomic_uint inCount;                                                    \n"
	"layout(binding = 1, offset = 0) uniform atomic_uint outCount;                                                   \n"
	"layout(location = 0) uniform float deltaTime;                                                                   \n"
	"                                                                                                                \n"
	"const vec3 windAccel = vec3(0.015, 0, 0);                                                                       \n"
	"                                                                                                                \n"
	"void main(void)                                                                                                 \n"
	"{                                                                                                               \n"
	"    uint idx = gl_GlobalInvocationID.x;                                                                         \n"
	"    if(idx < atomicCounter(inCount))                                                                            \n"
	"    {                                                                                                           \n"
	"        float lifeTime = inParticles[idx].lifeTime + deltaTime;                                                 \n"
	"        if(lifeTime < 10.0)                                                                                     \n"
	"        {                                                                                                       \n"
	"            uint outIdx = atomicCounterIncrement(outCount);                                                     \n"
	"            outParticles[outIdx].position = inParticles[idx].position + inParticles[idx].velocity * deltaTime;  \n"
	"            outParticles[outIdx].velocity = inParticles[idx].velocity + windAccel * deltaTime;                  \n"
	"            outParticles[outIdx].lifeTime = lifeTime;                                                           \n"
	"        }                                                                                                       \n"
	"    }                                                                                                           \n"
	"}                                                                                                               \n"
};

const char* render_vs_source[] =
{
	"#version 430 core                                                      \n"
	"                                                                       \n"
	"struct Particle                                                        \n"
	"{                                                                      \n"
	"    vec3 position;                                                     \n"
	"    vec3 velocity;                                                     \n"
	"    float lifeTime;                                                    \n"
	"};                                                                     \n"
	"                                                                       \n"
	"layout(std140, binding=0) buffer Particles                             \n"
	"{                                                                      \n"
	"     Particle particles[1000];                                         \n"
	"};                                                                     \n"
	"                                                                       \n"
	"layout(location = 0) uniform mat4 mv;                                  \n"
	"                                                                       \n"
	"out vec4 particlePosition;                                             \n"
	"out float particleSize;                                                \n"
	"out float particleAlpha;                                               \n"
	"                                                                       \n"
	"void main(void)                                                        \n"
	"{                                                                      \n"
	"    particlePosition = mv * vec4(particles[gl_VertexID].position, 1.0);\n"
	"    float lifeTime = particles[gl_VertexID].lifeTime;                  \n"
	"    particleSize = 0.01 + lifeTime * 0.02;                             \n"
	"    particleAlpha = pow((10.0 - lifeTime) * 0.1, 7.0) * 0.7;           \n"
	"}                                                                      \n"
};

const char* render_gs_source[] =
{
	"#version 430 core                                                               \n"
	"                                                                                \n"
	"layout(points, invocations = 1) in;                                             \n"
	"layout(triangle_strip, max_vertices = 4) out;                                   \n"
	"                                                                                \n"
	"layout(location = 1) uniform mat4 p;                                            \n"
	"                                                                                \n"
	"in vec4 particlePosition[];                                                     \n"
	"in float particleSize[];                                                        \n"
	"in float particleAlpha[];                                                       \n"
	"                                                                                \n"
	"out float particleAlphaOut;                                                     \n"
	"out vec2 texcoord;                                                              \n"
	"                                                                                \n"
	"void main(void)                                                                 \n"
	"{                                                                               \n"
	"    vec4 verts[4];                                                              \n"
	"    verts[0] = p * (particlePosition[0] + vec4(-1, -1, 0, 0) * particleSize[0]);\n"
	"    verts[1] = p * (particlePosition[0] + vec4(1, -1, 0, 0) * particleSize[0]); \n"
	"    verts[2] = p * (particlePosition[0] + vec4(1, 1, 0, 0) * particleSize[0]);  \n"
	"    verts[3] = p * (particlePosition[0] + vec4(-1, 1, 0, 0) * particleSize[0]); \n"
	"                                                                                \n"
	"    gl_Position = verts[0];                                                     \n"
	"    particleAlphaOut = particleAlpha[0];                                        \n"
	"    texcoord = vec2(0, 0);                                                      \n"
	"    EmitVertex();                                                               \n"
	"                                                                                \n"
	"    gl_Position = verts[1];                                                     \n"
	"    particleAlphaOut = particleAlpha[0];                                        \n"
	"    texcoord = vec2(1, 0);                                                      \n"
	"    EmitVertex();                                                               \n"
	"                                                                                \n"
	"    gl_Position = verts[3];                                                     \n"
	"    particleAlphaOut = particleAlpha[0];                                        \n"
	"    texcoord = vec2(0, 1);                                                      \n"
	"    EmitVertex();                                                               \n"
	"                                                                                \n"
	"    gl_Position = verts[2];                                                     \n"
	"    particleAlphaOut = particleAlpha[0];                                        \n"
	"    texcoord = vec2(1, 1);                                                      \n"
	"    EmitVertex();                                                               \n"
	"                                                                                \n"
	"    EndPrimitive();                                                             \n"
	"}                                                                               \n"
};

const char* render_fs_source[] =
{
	"#version 430 core                                                 \n"
	"                                                                  \n"
	"layout(binding = 0) uniform sampler2D particleTex;                \n"
	"                                                                  \n"
	"layout(location = 0) out vec4 fragColor;                          \n"
	"                                                                  \n"
	"in float particleAlphaOut;                                        \n"
	"in vec2 texcoord;                                                 \n"
	"                                                                  \n"
	"void main(void)                                                   \n"
	"{                                                                 \n"
	"    fragColor = texture(particleTex, texcoord) * particleAlphaOut;\n"
	"}                                                                 \n"
};

void My_Init()
{
	srand(time(NULL));
	{
		// Create shader program for adding particles.
		GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(cs, 1, add_cs_source, NULL);
		glCompileShader(cs);
		addProgram = glCreateProgram();
		glAttachShader(addProgram, cs);
		glLinkProgram(addProgram);
		printGLShaderLog(cs);
	}
	{
		// Create shader program for updating particles. (from input to output)
		GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(cs, 1, update_cs_source, NULL);
		glCompileShader(cs);
		updateProgram = glCreateProgram();
		glAttachShader(updateProgram, cs);
		glLinkProgram(updateProgram);
		printGLShaderLog(cs);
	}
	{
		// Create shader program for rendering particles.
		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 1, render_vs_source, NULL);
		glCompileShader(vs);
		GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(gs, 1, render_gs_source, NULL);
		glCompileShader(gs);
		GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 1, render_fs_source, NULL);
		glCompileShader(fs);
		renderProgram = glCreateProgram();
		glAttachShader(renderProgram, vs);
		glAttachShader(renderProgram, gs);
		glAttachShader(renderProgram, fs);
		glLinkProgram(renderProgram);
		printGLShaderLog(vs);
		printGLShaderLog(gs);
		printGLShaderLog(fs);
	}
	{
		// Create shader storage buffers & indirect buffers. (which are also used as atomic counter buffers)
		glGenBuffers(1, &particleIn.shaderStorageBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleIn.shaderStorageBuffer);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * MAX_PARTICLE_COUNT, NULL, GL_DYNAMIC_STORAGE_BIT);

		glGenBuffers(1, &particleIn.indirectBuffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, particleIn.indirectBuffer);
		glBufferStorage(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &defalutDrawArraysCommand, GL_DYNAMIC_STORAGE_BIT);

		glGenBuffers(1, &particleOut.shaderStorageBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleOut.shaderStorageBuffer);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * MAX_PARTICLE_COUNT, NULL, GL_DYNAMIC_STORAGE_BIT);

		glGenBuffers(1, &particleOut.indirectBuffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, particleOut.indirectBuffer);
		glBufferStorage(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), &defalutDrawArraysCommand, GL_DYNAMIC_STORAGE_BIT);
	}

	// Create particle texture.
	TextureData textureData = loadPNG("../../Media/Textures/smoke.jpg");
	glGenTextures(1, &particleTexture);
	glBindTexture(GL_TEXTURE_2D, particleTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, textureData.width, textureData.height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureData.width, textureData.height, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	delete[] textureData.data;

	// Create VAO. We don't have any input attributes, but this is still required.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
}

void AddParticle(uint count)
{
	// Add count particles to input buffers.
    glUseProgram(addProgram);
    glUniform1ui(0, count);
    glUniform2f(1, static_cast<float>(rand()), static_cast<float>(rand()));
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleIn.shaderStorageBuffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, particleIn.indirectBuffer);
    glDispatchCompute(1, 1, 1);
}

void My_Display()
{
	// Update particles.
	glUseProgram(updateProgram);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleIn.shaderStorageBuffer);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, particleIn.indirectBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particleOut.shaderStorageBuffer);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, particleOut.indirectBuffer);
    glUniform1f(0, 0.016f); // We use a fixed update step of 0.016 seconds.
    glNamedBufferSubData(particleOut.indirectBuffer, 0, sizeof(DrawArraysIndirectCommand), &defalutDrawArraysCommand);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthMask(GL_FALSE); // Disable depth writing for additive blending. Remember to turn it on later...

	// Draw particles using updated buffers using additive blending.
    glBindVertexArray(vao);
    glUseProgram(renderProgram);
    glUniformMatrix4fv(0, 1, GL_FALSE, value_ptr(mv));
	glUniformMatrix4fv(1, 1, GL_FALSE, value_ptr(p));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, particleTexture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleOut.shaderStorageBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, particleOut.indirectBuffer);
    glDrawArraysIndirect(GL_POINTS, 0);

	// Swap input and output buffer.
    std::swap(particleIn, particleOut);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	float aspect = static_cast<float>(width) / static_cast<float>(height);
	mv = lookAt(vec3(0, 1, 1), vec3(0, 0.5, 0), vec3(0, 1, 0));
	p = perspective(deg2rad(45.0f), aspect, 0.1f, 10.0f);
}

void My_Timer(int val)
{
	// Emit 1 particle every 0.016 seconds.
	AddParticle(1);
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
	glutInitWindowSize(800, 800);
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