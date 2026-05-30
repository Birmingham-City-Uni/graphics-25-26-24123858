#pragma once
#include <Eigen/Dense>
#include <vector>
#include <memory>
#include <lodepng.h>

#include "Mesh.hpp"
#include "Light.hpp"
#include "LinAlg.hpp"
#include "Image.hpp"
#include "Shading.hpp"

//Scene object class
//uses C++ encapsulation to group everything needed for rendering objects into the scene:
//geometry, texture, position/orientation/scale in scene (modelToWorld matrix), material properties
//Before this class existed, all of these were separate loose variables in rasteticer.cpp main and 
//had to be passed individually to drawMesh(): now grouped into a reusable scene class

class SceneObject 
{
public:
    //geometry of the object (vertices, normals, UVs, faces)
    Mesh mesh;

    //texture image data, dimensions
    std::vector<uint8_t> texture;
    int texWidth = 0;
    int texHeight = 0;

    //normal map: stores per pixel surface normals encoded as RGB
    //r=x, g=y, b=z, each in range 0-255 which represents -1 to +1
    std::vector<uint8_t> normalMap;
    int normalMapWidth = 0;
    int normalMapHeight = 0;
    bool hasNormalMap = false;

    //specular map: stores per pixel shininess/specular intensity
    //brighter pixels= shinier, darker= more matte
    std::vector<uint8_t> specularMap;
    int specularMapWidth = 0;
    int specularMapHeight = 0;
    bool hasSpecularMap = false;

    //emissive mapping members (for sign, button, cube and door objects)
    //stores the colour and intensity of light that surface
    //emits, independent of light sources in scene to make things look lit up
    //black pixels= normal shading/no emmisive, coloured/bright pixels= glowing
    std::vector<uint8_t> emissiveMap;
    int emissiveMapWidth = 0;
    int emissiveMapHeight = 0;
    bool hasEmissiveMap = false;
    float emissiveStrength = 1.0f;//emissive strength multiplier to be able to increase glow for individual onjects

    //model to world transform matrix: controls where object is placed, rotated, scaled
    Eigen::Matrix4f modelToWorld;

    //materials specular highlight colour 
    Eigen::Vector3f specularColor;

    //materials shininess
    float specularExponent;

    //if useFlatColor is true flatColor is used as the albedo instead of the texture (some models have no tex)
    bool useModelColor = false;
    Eigen::Vector3f modelColor = Eigen::Vector3f::Zero();

    //marks the obj as transparent (alpha blended) for glass obj
    //if this is true the texture alpha channel controls blending with the framebuffer
    bool isTransparent = false;
    float transparencyStrength = 1.0f; //1 to fully use texture alpha, 0 means not transparent

    //constructor: takes everything needed to define a scene object
    //initialiser list sets member variables directly instead of assigning inside the body
    SceneObject
    (
        const Mesh& mesh,
        const std::vector<uint8_t>& texture, int texWidth, int texHeight,
        const Eigen::Matrix4f& modelToWorld,
        const Eigen::Vector3f& specularColor = Eigen::Vector3f::Ones(),
        float specularExponent = 50.0f
    )
        : mesh(mesh),
        texture(texture), texWidth(texWidth), texHeight(texHeight),
        modelToWorld(modelToWorld),
        specularColor(specularColor),
        specularExponent(specularExponent)
    {
    }


    //constructor for flat model colours (no texture file)
    SceneObject
    (
        const Mesh& mesh,
        const Eigen::Vector3f& modelColor,
        const Eigen::Matrix4f& modelToWorld,
        const Eigen::Vector3f& specularColor = Eigen::Vector3f::Ones(),
        float specularExponent = 50.0f
    )
        : mesh(mesh),
        texture(), texWidth(0), texHeight(0),
        modelToWorld(modelToWorld),
        specularColor(specularColor),
        specularExponent(specularExponent),
        useModelColor(true),
        modelColor(modelColor)
    {
    }


    //helper: loads png tex map from file into vector
    //used for normal maps and specular maps 
    //returns false if the file fails to load (so missing maps dont cause issues as some models have no maps)
    static bool loadMapFromFile(const std::string& path, std::vector<uint8_t>& outData, int& outWidth, int& outHeight)
    {
        unsigned int w, h;
        unsigned int error = lodepng::decode(outData, w, h, path);
        if (error) 
        {
            //object doesnt have that map
            return false;
        }
        outWidth = (int)w;
        outHeight = (int)h;
        return true;
    }

    //attach normal map to object after construction
    void setNormalMap(const std::string& path)
    {
        hasNormalMap = loadMapFromFile(path, normalMap, normalMapWidth, normalMapHeight);
        if (!hasNormalMap)
            std::cerr << "Warning: could not load normal map: " << path << std::endl;
    }

    //attach specular map to obj after construction
    void setSpecularMap(const std::string& path)
    {
        hasSpecularMap = loadMapFromFile(path, specularMap, specularMapWidth, specularMapHeight);
        if (!hasSpecularMap)
            std::cerr << "Warning: could not load specular map: " << path << std::endl;
    }

    //attach emissive map to obj after construction
    //works the same way as setNormalMap and setSpecularMap for any object that should have glowing parts
    //objects without an emissive map are unaffected so wont cause errors/crashes
    void setEmissiveMap(const std::string& path)
    {
        hasEmissiveMap = loadMapFromFile(path, emissiveMap, emissiveMapWidth, emissiveMapHeight);
        if (!hasEmissiveMap)
            std::cerr << "Warning: could not load emissive map: " << path << std::endl;
    }

    //static method
    static SceneObject loadFromFileColour
    (
        const std::string& meshPath,
        const Eigen::Vector3f& modelColor,
        const Eigen::Matrix4f& modelToWorld,
        const Eigen::Vector3f& specularColor = Eigen::Vector3f::Ones(),
        float specularExponent = 50.0f
    )

    {
        Mesh mesh = loadMeshFile(meshPath);
        return SceneObject(mesh, modelColor, modelToWorld, specularColor, specularExponent);
    }


    //static method to load a mesh and texture from file and constructs SceneObject
    //creates and returns an instance of the class, keeping construction logic self contained
    //so settin up objects is simpler in main
    static SceneObject loadFromFile
    (
        const std::string& meshPath,
        const std::string& texturePath,
        const Eigen::Matrix4f& modelToWorld,
        const Eigen::Vector3f& specularColor = Eigen::Vector3f::Ones(),
        float specularExponent = 50.0f
    )

    {
        //load mesh geometry from the obj file
        Mesh mesh = loadMeshFile(meshPath);

        //load texture image png
        std::vector<uint8_t> texture;
        unsigned int texWidth, texHeight;
        unsigned int error = lodepng::decode(texture, texWidth, texHeight, texturePath);
        if (error) 
        {
            throw std::runtime_error("Error loading texture: " + texturePath);
        }

        //construct and return the SceneObject with all loaded data
        return SceneObject(mesh, texture, (int)texWidth, (int)texHeight,
            modelToWorld, specularColor, specularExponent);
    }



    //method for objects with alpha blending (glass obj), same as loadFromFile but sets isTransparent flag
    static SceneObject loadFromFileTransparent
    (
        const std::string& meshPath,
        const std::string& texturePath,
        const Eigen::Matrix4f& modelToWorld,
        float transparencyStrength = 1.0f,
        const Eigen::Vector3f& specularColor = Eigen::Vector3f::Ones(),
        float specularExponent = 50.0f
    )

    {
        //load mesh geom from obj
        Mesh mesh = loadMeshFile(meshPath);

        //load tex image png
        std::vector<uint8_t> texture;
        unsigned int texWidth, texHeight;
        unsigned int error = lodepng::decode(texture, texWidth, texHeight, texturePath);
        if (error) 
        {
            throw std::runtime_error("Error loading texture: " + texturePath);
        }

        SceneObject obj(mesh, texture, (int)texWidth, (int)texHeight,
            modelToWorld, specularColor, specularExponent);

        //mark as transparent so renderer handles it differently
        obj.isTransparent = true;
        obj.transparencyStrength = transparencyStrength;
        return obj;
    }

};


//shading mode enum (represents fixed set of named options)
//moved here from Rasteriser.cpp so that Scene and SceneObject
//can share this type
enum ShadingMode 
{
    PHONG,
    BLINN_PHONG
};


//triangle struct
//holds the pervertex data for a single triangle as it moves
//through the rendering pipeline (world space, camera space,
//screen space, normals, UVs)
//added here so Scene can use it internally
struct Triangle 
{
    std::array<Eigen::Vector3f, 3> screen; //coordinates of the triangle: screen space
    std::array<Eigen::Vector3f, 3> verts; //vertices of the triangle: world space
    std::array<Eigen::Vector3f, 3> cam; //vertices of the triangle: camera space
    std::array<Eigen::Vector3f, 3> norms; //normals of the triangle corners: world space
    std::array<Eigen::Vector2f, 3> texs; //texture coordinates of the triangle corners

    //TBN matrix for normal mapping
    //columns are Tangent, Bitangent, Normal in world space
    //transforms sampled tangentspace normal into world space
    Eigen::Matrix3f tbn = Eigen::Matrix3f::Identity();
};


//scene class
//encapsulates scene and is responsible for managing the entire scene: list of objects, lights, cam, image/depth buffers

class Scene 
{
public:
    //final image dimensions
    int width, height;

    //pixel colour buffer (RGBA, 8 bits per channel)
    std::vector<uint8_t> imageBuffer;

    //depth buffer (one float per pixel, initialised to 1.0)
    std::vector<float> zBuffer;

    //cams pos in world space (for specular calculation)
    Eigen::Vector3f camWorldPos;

    //World to camera and camera to clip (projection) matrices
    Eigen::Matrix4f worldToCamera;
    Eigen::Matrix4f projection;

    //which specular model to use (Phong or Blinn-Phong)
    ShadingMode shadingMode;

    //all the objects in the scene, using a vect means objects can be added/remnoved easily
    std::vector<SceneObject> objects;

    //lights in the scene
    //unique_ptr is used because Light is an abstract base class: can hold any type of light
    //(Ambient, Point, Directional, Spot) in the same list, my scene uses ambient and spot
    std::vector<std::unique_ptr<Light>> lights;

    //constructor: sets up the image and depth buffers and stores camera/projection info
    Scene
    (
        int width, int height,
        const Eigen::Matrix4f& worldToCamera,
        const Eigen::Matrix4f& projection,
        const Eigen::Vector3f& camWorldPos,
        ShadingMode shadingMode = ShadingMode::BLINN_PHONG
    )
        : width(width), height(height),
        worldToCamera(worldToCamera),
        projection(projection),
        camWorldPos(camWorldPos),
        shadingMode(shadingMode)
    {
        //allocate and clear the image buffer (black, fully opaque with full alpha)
        imageBuffer.resize(width * height * 4);
        Color black{ 0, 0, 0, 255 };
        for (int r = 0; r < height; ++r)
            for (int c = 0; c < width; ++c)
                setPixel(imageBuffer, c, r, width, height, black);

        //allocate and clear the depth buffer (1.0= max depth)
        zBuffer.assign(width * height, 1.0f);
    }

    //add object to the scene
    void addObject(const SceneObject& obj) 
    {
        objects.push_back(obj);
    }

    //add light to the scene
    void addLight(std::unique_ptr<Light> light) 
    {
        lights.push_back(std::move(light));
    }

    ////render the entire scene into the image buffer: loops over every object and draws it using the shared lights and cam
    //void render() 
    //{
    //    for (const SceneObject& obj : objects) 
    //    {
    //        drawMesh(obj);
    //    }
    //}

    void render() 
    {
        //draws all non transparent objects first so the framebuffer has them
        //before any transparent objects try to blend with it as
        //blending with an empty/black buffer gives black 
        for (const SceneObject& obj : objects) 
        {
            if (!obj.isTransparent)
                drawMesh(obj);
        }

        //then draw transparent objects on top blending with whats already there
        for (const SceneObject& obj : objects) 
        {
            if (obj.isTransparent)
                drawMesh(obj);
        }
    }

    //save the rendered image to a PNG file
    void saveImage(const std::string& filename) 
    {
        int errorCode = lodepng::encode(filename, imageBuffer, width, height);
        if (errorCode) 
        {
            throw std::runtime_error("lodepng error: " + std::string(lodepng_error_text(errorCode)));
        }
    }

private:

    //private helper findScreenBoundingBox
    //computes the axis aligned bounding box of a triangle in screen space, clamped to the image boundaries
    //used to limit the pixel loop to only the relevant region

    void findScreenBoundingBox(const Triangle& t, int& minX, int& minY, int& maxX, int& maxY)
    {
        //find a bounding box around the triangle
        minX = std::min({ (int)t.screen[0].x(), (int)t.screen[1].x(), (int)t.screen[2].x() });
        minY = std::min({ (int)t.screen[0].y(), (int)t.screen[1].y(), (int)t.screen[2].y() });
        maxX = std::max({ (int)t.screen[0].x(), (int)t.screen[1].x(), (int)t.screen[2].x() });
        maxY = std::max({ (int)t.screen[0].y(), (int)t.screen[1].y(), (int)t.screen[2].y() });

        //constrain it to keep within image
        minX = std::min(std::max(minX, 0), width - 1);
        maxX = std::min(std::max(maxX, 0), width - 1);
        minY = std::min(std::max(minY, 0), height - 1);
        maxY = std::min(std::max(maxY, 0), height - 1);
    }


    //private helper: drawTriangle
    //rasterises a single triangle into the image buffer
    //now a private method of Scene:
    //accesses the shared zBuffer, imageBuffer, lights
    //directly through the class (rather than needing them all passed as params)

    void drawTriangle(const Triangle& t, const SceneObject& obj)
    {
        int minX, minY, maxX, maxY;
        findScreenBoundingBox(t, minX, minY, maxX, maxY);

        //compute the signed area of the triangle in screen space
        //if negative, the triangle is backfacing and we skip it (back-face culling)
        Eigen::Vector2f edge1 = v2(t.screen[2] - t.screen[0]);
        Eigen::Vector2f edge2 = v2(t.screen[1] - t.screen[0]);
        float triangleArea = 0.5f * vec2Cross(edge2, edge1);
        if (triangleArea < 0) return;

        for (int x = minX; x <= maxX; ++x) 
        {
            for (int y = minY; y <= maxY; ++y) 
            {
                Eigen::Vector2f p(x, y);

                //compute the area of the three sub triangles formed with point P
                //these give us the barycentric coordinates of P within the triangle
                //find subtriangle areas
                float a0 = 0.5f * fabsf(vec2Cross(v2(t.screen[1]) - v2(t.screen[2]), p - v2(t.screen[2])));
                float a1 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[2]), p - v2(t.screen[2])));
                float a2 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[1]), p - v2(t.screen[1])));

                //find barycentrics
                float b0 = a0 / triangleArea;
                float b1 = a1 / triangleArea;
                float b2 = a2 / triangleArea;

                //if the sum of barycentrics exceeds 1 (with small tolerance), P is outside the triangle
                //if outside triangle exit early
                if (b0 + b1 + b2 > 1.0001f) continue;

                //camera space depths at each vertex (positive, since camera looks down +Z)
                //get the depths from the camera space position of the 3 corners
                //float depth0= 0.0f, depth1= 0.0f, depth2= 0.0f;
                float depth0 = -t.cam[0].z();
                float depth1 = -t.cam[1].z();
                float depth2 = -t.cam[2].z();

                //perspective correct depth at P
                //work out the depth at the point P
                float depthP = 1.0f / (b0 / depth0 + b1 / depth1 + b2 / depth2);

                //perspective correct world space position at P
                //interpolate to find the world space position of this pixel (perspective correct)
                Eigen::Vector3f worldP =
                    (t.verts[0] * (b0 / depth0) +
                        t.verts[1] * (b1 / depth1) +
                        t.verts[2] * (b2 / depth2)) * depthP;

                //perspective correct interpolated normal at P (then renormalised)
                //interpolate to find the normal of this pixel (perspective correct)
                Eigen::Vector3f normP =
                    (t.norms[0] * (b0 / depth0) +
                        t.norms[1] * (b1 / depth1) +
                        t.norms[2] * (b2 / depth2)).normalized();

                //perspective correct UV coordinates at P
                //perspective correct uv interpolation
                Eigen::Vector2f texP =
                    (t.texs[0] * (b0 / depth0) +
                        t.texs[1] * (b1 / depth1) +
                        t.texs[2] * (b2 / depth2)) * depthP;


                Eigen::Vector3f albedo;
                if (obj.useModelColor)
                {
                    //use the flat colour directly (already in linear space)
                    albedo = obj.modelColor;
                }
                else
                {
                    //convert UV to integer texture coordinates
                    //convert to texture space
                    int texX = (int)(texP.x() * (obj.texWidth - 1));
                    int texY = (int)((1.0f - texP.y()) * (obj.texHeight - 1)); // flip Y (UV origin is bottom-left)

                    //clamp to texture dimensions
                    texX = std::max(0, std::min(texX, obj.texWidth - 1));
                    texY = std::max(0, std::min(texY, obj.texHeight - 1));

                    ////gamma-decode the texture colour to bring it into linear light space
                    ////(2.2 converts from sRGB to linear)
                    ////convert to linear space (gamma correct)
                    //Eigen::Vector3f albedo
                    //(
                    //    powf(texColor.r / 255.0f, 2.2f),
                    //    powf(texColor.g / 255.0f, 2.2f),
                    //    powf(texColor.b / 255.0f, 2.2f)
                    //);
             
                    //sample texture and gammadecode to linear light space
                    Color texColor = getPixel(obj.texture, texX, texY, obj.texWidth, obj.texHeight);
                    albedo = Eigen::Vector3f 
                    (
                        powf(texColor.r / 255.0f, 2.2f),
                        powf(texColor.g / 255.0f, 2.2f),
                        powf(texColor.b / 255.0f, 2.2f)
                    );

                }


                //normal map sampling
                //if object has normal map, replace the interpolated vertex normal
                //with a per pixel normal sampled from the map
                //normal maps store XYZ as RGB (range 0-255), decoded back to -1 to +1
                Eigen::Vector3f shadingNormal = normP; //use vertex normal

                if (obj.hasNormalMap)
                {
                    //convert uv to normal map pixel coordinates (same as texture sampling), also clamped
                    int nmX = (int)(texP.x() * (obj.normalMapWidth - 1));
                    int nmY = (int)((1.0f - texP.y()) * (obj.normalMapHeight - 1));
                    nmX = std::max(0, std::min(nmX, obj.normalMapWidth - 1));
                    nmY = std::max(0, std::min(nmY, obj.normalMapHeight - 1));

                    //sample normal map
                    Color nmSample = getPixel(obj.normalMap, nmX, nmY, obj.normalMapWidth, obj.normalMapHeight);

                    //decode rgb 0-255 back to xyz -1 to +1
                    //to give normal in tangent space 
                    Eigen::Vector3f tangentNormal
                    (
                        (nmSample.r / 255.0f) * 2.0f - 1.0f,
                        (nmSample.g / 255.0f) * 2.0f - 1.0f,
                        (nmSample.b / 255.0f) * 2.0f - 1.0f
                    );

                    //transform from tangent space into world space using the TBN matrix
                    //to align normal map detail with actual geometry
                    shadingNormal = (t.tbn * tangentNormal).normalized();
                }

                //specular map sampling
                //if object has specular map its used to vary shininess per pixel
                //bright areas of the map= shiny/metallic/glossy, dark= matte
                float specularStrength = 1.0f; //objects flat specular exponent

                if (obj.hasSpecularMap)
                {
                    //convert uv to specular map pixel coordinates, clamped to stay in image boundaries
                    int smX = (int)(texP.x() * (obj.specularMapWidth - 1));
                    int smY = (int)((1.0f - texP.y()) * (obj.specularMapHeight - 1));
                    smX = std::max(0, std::min(smX, obj.specularMapWidth - 1));
                    smY = std::max(0, std::min(smY, obj.specularMapHeight - 1));

                    //sample specular map and convert to a 0-1 strength value
                    //using luminance so it works with greyscale and colour spec maps depending on what came with asset
                    Color smSample = getPixel(obj.specularMap, smX, smY, obj.specularMapWidth, obj.specularMapHeight);
                    specularStrength = (0.299f * smSample.r + 0.587f * smSample.g + 0.114f * smSample.b) / 255.0f;
                }

                //emissive map sampling
                //emissive map says how much light pixel emits on its own
                //unlike diffuse/specular which depend on lights in the scene, emissive
                //is added on top of everything else at the end
                Eigen::Vector3f emissiveColor = Eigen::Vector3f::Zero(); //default no emission

                if (obj.hasEmissiveMap)
                {
                    //convert interpolated uv coord to pixel coords in emissive map
                    //Y flipped: uv origin is bottomleft but image origin is topleft
                    int emX = (int)(texP.x() * (obj.emissiveMapWidth - 1));
                    int emY = (int)((1.0f - texP.y()) * (obj.emissiveMapHeight - 1));
                    //clamp to stay within the image bounds
                    emX = std::max(0, std::min(emX, obj.emissiveMapWidth - 1));
                    emY = std::max(0, std::min(emY, obj.emissiveMapHeight - 1));

                    //sample emissive map pixel at this uv location
                    Color emSample = getPixel(obj.emissiveMap, emX, emY, obj.emissiveMapWidth, obj.emissiveMapHeight);

                    //convert sampled rgb from 0-255 int range to 0-1 float range, then gamma decode
                    emissiveColor = Eigen::Vector3f
                    (
                        powf(emSample.r / 255.0f, 2.2f),
                        powf(emSample.g / 255.0f, 2.2f),
                        powf(emSample.b / 255.0f, 2.2f)
                    );

                    //apply strength multiplier
                    //above 1= glow brighter/intense
                    //black pixels in the map stay black as 0
                    emissiveColor *= obj.emissiveStrength;
                }


                //perspective correct clip space depth (used for depth testing)
                //interpolate to find the correct clip space depth (perspective correct)
                float depth =
                    (t.screen[0].z() * (b0 / depth0) +
                        t.screen[1].z() * (b1 / depth1) +
                        t.screen[2].z() * (b2 / depth2)) * depthP;

                //depth test: skip this pixel if something closer has already been drawn
                int depthIdx = x + y * width;
                if (depth > zBuffer[depthIdx]) continue;
                zBuffer[depthIdx] = depth; //update depth buffer with this pixels depth

                //shading: accumulate contributions from all lights
                //work out colour at this position
                Eigen::Vector3f color = Eigen::Vector3f::Zero();

                //view direction: from surface point towards the camera
                Eigen::Vector3f viewDir = (camWorldPos - worldP).normalized();

                //iterate over lights and sum to find colour
                for (auto& light : lights) 
                {

                    //work out the intensity of this light source at the point worldP
                    Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);

                    //only need the following if the light isnt an ambient light
                    if (light->getType() != Light::Type::AMBIENT) 
                    {
                        //direction the light is coming from
                        Eigen::Vector3f incomingLightDir = light->getDirection(worldP);

                        ////choose specular model based on shadingMode
                        //float specularTerm;
                        //if (shadingMode == ShadingMode::PHONG) 
                        //{
                        //    specularTerm = phongSpecularTerm(incomingLightDir, normP, viewDir, obj.specularExponent);
                        //}
                        //else 
                        //{
                        //    specularTerm = blinnPhongSpecularTerm(incomingLightDir, normP, viewDir, obj.specularExponent);
                        //}

                        //using shadingNormal instead of normP so normal mapping affects specular highlights too
                        //specularStrength from the spec map scales the exponent: bright map areas= sharper highlight
                        float mappedExponent = obj.specularExponent * (0.1f + 0.9f * specularStrength);
                        float specularTerm;
                        if (shadingMode == ShadingMode::PHONG) 
                        {
                            specularTerm = phongSpecularTerm(incomingLightDir, shadingNormal, viewDir, mappedExponent);
                        }
                        else 
                        {
                            specularTerm = blinnPhongSpecularTerm(incomingLightDir, shadingNormal, viewDir, mappedExponent);
                        }

                        //specular contribution: specular colour * specular term * light intensity
                        //Eigen::Vector3f specularOut = coeffWiseMultiply(obj.specularColor * specularTerm, lightIntensity);
                        Eigen::Vector3f specularOut = coeffWiseMultiply
                        (
                            Eigen::Vector3f(obj.specularColor * specularTerm),Eigen::Vector3f(lightIntensity)
                        );

                        //diffuse contribution: Lambert's cosine law
                        //dot product of surface normal with incoming light direction (negated to face the surface)
                        //take the dot product of the normal with the light direction
                        //dont want negative light so if dot product less than 0, set it to 0
                        //float dotProd = std::max(normP.dot(-incomingLightDir), 0.0f);
                        //using shadingNormal here too so normal map also affects diffuse shading 
                        float dotProd = std::max(shadingNormal.dot(-incomingLightDir), 0.0f);
                        //multiply the light intensity by the dot product
                        //Eigen::Vector3f diffuseOut = coeffWiseMultiply(lightIntensity * dotProd, albedo);
                        Eigen::Vector3f diffuseOut = coeffWiseMultiply
                        (
                            Eigen::Vector3f(lightIntensity * dotProd),
                            Eigen::Vector3f(albedo)
                        );

                        color += specularOut + diffuseOut;
                    }
                    else 
                    {
                        //ambient: flat colour contribution, no direction needed
                        //light is ambient: multiply light intensity with albedo
                        //color += coeffWiseMultiply(lightIntensity, albedo);
                        color += coeffWiseMultiply
                        (
                            Eigen::Vector3f(lightIntensity),
                            Eigen::Vector3f(albedo)
                        );
                    }
                }

                if (obj.isTransparent)
                {
                    //for transparent objects use texture brightness as the alpha mask
                    //glass tex is white= frosted and black= transparent
                    //luminance of the albedo= opacity value

                    //luminance
                    float luminance = 0.299f * albedo.x() + 0.587f * albedo.y() + 0.114f * albedo.z();
                    float alpha = luminance * obj.transparencyStrength;

                    //if nearly fully transparent (black area of tex) skip pixel entirely 
                    if (alpha < 0.05f) continue;

                    //gamma encode shaded colour 
                    //emissive added on top before blending, same logic as the opaque branch
                    Eigen::Vector3f shadedWithEmissive = color + emissiveColor;

                    Color newC;
                    newC.r = (uint8_t)(std::min(powf(shadedWithEmissive.x(), 1.0f / 2.2f), 1.0f) * 255);
                    newC.g = (uint8_t)(std::min(powf(shadedWithEmissive.y(), 1.0f / 2.2f), 1.0f) * 255);
                    newC.b = (uint8_t)(std::min(powf(shadedWithEmissive.z(), 1.0f / 2.2f), 1.0f) * 255);
                    newC.a = 255;

                    //read the scene behind the glass: what is already in the framebuffer at this pixel
                    Color existing = getPixel(imageBuffer, x, y, width, height);

                    //blend is result= alpha * new + (1 - alpha) * existing
                    //this mixes the frosted glass colour with what was drawn behind it
                    Color blended;
                    blended.r = (uint8_t)(alpha * newC.r + (1.0f - alpha) * existing.r);
                    blended.g = (uint8_t)(alpha * newC.g + (1.0f - alpha) * existing.g);
                    blended.b = (uint8_t)(alpha * newC.b + (1.0f - alpha) * existing.b);
                    blended.a = 255;

                    //depth buffer for transparent pixels is not updated, which means opaque 
                    //objects drawn later still appear in front of the glass (cube, button)
                    setPixel(imageBuffer, x, y, width, height, blended);
                }
                else
                {
                    //emissive added on top of the fully lit/shaded colour
                    //done after all light calculations as emissive is not affected by lights
                    //its just adds straight on top
                    //clamp keeps values in 0-1 range before gamma encoding to prevent
                    //colour channels wrapping/overflowing when emission is strong
                    Eigen::Vector3f finalColor = color + emissiveColor;

                    //gamma encode final colour: convert from linear light space back to
                    //sRGB (raise to power of 1/2.2) and clamp to 0-1 before converting to
                    //8bit int per channel 
                    Color c;
                    c.r = (uint8_t)(std::min(powf(finalColor.x(), 1.0f / 2.2f), 1.0f) * 255);
                    c.g = (uint8_t)(std::min(powf(finalColor.y(), 1.0f / 2.2f), 1.0f) * 255);
                    c.b = (uint8_t)(std::min(powf(finalColor.z(), 1.0f / 2.2f), 1.0f) * 255);
                    c.a = 255;

                    setPixel(imageBuffer, x, y, width, height, c);
        
                }
            }
        }
    }


    //private helper: drawMesh
    //transforms every triangle in a SceneObjects mesh through the
    //rendering pipeline and calls drawTriangle for each one

    void drawMesh(const SceneObject& obj)
    {
        for (int i = 0; i < (int)obj.mesh.vFaces.size(); ++i) 
        {
            //retrieve the three vertices and normals for this face
            Eigen::Vector3f
                v0 = obj.mesh.verts[obj.mesh.vFaces[i][0]],
                v1 = obj.mesh.verts[obj.mesh.vFaces[i][1]],
                v2 = obj.mesh.verts[obj.mesh.vFaces[i][2]];
            Eigen::Vector3f
                n0 = obj.mesh.norms[obj.mesh.nFaces[i][0]],
                n1 = obj.mesh.norms[obj.mesh.nFaces[i][1]],
                n2 = obj.mesh.norms[obj.mesh.nFaces[i][2]];

            Triangle t;

            //world space vertex pos (model transform applied)
            t.verts[0] = (obj.modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
            t.verts[1] = (obj.modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
            t.verts[2] = (obj.modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

            //camera space vertex pos 
            t.cam[0] = (worldToCamera * obj.modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
            t.cam[1] = (worldToCamera * obj.modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
            t.cam[2] = (worldToCamera * obj.modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

            ////clipspace positions (apply projection, then perspective divide)
            //auto toClip = [&](const Eigen::Vector3f& v) {
            //    Eigen::Vector4f c = projection * worldToCamera * obj.modelToWorld * vec3ToVec4(v);
            //    c /= c.w();
            //    return c;
            //};
            //Eigen::Vector4f vClip0 = toClip(v0);
            //Eigen::Vector4f vClip1 = toClip(v1);
            //Eigen::Vector4f vClip2 = toClip(v2);

            ////frustum culling: skip triangles entirely outside the clip box
            //if (outsideClipBox(vClip0) || outsideClipBox(vClip1) || outsideClipBox(vClip2)) continue;


            //work out the clip space coordinates by multiplying and doing perspective divide
            Eigen::Vector4f vClip0 = projection * worldToCamera * obj.modelToWorld * vec3ToVec4(v0);
            vClip0 /= vClip0.w();

            Eigen::Vector4f vClip1 = projection * worldToCamera * obj.modelToWorld * vec3ToVec4(v1);
            vClip1 /= vClip1.w();

            Eigen::Vector4f vClip2 = projection * worldToCamera * obj.modelToWorld * vec3ToVec4(v2);
            vClip2 /= vClip2.w();

            //check if triangle is outside clip space
            if (outsideClipBox(vClip0) || outsideClipBox(vClip1) || outsideClipBox(vClip2)) continue;


            //screen space positions: map clip coords (-1,1) to pixel coordinates
            //work out the screen space coordinates based on the image height and width
            t.screen[0] = Eigen::Vector3f((vClip0.x() + 1.0f) * width / 2, (-vClip0.y() + 1.0f) * height / 2, vClip0.z());
            t.screen[1] = Eigen::Vector3f((vClip1.x() + 1.0f) * width / 2, (-vClip1.y() + 1.0f) * height / 2, vClip1.z());
            t.screen[2] = Eigen::Vector3f((vClip2.x() + 1.0f) * width / 2, (-vClip2.y() + 1.0f) * height / 2, vClip2.z());

            //transform normals using inverse transpose of model matrixs 3x3 block
            //correctly handles non uniform scaling so normals stay perpendicular to surfaces
            Eigen::Matrix3f normalMatrix = obj.modelToWorld.block<3, 3>(0, 0).inverse().transpose();
            t.norms[0] = (normalMatrix * n0).normalized();
            t.norms[1] = (normalMatrix * n1).normalized();
            t.norms[2] = (normalMatrix * n2).normalized();

            //texture coordinates for this face
            t.texs[0] = obj.mesh.texs[obj.mesh.tFaces[i][0]];
            t.texs[1] = obj.mesh.texs[obj.mesh.tFaces[i][1]];
            t.texs[2] = obj.mesh.texs[obj.mesh.tFaces[i][2]];

            //TBN for normal mapping
            //normal map stores normals in tangent space 
            //need to build TBN matrix to rotate normals into world space

            //world space edge vectors of triangle
            Eigen::Vector3f edge1 = t.verts[1] - t.verts[0];
            Eigen::Vector3f edge2 = t.verts[2] - t.verts[0];

            //uv deltas of edges
            Eigen::Vector2f dUV1 = t.texs[1] - t.texs[0];
            Eigen::Vector2f dUV2 = t.texs[2] - t.texs[0];

            //using uv/edge relationship to solve tangent and bitangent
            float uvDet = dUV1.x() * dUV2.y() - dUV2.x() * dUV1.y();
            Eigen::Vector3f tangent = Eigen::Vector3f::Zero();
            Eigen::Vector3f bitangent = Eigen::Vector3f::Zero();

            if (fabsf(uvDet) > 1e-6f)
            {
                float invDet = 1.0f / uvDet;
                tangent = (dUV2.y() * edge1 - dUV1.y() * edge2) * invDet;
                bitangent = (dUV1.x() * edge2 - dUV2.x() * edge1) * invDet;
            }

            //transform tangent and bitangent into world space using normal matrix
            tangent = (normalMatrix * tangent).normalized();
            bitangent = (normalMatrix * bitangent).normalized();

            //store the perface TBN 
            //used in drawTriangle to rotate sampled normals into world space
            t.tbn = Eigen::Matrix3f();
            t.tbn.col(0) = tangent;
            t.tbn.col(1) = bitangent;
            t.tbn.col(2) = t.norms[0]; //vertex 0 normal as face normal basis


            //rasterise this triangle
            drawTriangle(t, obj);
        }
    }
};