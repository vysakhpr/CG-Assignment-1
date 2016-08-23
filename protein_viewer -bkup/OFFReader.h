#ifndef OFFREADER_H_
#define OFFREADER_H_

typedef struct Vt {
	float x,y,z;
}Vertex;

typedef struct Pgn {
	int noSides;
	int *v;
}Polygon;

typedef struct offmodel {
	Vertex *vertices;
	Polygon *polygons;
	int numberOfVertices;
 	int numberOfPolygons;
}OffModel;


OffModel* readOffFile(char * );

int FreeOffModel(OffModel  *);

#endif