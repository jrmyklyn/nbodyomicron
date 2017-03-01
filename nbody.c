#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <unistd.h>
#include <string.h>
#include "csvparser.h"

int n = 2;
double dt = 60 * 60 * 24; // seconds
int steps = 27;
double angle = 0;
double aspectRatio = 0;

#define GRAVITY_CONST 6.67408E-20
#define VIEW_ANGLE M_PI/6

typedef struct {
	double mass, // kg
			radius, // km
			x, y, z, // km
			vx, vy, vz; // km/s
} body_t;

body_t *bodies;

void printBody(body_t b, char *name){
	printf("%s  position=(%.5e, %.5e, %.5e)  velocity=(%.5e, %.5e, %.5e)\n", name, b.x, b.y, b.z, b.vx, b.vy, b.vz);
}

void accelerateBodies() {
	for(int i = 0; i < n; i++) {
		body_t *b1 = &(bodies[i]);
		double accx = 0;
		double accy = 0;
		double accz = 0;
		for(int j = 0; j < n; j++){
			body_t *b2 = &(bodies[j]);
			if (j != i){
				double xdiff = b2->x - b1->x;
				double ydiff = b2->y - b1->y;
				double zdiff = b2->z - b1->z;
				double diff = sqrt(xdiff * xdiff + ydiff * ydiff + zdiff * zdiff);
				double temp = b2->mass / (diff * diff * diff);
				accx += temp * xdiff;
				accy += temp * ydiff;
				accz += temp * zdiff;
			}
		}
		b1->vx += GRAVITY_CONST * accx * dt ;
		b1->vy += GRAVITY_CONST * accy * dt;
		b1->vz += GRAVITY_CONST * accz * dt;
	}
}

void moveBodies() {
	for(int i = 0; i < n; i++) {
		body_t *b = &(bodies[i]);
		b->x += b->vx * dt;
		b->y += b->vy * dt;
		b->z += b->vz * dt;
	}
}

void displayDrawCallback() {
	double maxDistance = 0;
	accelerateBodies();
	moveBodies();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
		glRotated(angle, 0, 0, 1);
		for(int i = 0; i < n; i++) {
			body_t b = bodies[i];
			double distance = sqrt(b.x * b.x + b.y * b.y + b.z * b.z) + b.radius;
			if(distance > maxDistance)
				maxDistance = distance;
			glColor3d(0, 0, 1);
			glBegin(GL_LINES);
				glVertex3f(b.x, b.y, b.z);
				glVertex3f(b.x, b.y, 0);
			glEnd();
			glPushMatrix();
				glTranslated(b.x, b.y, b.z);
				glutSolidSphere(b.radius, 20, 20);
			glPopMatrix();
		}
		glPushMatrix();
			glBegin(GL_LINES);
			glColor3f(1, 0, 0);
			glVertex3f(-maxDistance, 0, 0);
			glVertex3f(maxDistance, 0, 0);
			glColor3f(0, 1, 0);
			glVertex3f(0, -maxDistance, 0);
			glVertex3f(0, maxDistance, 0);
			glColor3f(0, 0, 1);
			glVertex3f(0, 0, -maxDistance);
			glVertex3f(0, 0, maxDistance);
			glEnd();
		glPopMatrix();
	glPopMatrix();
	glutSwapBuffers();
	
	// Set camera angle, height, width, depth
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(aspectRatio < 1)
		glFrustum(-maxDistance, maxDistance, -maxDistance / aspectRatio, maxDistance / aspectRatio, 2 * maxDistance, 4 * maxDistance);
	else
		glFrustum(-maxDistance * aspectRatio, maxDistance * aspectRatio, -maxDistance, maxDistance, 2 * maxDistance, 4 * maxDistance);
	gluLookAt(3 * maxDistance * cos(VIEW_ANGLE), 0, 3 * maxDistance * sin(VIEW_ANGLE), 0, 0, 0, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void displayIdleCallback() {
	angle = fmod(angle + 1, 360);
	glutPostRedisplay();
}

void displayReshapeCallback(int width, int height) {
	glViewport(0, 0, width, height);
	aspectRatio = (double)width / (double)height;
}

int main(int argc, char *argv[]) {
	// Set up parameters
	
	int option;
	char* dataFileName;
	
	if (argc < 2){
		fprintf(stderr, "Use the correct number of args.\n");
		return 1;
	}
	
	dataFileName = (char*)malloc(strlen(argv[1]));
	strcpy(dataFileName, argv[1]);
	
	FILE *dataFile;
	//check for file existence
	dataFile = fopen(dataFileName, "r");
	if (dataFile == NULL){
		fprintf(stderr, "File \'%s\' not found in current directory.\n", dataFileName);
		return 1;
	}
	
	fscanf(dataFile, "%d", &n);
	if (n <= 1) {
		fprintf(stderr, "Boring (or impossible) simulation (n=%d). Aborting.\n", n);
		return 1;
	}
	fclose(dataFile);
	
	bodies = (body_t *)malloc(n * sizeof(body_t));
	
	while((option = getopt(argc, argv, "t:")) != -1) {
		switch(option) {
			case 't':
				dt = atof(optarg);
				break;
			case '?':
				return 1;
			default:
				abort();
		}
	}
	
	// Load Bodies
	int i = 0;
    CsvParser *csvparser = CsvParser_new("planet_data.csv", ",", 1);
    CsvRow *row;
    const CsvRow *header = CsvParser_getHeader(csvparser);

    if (header == NULL) {
        fprintf(stderr, "%s\n", CsvParser_getErrorMessage(csvparser));
        return 1;
    }
	
	/* Uncomment the following if we want headers later
    const char **headerFields = CsvParser_getFields(header);
    for (i = 0 ; i < CsvParser_getNumFields(header) ; i++) {
        printf("TITLE: %s\n", headerFields[i]);
    }
	*/
	i = 0;
    while ((row = CsvParser_getRow(csvparser)) ) {
        const char **rowFields = CsvParser_getFields(row);
        bodies[i].mass = atof(rowFields[1]);
        CsvParser_destroy_row(row);
		i++;
    }
    CsvParser_destroy(csvparser);
		
	// if (n == 10) return 1;
	// bodies[0].x = -2.675244393757571E4;
	// bodies[0].y = -3.991567778823322E5;
	// bodies[0].z = 3.408771797387548E4;
	// bodies[0].vx = 9.735210145581824E-1;
	// bodies[0].vy = -3.777496830110240E-2;
	// bodies[0].vz = -3.756741770019315E-2;
	// bodies[0].mass = 7.34767309E22;
	// bodies[0].radius = 1736;
	// bodies[1].x = 0,
	// bodies[1].y = 0;
	// bodies[1].z = 0;
	// bodies[1].vx = 0;
	// bodies[1].vy = 0;
	// bodies[1].vz = 0;
	// bodies[1].mass = 5.972E24;
	// bodies[1].radius = 6371;
	
	// Setup Display Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glClearColor(1, 1, 1, 1);
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("N-Body Simulation");
	
	// Initialize Display Callbacks
	glutDisplayFunc(displayDrawCallback);
	glutReshapeFunc(displayReshapeCallback);
	glutIdleFunc(displayIdleCallback);
	
	// Setup Lighting
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	
	// Setup Other Display Options
	glEnable(GL_CULL_FACE); // Enabled for efficiency
	glEnable(GL_DEPTH_TEST); // Enabled for the depth buffer
	
	glutMainLoop();
}