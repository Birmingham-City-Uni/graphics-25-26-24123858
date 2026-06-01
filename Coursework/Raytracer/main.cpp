//RAYTRACER: FEATURES IMPLEMENTED

//Gammma correction
//converts linear light values to sRGB
//need more info

//Additional light type: Spotlight
//SpotLight.hpp created and added into scene, for final scene point light was used 
//as it better matched reference image
//need more info

//Emissive lighting (cube, button, door, sign)
//EmissiveShader.hpp created and added to scene to support emissive texture png files
//to allow parts of a texture to appear illuminated independently of scene lighting 
//Custom EmissiveShader was developed using a decorator style approach,
//wrapping an existing material shader and adding an emissive texture pass
//Emissive texture values are sampled using the interpolated UV coordinates
//converted from srgb into linear colour space, then added directly to the final shaded result

//Textured phong shading (cube, button, gun) using Blinn-Phong
//TexturedPhongShader used to combine texture mapping with Blinn-Phong lighting
//Shader supports normal maps and specular maps
//Surface colour is sampled from an albedo (diffuse) texture, diffuse
//lighting is calculated using Lamberts cosine law, specular highlights are
//generated using the BlinnPhong half vector

//Normal maping (cube, button, gun)
//Implemented to add surface detail without increasing mesh complexity
//Project was extended to calculate tangent and bitangent vectors for each 
//triangle from its geometry and UV coordinates
//Vectors are stored in HitInfo and with the interpolated surface normal form TBN
//(tangent, bitangent, normal) 
//Normals sampled from the normal map are transformed from tangent space into world space 
//using the TBN matrix and used during lighting calculations
//This then allows surface detail to influence diffuse and specular lighting with no modification to mesh

//Specular maps(cube, button, gun)
//Allow different areas of a model to have varying reflection across the surface on a perpixel basis
//Brightness of the specular texture is sampled and used to influence the strength and sharpness of 
//Blinn-Phong specular highlights
//Allowing different parts of a model to appear sniny/matte 

//Frosted mirror
//

//Soft shadows
//


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
#include "SpotLight.hpp"
#include "LambertianShader.hpp"
#include "TexturedLambertianShader.hpp"
#include "PhongShader.hpp"
#include "MirrorShader.hpp"
#include "TexCoordTestShader.hpp"
#include "EmissiveShader.hpp"
#include "Model.hpp"
#include "TexturedPhongShader.hpp"
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
Eigen::Matrix4f makeScaleMatrix(float s) 
{ 
	return makeScaleMatrix(s, s, s); 
}


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
		70.f //same fov as rasteriser
	);


	std::vector<uint8_t> outImage(pixHeight * pixWidth * nChannels);

	Eigen::Vector3f
		red(1.f, 0.f, 0.f),
		blue(0.f, 0.f, 1.f),
		aqua(0.f, .8f, .8f),
		lavender(178.f / 255.f, 164.f / 255.f, 212.f / 255.f);

	// *** Load shaders and textures ***
	
	//loading textures (albedo, normal, specular, emissive texture maps)
	auto loadTex = [](const std::string& path, std::vector<uint8_t>& data, unsigned int& w, unsigned int& h)
		{
			unsigned int err = lodepng::decode(data, w, h, path);
			if (err) throw std::runtime_error("Failed to load texture: " + path + " — " + lodepng_error_text(err));
		};

	//std::vector<uint8_t> spotTexture;
	//unsigned int width, height;
	//lodepng::decode(spotTexture, width, height, "../models/spot.png");
	
	unsigned int tw, th; //new texture width and height, reused for every texture load without declaring a new pair each time

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
	//TexturedLambertianShader cubeShader(&cubeTex, cubeW, cubeH);
	//TexturedLambertianShader buttonShader(&buttonTex, buttonW, buttonH);
	//PhongShader gunShader(Eigen::Vector3f(1, 1, 1), Eigen::Vector3f(1, 1, 1), 80.f); //gun has spec map in rasteriser, for now using Phong with white base
	//TexturedLambertianShader gunShader(&gunTex, gunW, gunH);
	//TexturedLambertianShader glassShader(&glassTex, glassW, glassH); //uses mirror shader
	TexturedLambertianShader doorShader(&doorTex, doorW, doorH);
	TexturedLambertianShader wallShader(&wallTex, wallW, wallH);
	TexturedLambertianShader signShader(&signTex, signW, signH);
	//flat colour shaders for floor1 and floor2
	LambertianShader floor1Shader(Eigen::Vector3f(0.85f, 0.83f, 0.80f)); //whiteish
	LambertianShader floor2Shader(Eigen::Vector3f(0.18f, 0.18f, 0.18f)); //dark grey


	//Load normal and spec maps
	//normal maps store perpixel surface directions that are used to simulate detail
	//specular maps control how reflective parts of a surface appear
	//white areas on map= reflective 
	//darker areas= more matte/not reflective
	std::vector<uint8_t> cubeNormTex, cubeSpecTex, buttonNormTex, buttonSpecTex, gunNormTex, gunSpecTex;

	loadTex("../models/cubeNorm.png", cubeNormTex, tw, th); unsigned int cubeNormW = tw, cubeNormH = th;
	loadTex("../models/cubeSpec.png", cubeSpecTex, tw, th); unsigned int cubeSpecW = tw, cubeSpecH = th;
	loadTex("../models/buttonNorm.png", buttonNormTex, tw, th); unsigned int buttonNormW = tw, buttonNormH = th;
	loadTex("../models/buttonSpec.png", buttonSpecTex, tw, th); unsigned int buttonSpecW = tw, buttonSpecH = th;
	loadTex("../models/gunNorm.png", gunNormTex, tw, th); unsigned int gunNormW = tw, gunNormH = th;
	loadTex("../models/gunSpec.png", gunSpecTex, tw, th); unsigned int gunSpecW = tw, gunSpecH = th;

	//create TexturedPhongShader materials
	//these combine texture mapping with BlinnPhong lighting
	//and support both normal maps and spec maps
	TexturedPhongShader cubePhongShader(&cubeTex, cubeW, cubeH, 60.f);
	cubePhongShader.setNormalMap(&cubeNormTex, cubeNormW, cubeNormH);
	cubePhongShader.setSpecularMap(&cubeSpecTex, cubeSpecW, cubeSpecH);

	TexturedPhongShader buttonPhongShader(&buttonTex, buttonW, buttonH, 60.f);
	buttonPhongShader.setNormalMap(&buttonNormTex, buttonNormW, buttonNormH);
	buttonPhongShader.setSpecularMap(&buttonSpecTex, buttonSpecW, buttonSpecH);

	TexturedPhongShader gunPhongShader(&gunTex, gunW, gunH, 80.f);
	gunPhongShader.setNormalMap(&gunNormTex, gunNormW, gunNormH);
	gunPhongShader.setSpecularMap(&gunSpecTex, gunSpecW, gunSpecH);

	//emissive tex
	//bright parts of image= self illuminated areas
	//these contribute/add light independently of scene illumination and lighting
	std::vector<uint8_t> cubeEmisTex, buttonEmisTex, doorEmisTex, signEmisTex;

	loadTex("../models/cubeEmis.png", cubeEmisTex, tw, th); unsigned int cubeEmisW = tw, cubeEmisH = th;
	loadTex("../models/buttonEmis.png", buttonEmisTex, tw, th); unsigned int buttonEmisW = tw, buttonEmisH = th;
	loadTex("../models/doorEmis.png", doorEmisTex, tw, th); unsigned int doorEmisW = tw, doorEmisH = th;
	loadTex("../models/signEmis.png", signEmisTex, tw, th); unsigned int signEmisW = tw, signEmisH = th;

	//emissive shaders (wrap the existing texture shaders)
	//emissive tex is sampled separately and added to the final shaded colour to create glowing parts on models
	EmissiveShader cubeEmissiveShader(&cubePhongShader, &cubeEmisTex, cubeEmisW, cubeEmisH, 2.5f);
	EmissiveShader buttonEmissiveShader(&buttonPhongShader, &buttonEmisTex, buttonEmisW, buttonEmisH, 2.5f);
	EmissiveShader doorEmissiveShader(&doorShader, &doorEmisTex, doorEmisW, doorEmisH, 2.0f);
	EmissiveShader signEmissiveShader(&signShader, &signEmisTex, signEmisW, signEmisH, 2.5f);


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


	//declaring all models at main so they stay alive
	Model cubeModel("../models/cube.obj");
	Model buttonModel("../models/button.obj");
	Model floor1Model("../models/floor1.obj");
	Model glassModel("../models/glass.obj");
	Model gunModel("../models/gun.obj");
	Model floor2Model("../models/floor2.obj");
	Model doorModel("../models/door.obj");
	Model wallModel("../models/wall.obj");
	Model signModel("../models/sign.obj");

	//check for missing normals before adding anything due to a previous error loading models
	auto checkNormals = [](const Model& m, const char* name) 
		{
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

	//add to scene 
	//pos= HORIZONTAL/LR, VERTICAL/HEIGHT, DEPTH
	scene.renderables.push_back(std::make_shared<BVHNode>
		(
			cubeModel, &cubeEmissiveShader, 4,
			makeTranslationMatrix(Eigen::Vector3f(2.6f, -1.1f, -1.7f))* 
			makeScaleMatrix(0.45f)
		));
	
	scene.renderables.push_back(std::make_shared<BVHNode>
		(
			buttonModel, &buttonEmissiveShader, 4,
			makeTranslationMatrix(Eigen::Vector3f(2.3f, -1.8f, -1.7f))* 
			makeScaleMatrix(0.7f)
		));

	scene.renderables.push_back(std::make_shared<BVHNode>
		(
			floor1Model, &floor1Shader, 4,
			makeTranslationMatrix(Eigen::Vector3f(0.0f, -2.0f, 0.0f))
		));

	scene.renderables.push_back(std::make_shared<BVHNode>
		(
			glassModel, &mirrorShader, 4,
			makeTranslationMatrix(Eigen::Vector3f(-1.6f, 0.8f, -1.2f))
			* makeScaleMatrix(0.5f, 0.8f, 0.8f)
			* rotateX(-3.0f * M_PI / 180.f)
		));

	scene.renderables.push_back(std::make_shared<BVHNode>
		(
			gunModel, &gunPhongShader, 4,
			makeTranslationMatrix(Eigen::Vector3f(6.8f, -0.5f, -4.8f))
			* makeScaleMatrix(2.0f)
			* rotateY(-55.f * M_PI / 180.f)
		));

	scene.renderables.push_back(std::make_shared<BVHNode>
		(
			floor2Model, &floor2Shader, 4,
			makeTranslationMatrix(Eigen::Vector3f(1.7f, -2.4f, 2.9f))
		));

	scene.renderables.push_back(std::make_shared<BVHNode>
		(
			doorModel, &doorEmissiveShader, 4,
			makeTranslationMatrix(Eigen::Vector3f(2.5f, -2.0f, 5.5f))* 
			makeScaleMatrix(1.5f)*
			rotateY(M_PI)
		));

	scene.renderables.push_back(std::make_shared<BVHNode>
		(
			wallModel, &wallShader, 4,
			makeTranslationMatrix(Eigen::Vector3f(5.0f, -1.9f, 4.1f))
		));

	scene.renderables.push_back(std::make_shared<BVHNode>
		(
			signModel, &signEmissiveShader, 4,
			makeTranslationMatrix(Eigen::Vector3f(2.0f, 0.05f, 7.2f))
			* makeScaleMatrix(0.8f)
			* rotateY(145.0f * M_PI / 180.0f)
		));



	// *** Add lights to scene ***
	Eigen::Vector3f ambientLight(0.3f, 0.3f, 0.3f);

	std::vector<std::unique_ptr<Light>> lightSources;
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(0.0f, 4.0f, -2.2f), 18.0f * Eigen::Vector3f(1.0f, 1.0f, 1.0f)));
	//lightSources.push_back(std::make_unique<DirectionalLight>(Eigen::Vector3f(0.f, -1.f, 1.f), .5f * Eigen::Vector3f(1.f, 1.f, 1.f)));
	
	//spotlight: implemented but not used for final scene as point light matched ref image more accurately
	//lightSources.push_back(std::make_unique<SpotLight>(Eigen::Vector3f(0.f, -1.f, -2.2f), 18.f * Eigen::Vector3f(1.5f, 1.5f, 1.5f), Eigen::Vector3f(15.f, -2.f, -10.f).normalized(), M_PI / 3.f));
	//position, colour * intensity, direction it points, cone half-angle (60 deg)

	// *** Render the scene ***

	// Shuffling the scanline order gets better CPU usage between threads
	// when some lines take longer to render than others.
	std::vector<unsigned int> scanlines(pixHeight);
	for (int i = 0; i < pixHeight; ++i) scanlines[i] = i;

	if (config["shuffleScanlines"]) 
	{
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
	for (int y = 0; y < pixHeight; ++y) 
	{
		for (int x = 0; x < pixWidth; ++x) 
		{
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

				//color.x() = std::min(color.x(), 1.f);
				//color.y() = std::min(color.y(), 1.f);
				//color.z() = std::min(color.z(), 1.f);

				//gamma correction (with clamp for negative values) to convert linear light values to srgb 
				//color.x() = std::min(powf(color.x(), 1.f / 2.2f), 1.f);
				//color.y() = std::min(powf(color.y(), 1.f / 2.2f), 1.f);
				//color.z() = std::min(powf(color.z(), 1.f / 2.2f), 1.f);
				//with clamp
				color.x() = std::min(powf(std::max(color.x(), 0.f), 1.f / 2.2f), 1.f);
				color.y() = std::min(powf(std::max(color.y(), 0.f), 1.f / 2.2f), 1.f);
				color.z() = std::min(powf(std::max(color.z(), 0.f), 1.f / 2.2f), 1.f);

				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = color.x() * 255;
				outImage[(x + line * pixWidth) * nChannels + 1] = color.y() * 255;
				outImage[(x + line * pixWidth) * nChannels + 2] = color.z() * 255;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
			else 
			{
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
	if (errorCode) 
	{   //check the error code, in case an error occurred
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
