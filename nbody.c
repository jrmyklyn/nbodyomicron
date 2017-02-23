#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

int n = 2;
double dt = 60*60*24;
int steps = 27;

#define G_CONST 6.67E-20

typedef struct {
	double mass, // kilograms
			radius, // meters
			x, y, z, // position (meters)
			vx, vy, vz; // velocity (meters/second)
} body_t;

body_t *bodies;

// void printBody(body_t* b){
	// printf("POS |x=%.5e y=%.5e z=%.5e|", b.x, b.y, b.z);
// }

void initBody(body_t b){
	b.mass = 0;
	b.x = 0;
	b.y = 0;
	b.z = 0;
	b.vx = 0;
	b.vy = 0;
	b.vz = 0;
}
double distance(body_t b1, body_t b2) {
	double xdiff = b1.x - b2.x;
	double ydiff = b1.y - b2.y;
	double zdiff = b1.z - b2.z;
	return sqrt(xdiff * xdiff + ydiff * ydiff + zdiff * zdiff);
}

double distanceComponent(double a, double b){
	return sqrt(a*a + b*b);
}

int collided(body_t b1, body_t b2) {
	return distance(b1, b2) < b1.radius + b2.radius;
}

void accelerateBodies() {

	for(int i = 0; i < n; i++) {
		double forcex = 0;
		double forcey = 0;
		double forcez = 0;
		for(int j = 0; j < n; j++){
			if (j != i){
				double massgrav = ((G_CONST) * bodies[j].mass);
				forcex += massgrav / ((bodies[i].x - bodies[j].x) * (bodies[i].x - bodies[j].x)) * (bodies[i].x > bodies[j].x ? -1.0 : 1.0);
				forcey += massgrav / ((bodies[i].y - bodies[j].y) * (bodies[i].y - bodies[j].y)) * (bodies[i].y > bodies[j].y ? -1.0 : 1.0);;
				forcez += massgrav / ((bodies[i].z - bodies[j].z) * (bodies[i].z - bodies[j].z)) * (bodies[i].z > bodies[j].z ? -1.0 : 1.0);;
			}
		}
		bodies[i].vx += forcex * dt ;
		bodies[i].vy += forcey * dt;
		bodies[i].vz += forcez * dt;
	}
}

void moveBodies() {
	for(int i = 0; i < n; i++) {
	
		// body_t *b = &(bodies[i]);
		bodies[i].x += bodies[i].vx * dt;
		bodies[i].y += bodies[i].vy * dt;
		bodies[i].z += bodies[i].vz * dt;
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
	bodies[0].x = -2.67524E4,
	bodies[0].y = -3.99156E5;
	bodies[0].z =  3.40877E4;
	bodies[0].vx =  9.735E-1;
	bodies[0].vy = -3.777E-02;
	bodies[0].vz = -3.756E-02;
	bodies[0].mass = 7.347E22;
	bodies[0].radius = 0.00000001;
	
	bodies[1].x = 0,
	bodies[1].y = 0;
	bodies[1].z = 0;
	bodies[1].vx = 0;
	bodies[1].vy = 0;
	bodies[1].vz = 0;
	bodies[1].mass = 5.972E24;
	bodies[1].radius = 6371;
	//TODO: Import bodies
	
	while(steps > 0) {
		printf("Steps %d | 0 ", steps);
		// printBody(bodies[0]);
		// printf("POS |x=%.5e y=%.5e z=%.5e|", bodies[0].x, bodies[0].y, bodies[0].z);
		// printf(" | 1 ");
		// // printBody(bodies[1]);
		// printf("POS |x=%.5e y=%.5e z=%.5e|", bodies[1].x, bodies[1].y, bodies[1].z);
		// printf("\n");
		accelerateBodies();
		// printf("        | 0 ", steps);
		// printBody(bodies[0]);
		printf("POS |x=%.5e y=%.5e z=%.5e|", bodies[0].x, bodies[0].y, bodies[0].z);
		printf(" | 1 ");
		// printBody(bodies[1]);
		printf("POS |x=%.5e y=%.5e z=%.5e|", bodies[1].x, bodies[1].y, bodies[1].z);
		printf("\n");
		moveBodies();
		steps--;
	}
}