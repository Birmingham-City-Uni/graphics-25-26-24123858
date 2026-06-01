//normal maps, textured phong and specular maps

#pragma once
#include "Shader.hpp"
#include "HitInfo.hpp"
#include "Scene.hpp"
#include "HitInfo.hpp"
#include "Light.hpp"
#include "GeomUtil.hpp"
#include <Eigen/Dense>
#include <vector>
#include <algorithm>
#include <cmath>

//function used when sampling textures, prevents uv coords from producing texture 
//indices outside bounds
template<typename T>
T clampValue(T value, T low, T high)
{
    return std::max(low, std::min(value, high));
}

//textured phong shader combines texture mapping, BlinnPhong specular highlights,
//normal mapping and specular mapping
class TexturedPhongShader : public Shader
{
public:
    //albedo texture/base colour texture
    //stores surface colour of material and is sampled
    //using interpolated uv coords from hit point
    const std::vector<uint8_t>* albedoTex_;
    unsigned int albedoW_, albedoH_;

    //normal map 
    //stores perpixel surface directions encoded as rgb values
    //simulats detail (geometric) without increasing mesh complexity
    const std::vector<uint8_t>* normalTex_ = nullptr;
    unsigned int normalW_ = 0, normalH_ = 0;

    //specular map 
    //controls how different areas of the tex contribute to specular reflections (shininess of model)
    const std::vector<uint8_t>* specularTex_ = nullptr;
    unsigned int specularW_ = 0, specularH_ = 0;

    //controls the size of specular highlights
    //larger= sharper highlights
    float specularExponent_;
    Eigen::Vector3f specularColor_;

    //creates textured material using BlinnPhong lighting
    TexturedPhongShader
    (
        const std::vector<uint8_t>* albedo, unsigned int aw, unsigned int ah,
        float specularExponent = 60.f,
        Eigen::Vector3f specularColor = Eigen::Vector3f::Ones()
    )
        : albedoTex_(albedo), albedoW_(aw), albedoH_(ah),
        specularExponent_(specularExponent), specularColor_(specularColor)
    {
    }

    //attach normal map to the obj
    void setNormalMap(const std::vector<uint8_t>* tex, unsigned int w, unsigned int h)
    {
        normalTex_ = tex; normalW_ = w; normalH_ = h;
    }

    void setSpecularMap(const std::vector<uint8_t>* tex, unsigned int w, unsigned int h)
    {
        specularTex_ = tex; specularW_ = w; specularH_ = h;
    }

    virtual Eigen::Vector3f getColor
    (
        const HitInfo& hitInfo,
        const Renderable* scene,
        const std::vector<std::unique_ptr<Light>>& lights,
        const Eigen::Vector3f& ambientLight,
        int currBounceCount, const int maxBounces
    ) 
        const override
    {
        //interpolated tex coords at ray intersection point
        Eigen::Vector2f uv = hitInfo.texCoords;

        //sample albedo tex using uv coords
        //uvs are converted into pixel coordinates 
        //clamped to prevent invalid texture access issues
        int ax = clampValue((int)(uv.x() * albedoW_), 0, (int)albedoW_ - 1);
        int ay = clampValue((int)((1.f - uv.y()) * albedoH_), 0, (int)albedoH_ - 1);
        int ai = (ax + ay * albedoW_) * 4;

        //convert tex colours from srgb to linear space
        Eigen::Vector3f albedo
        (
            powf((*albedoTex_)[ai + 0] / 255.f, 2.2f),
            powf((*albedoTex_)[ai + 1] / 255.f, 2.2f),
            powf((*albedoTex_)[ai + 2] / 255.f, 2.2f)
        );

        //use normal map if present, else vertex normal from hit 
        Eigen::Vector3f N = hitInfo.normal.normalized();
        if (normalTex_) 
        {
            int nx = clampValue((int)(uv.x() * normalW_), 0, (int)normalW_ - 1);
            int ny = clampValue((int)((1.f - uv.y()) * normalH_), 0, (int)normalH_ - 1);
            int ni = (nx + ny * normalW_) * 4;
            // Normal maps store vectors in tangent space
            //rgb values in range (0,255]) are remapped into vector range (-1,1)
            Eigen::Vector3f tangentNormal
            (
                (*normalTex_)[ni + 0] / 255.f * 2.f - 1.f,
                (*normalTex_)[ni + 1] / 255.f * 2.f - 1.f,
                (*normalTex_)[ni + 2] / 255.f * 2.f - 1.f
            );

            //transform from tangent space to world space using tbn from HitInfo
            if (hitInfo.tangent.norm() > 0.01f) 
            {
                Eigen::Vector3f T = hitInfo.tangent.normalized();
                Eigen::Vector3f B = hitInfo.bitangent.normalized();
                Eigen::Matrix3f TBN;
                TBN.col(0) = T;
                TBN.col(1) = B;
                TBN.col(2) = N;
                N = (TBN * tangentNormal).normalized();
            }
        }

        //spec strength from spec map
        //brighter texels= stronger spec reflections
        float specStrength = 1.f;
        if (specularTex_) 
        {
            int sx = clampValue((int)(uv.x() * specularW_), 0, (int)specularW_ - 1);
            int sy = clampValue((int)((1.f - uv.y()) * specularH_), 0, (int)specularH_ - 1);
            int si = (sx + sy * specularW_) * 4;
            //luminance of spec map controls shininess 
            //convert the rgb texel into a luminance value, then used as reflection strength multiplier
            specStrength = (0.299f * (*specularTex_)[si + 0]
                + 0.587f * (*specularTex_)[si + 1]
                + 0.114f * (*specularTex_)[si + 2]) / 255.f;
        }

        Eigen::Vector3f V = -hitInfo.inDirection.normalized(); //direction from the surface toward the camera
        Eigen::Vector3f color = coefftWiseMul(ambientLight, albedo);

        for (auto& light : lights) 
        {
            Eigen::Vector3f L = light->getVecToLight(hitInfo.location).normalized();
            Eigen::Vector3f lint = light->getIntensity(hitInfo.location);

            //lamberts cosine law
            //surfaces facing the light receive more illumination
            float NdotL = std::max(N.dot(L), 0.f);
            Eigen::Vector3f diffuse = coefftWiseMul(lint * NdotL, albedo);

            //BlinnPhong specular: uses the half vector between view and light directions
            //to approx glossy reflections 
            Eigen::Vector3f H = L + V;

            if (H.squaredNorm() > 1e-8f)
                H.normalize();
            else
                H = N;

            float mappedExp = specularExponent_ * (0.1f + 0.9f * specStrength);
            float specTerm = powf(std::max(N.dot(H), 0.f), mappedExp);
            Eigen::Vector3f spec = coefftWiseMul(lint, specularColor_ * specTerm);

            color += diffuse + spec;
        }

        return color;
    }
};