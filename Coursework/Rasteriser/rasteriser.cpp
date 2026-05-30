//RASTERISER: FEATURES IMPLEMENTED

//Z buffering (depth testing)
//A depth buffer was implemented to ensure objects are rendered in the correct order
//based on their distance from the camera. Each pixel stores the closest depth value 
//rendered so far, and any pixels that are further away are discarded. This prevents 
//objects from drawing over each other incorrectly and removes hindden surfaces from the scene.

//Texture mapping
//Texture mapping was implemented to apply png textures to 3d models using uv
//coordinates imported from the mesh. UV coords are perspective correctly 
//interpolated across each triangle and used to sample the texture for every pixel.
//Gamma correction is also applied, with textures converted from sRGB into linear 
//colour space before lighting calculations and converted back before being written 
//to the final image. Objects without textures can also be rendered using a flat 
//colour material.

//Multiple light types (including polymorphism)
//Renderer supports multiple light types through object oriented lighting system.
//A common Light base class is used by the renderer and AmbientLight,
//PointLight, DirectionalLight and SpotLight are implemented as derived classes. Each light 
//calculates its own direction and intensity behaviour, which allows different light types to be 
//used together within the scene. This design uses inheritance and polymorphism,
//enabling new light types to be added without modifying the rendering pipeline.
//My final scene uses both ambient and spotlight lighting, with other lighting implemented but not used.

//Blinn-Phong and Phong shading
//Both Phong and Blinn-Phong shading models were added to simulate realistic 
//lighting on object surfaces. Diffuse lighting is calculated using Lambers cosine 
//law and specular highlights can be created using either Phong reflection model or 
//Blinn-Phong halfway vector approach (more effectve)
//The active shading model can be set through the renderer, and I chose to use
//Blinn-Phong in my final sccene.

//OOP/c++ paradigms
//Object oriented programming principles were incluses and used to keep the code reusable,
//modular and easier to maintain. The new Scene class manages rendering, objects, 
//lights, cameras and buffers. SceneObject stores mesh, texture and material 
//information for each assets. Inheritance and polymorphism are used throughout 
//the lighting system, and pointers are used to manage memory automatically 
//without manual deletion.
//The project uses encapsulation through the Scene and SceneObject classes, inheritance
//through the Light hierarchy, abstraction through the Light base class interface and 
//polymorphism through lighting functions.

//Backface culling
//Implemented to avoid rendering triangles that face away from the camera
//Triangles with negative signed screenspace area are skipped before being
//rasterised to avoid rendering rear facing geometry

//Perspective correct interpolation
//Used when calculating UV coordinates, world positions and surface normals across triangles
//This prevents distortion and texture warping (occurs with screenspace interpolation) and 
//makes sure attributes stay accuratw no matter viewing angle or depth

//Alpha blending/translucency (glass obj)
//Transparency system was implemented to support the frosted glass panel within 
//the scene. The glass texture acts as an opacity mask, where darker areas become 
//more transparent and lighter areas remain visible (frosted). Transparent pixels are blended 
//with the existing framebuffer colour using alpha blending, so the geometry behind 
//the glass will be visible while having the appearance of a translucent surface.
//Non transparent objects are rendered first, followed by transparent objects to ensure correct 
//blending results, otherwise blending will result in black

//Normal mapping (button, cube, gun)
//Normal mapping was implemented to add additional surface detail without increasing mesh complexity
//Normal maps store perpixel surface directions which are transformed from tangent space into world
//space using the TBN matrix generated for each triangle
//These normals are then used during the lighting calculations, allowing surface details to affect 
//diffuse and specular lighting even though the actual geometry doesnt change

//Specular mapping (button, cube, gun)
//Was implemented to vary shininess of a model (on a perpixel basis)
//The renderer samples a specular texture and uses its brightness to control the 
//strength/sharpness of specular highlights, allowing different parts of the same 
//object to appear glossy/metallic/matte
//This creates more realistic materials than using a single specular value across the entire surface

//Emissive mapping (button, cube, door, sign)
//Emissive mapping was implemented to allow selected areas of a texture to appear 
//illuminated 
//Emissive textures are sampled independently of the scene lighting and added directly to the final 
//shaded result, allowing objects to appear as though they are glowing, even when no light source 
//is directly affecting them
//Intensity of the effect can be adjusted through a configurable emissive strength value, to vary 
//across models
//black pixels= no emission (normal shading), coloured/bright pixels = glowing
//This is how game engines make neon signs/glowing screens/LEDs look lit up




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


    //following models included but not used as positioning the scene exactly as blender and ref 
    //image was difficult to get right in a one dimensional render of image
    
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


