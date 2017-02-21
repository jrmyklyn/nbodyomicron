#include <stdlib.h>
#include <math.h>
#include <unistd.h>

int n = 10;
double dt = 0.01;
double steps = 1000;

typedef struct {
	double mass, // kilograms
			radius, // meters
			x, y, z, // position (meters)
			dx, dy, dz; // velocity (meters/second)
} Body;

Body *bodies;

float distance(Body b1, Body b2) {
	double xdiff = b1.x - b2.x;
	double ydiff = b1.y - b2.y;
	double zdiff = b1.z - b2.z;
	return sqrt(xdiff * xdiff + ydiff * ydiff + zdiff * zdiff);
}

int collided(Body b1, Body b2) {
	return distance(b1, b2) < b1.radius + b2.radius;
}

void accelerateBodies() {
	for(int i = 0; i < n; i++) {
		Body b = bodies[i];
		//TODO: use n other bodies to accelerate this one
	}
}

void moveBodies() {
	for(int i = 0; i < n; i++) {
		Body b = bodies[i];
		b.x += b.dx;
		b.y += b.dy;
		b.z += b.dz;
	}
}

int main(int argc, char *argv[]) {
	
	// Set up parameters
	int option;
	while((option = getopt(argc, argv, "n:s:t:")) != -1) {
		switch(option) {
			case 'n':
				n = atoi(optarg);
				break;
			case 's':
				steps = atoi(optarg);
				break;
			case 't':
				dt = atof(optarg);
				break;
			case '?':
				return 1;
			default:
				abort();
		}
	}
	
	// Initialize bodies
	bodies = (Body *)malloc(n * sizeof(Body));
	//TODO: Further initialize bodies
	
	while(steps > 0) {
		accelerateBodies();
		moveBodies();
		steps--;
	}
}