#include <iostream>
#include <lodepng.h>


int main()
{
	//the file paths for input and output images
	std::string inputFilename = "../images/stanford_bunny.png";  //original highres bunny
	std::string outputFilename = "output_bunny.png"; //modified image


	std::vector<uint8_t> imageBuffer; //this holds all pixel data as rgba (8 bits each)
	unsigned int width, height; //image size/dimensions
	lodepng::decode(imageBuffer, width, height, inputFilename);
	
	// *** Tasks ***
	// This code loads an image from a png file. This is an image of the famous 
	// Stanford Bunny https://engineering.stanford.edu/magazine/tale-ubiquitous-stanford-bunny
	// You'll need to load and manipulate images to add texturing to your rasteriser and raytracer.
	// Let's try changing this image.
	// If you'd like, you can use the setPixel function you wrote in the previous task.
	// The code below reduces the brightness of the image by 0.5x, as an example.


	//this brightness code is commented out for task 1 (making image negative)
	//for(int y = 0; y < height; ++y) 
	//	for (int x = 0; x < width; ++x) 
	//		for (int c = 0; c < 3; ++c) {
	//			int pixelIdx = x + y * width;
	//			imageBuffer[pixelIdx * 4 + c] *= 0.5;
	//		}


		// Once you have tested this code, comment out the for loops above and try the following tasks:
	// * Task 1: Try making a *negative* of the input image. Pixels that are bright in the input
	//           should be dark in your output. 
	//           Hint: if the pixels ranged in value from 0 to 1, you could replace each pixel value (v) with (1 - v). 
	//           but remember, the pixels have 8-bit unsigned values, so range from 0 to 255!
	// * Optional Task 2: Try downsampling this image to 1/2 resolution.
	//           You can either just keep one in every 4 pixels, or better yet, average the pixels in each 2x2 block.
	//           Hint: Be careful when averaging! You probably want to convert the pixel values to floating-point to
	//           do the averaging maths.


//NOTES:
//lab 1: for a negative image: new value = 255 - original value, this inverts the brightness of each channel
//lab2: imageBuffer stores pixels in row-major order - row by row, top to bottom
//4 channels per pixel (R,G,B,A)
//pixel index = x + y * width
//channel index = pixel index * 4 + c (c = 0->R, 1->G, 2->B, 3->A)




	//TASK 1: negative version of the image
	//bright pixels should become dark and dark pixels become bright
	//as the colour channels range from 0–255, each value is subtracted from 255
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			int pixelIdx = x + y * width;

			//loop through RGB channels only, leave alpha channel unchanged as we dont need to modify transparency
			for (int c = 0; c < 3; ++c)
			{
				imageBuffer[pixelIdx * 4 + c] = 255 - imageBuffer[pixelIdx * 4 + c];
			}
		}
	}



	//TASK 2: downsample image to half resolution by keeping only 1 pixel from each 2x2 block 
	//- simplest method, fastest, but loses detail as 3 pix are taken away
	//for each 2x2 block of pixels in the original image, just keep the top-left pixel and discard the other three

	//unsigned int newWidth = width / 2;
	//unsigned int newHeight = height / 2;

	////create a new buffer for the smaller image
	//std::vector<uint8_t> smallImage(newWidth * newHeight * 4);

	////loop over every pixel in the downsampled image
	//for (unsigned int y = 0; y < newHeight; ++y)
	//{
	//	for (unsigned int x = 0; x < newWidth; ++x)
	//	{
	//		// Corresponding top-left pixel in original image
	//		int oldX = x * 2;
	//		int oldY = y * 2;

	//		int oldIdx = oldX + oldY * width;
	//		int newIdx = x + y * newWidth;

	//		// Copy RGBA values directly
	//		for (int c = 0; c < 4; ++c)
	//		{
	//			smallImage[newIdx * 4 + c] = imageBuffer[oldIdx * 4 + c];
	//		}
	//	}
	//}

	////replace old image with smaller downsampled one
	//imageBuffer = smallImage;
	//width = newWidth;
	//height = newHeight;



	//TASK 2: average 4 pixels in each 2x2 block- more detailed than previous version to create half sized image
	//values are converted to float to avoid rounding errors while averaging

	unsigned int newWidth = width / 2;
	unsigned int newHeight = height / 2;

	std::vector<uint8_t> smallImage(newWidth * newHeight * 4); //buffer for downsampled image

	for (unsigned int y = 0; y < newHeight; ++y)
	{
		for (unsigned int x = 0; x < newWidth; ++x)
		{
			int oldX = x * 2;
			int oldY = y * 2;

			//the indices of the 4 pixels in the original image form this block
			int idx1 = (oldX)+(oldY)*width;
			int idx2 = (oldX + 1) + (oldY)*width;
			int idx3 = (oldX)+(oldY + 1) * width;
			int idx4 = (oldX + 1) + (oldY + 1) * width;

			int newIdx = x + y * newWidth;

			//average each channel (R,G,B,A)
			for (int c = 0; c < 4; ++c)
			{
				float average =
					imageBuffer[idx1 * 4 + c] +
					imageBuffer[idx2 * 4 + c] +
					imageBuffer[idx3 * 4 + c] +
					imageBuffer[idx4 * 4 + c];

				average /= 4.0f; //divide sum by 4 to get mean

				smallImage[newIdx * 4 + c] = static_cast<uint8_t>(average);
			}
		}
	}

	//replace original image with averaged smaller downsampled image
	imageBuffer = smallImage;
	width = newWidth;
	height = newHeight;



	int errorCode;
	errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
