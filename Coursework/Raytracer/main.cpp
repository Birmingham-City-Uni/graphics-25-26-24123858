#include <Eigen/Dense>
#include <lodepng.h>
#include <json/json.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "BVHNode.hpp"
#include "Triangle.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "PointLight.hpp"
#include "DirectionalLight.hpp"
#include "LambertianShader.hpp"
#include "TexturedLambertianShader.hpp"
#include "PhongShader.hpp"
#include "MirrorShader.hpp"
#include "TexCoordTestShader.hpp"
#include "Model.hpp"
#include <fstream>

/// <summary>
/// Load a JSON config file using the nlohmann library.
/// </summary>
nlohmann::json loadConfig(const std::string& filename)
{
	std::ifstream configStream(filename);
	nlohmann::json config = nlohmann::json::parse(configStream);
	return config;
}

/// <summary>
/// Load an Eigen Vector3f from a config file.
/// Call as for example loadVec3FromConfig(config["myVector3"]);
/// </summary>
Eigen::Vector3f loadVec3FromConfig(const nlohmann::json& config)
{
	return Eigen::Vector3f(config[0], config[1], config[2]);
}


//scale helper for transform matrix
Eigen::Matrix4f makeScaleMatrix(float x, float y, float z) {
	Eigen::Matrix4f s = Eigen::Matrix4f::Identity();
	s(0, 0) = x; s(1, 1) = y; s(2, 2) = z;
	return s;
}
//uniform scale shorthand
Eigen::Matrix4f makeScaleMatrix(float s) { return makeScaleMatrix(s, s, s); }


int main(int argc, char* argv[]) {

	// *** Load the config file ***
	auto config = loadConfig("../config/config.json");

	const int pixHeight = config["pixHeight"], pixWidth = config["pixWidth"];
	const int nChannels = 4;


	// *** Set up camera and output image ***
	//Camera cam(
	//	loadVec3FromConfig(config["cameraPos"]),
	//	loadVec3FromConfig(config["cameraForward"]),
	//	loadVec3FromConfig(config["cameraUp"]),
	//	pixWidth, pixHeight,
	//	config["cameraFov"]);

	//set up the same as rasteriser, using same transforms for cam, lights and models to position everything
	Camera cam(
		Eigen::Vector3f(7.0f, 0.7f, -7.0f),
		Eigen::Vector3f(-0.574f, 0.0f, 0.819f), //rotateY(-35deg) applied to (0,0,1)
		Eigen::Vector3f(0.f, 1.f, 0.f),
		pixWidth, pixHeight,
		70.f  // same fov as rasteriser
	);


	std::vector<uint8_t> outImage(pixHeight * pixWidth * nChannels);

	Eigen::Vector3f
		red(1.f, 0.f, 0.f),
		blue(0.f, 0.f, 1.f),
		aqua(0.f, .8f, .8f),
		lavender(178.f / 255.f, 164.f / 255.f, 212.f / 255.f);

	// *** Load shaders and textures ***
	
	////scale matrix helper since GeomUtil didnt have one
	//auto makeScale = [](float x, float y, float z) 
	//	{
	//	Eigen::Matrix4f s = Eigen::Matrix4f::Identity();
	//	s(0, 0) = x; s(1, 1) = y; s(2, 2) = z;
	//	return s;
	//	};

	//auto makeScaleU = [&makeScale](float s) 
	//	{ 
	//		return makeScale(s, s, s); 
	//	};

	//loading textures
	/*auto loadTex = [](const std::string& path, std::vector<uint8_t>& data, unsigned int& w, unsigned int& h) 
		{
		unsigned int err = lodepng::decode(data, w, h, path);
		if (err) throw std::runtime_error("Failed to load texture: " + path);
		};*/

	auto loadTex = [](const std::string& path, std::vector<uint8_t>& data, unsigned int& w, unsigned int& h)
		{
			unsigned int err = lodepng::decode(data, w, h, path);
			if (err) throw std::runtime_error("Failed to load texture: " + path
				+ " — " + lodepng_error_text(err));
		};

	//std::vector<uint8_t> spotTexture;
	//unsigned int width, height;
	//lodepng::decode(spotTexture, width, height, "../models/spot.png");
	
	unsigned int tw, th; //now texture width and height, reused for every texture load without declaring a new pair each time

	std::vector<uint8_t> cubeTex, buttonTex, gunTex, glassTex, doorTex, wallTex, signTex;
	
	loadTex("../models/cubeTex.png", cubeTex, tw, th);   unsigned int cubeW = tw, cubeH = th;
	loadTex("../models/buttonTex.png", buttonTex, tw, th);   unsigned int buttonW = tw, buttonH = th;
	loadTex("../models/gunTex.png", gunTex, tw, th);   unsigned int gunW = tw, gunH = th;
	loadTex("../models/glassTex.png", glassTex, tw, th);   unsigned int glassW = tw, glassH = th;
	loadTex("../models/doorTex.png", doorTex, tw, th);   unsigned int doorW = tw, doorH = th;
	loadTex("../models/wallTex.png", wallTex, tw, th);   unsigned int wallW = tw, wallH = th;
	loadTex("../models/signTex.png", signTex, tw, th);   unsigned int signW = tw, signH = th;


	//LambertianShader redLambertianShader(red);
	//PhongShader bluePlasticShader(blue, Eigen::Vector3f(1.f, 1.f, 1.f), 100.f);
	//LambertianShader aquaLambertianShader(aqua);
	//LambertianShader lavenderLambertianShader(lavender);
	//TexturedLambertianShader spotShader(&spotTexture, width, height);
	MirrorShader mirrorShader;
	//TexCoordTestShader texCoordTestShader;

	TexturedLambertianShader cubeShader(&cubeTex, cubeW, cubeH);
	TexturedLambertianShader buttonShader(&buttonTex, buttonW, buttonH);
	//PhongShader gunShader(Eigen::Vector3f(1, 1, 1), Eigen::Vector3f(1, 1, 1), 80.f); //gun has spec map in rasteriser, for now using Phong with white base
	TexturedLambertianShader gunShader(&gunTex, gunW, gunH);
	//TexturedLambertianShader glassShader(&glassTex, glassW, glassH); //uses mirror
	TexturedLambertianShader doorShader(&doorTex, doorW, doorH);
	TexturedLambertianShader wallShader(&wallTex, wallW, wallH);
	TexturedLambertianShader signShader(&signTex, signW, signH);
	//flat colour shaders for floor1 and floor2
	LambertianShader floor1Shader(Eigen::Vector3f(0.85f, 0.83f, 0.80f)); //whiteish
	LambertianShader floor2Shader(Eigen::Vector3f(0.18f, 0.18f, 0.18f)); //dark grey


	// *** Set up scene ***
	Scene scene;

	// Optional code: here's how to add the spot mesh to the scene, using a BVH
	// Try enabling this and comparing it to the non-BVH version below!
	//Model spotModel("../models/spot.obj");
	//scene.renderables.push_back(std::make_shared<BVHNode>(spotModel, &spotShader, 4, rotateY(M_PI / 4.0f)));

	// Here's how to add the mesh without using the BVH.
	// Try comparing performance to the BVH version above.
	//Model spotModel("../models/spot.obj");
	//scene.renderables.push_back(std::make_shared<Mesh>(&spotShader, &spotModel));
	//scene.renderables.back()->modelToWorld(rotateY(M_PI / 4.0f));

	//// cube
	//{
	//	Eigen::Matrix4f t = makeTranslationMatrix(Eigen::Vector3f(2.6f, -0.3f, -3.15f))
	//		* makeScaleU(0.45f);
	//	Model m("../models/cube.obj");
	//	scene.renderables.push_back(std::make_shared<BVHNode>(m, &cubeShader, 4, t));
	//}

	//// button
	//{
	//	Eigen::Matrix4f t = makeTranslationMatrix(Eigen::Vector3f(2.2f, -1.0f, -3.0f))
	//		* makeScaleU(0.7f);
	//	Model m("../models/button.obj");
	//	scene.renderables.push_back(std::make_shared<BVHNode>(m, &buttonShader, 4, t));
	//}

	//// floor1 (off-white)
	//{
	//	Eigen::Matrix4f t = makeTranslationMatrix(Eigen::Vector3f(0.0f, -2.0f, 0.0f));
	//	Model m("../models/floor1.obj");
	//	scene.renderables.push_back(std::make_shared<BVHNode>(m, &floor1Shader, 4, t));
	//}

	//// glass panel — using MirrorShader as a frosted mirror upgrade (see notes below)
	//{
	//	Eigen::Matrix4f t = makeTranslationMatrix(Eigen::Vector3f(-1.6f, 0.8f, -1.2f))
	//		* makeScale(0.5f, 0.8f, 0.8f)
	//		* rotateX(-3.0f * M_PI / 180.f);
	//	Model m("../models/glass.obj");
	//	scene.renderables.push_back(std::make_shared<BVHNode>(m, &mirrorShader, 4, t));
	//}

	//// gun
	//{
	//	Eigen::Matrix4f t = makeTranslationMatrix(Eigen::Vector3f(5.5f, -0.8f, -4.8f))
	//		* makeScaleU(2.0f)
	//		* rotateY(-55.f * M_PI / 180.f);
	//	Model m("../models/gun.obj");
	//	scene.renderables.push_back(std::make_shared<BVHNode>(m, &gunShader, 4, t));
	//}

	//// floor2 (dark grey)
	//{
	//	Eigen::Matrix4f t = makeTranslationMatrix(Eigen::Vector3f(1.7f, -2.4f, 2.9f));
	//	Model m("../models/floor2.obj");
	//	scene.renderables.push_back(std::make_shared<BVHNode>(m, &floor2Shader, 4, t));
	//}

	//// door
	//{
	//	Eigen::Matrix4f t = makeTranslationMatrix(Eigen::Vector3f(3.3f, -1.5f, 2.6f))
	//		* rotateY(M_PI);
	//	Model m("../models/door.obj");
	//	scene.renderables.push_back(std::make_shared<BVHNode>(m, &doorShader, 4, t));
	//}

	//// wall
	//{
	//	Eigen::Matrix4f t = makeTranslationMatrix(Eigen::Vector3f(5.0f, -1.9f, 4.1f));
	//	Model m("../models/wall.obj");
	//	scene.renderables.push_back(std::make_shared<BVHNode>(m, &wallShader, 4, t));
	//}

	//// sign
	//{
	//	Eigen::Matrix4f t = makeTranslationMatrix(Eigen::Vector3f(3.8f, 0.05f, 0.8f))
	//		* makeScaleU(0.3f)
	//		* rotateY(M_PI);
	//	Model m("../models/sign.obj");
	//	scene.renderables.push_back(std::make_shared<BVHNode>(m, &signShader, 4, t));
	//}







	//auto addModel = [&](const char* objPath, Shader* shader,
	//	const Eigen::Matrix4f& t, const char* label)
	//	{
	//		Model m(objPath);
	//		if (!m.hasNormals()) {
	//			std::cerr << "WARNING: " << label
	//				<< " has no normals - will likely crash. Re-export with normals.\n";
	//		}
	//		scene.renderables.push_back(std::make_shared<BVHNode>(m, shader, 4, t));
	//	};

	//addModel("../models/cube.obj", &cubeShader,
	//	makeTranslationMatrix(Eigen::Vector3f(2.6f, -0.3f, -3.15f))* makeScaleMatrix(0.45f),
	//	"cube");

	//addModel("../models/button.obj", &buttonShader,
	//	makeTranslationMatrix(Eigen::Vector3f(2.2f, -1.0f, -3.0f))* makeScaleMatrix(0.7f),
	//	"button");

	//addModel("../models/floor1.obj", &floor1Shader,
	//	makeTranslationMatrix(Eigen::Vector3f(0.0f, -2.0f, 0.0f)),
	//	"floor1");

	//// Glass panel uses MirrorShader — real reflections instead of alpha blending.
	//// This is Mirror Reflections as an advanced feature.
	//addModel("../models/glass.obj", &mirrorShader,
	//	makeTranslationMatrix(Eigen::Vector3f(-1.6f, 0.8f, -1.2f))
	//	* makeScaleMatrix(0.5f, 0.8f, 0.8f)
	//	* rotateX(-3.0f * M_PI / 180.f),
	//	"glass");

	//addModel("../models/gun.obj", &gunShader,
	//	makeTranslationMatrix(Eigen::Vector3f(5.5f, -0.8f, -4.8f))
	//	* makeScaleMatrix(2.0f)
	//	* rotateY(-55.f * M_PI / 180.f),
	//	"gun");

	//addModel("../models/floor2.obj", &floor2Shader,
	//	makeTranslationMatrix(Eigen::Vector3f(1.7f, -2.4f, 2.9f)),
	//	"floor2");

	//addModel("../models/door.obj", &doorShader,
	//	makeTranslationMatrix(Eigen::Vector3f(3.3f, -1.5f, 2.6f))* rotateY(M_PI),
	//	"door");

	//addModel("../models/wall.obj", &wallShader,
	//	makeTranslationMatrix(Eigen::Vector3f(5.0f, -1.9f, 4.1f)),
	//	"wall");

	//addModel("../models/sign.obj", &signShader,
	//	makeTranslationMatrix(Eigen::Vector3f(3.8f, 0.05f, 0.8f))
	//	* makeScaleMatrix(0.3f)
	//	* rotateY(M_PI),
	//	"sign");





	// Declare all models at main scope so they stay alive for the full program
// (BVHNode may store a pointer/reference to the Model internally)
	Model cubeModel("../models/cube.obj");
	Model buttonModel("../models/button.obj");
	Model floor1Model("../models/floor1.obj");
	Model glassModel("../models/glass.obj");
	Model gunModel("../models/gun.obj");
	Model floor2Model("../models/floor2.obj");
	Model doorModel("../models/door.obj");
	Model wallModel("../models/wall.obj");
	Model signModel("../models/sign.obj");

	// Check for missing normals before adding anything
	auto checkNormals = [](const Model& m, const char* name) {
		if (!m.hasNormals())
			std::cerr << "WARNING: " << name << " has no normals - re-export with normals!\n";
		};

	checkNormals(cubeModel, "cube");
	checkNormals(buttonModel, "button");
	checkNormals(floor1Model, "floor1");
	checkNormals(glassModel, "glass");
	checkNormals(gunModel, "gun");
	checkNormals(floor2Model, "floor2");
	checkNormals(doorModel, "door");
	checkNormals(wallModel, "wall");
	checkNormals(signModel, "sign");

	// Now add to scene — models are guaranteed to stay alive
	//pos= HORIZONTAL/LR, VERTICAL/HEIGHT, DEPTH
	scene.renderables.push_back(std::make_shared<BVHNode>(
		cubeModel, &cubeShader, 4,
		makeTranslationMatrix(Eigen::Vector3f(2.7f, -0.8f, -1.8f))* 
		makeScaleMatrix(0.45f)
	));

	scene.renderables.push_back(std::make_shared<BVHNode>(
		buttonModel, &buttonShader, 4,
		makeTranslationMatrix(Eigen::Vector3f(2.3f, -1.5f, -1.7f))* 
		makeScaleMatrix(0.7f)
	));

	scene.renderables.push_back(std::make_shared<BVHNode>(
		floor1Model, &floor1Shader, 4,
		makeTranslationMatrix(Eigen::Vector3f(0.0f, -2.0f, 0.0f))
	));

	scene.renderables.push_back(std::make_shared<BVHNode>(
		glassModel, &mirrorShader, 4,
		makeTranslationMatrix(Eigen::Vector3f(-1.6f, 0.8f, -1.2f))
		* makeScaleMatrix(0.5f, 0.8f, 0.8f)
		* rotateX(-3.0f * M_PI / 180.f)
	));

	scene.renderables.push_back(std::make_shared<BVHNode>(
		gunModel, &gunShader, 4,
		makeTranslationMatrix(Eigen::Vector3f(6.8f, -0.5f, -4.8f))
		* makeScaleMatrix(2.0f)
		* rotateY(-55.f * M_PI / 180.f)
	));

	scene.renderables.push_back(std::make_shared<BVHNode>(
		floor2Model, &floor2Shader, 4,
		makeTranslationMatrix(Eigen::Vector3f(1.7f, -2.4f, 2.9f))
	));

	scene.renderables.push_back(std::make_shared<BVHNode>(
		doorModel, &doorShader, 4,
		makeTranslationMatrix(Eigen::Vector3f(3.0f, -1.5f, 4.0f))* 
		rotateY(M_PI)
	));

	scene.renderables.push_back(std::make_shared<BVHNode>(
		wallModel, &wallShader, 4,
		makeTranslationMatrix(Eigen::Vector3f(5.0f, -1.9f, 4.1f))
	));

	scene.renderables.push_back(std::make_shared<BVHNode>(
		signModel, &signShader, 4,
		makeTranslationMatrix(Eigen::Vector3f(2.0f, 0.05f, 7.2f))
		* makeScaleMatrix(0.8f)
		* rotateY(145.0f * M_PI / 180.0f)
	));









	// *** Add lights to scene ***
	Eigen::Vector3f ambientLight(0.5f, 0.5f, 0.5f);

	std::vector<std::unique_ptr<Light>> lightSources;
	//lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(-1.f, 3.f, -1.f), 3.f * Eigen::Vector3f(1.f, 1.f, 1.f)));
	//elevated point light so it shines down
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(0.f, 4.f, -2.2f), 18.f * Eigen::Vector3f(1.f, 1.f, 1.f)));
	//lightSources.push_back(std::make_unique<DirectionalLight>(Eigen::Vector3f(0.f, -1.f, 1.f), .5f * Eigen::Vector3f(1.f, 1.f, 1.f)));
		


	// *** Render the scene ***

	// Shuffling the scanline order gets better CPU usage between threads
	// when some lines take longer to render than others.
	std::vector<unsigned int> scanlines(pixHeight);
	for (int i = 0; i < pixHeight; ++i) scanlines[i] = i;

	if (config["shuffleScanlines"]) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(scanlines.begin(), scanlines.end(), g);
	}

	auto startTime = std::chrono::steady_clock::now();

	Ray ray = cam.getRay(531, 325);
	HitInfo hitInfo;
	scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK);
	float x = hitInfo.hitT;


	#pragma omp parallel for
	for (int y = 0; y < pixHeight; ++y) {
		for (int x = 0; x < pixWidth; ++x) {
			Ray ray = cam.getRay(x, scanlines[y]);
			HitInfo hitInfo;
			if (scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK)) {
				if (hitInfo.shader == nullptr)
				{
					std::cout << "NULL SHADER!" << std::endl;
					continue;
				}
				Eigen::Vector3f color = hitInfo.shader->getColor(
					hitInfo, &scene,
					lightSources, ambientLight,
					0, config["maxBounces"]);

				color.x() = std::min(color.x(), 1.f);
				color.y() = std::min(color.y(), 1.f);
				color.z() = std::min(color.z(), 1.f);


				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = color.x() * 255;
				outImage[(x + line * pixWidth) * nChannels + 1] = color.y() * 255;
				outImage[(x + line * pixWidth) * nChannels + 2] = color.z() * 255;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
			else {
				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = 0;
				outImage[(x + line * pixWidth) * nChannels + 1] = 0;
				outImage[(x + line * pixWidth) * nChannels + 2] = 0;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
		}
		if (omp_get_thread_num() == omp_get_num_threads()-1) {
			std::clog << "\rScanlines remaining: " << (pixHeight - y) << ' ' << std::flush;
		}

	}

	auto renderTime = std::chrono::steady_clock::now() - startTime;

	std::cout << "Render duration " << std::chrono::duration_cast<std::chrono::milliseconds>(renderTime).count() * 1e-3f << " seconds." << std::endl;

	// *** Save the output image ***
	int errorCode;
	errorCode = lodepng::encode(config["outputFilename"], outImage, pixWidth, pixHeight);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
