// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <sstream>
#include <iostream>
using namespace std;

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;

// Include AntTweakBar
#include <AntTweakBar.h>

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <time.h>

// CONSTANTS
const int ncpoints = 6;


// RUNTIME VARIABLES
float cpoints[ncpoints][3];				// colored points [# of points][x, y, z]
float cpointsColor[ncpoints][3];		// colored points array [# of points] [r, g, b]


typedef struct {
	float XYZW[4];
	float RGBA[4];
} Vertex;

Vertex Vertices[ncpoints];

// FUNCTIONS
void initializeVertices();				// set points to random colors
float frand();
void initializeColor();
void idToRGB(int i, float &r, float &g, float &b);
float frand();

int main( void )
{
	srand(time(NULL));	// do this or all the random numbers will be the exact same
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Misc 05 - Simple but slow version", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Initialize the GUI
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(1024, 768);
	TwBar * GUI = TwNewBar("Picking");
	TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	std::string message;
	TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING, &message, NULL);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetCursorPos(window, 1024/2, 768/2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 ProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	//glm::mat4 ProjectionMatrix = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	glm::mat4 ViewMatrix = glm::lookAt(
		glm::vec3(0, 0, -5), // Camera is at (4,3,3), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
		);

	initializeVertices();
	/*
	// Define points
	
	Vertex Vertices[] =
	{	// xyzw
		{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 0
		{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }, // 1
		{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }, // 2
		{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }, // 3
	};
	*/

	unsigned short Indices[] = {
		0, 1, 2, 3, 4, 5
	};

	const size_t BufferSize = sizeof(Vertices);
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t RgbOffset = sizeof(Vertices[0].XYZW);
	const size_t IndexCount = sizeof(Indices) / sizeof(unsigned short);

	float pickingColor[IndexCount] = { 0 / 255.0f, 1 / 255.0f, 2 / 255.0f, 3 / 255.0f, 4 / 255.0f, 5 / 255.0f };		// set this procedurally for greater number of points

	// Create Vertex Array Object
	GLuint VertexArrayId;
	glGenVertexArrays(1, &VertexArrayId);
	glBindVertexArray(VertexArrayId);

	// Create and load Buffer for vertex data
	GLuint VertexBufferId;
	glGenBuffers(1, &VertexBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId);
	glBufferData(GL_ARRAY_BUFFER, BufferSize, Vertices, GL_STATIC_DRAW);

	// Create Buffer for indices
	GLuint IndexBufferId;
	glGenBuffers(1, &IndexBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
	
	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");
	GLuint pickingProgramID = LoadShaders("Picking.vertexshader", "Picking.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
	GLuint PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	// Get a handle for our "pickingColorID" uniform
	GLuint pickingColorArrayID = glGetUniformLocation(pickingProgramID, "PickingColorArray");
	GLuint pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");
	// Get a handle for our "LightPosition" uniform
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;

	do{
		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1sec ago
			// printf and reset
			printf("%f ms/frame\n", 1000.0/double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

		// PICKING IS DONE HERE
		// (Instead of picking each frame if the mouse button is down, 
		// you should probably only check if the mouse button was just released)
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)){

			// Clear the screen in white
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUseProgram(pickingProgramID);
			{
				// Only the positions are needed (not the colors)
				glEnableVertexAttribArray(0);

				glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
				glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

				// Send our transformation to the currently bound shader, in the "MVP" uniform
				glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);

				glUniform1fv(pickingColorArrayID, IndexCount, pickingColor);	// here we pass in the picking marker array

				// 1rst attribute buffer : vertices
				glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId);
				// Assign vertex attributes
				glVertexAttribPointer(
					0,                  // attribute
					4,                  // size
					GL_FLOAT,           // type
					GL_FALSE,           // normalized?
					VertexSize,         // stride
					(void*)0            // array buffer offset
					);

				// Index buffer
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId);

				// Draw the ponts
				glEnable(GL_PROGRAM_POINT_SIZE);
				glDrawElements(
					GL_POINTS,      // mode
					IndexCount,    // count
					GL_UNSIGNED_SHORT,   // type
					(void*)0           // element array buffer offset
					);

				glDisableVertexAttribArray(0);

				// Wait until all the pending drawing commands are really done.
				// Ultra-mega-over slow ! 
				// There are usually a long time between glDrawElements() and
				// all the fragments completely rasterized.
				glFlush();
				glFinish();

				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				// Read the pixel at the center of the screen.
				// You can also use glfwGetMousePos().
				// Ultra-mega-over slow too, even for 1 pixel, 
				// because the framebuffer is on the GPU.
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				unsigned char data[4];
				// reads colors
				glReadPixels(xpos, ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
				
				// Convert the color back to an integer ID
				int pickedID = int(data[0]);

				/* 
					vertices[pickedID].xyz[0] = xpos/width;
				
				*/

				if (pickedID == 255){ // Full white, must be the background !
					message = "background";
				}
				else{
					std::ostringstream oss;
					oss << "point " << pickedID;
					message = oss.str();
				}

			}
			// Uncomment these lines to see the picking shader in effect
			//glfwSwapBuffers(window);
			//continue; // skips the normal rendering
		}


		// Dark black background
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		// Re-clear the screen for real rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// Use our shader
		glUseProgram(programID);
		{
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);

			glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

			glm::vec3 lightPos = glm::vec3(4, 4, 4);
			glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);


			// 1rst attribute buffer : vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId);
			glVertexAttribPointer(
				0,                  // attribute
				4,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				VertexSize,         // stride
				(void*)0            // array buffer offset
				);

			// 2nd attribute buffer : colors
			glVertexAttribPointer(
				1,                  // attribute
				4,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				VertexSize,         // stride
				(GLvoid*)RgbOffset  // array buffer offset
				);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId);
			
			// Draw the triangles !
			glEnable(GL_PROGRAM_POINT_SIZE);
			glDrawElements(
				GL_POINTS,      // mode
				IndexCount,    // count
				GL_UNSIGNED_SHORT,   // type
				(void*)0           // element array buffer offset
				);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
		}
		// Draw GUI
		TwDraw();

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &VertexBufferId);
	glDeleteBuffers(1, &IndexBufferId);
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);
	glDeleteVertexArrays(1, &VertexArrayId);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

/* End of Main, Start of Functions */
float frand(){
	float x = (rand() % 1000) / 1000.0f;
	return (x * 2 - 1);
}
// Set up runtime variables

	void initializeVertices(){
		float one = 1.0f;
		initializeColor();
		for (int i = 0; i < ncpoints; ++i) {
			Vertices[i] = { { frand(), frand(), 0.0f, one },
			{ cpointsColor[i][0], cpointsColor[i][1], cpointsColor[i][2] , one } };
		}

		
		
	}
		
	void idToRGB(int i, float &r, float &g, float &b) {
		// Convert "i", the integer mesh ID, into an RGB color
		r = i & (0x000000FF >> 0);
		g = i & (0x0000FF00 >> 8);
		b = i & (0x00FF0000 >> 16);
	}

	void initializeColor(){
		// spread points on the screen
		// x,y,z
		
		float red, green, blue;
		float colorTotal = 255.0;
		for (int i = 0; i < ncpoints; i++){
			cpointsColor[i][0] = rand() % 255 / colorTotal;
			cpointsColor[i][1] = rand() % 255 / colorTotal;
			cpointsColor[i][2] = rand() % 255 / colorTotal;
		}
	}