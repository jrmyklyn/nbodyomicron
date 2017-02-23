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
			vx, vy, vz; // velocity (meters/second)
} body_t;

body_t *bodies;

float distance(body_t b1, body_t b2) {
	double xdiff = b1.x - b2.x;
	double ydiff = b1.y - b2.y;
	double zdiff = b1.z - b2.z;
	return sqrt(xdiff * xdiff + ydiff * ydiff + zdiff * zdiff);
}

int collided(body_t b1, body_t b2) {
	return distance(b1, b2) < b1.radius + b2.radius;
}

void accelerateBodies() {
	for(int i = 0; i < n; i++) {
		body_t b = bodies[i];
		//TODO: use n other bodies to accelerate this one
	}
}

void moveBodies() {
	for(int i = 0; i < n; i++) {
		body_t b = bodies[i];
<<<<<<< HEAD
		b.x += b.vx;
		b.y += b.vy;
		b.z += b.vz;
=======
		b.x += b.dx;
		b.y += b.dy;
		b.z += b.dz;
>>>>>>> 59a44532899601a47cb93bad57068f564019af37
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
	bodies = (body_t *)malloc(n * sizeof(body_t));
<<<<<<< HEAD
	
=======
	//TODO: Further initialize bodies
>>>>>>> 59a44532899601a47cb93bad57068f564019af37
	
	while(steps > 0) {
		accelerateBodies();
		moveBodies();
		steps--;
	}
}