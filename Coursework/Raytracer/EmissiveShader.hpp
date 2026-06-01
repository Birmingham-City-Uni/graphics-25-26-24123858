#pragma once
#include "Shader.hpp"
#include "HitInfo.hpp"
#include "Scene.hpp"
#include <lodepng.h>
#include <Eigen/Dense>
#include <vector>
#include <string>
#include <algorithm>

///wraps base shader and adds an emissive tex on top
///black pix in map= no glow, bright pix= glow
class EmissiveShader : public Shader
{
public:
    EmissiveShader(const Shader* baseShader,
        const std::vector<uint8_t>* emissiveTex,
        unsigned int emissiveW, unsigned int emissiveH,
        float emissiveStrength = 1.f)
        : base_(baseShader), emissiveTex_(emissiveTex),
        emissiveW_(emissiveW), emissiveH_(emissiveH),
        emissiveStrength_(emissiveStrength)
    {
    }

    Eigen::Vector3f getColor(
        const HitInfo& hitInfo,
        const Renderable* scene,
        const std::vector<std::unique_ptr<Light>>& lights,
        const Eigen::Vector3f& ambientLight,
        int depth, int maxDepth) const override
    {
        //get base shaded colour
        //calc normal shaded colour first using underlying material shader
        Eigen::Vector3f color = base_->getColor(hitInfo, scene, lights, ambientLight, depth, maxDepth);

        //safety check to prevent texture access issues if no emissive map was loaded. some models dont have emis tex(
        if (emissiveTex_ == nullptr)
        {
            return color;
        }

        if (emissiveTex_->empty())
        {
            return color;
        }

        //sample emissive map using UV coordinates from the hit (ray object intersect)
        Eigen::Vector2f uv = hitInfo.texCoords;

        //wrap uv coords into 0-1 range
        //supports tiled textures and avoids invalid lookups when uv values are beyond the texture bounds
        float u = uv.x() - floorf(uv.x());
        float v = uv.y() - floorf(uv.y());
        int ex = (int)(u * (emissiveW_ - 1));
        int ey = (int)((1.f - v) * (emissiveH_ - 1));
        //int ex = (int)(uv.x() * (emissiveW_ - 1));
        //int ey = (int)((1.f - uv.y()) * (emissiveH_ - 1));
        //clamp tex coords
        ex = std::max(0, std::min(ex, (int)emissiveW_ - 1));
        ey = std::max(0, std::min(ey, (int)emissiveH_ - 1));

        int idx = (ex + ey * emissiveW_) * 4;
        //convert emissive tex values from srgb into linear space
        //to combine correctly with the lighting calcs
        Eigen::Vector3f emissive(
            powf((*emissiveTex_)[idx + 0] / 255.f, 2.2f),
            powf((*emissiveTex_)[idx + 1] / 255.f, 2.2f),
            powf((*emissiveTex_)[idx + 2] / 255.f, 2.2f)
        );
 
        //emissive added directly to final colour
        //emissive materials generate their own light and independent of other lightin in scene
        return color + emissive * emissiveStrength_;
    }

private:
    const Shader* base_;
    const std::vector<uint8_t>* emissiveTex_;
    unsigned int emissiveW_, emissiveH_;
    float emissiveStrength_;
};