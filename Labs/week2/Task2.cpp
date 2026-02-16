#include <iostream>
#include <lodepng.h>
#include <fstream>
#include <sstream>
#include "Vector3.hpp"

// The goal for this lab is to draw a triangle mesh loaded from an OBJ file from scratch,
// building on the image drawing code from last week's lab.
// The mesh consists of a list of 3D vertices, that describe the points forming the mesh.
// It also has a list of triangle indices, that determine which vertices are used to form each triangle.
// This time, we'll also load the triangle indices, and use them to draw lines connecting the vertices.
// This will make a wireframe render of the mesh.

//This function changes one pixel in the image, basically manually colouring individual pixwls
//The image is stored as a 1D array (not 2D), so we have to convert (x,y) into a single index.
//Each pixel has 4 values: R, G, B, A (alpha), so once we find the pixel position, we multiply by 
//4 to get to the correct spot


void setPixel(std::vector<uint8_t>& image, int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	int pixelIdx = x + y * width; //Convert 2D (x,y) into 1D index. y * width moves us down rows, then + x moves us across columns
	image[pixelIdx * 4 + 0] = r; //Each pixel has 4 slots (R, G, B, A) so multiply pixelIdx by 4 to get to correct pixel in memory

	image[pixelIdx * 4 + 1] = g;
	image[pixelIdx * 4 + 2] = b;
	image[pixelIdx * 4 + 3] = a;
}


//function draws a straight line between two points by manually calculating
//which pixels should be white
void drawLine(std::vector<uint8_t>& image, int width, int height, int startX, int startY, int endX, int endY)
{
	// Task 1: Bresenham's line algorithm
	// *** YOUR CODE HERE
	//Step 1: work out the gradient
	float gradient;

	//change in y over change in x
	//dx = how far to move horizontally
	//dy = how far to move vertically
	int dx = endX - startX; 
	int dy = endY - startY; 
	
	//avoid division by 0
	if (dx == 0) dx = 1; 
	gradient = (float)dy / (float)dx;

	// Step 2: check if it's steep (i.e. absolute value bigger than 1;)
	//if the slope is bigger than 1 the line is "steep" (it goes more vertically than horizontally)
	//steep and shallow lines handled differently
	bool steep;

	steep = std::abs(gradient) > 1.0f;

	if (steep) {
		// Step 3: The steep version of the code, iterating over Y
		// First, make sure that startY is less than endY. 
		// If they're in the wrong order, swap both X and Y.
		//if going backwards in Y, swap start and end to make sure draw is top to bottom
		//othwrwise loop would break
		if (startY > endY)
		{
			std::swap(startX, endX);
			std::swap(startY, endY);
		}

		// Now, iterate from startY to endY. 
		for (int y = startY; y <= endY; ++y) 
		{
			// Draw the line, following the formula!
			//rearrange line equation to find x from y
			//since we're looping over y (because it's steep),solve the line equation to find matching x
			int x = startX + (y - startY) / gradient;

			//draw pix, check inside bounds to oprevent crashes
			if (x >= 0 && x < width && y >= 0 && y < height)
				setPixel(image, x, y, width, height, 255, 255, 255);
		}
	}

	else {
		// Step 4: The shallow version of the code, iterating over X
		// First, make sure that startx is less than endX. 
		// If they're in the wrong order, swap both X and Y.
		//same idea as before, but this time we're making sure its going left to right
		if (startX > endX)
		{
			std::swap(startX, endX);
			std::swap(startY, endY);
		}

		// Now, iterate from startY to endY. 
		for (int x = startX; x <= endX; ++x) 
		{
			// Draw the line, following the formula!
			//standard line equation: y = mx + c, since we're looping over x, we calculate y from it
			int y = startY + gradient * (x - startX);

			if (x >= 0 && x < width && y >= 0 && y < height)
				setPixel(image, x, y, width, height, 255, 255, 255);
		}
	}
}

int main()
{
	std::string outputFilename = "output.png";

	const int width = 512, height = 512;
	const int nChannels = 4;

	// Setting up an image buffer
	// This std::vector has one 8-bit value for each pixel in each row and column of the image, and
	// for each of the 4 channels (red, green, blue and alpha).
	// Remember 8-bit unsigned values can range from 0 to 255.
	std::vector<uint8_t> imageBuffer(height*width*nChannels);

	// This line sets the memory block occupied by the image to all zeros.
	memset(&imageBuffer[0], 0, width * height * nChannels * sizeof(uint8_t));

	std::string bunnyFilename = "../models/stanford_bunny_simplified.obj";

	std::ifstream bunnyFile(bunnyFilename);


	// *** Task 2 ***
	// Your next task is to load all the vertices from the OBJ file.
	// I've given you some starter code here that reads through each line of the
	// OBJ file and makes it into a stringstream.
	// For these V lines, you should load the X, Y and Z coordinates into a new vector
	// and push it back into your array of vertices.

	//loop through the OBJ file line by line
	//onj files store: v (vertex pos) and  f faces (which vertices make up triangles)

	std::vector<Vector3> vertices;
	std::vector<std::vector<unsigned int>> faces;
	std::string line;
	while (!bunnyFile.eof())
	{
		std::getline(bunnyFile, line);
		std::stringstream lineSS(line.c_str());
		char lineStart;
		lineSS >> lineStart;
		char ignoreChar;

		//load vertices
		//if the line starts with 'v', it's a vertex. OBJ file format is v x y z,
		//so read the 3 numbers and store them
		if (lineStart == 'v') {
			Vector3 v;
			for (int i = 0; i < 3; ++i) lineSS >> v[i];
			vertices.push_back(v); //to add this vertex to list of all vertices,to make array of 3D points
		}

		//if the line starts with f, it's a face, these say which 3 vertices form a triangle.
		//load faces
		if (lineStart == 'f') 
		{

			std::vector<unsigned int> face;
			// *** YOUR CODE HERE ***
			// This time we care about faces!
			// Load this face from the line, pushing it back into the list of faces.
			// Be careful to ignore the "/" characters, and the extra texture and normal indices.

			//each face has 3 vertices (because it's a triangle) so read 3 vertex indicies
			for (int i = 0; i < 3; ++i)
			{
				unsigned int vertexIndex;
				lineSS >> vertexIndex; //to read vertex index
				face.push_back(vertexIndex - 1); //obj files count from 1, but C++ vectors count from 0, so 
				                                 //subtract 1 to convert

				//ignore texture/normal data after the /
				if (lineSS.peek() == '/')
				{
					lineSS >> ignoreChar; //ignore/skip the /
					while (lineSS.peek() != ' ' && lineSS.peek() != EOF)
						lineSS >> ignoreChar;
				}
			}
			//add this triangle to list of faces
			faces.push_back(face);
		}
	}

	

	for (int f = 0; f < faces.size(); ++f) {
		// **** Task 3 ****
		// Finally, let's draw the faces!

		// First, load the vertices, and resize them like we did in Task1.cpp
		// Then, call DrawLine three times, to draw each side of the triangle!

		//loop through every triangle in the mesh, for each face, draw lines between its 3 vertices to create wireframe
		
		//get the three vertex indices for face
		unsigned int i0 = faces[f][0];
		unsigned int i1 = faces[f][1];
		unsigned int i2 = faces[f][2];

		//load vertex pos
		Vector3 v0 = vertices[i0];
		Vector3 v1 = vertices[i1];
		Vector3 v2 = vertices[i2];

		//model coords are between -0.5 and 0.5 but screen is 512 pix,
		//scale by 250 to make it bigger then move it to the center of the screen
		//flip y as screen Y goes downward,but 3D space Y goes upward
		int x0 = v0[0] * 250 + width / 2;
		int y0 = -v0[1] * 250 + height / 2;

		int x1 = v1[0] * 250 + width / 2;
		int y1 = -v1[1] * 250 + height / 2;

		int x2 = v2[0] * 250 + width / 2;
		int y2 = -v2[1] * 250 + height / 2;

		//to draw 3 triangle edges
		drawLine(imageBuffer, width, height, x0, y0, x1, y1);
		drawLine(imageBuffer, width, height, x1, y1, x2, y2);
		drawLine(imageBuffer, width, height, x2, y2, x0, y0);

	}

	// *** Encoding image data ***
	// PNG files are compressed to save storage space. 
	// The lodepng::encode function applies this compression to the image buffer and saves the result 
	// to the filename given.
	int errorCode;
	errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
