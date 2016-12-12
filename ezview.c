#define GL_GLEXT_PROTOTYPES
#define GLFW_INCLUDE_ES2 1
#define GLFW_DLL 1

#include <GLFW/glfw3.h>
#include <GLES2/gl2.h>

#include "linmath.h"

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
				fscanf(fh, "%hhd %hhd %hhd", &pixel->r, &pixel->g, &pixel->b);
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
	return (0);
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
				fprintf(outputFile, "%d %d %d ", buffer->data[i*buffer->width*3+j*3], buffer->data[i*buffer->width*3+j*3+1], buffer->data[i*buffer->width*3+j*3+2]);
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



typedef struct Vertex {
	float Position[2];
	float TexCoord[2];
} Vertex;

Vertex Vertexes[] = {
	{ { -1, 1 },{ 0, 0 } },
	{ { 1, 1 },{ 0.99999, 0 } },
	{ { -1, -1 },{ 0, 0.99999 } },
	{ { 1, 1 },{ 0.99999, 0 } },
	{ { 1, -1 },{ 0.99999, 0.99999 } },
	{ { -1, -1 },{ 0, 0.99999 } }
};


static const char* vertex_shader_src =
"uniform mat4 MVP;\n"
"attribute vec2 vPos;\n"
"attribute vec2 TexCoordIn;\n"
"varying vec2 TexCoordOut;\n"
"\n"
"void main(void) {\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    TexCoordOut = TexCoordIn;\n"
"}";


static const char* fragment_shader_src =
"varying lowp vec2 TexCoordOut;\n"
"uniform sampler2D Texture;\n"
"\n"
"void main(void) {\n"
"    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
"}";


static void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}


float scaleTo[2] = { 1.0, 1.0 };
float shearTo[2] = { 0.0, 0.0 };
float translationTo[2] = { 0.0, 0.0 };
float rotationTo = 0;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) {
		// scale
		// scale the image up
		if (key == GLFW_KEY_UP) {
			scaleTo[0] *= 2;
			scaleTo[1] *= 2;
		}
		// scale the image down
		if (key == GLFW_KEY_DOWN) {
			scaleTo[0] /= 2;
			scaleTo[1] /= 2;	
		}
		// scale the x up
		if (key == GLFW_KEY_D) {
			scaleTo[0] *= 2;
		}
		// scale the x down
		if (key == GLFW_KEY_A) {
			scaleTo[0] /= 2;
		}
		// scale the y up
		if (key == GLFW_KEY_W) {
			scaleTo[1] *= 2;
		}
		// scale the y down
		if (key == GLFW_KEY_S) {
			scaleTo[1] /= 2;
		}
	
		// translation
		// translate the x up 
		if (key == GLFW_KEY_L) {
			translationTo[0] += 1;
		}
		// translate the x down
		if (key == GLFW_KEY_J) {
			translationTo[0] -= 1;
		}
		// translate the y up
		if (key == GLFW_KEY_I) {
			translationTo[1] += 1;
		}
		// translate the y down;
		if (key == GLFW_KEY_K) {
			translationTo[1] -= 1;
		}

		// shear
		// shear the x up
		if (key == GLFW_KEY_M) {
			shearTo[0] += 1;
		}
		// shear the x down
		if (key == GLFW_KEY_N) {
			shearTo[0] -= 1;
		}
		// shear the y up
		if (key == GLFW_KEY_Y) {
			shearTo[1] += 1;
		}
		// shear the y down
		if (key == GLFW_KEY_U) {
			shearTo[1] -= 1;
		}

		// rotation
		// counterclockwise rotation
		if (key == GLFW_KEY_C) {
			rotationTo += 0.5;
		}
		// clockwise rotation
		if (key == GLFW_KEY_Z) {
			rotationTo -= 0.5;
		}


		// reset all values
		if (key == GLFW_KEY_R){
			scaleTo[0] = 1;
			scaleTo[1] = 1;
			translationTo[0] = 0;
			translationTo[1] = 0;
			shearTo[0] = 0;
			shearTo[1] = 0;
			rotationTo = 0;
		}

		// close the window
		if (key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(window, 1);
		}

	}
}

void glCompileShaderOrDie(GLuint shader) {
	GLint compiled;
	glCompileShader(shader);
	glGetShaderiv(shader,
		GL_COMPILE_STATUS,
		&compiled);
	if (!compiled) {
		GLint infoLen = 0;
		glGetShaderiv(shader,
			GL_INFO_LOG_LENGTH,
			&infoLen);
		char* info = malloc(infoLen + 1);
		GLint done;
		glGetShaderInfoLog(shader, infoLen, &done, info);
		printf("Unable to compile shader: %s\n", info);
		exit(1);
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
	GLuint program_id, vertex_shader, fragment_shader;
	GLuint vertex_buffer;
	GLint mvp_location, vpos_location, texcoord_location, texture_location;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		return -1;

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
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
	glfwSwapInterval(1);

	program_id = glCreateProgram();
	

	// mapping the c side vertex data to an internal OpenGl representation
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertexes), Vertexes, GL_STATIC_DRAW);


	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
	glCompileShaderOrDie(vertex_shader);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
	glCompileShaderOrDie(fragment_shader);

	glAttachShader(program_id, vertex_shader);
	glAttachShader(program_id, fragment_shader);
	glLinkProgram(program_id);

	
	mvp_location = glGetUniformLocation(program_id, "MVP");
	assert(mvp_location != -1);
	
	vpos_location = glGetAttribLocation(program_id, "vPos");
	assert(vpos_location != -1);

	texcoord_location = glGetAttribLocation(program_id, "TexCoordIn");
	assert(texcoord_location != -1);

	texture_location = glGetUniformLocation(program_id, "Texture");
	assert(texture_location != -1);
	
	glEnableVertexAttribArray(vpos_location);
	glEnableVertexAttribArray(texcoord_location);

	glVertexAttribPointer(vpos_location,
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
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, buffer->width, buffer->height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer->data);



	while (!glfwWindowShouldClose(window)) {
		int bufferWidth, bufferHeight;
		glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);

		float ratio;
		mat4x4 s, sh, t, r, p, ssh, ssht, mvp;

		ratio = bufferWidth / (float)bufferHeight;

		glViewport(0, 0, bufferWidth, bufferHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		// scale the matrix
		mat4x4_identity(s);
		s[0][0] = s[0][0] * scaleTo[0];
		s[1][1] = s[1][1] * scaleTo[1];
		
		// shear the matrix
		mat4x4_identity(sh);
		sh[0][1] = shearTo[0];
		sh[1][0] = shearTo[1];
		

		// translate the matrix
		mat4x4_identity(t);
		mat4x4_translate(t, translationTo[0], translationTo[1], 0);

		// rotate the matrix
		mat4x4_identity(r);
		mat4x4_rotate_Z(r, r, rotationTo);


		mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		mat4x4_mul(ssh, s, sh);
		mat4x4_mul(ssht, ssh, t);
		mat4x4_mul(mvp, ssht, r);

		
		
		glUseProgram(program_id);
		glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)mvp);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
