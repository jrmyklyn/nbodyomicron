#include <GL/glut.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "csvparser.h"



// Model Constants
#define GRAVITY_CONST 6.67408E-20 // Converted from m to km
#define UPDATES_PER_SECOND 15

// Display Constants
#define VIEW_ANGLE (M_PI/6.0) // Looking down on the model from (theta) degrees
#define ROTATION_DEGREES_PER_SECOND 5.0
#define VIEW_DISTANCE_FACTOR 3 // Looking at origin from (factor) times as far as the farthest body
#define LARGEST_BODY_MIN_RADIUS 0.05 // Minimum radius of the largest body relative to the display size
#define SMALLEST_BODY_MIN_RADIUS 0.005 // Minimum radius of the smallest body relative to the display size



// Structs
typedef struct {
	double mass, // kg
			radius, // km
			x, y, z, // km
			vx, vy, vz; // km/s
} body_t;



// Model Globals
double dt = 60 * 60 * 24 * 7; // 1 day by default
long iterations = 0;
body_t *bodies; // Array for body data
int numBodies; // The total number of bodies
int bodiesIndex; // The body index that the next thread will use
pthread_mutex_t indexMutex = PTHREAD_MUTEX_INITIALIZER; // Mutually exlude bodiesIndex for each model thread

// Display Globals
double maxDistance = 0; // The distance of the furthest body from the origin for display purposes
double maxBodyRadius = INT_MIN; // The minimum body size for display purposes
double minBodyRadius = INT_MAX; // The maximum body size for display purposes
int windowWidth;
int windowHeight;
double aspectRatio; // Window aspect ratio
pthread_mutex_t positionsMutex = PTHREAD_MUTEX_INITIALIZER; // Mutually exlude body position reads in the display thread and from writes in the model thread



// Simulation Functions
void *accelerateBodyThread(void *param) {
	int i;
	pthread_mutex_lock(&indexMutex);
		i = bodiesIndex;
		bodiesIndex++;
	pthread_mutex_unlock(&indexMutex);
	
	while(i < numBodies) {
		body_t *b1 = &(bodies[i]);
		double accx = 0;
		double accy = 0;
		double accz = 0;
		for(int j = 0; j < numBodies; j++){
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
		
		pthread_mutex_lock(&indexMutex);
			i = bodiesIndex;
			bodiesIndex++;
		pthread_mutex_unlock(&indexMutex);
	}
}

void accelerateBodies() {
	// Start threads
	bodiesIndex = 0;
	int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
	pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
	for(int i = 0; i < num_threads; i++) {
		int rc = pthread_create(&threads[i], NULL, accelerateBodyThread, NULL);
		if(rc) {
			printf("ERROR: Return code from pthread_create() is %d.", rc);
			exit(-1);
		}
	}
	
	// Wait for threads to finish
	for(int i = 0; i < num_threads; i++) {
		int rc = pthread_join(threads[i], NULL);
		if(rc) {
			printf("ERROR: Return code from pthread_join() is %d.", rc);
			exit(-1);
		}
	}
	
	// Clean up
	free(threads);
}

void moveBodies() {
	// We do not multithread here because it is a simple computation and threading and mutual exclusion would make it unnecessarily complex.
	pthread_mutex_lock(&positionsMutex);
		for(int i = 0; i < numBodies; i++) {
			body_t *b = &(bodies[i]);
			b->x += b->vx * dt;
			b->y += b->vy * dt;
			b->z += b->vz * dt;
			
			// While we are here, we might as well compute the maximum distance from the origin for viewing purposes
			double originDistance = sqrt(b->x * b->x + b->y * b->y + b->z * b->z) + b->radius;
			if(originDistance > maxDistance)
				maxDistance = originDistance;
		}
	pthread_mutex_unlock(&positionsMutex);
}

void *runSimulationThread(void *param) {
	struct timespec start, end;
	while(1) {
		// Get simulation start time
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);
		
		// Run simulation
		accelerateBodies();
		moveBodies();
		iterations++;
		
		// Wait until it is time to update again
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		long sleepTime = 1000000 / UPDATES_PER_SECOND + (start.tv_sec - end.tv_sec) * 1000000 + (start.tv_nsec - end.tv_nsec) / 1000;
		if(sleepTime > 0)
			usleep(sleepTime * 1000000 / CLOCKS_PER_SEC);
	}
}



// Display Functions
void displayDrawCallback() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
		// Rotate Model
		struct timespec t;
		clock_gettime(CLOCK_MONOTONIC_RAW, &t);
		glRotated(fmod(ROTATION_DEGREES_PER_SECOND * (t.tv_sec + t.tv_nsec / 1000000000.0), 360), 0, 0, 1);
		
		pthread_mutex_lock(&positionsMutex);
			// Compute Radius Resize Parameters
			double rFactor = 1;
			double rConstant = 0;
			double lbmr = LARGEST_BODY_MIN_RADIUS * maxDistance;
			double sbmr = SMALLEST_BODY_MIN_RADIUS * maxDistance;
			if(maxBodyRadius < lbmr) // Favor proportional increases required by the largest body
				rFactor = lbmr / maxBodyRadius;
			if(rFactor * minBodyRadius < sbmr) { // Check smallest body requirements and adjust as needed
				rFactor = (lbmr - sbmr) / (maxBodyRadius - minBodyRadius);
				rConstant = sbmr - rFactor * minBodyRadius;
			}
			
			// Draw Bodies
			for(int i = 0; i < numBodies; i++) {
				body_t b = bodies[i];
				
				// Draw Body
				glColor3d(0, 1, 0);
				glPushMatrix();
					glTranslated(b.x, b.y, b.z);
					glutSolidSphere(rFactor * b.radius + rConstant, 10, 10);
				glPopMatrix();
				
				// Draw line from body to x-y plane to better illustrate depth
				glColor3d(1, 0, 0);
				glBegin(GL_LINES);
					glVertex3d(b.x, b.y, b.z);
					glVertex3d(b.x, b.y, 0);
				glEnd();
			}
			
			// Save a local copy of maxDistance
			double localMD = maxDistance;
		pthread_mutex_unlock(&positionsMutex);
		
		// Draw axes
		glPushMatrix();
			glBegin(GL_LINES);
			glColor3d(1, 0, 0);
			glVertex3d(-localMD, 0, 0);
			glVertex3d(localMD, 0, 0);
			glColor3d(0, 1, 0);
			glVertex3d(0, -localMD, 0);
			glVertex3d(0, localMD, 0);
			glColor3d(0, 0, 1);
			glVertex3d(0, 0, -localMD);
			glVertex3d(0, 0, localMD);
			glEnd();
		glPopMatrix();
	glPopMatrix();
	
	// Set camera angle, height, width, depth
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(aspectRatio < 1)
		glFrustum(-localMD, localMD, -localMD / aspectRatio, localMD / aspectRatio, (VIEW_DISTANCE_FACTOR - 1) * localMD, (VIEW_DISTANCE_FACTOR + 1) * localMD);
	else
		glFrustum(-localMD * aspectRatio, localMD * aspectRatio, -localMD, localMD, (VIEW_DISTANCE_FACTOR - 1) * localMD, (VIEW_DISTANCE_FACTOR + 1) * localMD);
	gluLookAt(VIEW_DISTANCE_FACTOR * localMD * cos(VIEW_ANGLE), 0, VIEW_DISTANCE_FACTOR * localMD * sin(VIEW_ANGLE), 0, 0, 0, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Print Text
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0, windowWidth, 0, windowHeight);
		glMatrixMode(GL_MODELVIEW);
		glColor3d(1, 1, 1);
		glPushMatrix();
			glLoadIdentity();
			glRasterPos2i(10, 10);
			
			char str[30];
			double timeElapsed = iterations * dt;
			long days = (long)timeElapsed / (60 * 60 * 24);
			int hours = (long)timeElapsed / (60 * 60) % 24;
			int minutes = (long)timeElapsed / 60 % 60;
			double seconds = fmod(timeElapsed, 60);
			sprintf(str, "View Radius: %.4e km     Day: %d     Time: %02d:%02d:%05.02f", maxDistance, days, hours, minutes, seconds);
			for(int i = 0; i < strlen(str); i++)
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, str[i]);
			
			glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	
	glFlush();
	glutSwapBuffers();
}

void displayReshapeCallback(int width, int height) {
	glViewport(0, 0, width, height);
	// We don't add a mutex here because this is the only place that we write and we don't really care if the display is odd shaped for just a single frame.
	windowWidth = width;
	windowHeight = height;
	aspectRatio = (double)width / (double)height;
}



//Main Function
int main(int argc, char *argv[]) {
	
	// Set up parameters
	char* dataFileName;
	if (argc < 2){
		fprintf(stderr, "The N-Body program requires a csv-formatted file of body data.\n");
		return 1;
	}
	dataFileName = (char*)malloc(strlen(argv[1]));
	strcpy(dataFileName, argv[1]);
	
	int option;
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
	
	// Check File
	FILE *dataFile;
	dataFile = fopen(dataFileName, "r");
	if (dataFile == NULL){
		fprintf(stderr, "File \'%s\' not found in current directory.\n", dataFileName);
		return 1;
	}
	fscanf(dataFile, "%d", &numBodies);
	if (numBodies <= 1) {
		fprintf(stderr, "Boring (or impossible) simulation (numBodies=%d). Aborting.\n", numBodies);
		return 1;
	}
	fclose(dataFile);
	
	// Load Body Data
	bodies = (body_t *)malloc(numBodies * sizeof(body_t));
	CsvParser *csvparser = CsvParser_new(dataFileName, ",", 1);
	CsvRow *row;
	const CsvRow *header = CsvParser_getHeader(csvparser);
	if(header == NULL) {
		fprintf(stderr, "%s\n", CsvParser_getErrorMessage(csvparser));
		return 1;
	}
	
	int i = 0;
	while(i < numBodies) {
		row = CsvParser_getRow(csvparser);
		const char **rowFields = CsvParser_getFields(row);
		
		body_t *b = &(bodies[i]);
		b->mass = atof(rowFields[1]);
		b->radius = atof(rowFields[2]);
		b->x = atof(rowFields[3]);
		b->y = atof(rowFields[4]);
		b->z = atof(rowFields[5]);
		b->vx = atof(rowFields[6]);
		b->vy = atof(rowFields[7]);
		b->vz = atof(rowFields[8]);
		
		if(b->radius < minBodyRadius)
			minBodyRadius = b->radius;
		if(b->radius > maxBodyRadius)
			maxBodyRadius = b->radius;
		
		CsvParser_destroy_row(row);
		i++;
	}
	CsvParser_destroy(csvparser);
	
	// Start Simulation Thread
	pthread_t model_thread;
	int rc = pthread_create(&model_thread, NULL, runSimulationThread, NULL);
	if(rc) {
		printf("ERROR: Return code from pthread_create() is %d.", rc);
		exit(-1);
	}
	
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
	glutIdleFunc(glutPostRedisplay);
	
	// Setup Lighting
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	
	// Setup Other Display Options
	glEnable(GL_CULL_FACE); // Enabled for efficiency
	glEnable(GL_DEPTH_TEST); // Enabled for the depth buffer
	
	// Start Display
	glutMainLoop(); // This function never returns. #YOLO
	
	// Clean Up
	free(bodies);
}
