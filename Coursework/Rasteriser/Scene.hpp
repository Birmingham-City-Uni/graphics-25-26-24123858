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

// SCENE OBJECT CLASS
// This class uses C++ encapsulation to bundle together everything
// that describes a single renderable object in the scene:
//   - its geometry (Mesh)
//   - its texture (loaded PNG)
//   - its position/orientation/scale in the world (modelToWorld matrix)
//   - its material properties (specular colour and exponent)
//
// Before this class existed, all of these were separate loose variables
// in main() and had to be passed individually to drawMesh(). By grouping
// them here we make the code cleaner, easier to read, and reusable.

class SceneObject {
public:
    // The geometry of this object (vertices, normals, UVs, faces)
    Mesh mesh;

    // The texture image data and its dimensions
    std::vector<uint8_t> texture;
    int texWidth = 0;
    int texHeight = 0;

    // The model-to-world transform matrix.
    // This controls where the object is placed, rotated, and scaled in the scene.
    Eigen::Matrix4f modelToWorld;

    // Material: specular highlight colour (white = neutral)
    Eigen::Vector3f specularColor;

    // Material: how shiny the object is (higher = tighter highlight)
    float specularExponent;

    //if useFlatColor is true flatColor is used as the albedo instead of the texture
    bool useModelColor = false;
    Eigen::Vector3f modelColor = Eigen::Vector3f::Zero();

    // Constructor: takes everything needed to define a scene object.
    // Using an initialiser list sets member variables directly,
    // which is slightly more efficient than assigning inside the body.
    SceneObject(
        const Mesh& mesh,
        const std::vector<uint8_t>& texture, int texWidth, int texHeight,
        const Eigen::Matrix4f& modelToWorld,
        const Eigen::Vector3f& specularColor = Eigen::Vector3f::Ones(),
        float specularExponent = 50.f)
        : mesh(mesh),
        texture(texture), texWidth(texWidth), texHeight(texHeight),
        modelToWorld(modelToWorld),
        specularColor(specularColor),
        specularExponent(specularExponent)
    {
    }


    //constructor for flat model colours— no texture file 
    SceneObject(
        const Mesh& mesh,
        const Eigen::Vector3f& modelColor,
        const Eigen::Matrix4f& modelToWorld,
        const Eigen::Vector3f& specularColor = Eigen::Vector3f::Ones(),
        float specularExponent = 50.f)
        : mesh(mesh),
        texture(), texWidth(0), texHeight(0),
        modelToWorld(modelToWorld),
        specularColor(specularColor),
        specularExponent(specularExponent),
        useModelColor(true),
        modelColor(modelColor)
    {
    }

    //static method
    static SceneObject loadFromFileColour(
        const std::string& meshPath,
        const Eigen::Vector3f& modelColor,
        const Eigen::Matrix4f& modelToWorld,
        const Eigen::Vector3f& specularColor = Eigen::Vector3f::Ones(),
        float specularExponent = 50.f)
    {
        Mesh mesh = loadMeshFile(meshPath);
        return SceneObject(mesh, modelColor, modelToWorld, specularColor, specularExponent);
    }


    // Static factory method: loads a mesh and texture from file and constructs a SceneObject.
    // A "factory" method is a common C++ pattern where a static function creates and returns
    // an instance of the class, keeping construction logic self-contained.
    // This means in main() we only need one line to create a fully set-up object.
    static SceneObject loadFromFile(
        const std::string& meshPath,
        const std::string& texturePath,
        const Eigen::Matrix4f& modelToWorld,
        const Eigen::Vector3f& specularColor = Eigen::Vector3f::Ones(),
        float specularExponent = 50.f)
    {
        // Load the mesh geometry from the .obj file
        Mesh mesh = loadMeshFile(meshPath);

        // Load the texture image using lodepng
        std::vector<uint8_t> texture;
        unsigned int texWidth, texHeight;
        unsigned int error = lodepng::decode(texture, texWidth, texHeight, texturePath);
        if (error) {
            throw std::runtime_error("Error loading texture: " + texturePath);
        }

        // Construct and return the SceneObject with all loaded data
        return SceneObject(mesh, texture, (int)texWidth, (int)texHeight,
            modelToWorld, specularColor, specularExponent);
    }
};


// SHADING MODE ENUM
// Moved here from Rasteriser.cpp so that Scene and SceneObject
// can share this type. An enum is a clean way to represent a
// fixed set of named options without using magic numbers or strings.

enum ShadingMode {
    PHONG,
    BLINN_PHONG
};



// TRIANGLE STRUCT

// Holds the per-vertex data for a single triangle as it moves
// through the rendering pipeline (world space, camera space,
// screen space, normals, UVs).
// Kept here so Scene can use it internally.
struct Triangle {
    std::array<Eigen::Vector3f, 3> screen; //coordinates of the triangle in screen space
    std::array<Eigen::Vector3f, 3> verts; //vertices of the triangle in world space
    std::array<Eigen::Vector3f, 3> cam; //vertices of the triangle in camera space
    std::array<Eigen::Vector3f, 3> norms; //normals of the triangle corners in world space
    std::array<Eigen::Vector2f, 3> texs; //texture coordinates of the triangle corners
};


// SCENE CLASS

// The Scene class is responsible for managing the entire renderable
// scene: the list of objects, the list of lights, the camera, and
// the image/depth buffers. It exposes a clean render() method so
// that main() doesn't need to know the details of rasterisation.
//
// This is an example of the OOP principle of "encapsulation":
// the rendering pipeline details are hidden inside the class,
// and the outside world only needs to call render().

class Scene {
public:
    // Image dimensions
    int width, height;

    // The pixel colour buffer (RGBA, 8 bits per channel)
    std::vector<uint8_t> imageBuffer;

    // The depth buffer (one float per pixel, initialised to 1.0)
    std::vector<float> zBuffer;

    // The camera's position in world space (used for specular calculation)
    Eigen::Vector3f camWorldPos;

    // World-to-camera and camera-to-clip (projection) matrices
    Eigen::Matrix4f worldToCamera;
    Eigen::Matrix4f projection;

    // Which specular model to use (Phong or Blinn-Phong)
    ShadingMode shadingMode;

    // All the objects in the scene.
    // Using a vector of SceneObject means we can add/remove objects easily.
    std::vector<SceneObject> objects;

    // All the lights in the scene.
    // unique_ptr is used because Light is an abstract base class —
    // we store polymorphic pointers so we can hold any type of light
    // (Ambient, Point, Directional, Spot) in the same list.
    std::vector<std::unique_ptr<Light>> lights;

    // Constructor: sets up the image and depth buffers and stores camera/projection info
    Scene(int width, int height,
        const Eigen::Matrix4f& worldToCamera,
        const Eigen::Matrix4f& projection,
        const Eigen::Vector3f& camWorldPos,
        ShadingMode shadingMode = ShadingMode::BLINN_PHONG)
        : width(width), height(height),
        worldToCamera(worldToCamera),
        projection(projection),
        camWorldPos(camWorldPos),
        shadingMode(shadingMode)
    {
        // Allocate and clear the image buffer (black, fully opaque)
        imageBuffer.resize(width * height * 4);
        Color black{ 0, 0, 0, 255 };
        for (int r = 0; r < height; ++r)
            for (int c = 0; c < width; ++c)
                setPixel(imageBuffer, c, r, width, height, black);

        // Allocate and clear the depth buffer (1.0 = maximum depth)
        zBuffer.assign(width * height, 1.0f);
    }

    // Add a scene object to the scene
    void addObject(const SceneObject& obj) {
        objects.push_back(obj);
    }

    // Add a light to the scene.
    // We use a unique_ptr here for polymorphism — the caller passes ownership
    // of the Light object to the Scene.
    void addLight(std::unique_ptr<Light> light) {
        lights.push_back(std::move(light));
    }

    // Render the entire scene into the image buffer.
    // This loops over every object and draws it using the shared lights and camera.
    void render() {
        for (const SceneObject& obj : objects) {
            drawMesh(obj);
        }
    }

    // Save the rendered image to a PNG file
    void saveImage(const std::string& filename) {
        int errorCode = lodepng::encode(filename, imageBuffer, width, height);
        if (errorCode) {
            throw std::runtime_error("lodepng error: " + std::string(lodepng_error_text(errorCode)));
        }
    }

private:

    // PRIVATE HELPER: findScreenBoundingBox

    // Computes the axis-aligned bounding box of a triangle in screen
    // space, clamped to the image boundaries. Used to limit the
    // pixel loop to only the relevant region.

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


    // PRIVATE HELPER: drawTriangle

    // Rasterises a single triangle into the image buffer.
    // Now a private method of Scene rather than a free function —
    // it accesses the shared zBuffer, imageBuffer, lights, etc.
    // directly through the class, rather than needing them all
    // passed as parameters.

    void drawTriangle(const Triangle& t, const SceneObject& obj)
    {
        int minX, minY, maxX, maxY;
        findScreenBoundingBox(t, minX, minY, maxX, maxY);

        // Compute the signed area of the triangle in screen space.
        // If negative, the triangle is back-facing and we skip it (back-face culling).
        Eigen::Vector2f edge1 = v2(t.screen[2] - t.screen[0]);
        Eigen::Vector2f edge2 = v2(t.screen[1] - t.screen[0]);
        float triangleArea = 0.5f * vec2Cross(edge2, edge1);
        if (triangleArea < 0) return;

        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                Eigen::Vector2f p(x, y);

                // Compute the area of the three sub-triangles formed with point P.
                // These give us the barycentric coordinates of P within the triangle.
                //find subtriangle areas
                float a0 = 0.5f * fabsf(vec2Cross(v2(t.screen[1]) - v2(t.screen[2]), p - v2(t.screen[2])));
                float a1 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[2]), p - v2(t.screen[2])));
                float a2 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[1]), p - v2(t.screen[1])));

                //find barycentrics
                float b0 = a0 / triangleArea;
                float b1 = a1 / triangleArea;
                float b2 = a2 / triangleArea;

                // If the sum of barycentrics exceeds 1 (with small tolerance), P is outside the triangle
                //if outside triangle exit early
                if (b0 + b1 + b2 > 1.0001f) continue;

                // Camera-space depths at each vertex (positive, since camera looks down +Z)
                //get the depths from the camera space position of the 3 corners
                //float depth0 = 0.f, depth1 = 0.f, depth2 = 0.f;
                float depth0 = -t.cam[0].z();
                float depth1 = -t.cam[1].z();
                float depth2 = -t.cam[2].z();

                // Perspective-correct depth at P
                //work out the depth at the point P
                float depthP = 1.0f / (b0 / depth0 + b1 / depth1 + b2 / depth2);

                // Perspective-correct world-space position at P
                //interpolate to find the world-space position of this pixel (perspective-correct)
                Eigen::Vector3f worldP =
                    (t.verts[0] * (b0 / depth0) +
                        t.verts[1] * (b1 / depth1) +
                        t.verts[2] * (b2 / depth2)) * depthP;

                // Perspective-correct interpolated normal at P (then re-normalised)
                // Interpolate to find the normal of this pixel (perspective-correct)
                Eigen::Vector3f normP =
                    (t.norms[0] * (b0 / depth0) +
                        t.norms[1] * (b1 / depth1) +
                        t.norms[2] * (b2 / depth2)).normalized();

                // Perspective-correct UV coordinates at P
                //perspective-correct uv interpolation
                Eigen::Vector2f texP =
                    (t.texs[0] * (b0 / depth0) +
                        t.texs[1] * (b1 / depth1) +
                        t.texs[2] * (b2 / depth2)) * depthP;

                // Convert UV to integer texture coordinates
                //convert to texture space
                int texX = (int)(texP.x() * (obj.texWidth - 1));
                int texY = (int)((1.0f - texP.y()) * (obj.texHeight - 1)); // flip Y (UV origin is bottom-left)

                // Clamp to texture dimensions
                texX = std::max(0, std::min(texX, obj.texWidth - 1));
                texY = std::max(0, std::min(texY, obj.texHeight - 1));

                // Sample texture colour at this pixel
                Color texColor = getPixel(obj.texture, texX, texY, obj.texWidth, obj.texHeight);

                // Gamma-decode the texture colour to bring it into linear light space
                // (raise to power 2.2 converts from sRGB to linear)
                //convert to linear space (gamma correct)
               /* Eigen::Vector3f albedo(
                    powf(texColor.r / 255.0f, 2.2f),
                    powf(texColor.g / 255.0f, 2.2f),
                    powf(texColor.b / 255.0f, 2.2f)
                );*/

                Eigen::Vector3f albedo;
                if (obj.useModelColor)
                {
                    //use the flat colour directly (already in linear space)
                    albedo = obj.modelColor;
                }
                else
                {
                    //sample texture and gammadecode to linear light space
                    Color texColor = getPixel(obj.texture, texX, texY, obj.texWidth, obj.texHeight);
                    albedo = Eigen::Vector3f
                    (
                        powf(texColor.r / 255.0f, 2.2f),
                        powf(texColor.g / 255.0f, 2.2f),
                        powf(texColor.b / 255.0f, 2.2f)
                    );
                }



                // Perspective-correct clip-space depth (used for depth testing)
                // Interpolate to find the correct clip-space depth (perspective-correct)
                float depth =
                    (t.screen[0].z() * (b0 / depth0) +
                        t.screen[1].z() * (b1 / depth1) +
                        t.screen[2].z() * (b2 / depth2)) * depthP;

                // Depth test: skip this pixel if something closer has already been drawn
                int depthIdx = x + y * width;
                if (depth > zBuffer[depthIdx]) continue;
                zBuffer[depthIdx] = depth; // update depth buffer with this pixel's depth

                // Shading: accumulate contributions from all lights
                //work out colour at this position
                Eigen::Vector3f color = Eigen::Vector3f::Zero();

                // View direction: from surface point towards the camera
                Eigen::Vector3f viewDir = (camWorldPos - worldP).normalized();

                //iterate over lights and sum to find colour
                for (auto& light : lights) {

                    //work out the intensity of this light source at the point worldP
                    Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);

                    //only need the following if the light isn't an ambient light
                    if (light->getType() != Light::Type::AMBIENT) {
                        // Direction the light is coming from
                        Eigen::Vector3f incomingLightDir = light->getDirection(worldP);

                        // Choose specular model based on shadingMode
                        float specularTerm;
                        if (shadingMode == ShadingMode::PHONG) {
                            specularTerm = phongSpecularTerm(incomingLightDir, normP, viewDir, obj.specularExponent);
                        }
                        else {
                            specularTerm = blinnPhongSpecularTerm(incomingLightDir, normP, viewDir, obj.specularExponent);
                        }

                        // Specular contribution: specular colour * specular term * light intensity
                        //Eigen::Vector3f specularOut = coeffWiseMultiply(obj.specularColor * specularTerm, lightIntensity);
                        Eigen::Vector3f specularOut = coeffWiseMultiply(
                            Eigen::Vector3f(obj.specularColor * specularTerm),
                            Eigen::Vector3f(lightIntensity)
                        );

                        // Diffuse contribution: Lambert's cosine law
                        // dot product of surface normal with incoming light direction (negated to face the surface)
                        //take the dot product of the normal with the light direction
                        //don't want negative light so if dot product less than 0, set it to 0
                        float dotProd = std::max(normP.dot(-incomingLightDir), 0.0f);
                        //multiply the light intensity by the dot product
                        //Eigen::Vector3f diffuseOut = coeffWiseMultiply(lightIntensity * dotProd, albedo);
                        Eigen::Vector3f diffuseOut = coeffWiseMultiply(
                            Eigen::Vector3f(lightIntensity * dotProd),
                            Eigen::Vector3f(albedo)
                        );

                        color += specularOut + diffuseOut;
                    }
                    else {
                        // Ambient: flat colour contribution, no direction needed
                        //light is ambient- multiply light intensity with albedo
                        //color += coeffWiseMultiply(lightIntensity, albedo);
                        color += coeffWiseMultiply(
                            Eigen::Vector3f(lightIntensity),
                            Eigen::Vector3f(albedo)
                        );
                    }
                }

                // Gamma-encode the output colour (raise to 1/2.2, i.e. sRGB output)
                // and clamp to [0,1] before converting to 8-bit
                //gamma-correcting colours so the texture doesn't appear overly bright
                Color c;
                c.r = (uint8_t)(std::min(powf(color.x(), 1.f / 2.2f), 1.0f) * 255);
                c.g = (uint8_t)(std::min(powf(color.y(), 1.f / 2.2f), 1.0f) * 255);
                c.b = (uint8_t)(std::min(powf(color.z(), 1.f / 2.2f), 1.0f) * 255);
                c.a = 255;

                setPixel(imageBuffer, x, y, width, height, c);
            }
        }
    }


    // PRIVATE HELPER: drawMesh

    // Transforms every triangle in a SceneObject's mesh through the
    // rendering pipeline (model -> world -> camera -> clip -> screen)
    // and calls drawTriangle for each one.

    void drawMesh(const SceneObject& obj)
    {
        for (int i = 0; i < (int)obj.mesh.vFaces.size(); ++i) {
            // Retrieve the three vertices and normals for this face
            Eigen::Vector3f
                v0 = obj.mesh.verts[obj.mesh.vFaces[i][0]],
                v1 = obj.mesh.verts[obj.mesh.vFaces[i][1]],
                v2 = obj.mesh.verts[obj.mesh.vFaces[i][2]];
            Eigen::Vector3f
                n0 = obj.mesh.norms[obj.mesh.nFaces[i][0]],
                n1 = obj.mesh.norms[obj.mesh.nFaces[i][1]],
                n2 = obj.mesh.norms[obj.mesh.nFaces[i][2]];

            Triangle t;

            // World-space vertex positions (model transform applied)
            t.verts[0] = (obj.modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
            t.verts[1] = (obj.modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
            t.verts[2] = (obj.modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

            // Camera-space vertex positions (world -> camera transform applied)
            t.cam[0] = (worldToCamera * obj.modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
            t.cam[1] = (worldToCamera * obj.modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
            t.cam[2] = (worldToCamera * obj.modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

            //// Clip-space positions (apply projection, then perspective divide)
            //auto toClip = [&](const Eigen::Vector3f& v) {
            //    Eigen::Vector4f c = projection * worldToCamera * obj.modelToWorld * vec3ToVec4(v);
            //    c /= c.w();
            //    return c;
            //};
            //Eigen::Vector4f vClip0 = toClip(v0);
            //Eigen::Vector4f vClip1 = toClip(v1);
            //Eigen::Vector4f vClip2 = toClip(v2);

            //// Frustum culling: skip triangles entirely outside the clip box
            //if (outsideClipBox(vClip0) || outsideClipBox(vClip1) || outsideClipBox(vClip2)) continue;


            // Work out the clip space coordinates by multiplying by projection matrix
            /// and performing the perspective divide
            //work out the clip space coordinates, by multiplying by worldToClip and doing the 
        //perspective divide
            Eigen::Vector4f vClip0 = projection * worldToCamera * obj.modelToWorld * vec3ToVec4(v0);
            vClip0 /= vClip0.w();

            Eigen::Vector4f vClip1 = projection * worldToCamera * obj.modelToWorld * vec3ToVec4(v1);
            vClip1 /= vClip1.w();

            Eigen::Vector4f vClip2 = projection * worldToCamera * obj.modelToWorld * vec3ToVec4(v2);
            vClip2 /= vClip2.w();

            // Check if triangle is outside clip space
            //work out the clip space coordinates, by multiplying by worldToClip and doing the 
        //perspective divide
            if (outsideClipBox(vClip0) || outsideClipBox(vClip1) || outsideClipBox(vClip2)) continue;



            // Screen-space positions: map clip coords [-1,1] to pixel coordinates
            //work out the screen space coordinates based on the image height and width
            t.screen[0] = Eigen::Vector3f((vClip0.x() + 1.0f) * width / 2, (-vClip0.y() + 1.0f) * height / 2, vClip0.z());
            t.screen[1] = Eigen::Vector3f((vClip1.x() + 1.0f) * width / 2, (-vClip1.y() + 1.0f) * height / 2, vClip1.z());
            t.screen[2] = Eigen::Vector3f((vClip2.x() + 1.0f) * width / 2, (-vClip2.y() + 1.0f) * height / 2, vClip2.z());

            // Transform normals using the inverse-transpose of the model matrix's 3x3 block.
            // This correctly handles non-uniform scaling so normals remain perpendicular to surfaces.
           //transform normals (using the inverse transpose of the upper 3x3 block)
            Eigen::Matrix3f normalMatrix = obj.modelToWorld.block<3, 3>(0, 0).inverse().transpose();
            t.norms[0] = (normalMatrix * n0).normalized();
            t.norms[1] = (normalMatrix * n1).normalized();
            t.norms[2] = (normalMatrix * n2).normalized();

            // Texture coordinates for this face
            t.texs[0] = obj.mesh.texs[obj.mesh.tFaces[i][0]];
            t.texs[1] = obj.mesh.texs[obj.mesh.tFaces[i][1]];
            t.texs[2] = obj.mesh.texs[obj.mesh.tFaces[i][2]];

            // Rasterise this triangle
            drawTriangle(t, obj);
        }
    }
};