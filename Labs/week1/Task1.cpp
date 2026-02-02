#include <iostream>
#include <lodepng.h>

//#include <vector> //for std:vector
//#include <string> //for std:string


void setPixel(std::vector<uint8_t>& imageBuffer,
	int width,
	int height,
	int x,
	int y,
	uint8_t r,
	uint8_t g,
	uint8_t b,
	uint8_t a)
{
	// Safety check to check that pixel coordinates are valid. — prevents drawing outside the image boundaries to ignore invalid pixels,
	//so if x or y is not in valid range, its ignored to prevent crashes
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;

	int nChannels = 4;
	int pixelIdx = x + y * width;

	imageBuffer[pixelIdx * nChannels + 0] = r;
	imageBuffer[pixelIdx * nChannels + 1] = g;
	imageBuffer[pixelIdx * nChannels + 2] = b;
	imageBuffer[pixelIdx * nChannels + 3] = a;
}


int main()
{
	std::string outputFilename = "output.png";

	const int width = 1920, height = 1080;
	const int nChannels = 4; //red, green, blue, alpha

	// Setting up an image buffer
	// This std::vector has one 8-bit value for each pixel in each row and column of the image, and
	// for each of the 4 channels (red, green, blue and alpha).
	// Remember 8-bit unsigned values can range from 0 to 255.
	std::vector<uint8_t> imageBuffer(height*width*nChannels);


	//for task 3 circle
	int centerX = width / 2;
	int centerY = height / 2;
	int radius = 200;



	// This for loop sets all the pixels of the image to a cyan colour. 
	//if statement added, so that if y is more than hight divided by 2 (as lower half of screen is 540+) set these as green, 
	// otherwise everything else will by cyan
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x) 
		{
			//if (y > height / 2)
			//{ 
			//	//added to make bottom half green
			//	int pixelIdx = x + y * width;
			//	imageBuffer[pixelIdx * nChannels + 0] = 0; // Set red pixel values to 0
			//	imageBuffer[pixelIdx * nChannels + 1] = 255; // Set green pixel values to 255 (full brightness)
			//	imageBuffer[pixelIdx * nChannels + 2] = 0; // Set blue pixel values to 255 (full brightness)
			//	imageBuffer[pixelIdx * nChannels + 3] = 255; // Set alpha (transparency) pixel values to 255 (fully opaque)
			//}
			// 
			//else
			//{
			//	int pixelIdx = x + y * width;
			//	imageBuffer[pixelIdx * nChannels + 0] = 0; // Set red pixel values to 0
			//	imageBuffer[pixelIdx * nChannels + 1] = 255; // Set green pixel values to 255 (full brightness)
			//	imageBuffer[pixelIdx * nChannels + 2] = 255; // Set blue pixel values to 255 (full brightness)
			//	imageBuffer[pixelIdx * nChannels + 3] = 255; // Set alpha (transparency) pixel values to 255 (fully opaque)
			//}

			if (y > height / 2)
			{
				setPixel(imageBuffer, width, height, x, y, 0, 255, 0, 255); // Green
			}
			else
			{
				setPixel(imageBuffer, width, height, x, y, 0, 255, 255, 255); // Cyan
			}

		}
	}

	//for task 3 circle
	//If you draw the circle before setting the lower part of the image to be green, how does this modify the image?:it gets overwritten, 
	// current drawing order- backfround green/cyan, circle, individual pixel dots
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			int dx = x - centerX;
			int dy = y - centerY;

			//if (sqrt(dx * dx + dy * dy) < radius) - original implementation based on lab, 
			//my version avoids computing the square root for every pixel, which is faster with same result
			if (dx * dx + dy * dy < radius * radius) 
			{
				setPixel(imageBuffer, width, height, x, y, 200, 150, 255, 255); // purple circle
			}
		}
	}

	//draws a red pixel in top-left corner
	setPixel(imageBuffer, width, height, 10, 10, 255, 0, 0, 255);

	//draws a blue pixel near center
	setPixel(imageBuffer, width, height, width / 2, height / 2, 0, 0, 255, 255);

	//draws a red pixel in top-right corner
	setPixel(imageBuffer, width, height, 1909, 10, 255, 0, 0, 255);

	//draws a green pixel in top-right corner mirroring red - better method. left edge is 0, right edge is width - 1
	//setPixel(imageBuffer, width, height, width - 1 - 10, 10, 0, 255, 0, 255);





	/// *** Lab Tasks ***

	// * Task 1: Try adapting the code above to set the lower half of the image to be a green colour.
	
	// * Task 2: Doing the maths above to work out indices is a bit annoying! Write your own setPixel function.
	//           This should take x and y coordinates as input, and red, green, blue and alpha values.
	//           Remember to pass in your imageBuffer. Should it be passed in by reference or by value? Should
	//           the reference be const?
	//           We will use this setPixel function to build our rasteriser in the upcoming labs.
	//			 Test your setPixel function by setting pixels in your image to different colours.

	//TASK 2 NOTES: we want to be able to use a simple setPix function instead of all the int pixelIdx indexing stuff
	//setPixel(imageBuffer, width, height, x, y, red val, blue val, green val, alpha val); 
	//imageBuffer should be reference as we want to change the original image, and we would be changing the pix valus so not const

	// * Optional Task 3: Use your setPixel function to draw a circle in the centre of the image. Remember a point is
	//           in a circle if sqrt((x - x_0)^2 + (y - y_0)^2) < radius (here x_0, y_0 are the coordinates at the middle of 
	//           the circle). 
	//           Hint - use a similar for loop to the one above, and add an if statement to check if the current
	//           pixel lies in the circle.
	//           Try modifying the order you draw each component in. If you draw the circle before setting the lower 
	//           part of the image to be green, how does this modify the image?
	
	// * Optional Task 4: Work out how good the compression ratio of the saved PNG image is. PNG images
	//           use *lossless* compression, where all the pixel values of the original image are preserved.
	//           To work out the compression ratio, compare the size of the saved image to the memory
	//           occupied by the image buffer (this is based on the width, height and number of channels).
	//           Try setting the pixels to random values (use rand() and the % operator). What is the 
	//           compression ratio now, and why do you think this is?


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
