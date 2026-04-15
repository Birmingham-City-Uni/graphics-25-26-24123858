//TODO:
//scene hpp for models and textures
//add in all models and pos correctly
//pos camera in correct location
//normal mapping
//shadow mapping
//lighting- currently ambient and point


// This define is necessary to get the M_PI constant.
#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include <lodepng.h>
#include "Image.hpp"
#include "LinAlg.hpp"
#include "Light.hpp"
#include "Mesh.hpp"
#include "Shading.hpp"
#include "Scene.hpp"



enum ShadingMode {
	PHONG,
	BLINN_PHONG
};



struct Triangle {
	std::array<Eigen::Vector3f, 3> screen; //coordinates of the triangle in screen space
	std::array<Eigen::Vector3f, 3> verts; //vertices of the triangle in world space
	std::array<Eigen::Vector3f, 3> cam; //vertices of the triangle in camera space
	std::array<Eigen::Vector3f, 3> norms; //normals of the triangle corners in world space
	std::array<Eigen::Vector2f, 3> texs; //texture coordinates of the triangle corners
};


Eigen::Matrix4f projectionMatrix(int height, int width, float horzFov = 70.f * M_PI / 180.f, float zFar = 10.f, float zNear = 0.1f)
{
	float vertFov = horzFov * float(height) / width;
	Eigen::Matrix4f projection;
	projection <<
		1.0f / tanf(0.5f * horzFov), 0, 0, 0,
		0, 1.0f / tanf(0.5f * vertFov), 0, 0,
		0, 0, zFar / (zFar - zNear), -zFar * zNear / (zFar - zNear),
		0, 0, 1, 0;
	return projection;
}


//for adding model scaling
Eigen::Matrix4f scaleMatrix(Eigen::Vector3f s)
{
	Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
	m(0, 0) = s.x();
	m(1, 1) = s.y();
	m(2, 2) = s.z();
	return m;
}

void findScreenBoundingBox(const Triangle& t, int width, int height, int& minX, int& minY, int& maxX, int& maxY)
{
	//find a bounding box around the triangle
	minX = std::min(std::min(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	minY = std::min(std::min(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());
	maxX = std::max(std::max(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	maxY = std::max(std::max(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());

	//constrain it to keep within image
	minX = std::min(std::max(minX, 0), width - 1);
	maxX = std::min(std::max(maxX, 0), width - 1);
	minY = std::min(std::max(minY, 0), height - 1);
	maxY = std::min(std::max(maxY, 0), height - 1);
}


void drawTriangle(std::vector<uint8_t>& image, int width, int height,
	std::vector<float>& zBuffer,
	const Triangle& t,
	const std::vector<std::unique_ptr<Light>>& lights,
	//const Eigen::Vector3f& albedo, const Eigen::Vector3f& specularColor,
	const std::vector<uint8_t>& texture, int texWidth, int texHeight,
	const Eigen::Vector3f& specularColor,
	float specularExponent,
	ShadingMode shadingMode,
	const Eigen::Vector3f& camWorldPos)
{
	int minX, minY, maxX, maxY;
	findScreenBoundingBox(t, width, height, minX, minY, maxX, maxY);

	Eigen::Vector2f edge1 = v2(t.screen[2] - t.screen[0]);
	Eigen::Vector2f edge2 = v2(t.screen[1] - t.screen[0]);
	float triangleArea = 0.5f * vec2Cross(edge2, edge1);
	if (triangleArea < 0) {
		return;
	}

	for (int x = minX; x <= maxX; ++x)
		for (int y = minY; y <= maxY; ++y) {
			Eigen::Vector2f p(x, y);

			//find subtriangle areas
			float a0 = 0.5f * fabsf(vec2Cross(v2(t.screen[1]) - v2(t.screen[2]), p - v2(t.screen[2])));
			float a1 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[2]), p - v2(t.screen[2])));
			float a2 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[1]), p - v2(t.screen[1])));

			//find barycentrics
			float b0 = a0 / triangleArea;
			float b1 = a1 / triangleArea;
			float b2 = a2 / triangleArea;

			//if outside triangle exit early
			float sum = b0 + b1 + b2;
			if (sum > 1.0001) {
				continue;
			}

			//get the depths from the camera space position of the 3 corners
			//float depth0 = 0.f, depth1 = 0.f, depth2 = 0.f;
			float depth0 = -t.cam[0].z();
			float depth1 = -t.cam[1].z();
			float depth2 = -t.cam[2].z();

			//work out the depth at the point P
			//float depthP = 0.f;
			float depthP = 1.0f / (b0 / depth0 + b1 / depth1 + b2 / depth2);

			//interpolate to find the world-space position of this pixel (perspective-correct)
			//Eigen::Vector3f worldP = Eigen::Vector3f::Zero();
			Eigen::Vector3f worldP =
				(t.verts[0] * (b0 / depth0) +
					t.verts[1] * (b1 / depth1) +
					t.verts[2] * (b2 / depth2)) * depthP;

			// Interpolate to find the normal of this pixel (perspective-correct)
			//Eigen::Vector3f normP = Eigen::Vector3f::Zero();
			Eigen::Vector3f normP =
				(t.norms[0] * (b0 / depth0) +
					t.norms[1] * (b1 / depth1) +
					t.norms[2] * (b2 / depth2)).normalized();


			//perspective-correct uv interpolation
			Eigen::Vector2f texP =
				(t.texs[0] * (b0 / depth0) +
					t.texs[1] * (b1 / depth1) +
					t.texs[2] * (b2 / depth2)) * depthP;

			//convert to texture space
			int texX = texP.x() * (texWidth - 1);
			int texY = (1.0f - texP.y()) * (texHeight - 1);

			//clamp
			texX = std::max(0, std::min(texX, texWidth - 1));
			texY = std::max(0, std::min(texY, texHeight - 1));

			//sample texture
			Color texColor = getPixel(texture, texX, texY, texWidth, texHeight);

			//convert to linear space (gamma correct)
			Eigen::Vector3f albedo(
				powf(texColor.r / 255.0f, 2.2f),
				powf(texColor.g / 255.0f, 2.2f),
				powf(texColor.b / 255.0f, 2.2f)
			);


			// Interpolate to find the correct clip-space depth (perspective-correct)
			//float depth = 0.f;
			float depth =
				(t.screen[0].z() * (b0 / depth0) +
					t.screen[1].z() * (b1 / depth1) +
					t.screen[2].z() * (b2 / depth2)) * depthP;

			int depthIdx = static_cast<int>(p.x()) + static_cast<int>(p.y()) * width;
			if (depth > zBuffer[depthIdx]) continue;
			zBuffer[depthIdx] = depth;


			//work out colour at this position
			Eigen::Vector3f color = Eigen::Vector3f::Zero();

			Eigen::Vector3f viewDir = (camWorldPos - worldP).normalized();

			//iterate over lights and sum to find colour
			for (auto& light : lights) {

				//work out the intensity of this light source at the point worldP
				Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);

				//only need the following if the light isn't an ambient light
				if (light->getType() != Light::Type::AMBIENT) {
					Eigen::Vector3f incomingLightDir = light->getDirection(worldP);

					float specularTerm;
					if (shadingMode == ShadingMode::PHONG) {
						specularTerm = phongSpecularTerm(incomingLightDir, normP, viewDir, specularExponent);
					}
					else {
						specularTerm = blinnPhongSpecularTerm(incomingLightDir, normP, viewDir, specularExponent);
					}

					Eigen::Vector3f specularOut = specularColor * specularTerm;
					specularOut = coeffWiseMultiply(specularOut, lightIntensity);

					//take the dot product of the normal with the light direction
					float dotProd = normP.dot(-incomingLightDir);

					//don't want negative light so if dot product less than 0, set it to 0
					dotProd = std::max(dotProd, 0.0f);

					//multiply the light intensity by the dot product
					Eigen::Vector3f diffuseOut = lightIntensity * dotProd;
					diffuseOut = coeffWiseMultiply(diffuseOut, albedo);

					color += specularOut;
					color += diffuseOut;
					//color = (incomingLightDir + Eigen::Vector3f::Ones()) / 2;
				}
				else {
					//light is ambient- multiply light intensity with albedo
					color += coeffWiseMultiply(lightIntensity, albedo);
				}
			}
			//color = (worldP + Eigen::Vector3f::Ones()) / 2;
			//color = (viewDir + Eigen::Vector3f::Ones()) / 2;
			//color = (normP + Eigen::Vector3f::Ones()) / 2;




		//	// Convert it into an Eigen::Vector3f as an albedo
		//	// (Optional bonus task, if you checked out the slides on gamma correction:
		//	// gamma correct this colour, so the texture doesn't appear overly bright.
		//	// should you raise to the power 1/2.2, or 2.2?)
		//	//Eigen::Vector3f albedo = Eigen::Vector3f::Zero();
		//	// 
		//	// Convert to albedo (0–1 range)
		///*	Eigen::Vector3f albedo(
		//		texColor.r / 255.0f,
		//		texColor.g / 255.0f,
		//		texColor.b / 255.0f
		//	);*/

		//	Eigen::Vector3f albedo(
		//		powf(texColor.r / 255.0f, 2.2f),
		//		powf(texColor.g / 255.0f, 2.2f),
		//		powf(texColor.b / 255.0f, 2.2f)
		//	);


			Color c;
			//gamma-correcting colours so the texture doesn't appear overly bright
			c.r = std::min(powf(color.x(), 1 / 2.2f), 1.0f) * 255;
			c.g = std::min(powf(color.y(), 1 / 2.2f), 1.0f) * 255;
			c.b = std::min(powf(color.z(), 1 / 2.2f), 1.0f) * 255;

			c.a = 255;

			setPixel(image, x, y, width, height, c);
		}
}



void drawMesh(std::vector<unsigned char>& image,
	std::vector<float>& zBuffer,
	const Mesh& mesh,
	//const Eigen::Vector3f& albedo, const Eigen::Vector3f& specularColor,
	const std::vector<uint8_t>& texture, int texWidth, int texHeight,
	const Eigen::Vector3f& specularColor,
	float specularExponent,
	ShadingMode shadingMode,
	const Eigen::Vector3f& camWorldPos,
	const Eigen::Matrix4f& modelToWorld,
	const Eigen::Matrix4f& worldToCam,
	const Eigen::Matrix4f& camToClip,
	const std::vector<std::unique_ptr<Light>>& lights,
	int width, int height)
{
	for (int i = 0; i < mesh.vFaces.size(); ++i) {
		Eigen::Vector3f
			v0 = mesh.verts[mesh.vFaces[i][0]],
			v1 = mesh.verts[mesh.vFaces[i][1]],
			v2 = mesh.verts[mesh.vFaces[i][2]];
		Eigen::Vector3f
			n0 = mesh.norms[mesh.nFaces[i][0]],
			n1 = mesh.norms[mesh.nFaces[i][1]],
			n2 = mesh.norms[mesh.nFaces[i][2]];

		Triangle t;
		t.verts[0] = (modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
		t.verts[1] = (modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
		t.verts[2] = (modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

		t.cam[0] = (worldToCam * modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
		t.cam[1] = (worldToCam * modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
		t.cam[2] = (worldToCam * modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

		//work out the clip space coordinates, by multiplying by worldToClip and doing the 
		//perspective divide
		Eigen::Vector4f vClip0 = camToClip * worldToCam * modelToWorld * vec3ToVec4(v0);
		vClip0 /= vClip0.w();
		Eigen::Vector4f vClip1 = camToClip * worldToCam * modelToWorld * vec3ToVec4(v1);
		vClip1 /= vClip1.w();
		Eigen::Vector4f vClip2 = camToClip * worldToCam * modelToWorld * vec3ToVec4(v2);
		vClip2 /= vClip2.w();

		//check that all 3 vertices are in the clip box (-1 to 1 in x, y and z) and if not,
		//skip drawing this triangle
		if (outsideClipBox(vClip0) || outsideClipBox(vClip1) || outsideClipBox(vClip2)) continue;

		//work out the screen space coordinates based on the image height and width
		t.screen[0] = Eigen::Vector3f((vClip0.x() + 1.0f) * width / 2, (-vClip0.y() + 1.0f) * height / 2, vClip0.z());
		t.screen[1] = Eigen::Vector3f((vClip1.x() + 1.0f) * width / 2, (-vClip1.y() + 1.0f) * height / 2, vClip1.z());
		t.screen[2] = Eigen::Vector3f((vClip2.x() + 1.0f) * width / 2, (-vClip2.y() + 1.0f) * height / 2, vClip2.z());

		//transform normals (using the inverse transpose of the upper 3x3 block)
		t.norms[0] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n0).normalized();
		t.norms[1] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n1).normalized();
		t.norms[2] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n2).normalized();

		t.texs[0] = mesh.texs[mesh.tFaces[i][0]];
		t.texs[1] = mesh.texs[mesh.tFaces[i][1]];
		t.texs[2] = mesh.texs[mesh.tFaces[i][2]];

		//drawTriangle(image, width, height, zBuffer, t, lights, albedo, specularColor, specularExponent, shadingMode, camWorldPos);
		drawTriangle(image, width, height, zBuffer, t, lights,
			texture, texWidth, texHeight,
			specularColor, specularExponent, shadingMode, camWorldPos);
	}
}

int main()
{
	std::string outputFilename = "output.png";

	const int width = 1920, height = 1080;
	const int nChannels = 4;

	//set up an image buffer
	std::vector<uint8_t> imageBuffer(height*width*nChannels);

	std::vector<float> zBuffer(height * width);

	//initialise buffers
	Color black{ 0,0,0,255 };
	for (int r = 0; r < height; ++r)
		for (int c = 0; c < width; ++c) {
			setPixel(imageBuffer, c, r, width, height, black);
			zBuffer[r * width + c] = 1.0f;
		}

	//camera and projection setup 
	//Eigen::Matrix4f projection = projectionMatrix(height, width);
	//Eigen::Matrix4f projection = projectionMatrix(height, width, 70.f * M_PI / 180.f, 100.f, 0.1f);
	float zNear = 0.1f;
	float zFar = 100.f;
	Eigen::Matrix4f projection = projectionMatrix(height, width, 70.f * M_PI / 180.f, zFar, zNear);
	

	//need to change camera pos when all models have been added and placed
	//Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(0.f, 5.0f, -5.0f)) * rotateXMatrix(0.4f);
	Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(0.0f, 0.0f, -10.0f)); // (3.43438f, 1.24049f, -7.13643f));
		//rotateYMatrix(-26 * M_PI / 180)*
		//rotateXMatrix((90 - 73)* M_PI / 180);

	Eigen::Vector3f camWorldPos = (cameraToWorld * Eigen::Vector4f(0, 0, 0, 1)).block<3, 1>(0, 0);
	Eigen::Matrix4f worldToCamera = cameraToWorld.inverse();

	//lights- anbient and point
	std::vector<std::unique_ptr<Light>> lights;
	lights.emplace_back(new AmbientLight(Eigen::Vector3f(0.1f, 0.1f, 0.1f))); //(0.3f, 0.3f, 0.3f)
	lights.emplace_back(new PointLight(2.f * Eigen::Vector3f(1.1f, 1.1f, 1.1f), Eigen::Vector3f(0.f, 0.8f, 6.f))); //(0.f, 5.f, 5.f)


	//load and draw scene 
	//Mesh myMesh = loadMeshFile("../models/model.obj");
	//Eigen::Matrix4f meshTransform = translationMatrix(Eigen::Vector3f(0.f, 0.f, 5.f)) * rotateYMatrix(M_PI);;
	
	Mesh cube = loadMeshFile("../models/cube.obj");
	Eigen::Matrix4f transform1 =
		translationMatrix(Eigen::Vector3f(1.0f, -0.3f, -2.5f)) *
		scaleMatrix(Eigen::Vector3f(0.5f, 0.5f, 0.5f));
		/*rotateZMatrix(0.0f) *
		rotateYMatrix(180* M_PI/180) *
		rotateXMatrix(-90.0f* M_PI/180);*/

	Mesh button = loadMeshFile("../models/button.obj");
	Eigen::Matrix4f transform2 =
		translationMatrix(Eigen::Vector3f(1.0f, -6.1f, -2.0f))*
		scaleMatrix(Eigen::Vector3f(2.0f, 2.0f, 2.0f));
		/*rotateZMatrix(-3.16f) *
		rotateYMatrix(0.0f) *
		rotateXMatrix(90.0f * M_PI / 180);*/
		//rotateYMatrix(M_PI + 0.5f); 

	Mesh model3 = loadMeshFile("../models/model3.obj");
	Eigen::Matrix4f transform3 =
		translationMatrix(Eigen::Vector3f(0.0f, 0.0f, 0.0f)) * //move down
		scaleMatrix(Eigen::Vector3f(1.5f, 1.5f, 1.5f)) * //smaller
		//rotateYMatrix(M_PI);
		rotateYMatrix(0.0f);
	

	//std::vector<uint8_t> texture;
	//unsigned int texWidth, texHeight;
	//lodepng::decode(texture, texWidth, texHeight, "../models/texture.png");

	std::vector<uint8_t> texture1, texture2, texture3;
	unsigned int texWidth1, texHeight1;
	unsigned int texWidth2, texHeight2;
	unsigned int texWidth3, texHeight3;

	lodepng::decode(texture1, texWidth1, texHeight1, "../models/cubeTex.png");
	lodepng::decode(texture2, texWidth2, texHeight2, "../models/buttonTex.png");
	lodepng::decode(texture3, texWidth3, texHeight3, "../models/texture3.png");


	//drawMesh(imageBuffer, zBuffer, myMesh,
	//	texture, texWidth, texHeight,
	//	Eigen::Vector3f::Ones(),
	//	50.f,
	//	ShadingMode::BLINN_PHONG,
	//	camWorldPos,
	//	meshTransform, worldToCamera, projection,
	//	lights, width, height);

	//model1
	drawMesh(imageBuffer, zBuffer, cube,
		texture1, texWidth1, texHeight1,
		Eigen::Vector3f::Ones(), //specular colour
		50.f, //specular exponent- how shiny
		ShadingMode::BLINN_PHONG,
		camWorldPos,
		transform1, worldToCamera, projection,
		lights, width, height);

	//model2
	drawMesh(imageBuffer, zBuffer, button,
		texture2, texWidth2, texHeight2,
		Eigen::Vector3f::Ones(),
		50.f,
		ShadingMode::BLINN_PHONG,
		camWorldPos,
		transform2, worldToCamera, projection,
		lights, width, height);

	//model3
	drawMesh(imageBuffer, zBuffer, model3,
		texture3, texWidth3, texHeight3,
		Eigen::Vector3f::Ones(),
		50.f,
		ShadingMode::BLINN_PHONG,
		camWorldPos,
		transform3, worldToCamera, projection,
		lights, width, height);



    //save the image
    int errorCode;
        errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
        if (errorCode) { //check the error code, in case an error occurred.
            std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
            return errorCode;
        }

    return 0;
}
