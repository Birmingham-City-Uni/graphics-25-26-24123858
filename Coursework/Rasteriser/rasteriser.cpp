//RASTERISER: FEATURES IMPLEMENTED

//1. Z buffering (depth testing)
//A depth buffer was implemented to ensure objects are rendered in the correct order
//based on their distance from the camera.Each pixel stores the closest depth value 
//rendered so far, and any pixels that are further away are discarded.This prevents 
//objects from incorrectly drawing over each other and ensures proper hidden surface 
//removal throughout the scene.The depth test is performed per - pixel during triangle
//rasterisation using perspective - correct depth values.

//2. Texture mapping
//Texture mapping was implemented to apply PNG textures to 3D models using UV 
//coordinates imported from the mesh.UV coordinates are perspective - correctly 
//interpolated across each triangle and used to sample the texture for every pixel.
//Gamma correction is also applied, with textures converted from sRGB into linear 
//colour space before lighting calculations and converted back before being written 
//to the final image.Objects without textures can also be rendered using a flat 
//colour material.

//3. Multiple light types (polymorphism)
//The renderer supports multiple light types through an object - oriented lighting 
//system.A common Light base class is used, with Ambient, Point, Directional and 
//Spot lights implemented as derived classes.Each light calculates its own direction 
//and intensity behaviour, allowing different types of lights to be used together 
//within the same scene.The final scene uses both ambient and spotlight lighting.

//4. Blinn-Phong and Phong shading
//Both Phong and Blinn - Phong shading models were implemented to simulate realistic 
//lighting on object surfaces.Diffuse lighting is calculated using Lambert's cosine 
//law, while specular highlights can be generated using either the traditional 
//Phong reflection model or the more efficient Blinn-Phong halfway vector approach. 
//The active shading model can be selected through the renderer settings, with 
//Blinn-Phong being used in the final scene.

//5. OOP/c++ paradigms
//The renderer was developed using object - oriented programming principles to keep 
//the code modular and easier to maintain.The Scene class manages rendering, objects, 
//lights, cameras and buffers, while SceneObject stores mesh, texture and material 
//information for individual assets.Inheritance and polymorphism are used throughout 
//the lighting system, and smart pointers are used to manage memory automatically 
//without manual deletion.

//6. Backface culling
//Implemented to avoid rendering triangles that face away 
//from the camera.
//Triangles with negative signed screen-space area are skipped before
//rasterisation, avoiding rendering of rear-facing geometry.

//7. Perspective correct interpolation
//Perspective - correct interpolation is used when calculating UV coordinates, 
//world positions and surface normals across triangles.This prevents the distortion 
//and texture warping that occurs with simple screen - space interpolation and 
//ensures attributes remain accurate regardless of viewing angle or depth.

//8. Alpha blending/translucency (glass obj)
//A transparency system was implemented to support the frosted glass panel within 
//the scene.The glass texture acts as an opacity mask, where darker areas become 
//more transparent and lighter areas remain visible.Transparent pixels are blended 
//with the existing framebuffer colour using alpha blending, allowing geometry behind 
//the glass to remain visible while maintaining the appearance of a translucent surface.
//Opaque objects are rendered first, followed by transparent objects to ensure correct 
//blending results.

//9. Normal mapping
//Normal mapping was implemented to add additional surface detail without increasing 
//mesh complexity.Normal maps store per - pixel surface directions which are 
//transformed from tangent space into world space using a TBN matrix generated for 
//each triangle.These modified normals are then used during lighting calculations, 
//allowing small bumps, grooves and surface details to affect both diffuse and specular 
//lighting even though the underlying geometry remains unchanged.

//10. Specular/metallic Mapping
//Specular mapping was implemented to vary shininess across a model on a per - pixel 
//basis.The renderer samples a specular texture and uses its brightness to control the 
//strength and sharpness of specular highlights.This allows different parts of the same 
//object to appear glossy, metallic or matte, producing more realistic material
//responses than using a single specular value across the entire surface.

//11. Emissive mapping (cube and sign)
//Emissive mapping was implemented to allow selected areas of a texture to appear 
//self illuminated.Emissive textures are sampled independently of the scene lighting 
//and added directly to the final shaded result.This allows objects such as the cube 
//panels, sign, button and door elements to appear as though they are glowing, even 
//when no light source is directly affecting them.The intensity of the effect can be 
//adjusted through a configurable emissive strength value.
//Stores the colour and intensity of light that this surface
//emits by itself, completely independent of any light sources in the scene.
//black pixels = no emission (normal shading), coloured/bright pixels = glowing.
//this is how game engines make neon signs, glowing screens, LEDs etc look lit up






//this define is necessary to get the M_PI constant
#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include "LinAlg.hpp"
#include "Scene.hpp" //Scene and SceneObject classes now pulls in everything for models



//projection matrix helper
//builds a perspective projection matrix from field of view, aspect ratio,
//and near/far clip distances
//kept as a free function since its a oneoff calculation rather than something belonging to a class
Eigen::Matrix4f projectionMatrix(int height, int width, float horzFov = 70.0f * M_PI / 180.0f, float zFar = 10.0f, float zNear = 0.1f)
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


int main()
{
    const int width = 1920, height = 1080;

    //camera and projection setup 
    //build the camera to world matrix (where the camera is in the world
    //and what direction its pointing)
    // invert it to get world to camera
    //float zNear = 0.1f;
    //float zFar = 100.0f;
    Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(7.0f, 0.7f, -9.0f))* 
    rotateYMatrix(-35 * M_PI / 180);
    //rotateXMatrix(-10* M_PI / 180);

    Eigen::Vector3f camWorldPos = (cameraToWorld * Eigen::Vector4f(0, 0, 0, 1)).block<3, 1>(0, 0);
    Eigen::Matrix4f worldToCamera = cameraToWorld.inverse();
    Eigen::Matrix4f projection = projectionMatrix(height, width, 70.0f * M_PI / 180.0f, 100.0f, 0.1f); //zFar, zNear //zFar=100 zNear=0.1 uncomment from above


    //create the scene

    //the Scene constructor sets up the image buffer, depth buffer,
    //and stores the camera/projection matrices ready for rendering
    Scene scene(width, height, worldToCamera, projection, camWorldPos, ShadingMode::BLINN_PHONG);


    //lights: ambient and point
    //lights are added via addLight() using unique_ptr for ownership
    //the light base class uses polymorphism: can add any type of light (Ambient, Point, Directional, Spot) to the same list
    //SpotLight arguments: intensity, world pos, direction it points, cone angle (radians)
    scene.addLight(std::make_unique<AmbientLight>(Eigen::Vector3f(0.5f, 0.5f, 0.5f)));
    //scene.addLight(std::make_unique<PointLight>(8.0f * Eigen::Vector3f(1.1f, 1.1f, 1.1f), Eigen::Vector3f(-5.0f, 4.0f, 5.0f))); 
    scene.addLight(std::make_unique<SpotLight>(12.0f * Eigen::Vector3f(1.5f, 1.5f, 1.5f), Eigen::Vector3f(-0.0f, -1.0f, -2.2f), Eigen::Vector3f(15.0f, -2.0f, -10.0f).normalized(), M_PI / 3.0f));                                   
    //intensity, position, direction, cone half-angle (45 degrees) 
    //pos= HORIZONTAL/LR, VERTICAL/HEIGHT, DEPTH
   

    //adding scene objects

    //SceneObject::loadFromFile() loads the mesh + texture from file and packages them into a SceneObject
    //each object has its own transform (position, rotation, scale) and material (specular colour + shininess exponent)
    //This replaces the old approach of separate Mesh variables, separate texture vectors, and separate transform matrices all in main()

    //cube (has normal, specular, emissive maps)
    Eigen::Matrix4f transform1 =
        translationMatrix(Eigen::Vector3f(2.6f, -0.3f, -3.15f)) *
        scaleMatrix(Eigen::Vector3f(0.45f, 0.45f, 0.45f));

    SceneObject cube = SceneObject::loadFromFile(
        "../models/cube.obj",
        "../models/cubeTex.png",
        transform1,
        Eigen::Vector3f::Ones(),//specular colour: white
        60.0f //specular exponent
    );
    cube.setNormalMap("../models/cubeNorm.png");  
    cube.setSpecularMap("../models/cubeSpec.png");
    cube.setEmissiveMap("../models/cubeEmis.png");
    cube.emissiveStrength = 2.5f;
    scene.addObject(cube);
    

    //button (has normal, specular, emissive maps)
    Eigen::Matrix4f transform2 =
        translationMatrix(Eigen::Vector3f(2.2f, -1.0f, -3.0f)) *
        scaleMatrix(Eigen::Vector3f(0.7f, 0.7f, 0.7f));

    SceneObject button = SceneObject::loadFromFile(
        "../models/button.obj",
        "../models/buttonTex.png",
        transform2,
        Eigen::Vector3f::Ones(),
        60.0f
    );
    button.setNormalMap("../models/buttonNorm.png");
    button.setSpecularMap("../models/buttonSpec.png");
    button.setEmissiveMap("../models/buttonEmis.png");
    button.emissiveStrength = 2.5f;
    scene.addObject(button);
    

    //floor
    Eigen::Matrix4f transform3 =
        translationMatrix(Eigen::Vector3f(0.0f, -2.0f, 0.0f)) *
        scaleMatrix(Eigen::Vector3f(1.0f, 1.0f, 1.0f));
        //rotateXMatrix(M_PI);


    scene.addObject(SceneObject::loadFromFileColour
    (
        "../models/floor1.obj",
        Eigen::Vector3f(0.85f, 0.83f, 0.80f),  //offwhite
        transform3,
        Eigen::Vector3f::Ones(), 
        50.0f                     
    ));


    //glass (has transparency)
    Eigen::Matrix4f transform4 =
        translationMatrix(Eigen::Vector3f(-1.6f, 0.8f, -1.2f)) *
        scaleMatrix(Eigen::Vector3f(0.5f, 0.8f, 0.8f)) *
        rotateXMatrix(-3.0f * M_PI / 180);

    scene.addObject(SceneObject::loadFromFileTransparent
    (
        "../models/glass.obj",
        "../models/glassTex.png",
        transform4,
        0.65f, //transparency strength 0= clear 1= fully frosted
        Eigen::Vector3f::Ones(), 
        50.0f                     
    ));


    //gun (has normal and specular map)
    Eigen::Matrix4f transform5 =
        translationMatrix(Eigen::Vector3f(5.5f, -0.8f, -4.8f)) *
        scaleMatrix(Eigen::Vector3f(2.0f, 2.0f, 2.0f)) *
        rotateYMatrix(-55 * M_PI / 180);

    SceneObject gun = SceneObject::loadFromFile(
        "../models/gun.obj",
        "../models/gunTex.png",
        transform5,
        Eigen::Vector3f::Ones(),
        80.0f
    );
    gun.setNormalMap("../models/gunNorm.png");
    gun.setSpecularMap("../models/gunSpec.png");
    scene.addObject(gun);
    

    //floor2
    Eigen::Matrix4f transform6 =
        translationMatrix(Eigen::Vector3f(1.7f, -2.4f, 2.9f)) *
        scaleMatrix(Eigen::Vector3f(1.0f, 1.0f, 1.0f));

    scene.addObject(SceneObject::loadFromFileColour
    (
        "../models/floor2.obj",
        Eigen::Vector3f(0.18f, 0.18f, 0.18f), //dark grey
        transform6,
        Eigen::Vector3f::Ones(), 
        50.0f                     
    ));


    //door (has emissive map)
    Eigen::Matrix4f transform7 =
        translationMatrix(Eigen::Vector3f(3.3f, -1.5f, 2.6f)) *
        scaleMatrix(Eigen::Vector3f(1.0f, 1.0f, 1.0f)) *
        rotateYMatrix(M_PI);

    SceneObject door = SceneObject::loadFromFile
    (
        "../models/door.obj",
        "../models/doorTex.png",
        transform7,
        Eigen::Vector3f::Ones(), 
        50.0f                     
    );
    door.setEmissiveMap("../models/doorEmis.png"); 
    door.emissiveStrength = 2.0f; 
    scene.addObject(door);


    //wall
    Eigen::Matrix4f transform8 =
        translationMatrix(Eigen::Vector3f(5.0f, -1.9f, 4.1f)) *
        scaleMatrix(Eigen::Vector3f(1.0f, 1.0f, 1.0f));

    scene.addObject(SceneObject::loadFromFile
    (
        "../models/wall.obj",
        "../models/wallTex.png",
       // Eigen::Vector3f(0.10f, 0.10f, 0.10f), //darker grey
        transform8,
        Eigen::Vector3f::Ones(), 
        50.0f                    
    ));

    //sign (has emissive map)
    Eigen::Matrix4f transform9 =
        translationMatrix(Eigen::Vector3f(3.8f, 0.05f, 0.8f)) *
        scaleMatrix(Eigen::Vector3f(0.3f, 0.3f, 0.3f)) *
        rotateYMatrix(M_PI);

    SceneObject sign = SceneObject::loadFromFile(
        "../models/sign.obj",
        "../models/signTex.png",
        transform9,
        Eigen::Vector3f::Ones(),
        50.0f
    );
    sign.setEmissiveMap("../models/signEmis.png"); 
    sign.emissiveStrength = 2.5f;
    scene.addObject(sign);


    //following models included but not used as positioning the scene exactly as blender and ref image was difficult to get
    //right in a one dimensional render of image
    
    ////lights1  
    //Eigen::Matrix4f transform10 =
    //    translationMatrix(Eigen::Vector3f(6.0f, -0.5f, -5.0f)) *
    //    scaleMatrix(Eigen::Vector3f(0.3f, 0.3f, 0.3f));
    //    //rotateXMatrix(180 * M_PI / 180);

    //scene.addObject(SceneObject::loadFromFile
    //(
    //    "../models/lights1.obj",
    //    "../models/lights1Tex.png",
    //    transform10,
    //    Eigen::Vector3f::Ones(),
    //    50.0f
    //));

    ////lights2
    //Eigen::Matrix4f transform11 =
    //    translationMatrix(Eigen::Vector3f(0.0f, -0.5f, -8.0f)) *
    //    scaleMatrix(Eigen::Vector3f(0.2f, 0.2f, 0.2f));
    ////rotateYMatrix(M_PI);

    //scene.addObject(SceneObject::loadFromFile
    //(
    //    "../models/lights2.obj",
    //    "../models/lights2Tex.png",
    //    transform11,
    //    Eigen::Vector3f::Ones(),
    //    50.0f
    //));

    ////lights3
    //Eigen::Matrix4f transform12 =
    //    translationMatrix(Eigen::Vector3f(0.0f, 0.0f, 0.0f)) *
    //    scaleMatrix(Eigen::Vector3f(1.0f, 1.0f, 1.0f)) *
    //    rotateYMatrix(M_PI);

    //scene.addObject(SceneObject::loadFromFile
    //(
    //    "../models/lights3.obj",
    //    "../models/lights3Tex.png",
    //    transform12,
    //    Eigen::Vector3f::Ones(),
    //    50.0f
    //));


    //render and save

    //render() loops over all objects and draws each one using the
    //shared lights, camera and projection: all encapsulated in Scene

    scene.render();
    scene.saveImage("output.png");

    std::cout << "Render complete: output.png" << std::endl;
    return 0;
}


