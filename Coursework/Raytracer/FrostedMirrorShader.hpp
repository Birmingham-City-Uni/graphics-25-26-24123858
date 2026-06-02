#pragma once
#include "Shader.hpp"
#include "HitInfo.hpp"
#include "Scene.hpp"
#include "GeomUtil.hpp"
#include <Eigen/Dense>
#include <vector>
#include <algorithm>


//frosted mirror shader
//samples mask texture: white areas= frosted (diffuse lambertian), black= mirror reflections
//values blend between the two, same as glass alpha mask used in rasteriser (but applied to reflectivity here)
class FrostedMirrorShader : public Shader
{
public:
    FrostedMirrorShader(const std::vector<uint8_t>* maskTex, unsigned int maskW, unsigned int maskH, 
        const Eigen::Vector3f& frostColor = Eigen::Vector3f(0.9f, 0.9f, 0.95f))
        : maskTex_(maskTex), maskW_(maskW), maskH_(maskH), frostColor_(frostColor)
    {
    }

    Eigen::Vector3f getColor
    (
        const HitInfo& hitInfo,
        const Renderable* scene,
        const std::vector<std::unique_ptr<Light>>& lights,
        const Eigen::Vector3f& ambientLight,
        int currBounceCount,
        const int maxBounces
    ) 
        const override

    {
        Eigen::Vector2f uv = hitInfo.texCoords;
        //interpolated uv coords from triangle hit point
        //used to sample mask tex and determine which parts are frosted/mirror
        //sample mask tex to get frosted/mirror blend
        //convert uv coords (0-1 range) into texture pixel coords
        //values clamped: prevent sampling outside texture bounds
        int mx = std::max(0, std::min((int)(uv.x() * maskW_), (int)maskW_ - 1));

        int my = std::max(0, std::min((int)((1.f - uv.y()) * maskH_), (int)maskH_ - 1));

        //calc rgba pixel index in texture buffer.
        int mi = (mx + my * maskW_) * 4; //(R,G,B,A)
        //convert rgb mask value into a luminance value
        float frosted = (0.299f * (*maskTex_)[mi + 0] + 0.587f * (*maskTex_)[mi + 1] + 0.114f * 
            (*maskTex_)[mi + 2]) / 255.f;

        //mirror 
        Eigen::Vector3f mirrorColor(0.f, 0.f, 0.f);
        //reflection rays generated recursively
        //max bounce count prevents infinite reflection loops
        if (currBounceCount < maxBounces && frosted < 0.99f)
        {
            Ray reflectionRay;
            //mirror reflection direction uses incident ray direction and surface normal
            reflectionRay.direction = reflect(hitInfo.inDirection, hitInfo.normal);

            //offset reflection ray origin along normal to prevent shadow acne
            //caused by floating point precision errors
            reflectionRay.origin = hitInfo.location + 1e-4f * hitInfo.normal;

            HitInfo reflectionHit;

            if (scene->intersect (reflectionRay, 1e-6f, 1e4f, reflectionHit, VISIBLE_BITMASK))

            {
                mirrorColor = reflectionHit.shader->getColor (reflectionHit, scene, lights, 
                    ambientLight, currBounceCount + 1, maxBounces);
            }
        }

        //frosted (diffuse) 
        //lambertian diffuse lighting used to simulate light scattering (frosted glass)
        Eigen::Vector3f frostResult = ambientLight.cwiseProduct(frostColor_);
        Eigen::Vector3f N = hitInfo.normal.normalized();
        for (auto& light : lights) 
        {
            Eigen::Vector3f L = light->getVecToLight(hitInfo.location).normalized();
            Eigen::Vector3f lint = light->getIntensity(hitInfo.location);
            //Lambert cosine law: surfaces facing the light receive more illumination than
            //angles surfaces
            float NdotL = std::max(N.dot(L), 0.f);
            frostResult += lint * NdotL * 0.5f;
        }

        frostResult = frostResult.cwiseProduct(frostColor_);

        
        //frost shading based on the mask texture brightness
        //black pix= mirror
        //white pix= frosted glass.
        return (1.f - frosted) * mirrorColor + frosted * frostResult;
    }

private:
    const std::vector<uint8_t>* maskTex_;
    unsigned int maskW_, maskH_;
    Eigen::Vector3f frostColor_;
};