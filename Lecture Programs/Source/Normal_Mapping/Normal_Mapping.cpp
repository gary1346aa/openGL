#include "../../Include/Common.h"

using namespace glm;
using namespace std;

GLuint program;
mat4 proj_matrix;
int index_count;

struct
{
    GLuint color;
    GLuint normals;
} textures;

struct
{
    GLint mv_matrix;
    GLint proj_matrix;
    GLint light_pos;
	GLint tex_color;
	GLint tex_normal;
} uniforms;

const char *normalmapping_fs_glsl[] = 
{
    "#version 410 core                                                                 \n"
    "                                                                                  \n"
    "out vec4 color;                                                                   \n"
    "                                                                                  \n"
    "// Color and normal maps                                                          \n"
    "uniform sampler2D tex_color;                                                      \n"
    "uniform sampler2D tex_normal;                                                     \n"
    "                                                                                  \n"
    "in VS_OUT                                                                         \n"
    "{                                                                                 \n"
    "    vec2 texcoord;                                                                \n"
    "    vec3 eyeDir;                                                                  \n"
    "    vec3 lightDir;                                                                \n"
    "    vec3 normal;                                                                  \n"
    "} fs_in;                                                                          \n"
    "                                                                                  \n"
    "void main(void)                                                                   \n"
    "{                                                                                 \n"
    "    // Normalize our incomming view and light direction vectors.                  \n"
    "    vec3 V = normalize(fs_in.eyeDir);                                             \n"
    "    vec3 L = normalize(fs_in.lightDir);                                           \n"
    "    // Read the normal from the normal map and normalize it.                      \n"
    "    vec3 N = normalize(texture(tex_normal, fs_in.texcoord).rgb * 2.0 - vec3(1.0));\n"
    "    // Uncomment this to use surface normals rather than the normal map           \n"
    "    // N = vec3(0.0, 0.0, 1.0);                                                   \n"
    "    // Calculate R ready for use in Phong lighting.                               \n"
    "    vec3 R = reflect(-L, N);                                                      \n"
    "                                                                                  \n"
    "    // Fetch the diffuse albedo from the texture.                                 \n"
    "    vec3 diffuse_albedo = texture(tex_color, fs_in.texcoord).rgb;                 \n"
    "    // Calculate diffuse color with simple N dot L.                               \n"
    "    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;                          \n"
    "    // Uncomment this to turn off diffuse shading                                 \n"
    "    // diffuse = vec3(0.0);                                                       \n"
    "                                                                                  \n"
    "    // Assume that specular albedo is white - it could also come from a texture   \n"
    "    vec3 specular_albedo = vec3(1.0);                                             \n"
    "    // Calculate Phong specular highlight                                         \n"
    "    vec3 specular = max(pow(dot(R, V), 20.0), 0.0) * specular_albedo;             \n"
    "    // Uncomment this to turn off specular highlights                             \n"
    "    // specular = vec3(0.0);                                                      \n"
    "                                                                                  \n"
    "    // Final color is diffuse + specular                                          \n"
    "    color = vec4(diffuse + specular, 1.0);                                        \n"
    "}                                                                                 \n"
    "                                                                                  \n"
};

const char *normalmapping_vs_glsl[] = 
{
    "#version 410 core                                                         \n"
    "                                                                          \n"
    "layout (location = 0) in vec3 position;                                   \n"
    "layout (location = 1) in vec3 normal;                                     \n"
    "layout (location = 2) in vec3 tangent;                                    \n"
    "layout (location = 3) in vec2 texcoord;                                   \n"
    "                                                                          \n"
    "out VS_OUT                                                                \n"
    "{                                                                         \n"
    "    vec2 texcoord;                                                        \n"
    "    vec3 eyeDir;                                                          \n"
    "    vec3 lightDir;                                                        \n"
    "    vec3 normal;                                                          \n"
    "} vs_out;                                                                 \n"
    "                                                                          \n"
    "uniform mat4 mv_matrix;                                                   \n"
    "uniform mat4 proj_matrix;                                                 \n"
    "uniform vec3 light_pos;                                                   \n"
    "                                                                          \n"
    "void main(void)                                                           \n"
    "{                                                                         \n"
    "    // Calculate vertex position in view space.                           \n"
	"    vec4 P = mv_matrix * vec4(position, 1.0);                             \n"
    "                                                                          \n"
    "    // Calculate normal (N) and tangent (T) vectors in view space from    \n"
    "    // incoming object space vectors.                                     \n"
    "    vec3 V = P.xyz;                                                       \n"
    "    vec3 N = normalize(mat3(mv_matrix) * normal);                         \n"
    "    vec3 T = normalize(mat3(mv_matrix) * tangent);                        \n"
    "    // Calculate the bitangent vector (B) from the normal and tangent     \n"
    "    // vectors.                                                           \n"
    "    vec3 B = cross(N, T);                                                 \n"
    "                                                                          \n"
    "    // The light vector (L) is the vector from the point of interest to   \n"
    "    // the light. Calculate that and multiply it by the TBN matrix.       \n"
    "    vec3 L = light_pos - P.xyz;                                           \n"
    "    vs_out.lightDir = normalize(vec3(dot(L, T), dot(L, B), dot(L, N)));   \n"
    "                                                                          \n"
    "    // The view vector is the vector from the point of interest to the    \n"
    "    // viewer, which in view space is simply the negative of the position.\n"
    "    // Calculate that and multiply it by the TBN matrix.                  \n"
    "    V = -P.xyz;                                                           \n"
    "    vs_out.eyeDir = normalize(vec3(dot(V, T), dot(V, B), dot(V, N)));     \n"
    "                                                                          \n"
    "    // Pass the texture coordinate through unmodified so that the fragment\n"
    "    // shader can fetch from the normal and color maps.                   \n"
    "    vs_out.texcoord = texcoord;                                           \n"
    "                                                                          \n"
    "    // Pass the per-vertex normal so that the fragment shader can         \n"
    "    // turn the normal map on and off.                                    \n"
    "    vs_out.normal = N;                                                    \n"
    "                                                                          \n"
    "    // Calculate clip coordinates by multiplying our view position by     \n"
    "    // the projection matrix.                                             \n"
    "    gl_Position = proj_matrix * P;                                        \n"
    "}                                                                         \n"
    "                                                                          \n"
};

void My_Init()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

	program = glCreateProgram();
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, normalmapping_fs_glsl, NULL);
	glCompileShader(fs);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, normalmapping_vs_glsl, NULL);
	glCompileShader(vs);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);

	glLinkProgram(program);
	glUseProgram(program);

    uniforms.mv_matrix = glGetUniformLocation(program, "mv_matrix");
    uniforms.proj_matrix = glGetUniformLocation(program, "proj_matrix");
	uniforms.light_pos = glGetUniformLocation(program, "light_pos");
	uniforms.tex_color = glGetUniformLocation(program, "tex_color");
	uniforms.tex_normal = glGetUniformLocation(program, "tex_normal");

	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/ladybug.obj");

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint position_buffer;
	GLuint normal_buffer;
	GLuint tangent_buffer;
	GLuint texcoord_buffer;
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

	// Compute tangent
	float *tangents = new float[shapes[0].mesh.normals.size()];
    for (unsigned int i = 0; i < shapes[0].mesh.indices.size(); i += 3)
    {
        int idx0 = shapes[0].mesh.indices[i];
        int idx1 = shapes[0].mesh.indices[i + 1];
        int idx2 = shapes[0].mesh.indices[i + 2];

		vec3 p1(shapes[0].mesh.positions[idx0 * 3 + 0], shapes[0].mesh.positions[idx0 * 3 + 1], shapes[0].mesh.positions[idx0 * 3 + 2]);
		vec3 p2(shapes[0].mesh.positions[idx1 * 3 + 0], shapes[0].mesh.positions[idx1 * 3 + 1], shapes[0].mesh.positions[idx1 * 3 + 2]);
		vec3 p3(shapes[0].mesh.positions[idx2 * 3 + 0], shapes[0].mesh.positions[idx2 * 3 + 1], shapes[0].mesh.positions[idx2 * 3 + 2]);

		vec3 n1(shapes[0].mesh.normals[idx0 * 3 + 0], shapes[0].mesh.normals[idx0 * 3 + 1], shapes[0].mesh.normals[idx0 * 3 + 2]);
		vec3 n2(shapes[0].mesh.normals[idx1 * 3 + 0], shapes[0].mesh.normals[idx1 * 3 + 1], shapes[0].mesh.normals[idx1 * 3 + 2]);
		vec3 n3(shapes[0].mesh.normals[idx2 * 3 + 0], shapes[0].mesh.normals[idx2 * 3 + 1], shapes[0].mesh.normals[idx2 * 3 + 2]);

		vec2 uv1(shapes[0].mesh.texcoords[idx0 * 2 + 0], shapes[0].mesh.texcoords[idx0 * 2 + 1]);
		vec2 uv2(shapes[0].mesh.texcoords[idx1 * 2 + 0], shapes[0].mesh.texcoords[idx1 * 2 + 1]);
		vec2 uv3(shapes[0].mesh.texcoords[idx2 * 2 + 0], shapes[0].mesh.texcoords[idx2 * 2 + 1]);

		vec3 dp1 = p2 - p1;
		vec3 dp2 = p3 - p1;

		vec2 duv1 = uv2 - uv1;
		vec2 duv2 = uv3 - uv1;

        float r = 1.0f / (duv1[0] * duv2[1] - duv1[1] * duv2[0]);

        vec3 t;
        t[0] = (dp1[0] * duv2[1] - dp2[0] * duv1[1]) * r;
        t[1] = (dp1[1] * duv2[1] - dp2[1] * duv1[1]) * r;
        t[2] = (dp1[2] * duv2[1] - dp2[2] * duv1[1]) * r;

		vec3 t1 = glm::cross(n1, t);
		vec3 t2 = glm::cross(n2, t);
		vec3 t3 = glm::cross(n3, t);

        tangents[idx0 * 3 + 0] += t1[0];
        tangents[idx0 * 3 + 1] += t1[1];
        tangents[idx0 * 3 + 2] += t1[2];

        tangents[idx1 * 3 + 0] += t2[0];
        tangents[idx1 * 3 + 1] += t2[1];
        tangents[idx1 * 3 + 2] += t2[2];

        tangents[idx2 * 3 + 0] += t3[0];
        tangents[idx2 * 3 + 1] += t3[1];
        tangents[idx2 * 3 + 2] += t3[2];
    }
	glGenBuffers(1, &tangent_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, tangent_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.normals.size() * sizeof(float), tangents, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);
	delete tangents;

	glGenBuffers(1, &texcoord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.texcoords.size() * sizeof(float), shapes[0].mesh.texcoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(3);

	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[0].mesh.indices.size() * sizeof(unsigned int), shapes[0].mesh.indices.data(), GL_STATIC_DRAW);
	index_count = shapes[0].mesh.indices.size();

	TextureData diffuse_data = loadPNG("../../Media/Textures/ladybug_co.png");
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, diffuse_data.width, diffuse_data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, diffuse_data.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	delete[] diffuse_data.data;
	TextureData normal_data = loadPNG("../../Media/Textures/ladybug_nm.png");
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, normal_data.width, normal_data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, normal_data.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	delete[] normal_data.data;
}

void My_Display()
{
    static const GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 0.0f };
    static const GLfloat ones[] = { 1.0f };
    glClearBufferfv(GL_COLOR, 0, gray);
    glClearBufferfv(GL_DEPTH, 0, ones);
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

    glUseProgram(program);

    glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, &proj_matrix[0][0]);
    mat4 mv_matrix = translate(mat4(), vec3(0.0f, -0.2f, -5.5f)) *
                     rotate(mat4(), deg2rad(14.5f), vec3(1.0f, 0.0f, 0.0f)) *
                     rotate(mat4(), deg2rad(-20.0f), vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(uniforms.mv_matrix, 1, GL_FALSE, &mv_matrix[0][0]);
	vec3 light_pos = vec3(40.0f * sinf(currentTime), 30.0f + 20.0f * cosf(currentTime), 40.0f);
    glUniform3fv(uniforms.light_pos, 1, &light_pos[0]);
	glUniform1i(uniforms.tex_color, 0);
	glUniform1i(uniforms.tex_normal, 1);

    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(deg2rad(60.0f), viewportAspect, 0.1f, 1000.0f);
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