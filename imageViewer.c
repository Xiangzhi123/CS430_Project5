#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>
#define GLFW_INCLUDE_ES2 1
#define GFLW_DLL 1

#include "../deps/linmath.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// create a stuct that represents a single pixel, same as what we did in class
typedef struct PPMRGBpixel {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} PPMRGBpixel;

// create struct that represents a single image
typedef struct PPMimage {
	int width;
	int height;
	int maxColorValue;
	unsigned char *data;
} PPMimage;

// the buffer is used to store image data and some header data from ppm file
// so that I could use the data from buffer when we write into output file
PPMimage *buffer;

PPMimage PPMRead(char *inputFilename);
int PPMWrite(char *outPPMVersion, char *outputFilename);
int PPMDataWrite(char ppmVersionNum, FILE *outputFile);

// the PPMRead function is used to read data from ppm file
PPMimage PPMRead(char *inputFilename) {
        // allocate the memory for buffer
	buffer = (PPMimage*)malloc(sizeof(PPMimage));
	FILE* fh = fopen(inputFilename, "rb");
        // check whether the file open successfully. If not, show the error message
	if (fh == NULL) {
		fprintf(stderr, "Error: open the file unsuccessfully. \n");
		exit(1);
	}
	int c = fgetc(fh);            // get the first character of the file
        // check the file type, if the file does not start with the 'P'
        // then the file is not a ppm file, and show th error message
	if (c != 'P') {
		fprintf(stderr, "Error: incorrect input file formart, the file should be a PPM file. \n");
		exit(1);
	}
	c = fgetc(fh);              // get the second character of the file, which should be a number with char type
        // save the ppm version number as a char, I tried to save the ppmVersionNum as an integer
        // but it converts to the ascii value. Thus, I figured out that saving as a character would be a better idea
	char ppmVersionNum = c;
	// if ppm version number is not 3 and 6, show the error message
	if (ppmVersionNum != '3' && ppmVersionNum != '6') {
		fprintf(stderr, "Error: invalid magic number, the ppm version should be either P3 or P6. \n");
		exit(1);
	}

	// since I want to go to the next line, so I check the new line character.
	while (c != '\n') {
		c = fgetc(fh);
	}
	// once I get the new line character, get the first character of next line
	// and check if the first character is '#', skip comments if so
	c = fgetc(fh);
	while (c == '#') {
		while (c != '\n') {
			c = fgetc(fh);
		}
		c = fgetc(fh);
	}

	// After I skip comments, I would like to do fscanf here, but the first digit of the width
	// might not include in the width, so I want to go back to the previous character
	ungetc(c, fh);

	/* I store all the header data into buffer here, my basic idea is using fscanf to save
	width, height, and maxvalue. Also, I would like to check if the file has correct number of width, height and
	max color value. For instance, some files might havce 2 max color value, which should be considered as invalid file format */
	int wh = fscanf(fh, "%d %d", &buffer->width, &buffer->height);
	if (wh != 2) {
		fprintf(stderr, "Error: the size of image has to include width and height, invalid data for image. \n");
		exit(1);
	}
	int mcv = fscanf(fh, "%d", &buffer->maxColorValue);
	if (mcv != 1) {
		fprintf(stderr, "Error: the max color value has to be one single value. \n");
		exit(1);
	}
	if (buffer->maxColorValue != 255) {
		fprintf(stderr, "Error: the image has to be 8-bit per channel. \n");
		exit(1);
	}

	/* I did buffer->width * buffer->height * sizeof(PPMRGBpixel), so that each (buffer->width*buffer->height) number
	of pixel memory will be allocated. Also, each pixel includes r, g, b. The reason I did it is that it makes me easier to read
	every single element of body data */
	buffer->data = (unsigned char*)malloc(buffer->width*buffer->height*sizeof(PPMRGBpixel));
	if (buffer == NULL) {
		fprintf(stderr, "Error: allocate the memory unsuccessfully. \n");
		exit(1);
	}

	/* if the ppm version number is 3, then do the p3 reading. What I did is building two nested for loops so that
	the program will go through each element of body data. After that read all data into pixels in r, g, b respectively.
	Then, I could save those data into buffer->data */
	if (ppmVersionNum == '3') {
		int i, j;
		for (i = 0; i<buffer->height; i++) {
			for (j = 0; j<buffer->width; j++) {
				PPMRGBpixel *pixel = (PPMRGBpixel*)malloc(sizeof(PPMRGBpixel));
				fscanf(fh, "%d %d %d", &pixel->r, &pixel->g, &pixel->b);
				buffer->data[i*buffer->width * 3 + j * 3] = pixel->r;
				buffer->data[i*buffer->width * 3 + j * 3 + 1] = pixel->g;
				buffer->data[i*buffer->width * 3 + j * 3 + 2] = pixel->b;
			}
		}
	}
	// p6 reading, basically just save the whole body data into buffer->data directly
	else if (ppmVersionNum == '6') {
		// read the pixel data, the type size_t might be used
		size_t s = fread(buffer->data, sizeof(PPMRGBpixel), buffer->width*buffer->height, fh);
		if (s != buffer->width*buffer->height) {
			fprintf(stderr, "Error: read size and real size are not match");
			exit(1);
		}
	}
	else {
		fprintf(stderr, "Error: the ppm version cannot be read. \n");
		exit(1);
	}
	fclose(fh);
	// Since the function expects a PPMimage return value, so I return *buffer here.
	return *buffer;
}

// this function writes the header data from buffer to output file
int PPMWrite(char *outPPMVersion, char *outputFilename) {
	int width = buffer->width;
	int height = buffer->height;
	int maxColorValue = buffer->maxColorValue;
	char ppmVersionNum = outPPMVersion[1];
	FILE *fh = fopen(outputFilename, "wb");
	if (fh == NULL) {
		fprintf(stderr, "Error: open the file unscuccessfully. \n");
		return (1);
	}
	char *comment = "# output.ppm";
	fprintf(fh, "P%c\n%s\n%d %d\n%d\n", ppmVersionNum, comment, width, height, 255);
	// call the PPMDataWrite function which writes the body data
	PPMDataWrite(ppmVersionNum, fh);
	fclose(fh);
}

// this function writes the body data from buffer->data to output file
int PPMDataWrite(char ppmVersionNum, FILE *outputFile) {
	// write image data to the file if the ppm version is P6
	if (ppmVersionNum == '6') {
		// using fwrite to write data, basically it just like copy and paste data for P6
		fwrite(buffer->data, sizeof(PPMRGBpixel), buffer->width*buffer->height, outputFile);
		printf("The file saved successfully! \n");
		return (0);
	}
	// write image data to the file if the ppm version is p3
	else if (ppmVersionNum == '3') {
		int i, j;
		for (i = 0; i < buffer->height; i++) {
			for (j = 0; j < buffer->width; j++) {
				// similar thing as we did in reading body data for P3, but we use fprintf here to write data.
				fprintf(outputFile, "%d %d %d ", buffer->data[i*buffer->width*3+j*3], buffer->data[i*buffer->width+j*3+1], buffer->data[i*buffer->width*3+2]);
			}
			fprintf(outputFile, "\n");
		}

		printf("The file saved successfully! \n");
		return (0);
	}
	else {
		fprintf(stderr, "Error: incorrect ppm version. \n");
		return (1);
	}
}



static void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

typedef struct Vertex {
	float position[2];
	float TexCoord[2];
} Vertex;

Vertex Vertexes[] = {
	{ { 1, -1, 0 }, {0.99999, 0} },
	{ { 1, 1, 0 }, {0.99999 , 0.99999}},
	{ { -1, 1, 0 }, {0, 0.99999}},
	{ { -1, -1, 0 }, {0, 0}}
};

const GLubyte Indices[] = {
	0, 1, 2,
	2, 3, 0
};

char* vertex_shader_src =
"attribute vec2 Position;\n"
"attribute vec2 TexCoordIn;\n"
"uniform vec2 Scale;\n"
"uniform vec2 Translation;"
"uniform vec2 Shear;\n"
"uniform float Rotation;\n"
"varying vec2 TexCoordOut;\n"
"mat4 RotationMatrix = mat4( cos(Rotation), -sin(Rotation), 0.0, 0.0,\n"
"                            sin(Rotation),  cos(Rotation), 0.0, 0.0,\n"
"                            0.0,            0.0,           1.0, 0.0,\n"
"                            0.0,            0.0,           0.0, 1.0 );\n"
"\n"
"mat4 TranslationMatrix = mat4(1.0, 0.0, 0.0, Translation.x,\n"
"                              0.0, 1.0, 0.0, Translation.y,\n"
"                              0.0, 0.0, 1.0, 0.0,\n"
"                              0.0, 0.0, 0.0, 1.0 );\n"
"\n"
"mat4 ScaleMatrix = mat4(Scale.x, 0.0,     0.0, 0.0,\n"
"                        0.0,     Scale.y, 0.0, 0.0,\n"
"                        0.0,     0.0,     1.0, 0.0,\n"
"                        0.0,     0.0,     0.0, 1.0 );\n"
"\n"
"mat4 ShearMatrix = mat4(1.0,     Shear.x, 0.0, 0.0,\n"
"                        Shear.y, 1.0,     0.0, 0.0,\n"
"                        0.0,     0.0,     1.0, 0.0,\n"
"                        0.0,     0.0,     0.0, 1.0 );\n"
"\n"
"void main(void) {\n"
"    gl_Position = Position*ScaleMatrix*ShearMatrix*RotationMatrix*TranslationMatrix;\n"
"    TexCoordOut = TexCoordIn;\n"
"}";


char* fragment_shader_src =
"varying vec2 TexCoordOut;\n"
"uniform sampler2D Texture;\n"
"\n"
"void main(void) {\n"
"    gl_FragColor = texture2D(Texture, DestinationTexcoord);\n"
"}";


GLuint simple_shader(GLint shader_type, char* shader_src) {
	GLint compile_success = 0;

	GLuint shader_id = glCreateShader(shader_type);

	glShaderSource(shader_id, 1, &shader_src, 0);

	prinf("compiling shader...");

	glCompileShader(shader_id);

	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_success);

	// If it failed print an error
	if (compile_success == GL_FALSE) {
		GLchar message[256];
		glGetShaderInfoLog(shader_id, sizeof(message), 0, &message[0]);
		printf("glCompileShader Error: %s\n", message);
		exit(1);
	}

	return shader_id;
}


int simple_program() {

	GLint link_success = 0;

	GLuint program_id = glCreateProgram();
	// Compile the shaders
	GLuint vertex_shader = simple_shader(GL_VERTEX_SHADER, vertex_shader_src);
	GLuint fragment_shader = simple_shader(GL_FRAGMENT_SHADER, fragment_shader_src);

	glAttachShader(program_id, vertex_shader);
	glAttachShader(program_id, fragment_shader);

	glLinkProgram(program_id);

	glGetProgramiv(program_id, GL_LINK_STATUS, &link_success);

	if (link_success == GL_FALSE) {
		GLchar message[256];
		glGetProgramInfoLog(program_id, sizeof(message), 0, &message[0]);
		printf("glLinkProgram Error: %s\n", message);
		exit(1);
	}

	return program_id;
}

static void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}


float scaleTo = { 1.0, 1.0 };
float Scale[] = { 1.0, 1.0 };
float shearTo[] = { 0.0, 0.0 };
float Shear[] = { 0.0, 0.0 };
float translationTo[] = { 0.0, 0.0 };
float Translation[] = { 0, 0 };
float rotationTo = 0;
float Rotation = 0;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) {
		// scale
		// scale the image up
		if (key == GLFW_KEY_UP) {
			scaleTo[0] *= 2;
			scaleTo[1] *= 2;
		}
		// scale the image down
		else if (key == GLFW_KEY_DOWN) {
			scaleTo[0] /= 2;
			scaleTo[1] /= 2;
			
		}
		// scale the x up
		else if (key == GLFW_KEY_D) {
			scaleTo[0] *= 2;
		}
		// scale the x down
		else if (key == GLFW_KEY_A) {
			scaleTo[0] /= 2;
		}
		// scale the y up
		else if (key == GLFW_KEY_W) {
			scaleTo[1] *= 2;
		}
		// scale the y down
		else if (key == GLFW_KEY_S) {
			scaleTo[1] /= 2;
		}
	
		// translation
		// translate the x up 
		else if (key == GLFW_KEY_L) {
			translationTo[0] += 1;
		}
		// translate the x down
		else if (key == GLFW_KEY_J) {
			translationTo[0] -= 1;
		}
		// translate the y up
		else if (key == GLFW_KEY_K) {
			translationTo[1] += 1;
		}
		// translate the y down;
		else if (key == GLFW_KEY_I) {
			translationTo[1] -= 1;
		}

		// shear
		// shear the x up
		else if (key == GLFW_KEY_M) {
			shearTo[0] += 1;
		}
		// shear the x down
		else if (key == GLFW_KEY_N) {
			shearTo[0] -= 1;
		}
		// shear the y up
		else if (key == GLFW_KEY_Y) {
			shearTo[1] += 1;
		}
		// shear the y down
		else if (key == GLFW_KEY_U) {
			shearTo[1] -= 1;
		}

		// rotation
		// counterclockwise rotation
		else if (key == GLFW_KEY_C) {
			RotationTo += 0.1;
		}
		// clockwise rotation
		else if (key == GLFW_KEY_Z) {
			RotationTo -= 0.1;
		}


		// reset all values
		else if (key == GLFW_KEY_R){
			scaleTo[0] = 1;
			scaleTo[1] = 1;
			translationTo[0] = 0;
			translationTo[1] = 0;
			shearTo[0] = 0;
			shearTo[1] = 0;
			rotationTo = 0;
		}

		// close the window
		else if (key == GLFW_KEY_ESCAPE) {
			glfwWindowShouldClose(window, 1);
		}

	}
}



int main(int argc, char *argv[]) {
	// check the number of input arguments
	if (argc != 2) {
		fprintf(stderr, "Error: Wrong format\n");
		return 1;
	}

	char *fileName = argv[1];
	PPMRead(fileName);

	GLFWwindow* window;
	GLuint program_id, vertex_shader, fragment_shader, position, translation_location;
	GLuint scale_location, rotation_location, shear_location''
	GLuint texcoord_slot;
	GLuint index_buffer, vertex_buffer;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		return -1;

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	// Create and open a window named 'Project 5!!!'
	window = glfwCreateWindow(640, 480, "Project 5!!!", NULL, NULL);

	if (!window) {
		glfwTerminate();
		exit(1);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);

	program_id = simple_program();

	glUseProgram(program_id);

	position = glGetAttribLocation(program_id, "Position");
	assert(position != -1);
	texcoord_location = glGetAttribLocation(program_id, "TexCoordIn");
	assert(texcoord_location != -1);
	scale_location = glGetUniformLocation(program_id, "Scale");
	assert(scale_location != -1);
	translation_location = glGetUniformLocation(program_id, "Translation");
	assert(translation_location != -1);
	rotation_location = glGetUniformLocation(program_id, "Rotation");
	assert(roration_location != -1);
	shear_location = glGetUniformLocation(program_id, "Shear");
	assert(shear_location != -1);
	glEnableVertexAttribArray(position_location);
	glEnableVertexAttribArray(texcoord_location);

	// mapping the c side vertex data to an internal OpenGl representation
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertexes), Vertexes, GL_STATIC_DRAW);

	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);


	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(fragment_shader, 1, &fragment)

	glVertexAttribPointer(position,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		0);

	glVertexAttribPointer(texcoord_location,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		(GLvoid*)(sizeof(float) * 2));


	GLuint texID;
	PPMimage image = PPMRead(fileName);
	width = image.width;
	height = image.height;
	data = image.data;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widht, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	


	while (!glfwWindowShouldClose(window)) {
		int bufferWidth, bufferHeight;
		glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);

		Scale[0] = ;
		Scale[1] = ;
		Translation[0] = ;
		Translation[1] = ;
		Shear[0] = ;
		Shear[1] = ;
		Rotation += rotationTo;

		// update the value 
		glUniform2f(scale_location, Scale[0], Scale[1]);
		glUniform2f(translation_location, Translation[0], Translation[1]);
		glUniform2f(shear_location, Shear[0], Shear[1]);
		glUniform1f(rotation_location, Rotation);

		glViewport(0, 0, bufferWidth, bufferHeight);
		glClearColor(0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		glDrawElements(GL_TRIANGLES,
			sizeof(Indices) / sizeof(GLubyte),
			GL_UNSIGNED_BYTE, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}