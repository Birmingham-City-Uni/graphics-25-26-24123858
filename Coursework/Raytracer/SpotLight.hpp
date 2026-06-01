#pragma once
#include "Light.hpp"
#include <Eigen/Dense>
#include <cmath>


///spotlight: has a pos, a direction it points, and a cone half angle
///outside cone= no light, inside= behaves like a point light (in cone shape)
class SpotLight : public Light
{

public:
    SpotLight
    (
        const Eigen::Vector3f& position,
        const Eigen::Vector3f& intensity,
        const Eigen::Vector3f& direction,
        float coneHalfAngle) //half angle of spotlight cone in radians:points outside angle receives no light from spotlight
        : position_(position),
        intensity_(intensity),
        direction_(direction.normalized()),
        coneHalfAngle_(coneHalfAngle)
    {
    }

    virtual bool visibilityCheck(const Eigen::Vector3f& location, const Renderable* renderable) const override
    {
        //first check if point lies inside spotlight cone
        Eigen::Vector3f toPoint = (location - position_).normalized();

        float cosAngle = toPoint.dot(direction_); //gives cosine of angle between spotlight direction and point being tested
        float cosHalf = cosf(coneHalfAngle_);

        if (cosAngle < cosHalf) //if outside cone, no illumination
            return false;

        //normal shadow test to see if another onj blocks light 
        Ray shadowRay;
        shadowRay.origin = location;
        shadowRay.direction = (position_ - location).normalized();
        float maxT = (position_ - location).norm();
        HitInfo info;
        return !renderable->intersect(shadowRay, 1e-4f, maxT, info, SHADOW_BITMASK); //if no obj hit, visable to spotlight
    }

    virtual Eigen::Vector3f getIntensity(const Eigen::Vector3f& location) const override
    {
        Eigen::Vector3f toPoint = (location - position_).normalized();

        float cosAngle = toPoint.dot(direction_);
        float cosHalf = cosf(coneHalfAngle_);

        if (cosAngle < cosHalf) //no light if outside cone
            return Eigen::Vector3f::Zero();

        float dist = (position_ - location).norm();

        return intensity_ / (dist * dist);
    }

    virtual Eigen::Vector3f getVecToLight(const Eigen::Vector3f& location) const override
    {
        return (position_ - location).normalized(); //return norm vector towards light source, used by shading calcs (phong specular)
    }

private:
    Eigen::Vector3f position_;
    Eigen::Vector3f intensity_;
    Eigen::Vector3f direction_;
    float coneHalfAngle_;

};