#include <stdio.h>
#include <iostream>
#include <new>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <string>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "file_utils.h"
#include "math_utils.h"

/********************************************************************/
/*   Variables */

string theProgramTitle = "Protein Viewer By Vysakh P R";
int theWindowWidth = 1260, theWindowHeight = 1000;
int theWindowPositionX = 40, theWindowPositionY = 40;
bool isFullScreen = false;
bool isAnimating = true;
float rotation = 0.0f;
GLuint VBO, VAO, IBO;
GLuint gWorldLocation;
int numberOfVertices, numberOfPolygons, numberOfEdges, numberOfSides;
float PolygonsPerSecond;
//int *Indices;

/* Constants */
const int ANIMATION_DELAY = 10; /* milliseconds between rendering */
const char* pVSFileName = "shader.vs";
const char* pFSFileName = "shader.fs";

/********************************************************************
  Utility functions
 */

/* post: compute frames per second and display in window's title bar */
void computeFPS() {
	static int frameCount = 0;
	static int lastFrameTime = 0;
	static char * title = NULL;
	int currentTime;

	if (!title)
		title = (char*) malloc((strlen(theProgramTitle.c_str()) + 40) * sizeof (char));
	frameCount++;
	currentTime = glutGet((GLenum) (GLUT_ELAPSED_TIME));
	if (currentTime - lastFrameTime > 1000) {
		sprintf(title, "%s [ FPS: %4.2f ] [ PPS: %4.2f ]",
			theProgramTitle.c_str(),
			frameCount * 1000.0 / (currentTime - lastFrameTime),
			PolygonsPerSecond);
		glutSetWindowTitle(title);
		lastFrameTime = currentTime;
		frameCount = 0;
	}
}

static void CreateVertexAndIndexBuffer() {
	//glGenVertexArrays(1, &VAO);
	//cout << "VAO: " << VAO << endl;
	//glBindVertexArray(VAO);


	ifstream offobject;
	offobject.open("protein_database/1GRM_16.off");
	//offobject.open("protein_database/2OAR_1.off");
	//offobject.open("protein_database/3SY7_1.off");
	string type;
	offobject >> type;	
	if(type.compare("OFF")!=0)
	{
		cout<<"NOT AN OFF FILE"<< endl;
		exit(1);
	}
	int i;
	float x,y,z;
	offobject >> numberOfVertices;
	offobject >> numberOfPolygons; 
	offobject >> numberOfEdges;

	Vector3f Vertices[numberOfVertices];
	for (i = 0; i < numberOfVertices; i++)
	 {
	 	offobject >>x >>y >>z;
	 	Vertices[i]=Vector3f(x,y,z);
	 }
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof (Vertices), Vertices, GL_STATIC_DRAW);

	 int* Indices = NULL;
	 Indices=new int[numberOfPolygons*3];
	 if (Indices==NULL){
	 	cout << "Error: Memory couldnot be allocated"<< endl;
	 	exit(1);
	 }
	 for (i=0;i < numberOfPolygons*3; i+=3)
	 {
	 	offobject >> numberOfSides >>x >>y >> z;
	 	Indices[i]=x;
	 	Indices[i+1]=y;
	 	Indices[i+2]=z;
	 }
	glGenBuffers(1,&IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*numberOfPolygons*3, Indices, GL_STATIC_DRAW);



	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,IBO);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	delete[] Indices;
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType) {
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}

	const GLchar * p[1];
	p[0] = pShaderText;
	GLint Lengths[1];
	Lengths[0] = strlen(pShaderText);
	glShaderSource(ShaderObj, 1, p, Lengths);
	glCompileShader(ShaderObj);
	GLint success;
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}

	glAttachShader(ShaderProgram, ShaderObj);
}

using namespace std;

static void CompileShaders() {
	GLuint ShaderProgram = glCreateProgram();

	if (ShaderProgram == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	string vs, fs;

	if (!ReadFile(pVSFileName, vs)) {
		exit(1);
	}

	if (!ReadFile(pFSFileName, fs)) {
		exit(1);
	}

	AddShader(ShaderProgram, vs.c_str(), GL_VERTEX_SHADER);
	AddShader(ShaderProgram, fs.c_str(), GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = {0};

	glLinkProgram(ShaderProgram);
	glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(ShaderProgram, sizeof (ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	glValidateProgram(ShaderProgram);
	glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(ShaderProgram, sizeof (ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	glUseProgram(ShaderProgram);
	gWorldLocation = glGetUniformLocation(ShaderProgram, "gWorld");
}

/********************************************************************
 Callback Functions
 These functions are registered with the glut window and called 
 when certain events occur.
 */

void onInit(int argc, char * argv[])
/* pre:  glut window has been initialized
   post: model has been initialized */ {
	/* by default the back ground color is black */
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	CreateVertexAndIndexBuffer();
	CompileShaders();

	/* set to draw in window based on depth  */
	glEnable(GL_DEPTH_TEST); 
}

static void onDisplay() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glPointSize(5);

	static float Scale=0.0f;

	Scale+=0.001f;
	Matrix4f World;

	World.m[0][0] = cosf(rotation); World.m[0][1] = 0.0f;		   World.m[0][2] = -sinf(rotation); World.m[0][3] = 0.0f;
	World.m[1][0] = 0.0f; 		 World.m[1][1] = 1.0f; 		   World.m[1][2] = 0.0f; World.m[1][3] = 0.0f;
	World.m[2][0] = sinf(rotation); World.m[2][1] = 0.0f;         World.m[2][2] = cosf(rotation); World.m[2][3] = 0.0f;
	World.m[3][0] = 0.0f;        World.m[3][1] = 0.0f;         World.m[3][2] = 0.0f; World.m[3][3] = 1.0f;

	glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &World.m[0][0]);

	//glEnableVertexAttribArray(0);
	//glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,IBO);
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	static int StartTime=0;
	int FinishTime;
	glDrawElements(GL_TRIANGLES,numberOfPolygons*3,GL_UNSIGNED_INT,0);
	glBindVertexArray(0);

	static int numberOfPolygonsRendered=0;
	numberOfPolygonsRendered+=numberOfPolygons;
	FinishTime=glutGet((GLenum) (GLUT_ELAPSED_TIME));
	if((FinishTime - StartTime)>20)
	{	
		PolygonsPerSecond=(numberOfPolygonsRendered*1000)/(FinishTime - StartTime);
		numberOfPolygonsRendered=0;
		StartTime=FinishTime;
	}

	//glDisableVertexAttribArray(0);

	/* check for any errors when rendering */
	GLenum errorCode = glGetError();
	if (errorCode == GL_NO_ERROR) {
		/* double-buffering - swap the back and front buffers */
		glutSwapBuffers();
	} else {
		fprintf(stderr, "OpenGL rendering error %d\n", errorCode);
	}
}


static void onIdle(){
	static int oldTime = 0;
        if (isAnimating) {
                int currentTime = glutGet((GLenum) (GLUT_ELAPSED_TIME));
                /* Ensures fairly constant framerate */
                if (currentTime - oldTime > ANIMATION_DELAY) {
                        // do animation....
                        rotation += 0.001;

                        oldTime = currentTime;
                        /* compute the frame rate */
                        computeFPS();
                        /* notify window it has to be repainted */
                        glutPostRedisplay();
                }
        }

}

/* pre:  glut window has been resized
 */
static void onReshape(int width, int height) {
	glViewport(0, 0, width, height);
	if (!isFullScreen) {
		theWindowWidth = width;
		theWindowHeight = height;
	}
	// update scene based on new aspect ratio....
}

/* pre:  glut window is not doing anything else
   post: scene is updated and re-rendered if necessary */

static void InitializeGlutCallbacks() {
	/* tell glut how to display model */
	glutDisplayFunc(onDisplay);
	/* tell glutwhat to do when it would otherwise be idle */
	glutIdleFunc(onIdle);
	/* tell glut how to respond to changes in window size */
	glutReshapeFunc(onReshape);
	/* tell glut how to handle changes in window visibility */
	//glutVisibilityFunc(onVisible);
	/* tell glut how to handle key presses */
	//glutKeyboardFunc(onAlphaNumericKeyPress);
	//glutSpecialFunc(onSpecialKeyPress);
	/* tell glut how to handle the mouse */
	//glutMotionFunc(onMouseMotion);
	//glutMouseFunc(onMouseButtonPress);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);

	/* request initial window size and position on the screen */
	glutInitWindowSize(theWindowWidth, theWindowHeight);
	glutInitWindowPosition(theWindowPositionX, theWindowPositionY);
	/* request full color with double buffering and depth-based rendering */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
	/* create window whose title is the name of the executable */
	glutCreateWindow(theProgramTitle.c_str());

	InitializeGlutCallbacks();

	// Must be done after glut is initialized!
	GLenum res = glewInit();
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	printf("GL version: %s\n", glGetString(GL_VERSION));

	/* initialize model */
	onInit(argc, argv);

	/* give control over to glut to handle rendering and interaction  */
	glutMainLoop();

	/* program should never get here */

	return 0;
}

