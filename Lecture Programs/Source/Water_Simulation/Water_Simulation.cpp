#include "../../Include/Common.h"

////////////////////////////////////////
// The algorithm is inspired by:
// https://github.com/evanw/webgl-water
////////////////////////////////////////

using namespace glm;
using namespace std;

struct WaterColumn
{
    float height;
    float flow;
};

GLuint texture_env;
GLuint program_water;
GLuint program_drop;
GLuint program_render;
GLuint vao;
mat4 mvp;
float timeElapsed = 0.0f;
GLuint waterBufferIn;
GLuint waterBufferOut;

const char* water_cs[] =
{
	"#version 430 core                                                                                \n"
	"                                                                                                 \n"
	"struct WaterColumn                                                                               \n"
	"{                                                                                                \n"
	"    float height;                                                                                \n"
	"    float flow;                                                                                  \n"
	"};                                                                                               \n"
	"                                                                                                 \n"
	"layout(std430, binding = 0) buffer WaterGridIn                                                   \n"
	"{                                                                                                \n"
	"    WaterColumn columnsIn[];                                                                     \n"
	"};                                                                                               \n"
	"                                                                                                 \n"
	"layout(std430, binding = 1) buffer WaterGridOut                                                  \n"
	"{                                                                                                \n"
	"    WaterColumn columnsOut[];                                                                    \n"
	"};                                                                                               \n"
	"                                                                                                 \n"
	"#define GRID_SIZE 18                                                                             \n"
	"#define ROW_SIZE (GRID_SIZE * gl_NumWorkGroups.x)                                                \n"
	"                                                                                                 \n"
	"layout(local_size_x = GRID_SIZE, local_size_y = GRID_SIZE, local_size_z = 1) in;                 \n"
	"                                                                                                 \n"
	"void main(void)                                                                                  \n"
	"{                                                                                                \n"
	"    uint row_size = ROW_SIZE;                                                                    \n"
	"    uint globalIdx = gl_GlobalInvocationID.y * ROW_SIZE + gl_GlobalInvocationID.x;               \n"
	"    float avg = 0;                                                                               \n"
	"                                                                                                 \n"
	"    if(gl_GlobalInvocationID.x < row_size - 1)                                                   \n"
	"    {                                                                                            \n"
	"        avg += columnsIn[globalIdx + 1].height;                                                  \n"
	"    }                                                                                            \n"
	"    else                                                                                         \n"
	"    {                                                                                            \n"
	"        avg += columnsIn[globalIdx].height;                                                      \n"
	"    }                                                                                            \n"
	"                                                                                                 \n"
	"    if(gl_GlobalInvocationID.x > 0)                                                              \n"
	"    {                                                                                            \n"
	"        avg += columnsIn[globalIdx - 1].height;                                                  \n"
	"    }                                                                                            \n"
	"    else                                                                                         \n"
	"    {                                                                                            \n"
	"        avg += columnsIn[globalIdx].height;                                                      \n"
	"    }                                                                                            \n"
	"                                                                                                 \n"
	"    if(gl_GlobalInvocationID.y < row_size - 1)                                                   \n"
	"    {                                                                                            \n"
	"        avg += columnsIn[globalIdx + row_size].height;                                           \n"
	"    }                                                                                            \n"
	"    else                                                                                         \n"
	"    {                                                                                            \n"
	"        avg += columnsIn[globalIdx].height;                                                      \n"
	"    }                                                                                            \n"
	"                                                                                                 \n"
	"    if(gl_GlobalInvocationID.y > 0)                                                              \n"
	"    {                                                                                            \n"
	"        avg += columnsIn[globalIdx - row_size].height;                                           \n"
	"    }                                                                                            \n"
	"    else                                                                                         \n"
	"    {                                                                                            \n"
	"        avg += columnsIn[globalIdx].height;                                                      \n"
	"    }                                                                                            \n"
	"    avg *= 0.25;                                                                                 \n"
	"    float flow = (columnsIn[globalIdx].flow + (avg - columnsIn[globalIdx].height) * 2.0) * 0.995;\n"
	"                                                                                                 \n"
	"    columnsOut[globalIdx].flow = flow;                                                           \n"
	"    columnsOut[globalIdx].height = columnsIn[globalIdx].height + flow;                           \n"
	"}                                                                                                \n"
};

const char* drop_cs[] =
{
	"#version 430 core                                                                 \n"
	"                                                                                  \n"
	"struct WaterColumn                                                                \n"
	"{                                                                                 \n"
	"    float height;                                                                 \n"
	"    float flow;                                                                   \n"
	"};                                                                                \n"
	"                                                                                  \n"
	"layout(std430, binding = 0) buffer WaterGrid                                      \n"
	"{                                                                                 \n"
	"    WaterColumn columns[];                                                        \n"
	"};                                                                                \n"
	"                                                                                  \n"
	"#define GRID_SIZE 18                                                              \n"
	"#define ROW_SIZE (GRID_SIZE * gl_NumWorkGroups.x)                                 \n"
	"                                                                                  \n"
	"layout(local_size_x = GRID_SIZE, local_size_y = GRID_SIZE, local_size_z = 1) in;  \n"
	"                                                                                  \n"
	"layout(location = 0) uniform uvec2 dropCoord;                                     \n"
	"                                                                                  \n"
	"const float RADIUS = 0.03;                                                        \n"
	"const float PI = 3.141592653589793;                                               \n"
	"const float STRENGTH = 5.0;                                                       \n"
	"                                                                                  \n"
	"void main(void)                                                                   \n"
	"{                                                                                 \n"
	"    float row_size_1 = float(ROW_SIZE) - 1.0;                                     \n"
	"    vec2 pos = vec2(gl_GlobalInvocationID.xy) / row_size_1;                       \n"
	"    vec2 center = vec2(dropCoord) / row_size_1;                                   \n"
	"    float dist = length(center - pos);                                            \n"
	"    float drop = max(0, 1.0 - dist / RADIUS);                                     \n"
	"    drop = 0.5 - cos(drop * PI) * 0.5;                                            \n"
	"                                                                                  \n"
	"    uint globalIdx = gl_GlobalInvocationID.y * ROW_SIZE + gl_GlobalInvocationID.x;\n"
	"    columns[globalIdx].height += drop * STRENGTH;                                 \n"
	"}                                                                                 \n"
};

const char* render_vs[] =
{
	"#version 430 core                                                                               \n"
	"                                                                                                \n"
	"struct WaterColumn                                                                              \n"
	"{                                                                                               \n"
	"    float height;                                                                               \n"
	"    float flow;                                                                                 \n"
	"};                                                                                              \n"
	"                                                                                                \n"
	"layout(std430, binding = 0) buffer WaterGrid                                                    \n"
	"{                                                                                               \n"
	"    WaterColumn columns[];                                                                      \n"
	"};                                                                                              \n"
	"                                                                                                \n"
	"layout(location = 0) uniform mat4 mvp;                                                          \n"
	"layout(location = 1) uniform vec3 eye;                                                          \n"
	"                                                                                                \n"
	"out vec3 normal;                                                                                \n"
	"out vec3 view;                                                                                  \n"
	"                                                                                                \n"
	"#define ROW_SIZE 180                                                                            \n"
	"                                                                                                \n"
	"const uint offset[4] = uint[4](0, 1, ROW_SIZE, ROW_SIZE + 1);                                   \n"
	"const vec2 poffset[4] = vec2[4](                                                                \n"
	"    vec2(0, 0),                                                                                 \n"
	"    vec2(1, 0),                                                                                 \n"
	"    vec2(0, 1),                                                                                 \n"
	"    vec2(1, 1)                                                                                  \n"
	");                                                                                              \n"
	"                                                                                                \n"
	"void main(void)                                                                                 \n"
	"{                                                                                               \n"
	"    uvec2 coord = uvec2(mod(gl_InstanceID, ROW_SIZE - 1), uint(gl_InstanceID / (ROW_SIZE - 1)));\n"
	"    uint idx = coord.y * ROW_SIZE + coord.x;                                                    \n"
	"    idx = idx + offset[gl_VertexID];                                                            \n"
	"    vec3 pos = vec3(                                                                            \n"
	"        float(coord.x) - float(ROW_SIZE / 2) + poffset[gl_VertexID].x,                          \n"
	"        columns[idx].height,                                                                    \n"
	"        float(coord.y) - float(ROW_SIZE / 2) + poffset[gl_VertexID].y);                         \n"
	"    gl_Position = mvp * vec4(pos, 1.0);                                                         \n"
	"                                                                                                \n"
	"    vec3 dx = vec3(1, 0, 0);                                                                    \n"
	"    vec3 dy = vec3(0, 0, 1);                                                                    \n"
	"    if(coord.x < ROW_SIZE - 2)                                                                  \n"
	"    {                                                                                           \n"
	"        dx = vec3(1, columns[idx + 1].height - columns[idx].height, 0);                         \n"
	"    }                                                                                           \n"
	"    if(coord.y < ROW_SIZE - 2)                                                                  \n"
	"    {                                                                                           \n"
	"        dy = vec3(0, columns[idx + ROW_SIZE].height - columns[idx].height, 1);                  \n"
	"    }                                                                                           \n"
	"    normal = normalize(cross(dy, dx));                                                          \n"
	"    view = eye - pos;                                                                           \n"
	"}                                                                                               \n"
};

const char* render_fs[] =
{
	"#version 430 core                                             \n"
	"                                                              \n"
	"layout(location = 0) out vec4 fragColor;                      \n"
	"                                                              \n"
	"layout(binding = 0) uniform samplerCube envMap;               \n"
	"                                                              \n"
	"in vec3 normal;                                               \n"
	"in vec3 view;                                                 \n"
	"                                                              \n"
	"const vec3 waterColor = vec3(0.55, 0.8, 0.95);                \n"
	"const float ambient = 0.2;                                    \n"
	"const float shininess = 200.0;                                \n"
	"const vec3 lightVec = vec3(1, 1, -3);                         \n"
	"                                                              \n"
	"void main(void)                                               \n"
	"{                                                             \n"
	"    vec3 V = normalize(view);                                 \n"
	"    vec3 N = normalize(normal);                               \n"
	"    vec3 L = normalize(lightVec);                             \n"
	"    vec3 H = normalize(L + V);                                \n"
	"                                                              \n"
	"    vec3 diffuse = waterColor * (ambient + max(0, dot(N, L)));\n"
	"    vec3 spec = vec3(1) * pow(max(0, dot(H, N)), shininess);  \n"
	"    vec3 env = texture(envMap, reflect(-V, N)).rgb;           \n"
	"    vec3 color = env * 0.5 + (diffuse + spec) * 0.5;          \n"
	"    fragColor = vec4(color, 1.0);                             \n"
	"}                                                             \n"
};

void AddDrop();

void My_Init()
{
	// Initialize random seed. Required in AddDrop() function.
	srand(time(NULL));
	{
		program_drop = glCreateProgram();
		GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(cs, 1, drop_cs, NULL);
		glCompileShader(cs);
		glAttachShader(program_drop, cs);
		glLinkProgram(program_drop);
	}
	{
		program_water = glCreateProgram();
		GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(cs, 1, water_cs, NULL);
		glCompileShader(cs);
		glAttachShader(program_water, cs);
		glLinkProgram(program_water);
	}
	{
		program_render = glCreateProgram();
		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 1, render_vs, NULL);
		glCompileShader(vs);
		GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 1, render_fs, NULL);
		glCompileShader(fs);
		glAttachShader(program_render, vs);
		glAttachShader(program_render, fs);
		glLinkProgram(program_render);
	}
	{
		TextureData envmap_data = loadPNG("../../Media/Textures/mountaincube.png");
		glGenTextures(1, &texture_env);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture_env);
		for(int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, envmap_data.width, envmap_data.height / 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, envmap_data.data + i * (envmap_data.width * (envmap_data.height / 6) * sizeof(unsigned char) * 4));
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		delete[] envmap_data.data;

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	}

	// Create two water grid buffers of 180 * 180 water columns.
    glGenBuffers(1, &waterBufferIn);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, waterBufferIn);
	// Create initial data.
    WaterColumn *data = new WaterColumn[32400];
    for (int x = 0; x < 180; ++x)
    {
        for (int y = 0; y < 180; ++y)
        {
            int idx = y * 180 + x;
            data[idx].height = 60.0f;
            data[idx].flow = 0.0f;
        }
    }
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(WaterColumn) * 32400, data, GL_DYNAMIC_STORAGE_BIT);
    delete[] data;

    glGenBuffers(1, &waterBufferOut);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, waterBufferOut);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(WaterColumn) * 32400, NULL, GL_DYNAMIC_STORAGE_BIT);

	glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

	// Create an index buffer of 2 triangles, 6 vertices.
    uint indices[] = { 0, 2, 3, 0, 3, 1 };
    GLuint ebo;
    glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * 6, indices, GL_DYNAMIC_STORAGE_BIT);

	AddDrop();
}

void AddDrop()
{
	// Randomly add a "drop" of water into the grid system.
    glUseProgram(program_drop);
    glUniform2ui(0, rand() % 180, rand() % 180);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, waterBufferIn);
    glDispatchCompute(10, 10, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void My_Display()
{
	// Update water grid.
    glUseProgram(program_water);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, waterBufferIn);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, waterBufferOut);
	// Each group updates 18 * 18 of the grid. We need 10 * 10 groups in total.
    glDispatchCompute(10, 10, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// Render water surface.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao);
    glUseProgram(program_render);
    glUniformMatrix4fv(0, 1, GL_FALSE, value_ptr(mvp));
    glUniform3f(1, 0, 120, 200);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, waterBufferOut);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_env);
	// Draw 179 * 179 triangles based on the 180 * 180 water grid.
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 179 * 179);

	std::swap(waterBufferIn, waterBufferOut);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

    float aspect = static_cast<float>(width) / static_cast<float>(height);
    mvp = perspective(deg2rad(50.0f), aspect, 1.0f, 500.0f) * lookAt(vec3(0, 120, 200), vec3(0, 70, 0), vec3(0, 1, 0));
}

void My_Timer(int val)
{
	timeElapsed += 0.016;
	if(timeElapsed > 10.0f)
	{
		AddDrop();
		timeElapsed = 0;
	}
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
	glutInitWindowSize(800, 600);
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