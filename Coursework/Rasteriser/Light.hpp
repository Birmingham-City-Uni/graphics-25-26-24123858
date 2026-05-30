#pragma once
#include <Eigen/Dense>
#include <vector>
#include <fstream>


//sbstract class representing a light source
class Light 
{
protected:
	Eigen::Vector3f _intensity;
public:
	Light(const Eigen::Vector3f& intensity)
		:_intensity(intensity)
	{
	};

	enum Type { POINT, SPOT, DIRECTIONAL, AMBIENT };

	//returns the intensity of this light source at a given location in world space
	//<param name="surfaceLocation"></param>
	//<returns>RGB intensity at the given location.</returns>
	virtual Eigen::Vector3f getIntensityAt(const Eigen::Vector3f& surfaceLocation) = 0;


	//Check if the current light source is an ambient light
	//For ambient lights, getDirection should not be called (ambient lights have no
	//world space location or direction)
	//<returns>True if ambient, false otherwise</returns>
	virtual Type getType() = 0;

	//gets a unit direction vector representing the incoming direction of the light
	//source at a given surface point
	//<param name="surfaceLocation"></param>
	//<returns>Unit vector following direction of incoming light.</returns>
	virtual Eigen::Vector3f getDirection(const Eigen::Vector3f& surfaceLocation) = 0;

	virtual Eigen::Vector3f getLightLocation() = 0;

	Eigen::Vector3f getLightIntensity()
	{
		return _intensity;
	}

};

//an ambient light is applied uniformly to all scene locations, having no direction
//or location
class AmbientLight : public Light 
{
private:

public:
	AmbientLight(const Eigen::Vector3f& intensity)
		:Light(intensity)
	{
	};

	virtual Eigen::Vector3f getIntensityAt(const Eigen::Vector3f& surfaceLocation) override
	{
		return _intensity;
	}

	virtual Type getType() override
	{
		return Type::AMBIENT;
	}

	virtual Eigen::Vector3f getDirection(const Eigen::Vector3f& surfaceLocation) override
	{
		//ambient lights do not have a direction, so throw an error!
		throw std::runtime_error("ERROR: Ambient lights have no light direction.");
	}

	virtual Eigen::Vector3f getLightLocation() override
	{
		//ambient lights do not have a location, so throw an error!
		throw std::runtime_error("ERROR: Ambient lights have no location.");
	}
};

//directional lights have a fixed direction and uniformly illuminate all world points
//from that direction
//they have no falloff (unlike point and spot lights)
class DirectionalLight : public Light 
{
private:
	Eigen::Vector3f _direction;

public:
	DirectionalLight(const Eigen::Vector3f& intensity, const Eigen::Vector3f& direction)
		:Light(intensity), _direction(direction.normalized())
	{
	};

	virtual Eigen::Vector3f getIntensityAt(const Eigen::Vector3f& surfaceLocation) override
	{
		return _intensity;
	}

	virtual Type getType() override
	{
		return Type::DIRECTIONAL;
	}

	virtual Eigen::Vector3f getDirection(const Eigen::Vector3f& surfaceLocation) override
	{
		return _direction;
	}

	virtual Eigen::Vector3f getLightLocation() override
	{
		throw std::runtime_error("ERROR: Directional lights have no location.");
	}
};

//point lights have a location in the world, and their intensity falls off with the 
//inverse square law
//their direction is determined by the location of the surface relative to the light
class PointLight : public Light 
{
private:
	Eigen::Vector3f _location;

public:
	PointLight(const Eigen::Vector3f& intensity, const Eigen::Vector3f& location)
		:Light(intensity), _location(location)
	{
	};

	virtual Eigen::Vector3f getIntensityAt(const Eigen::Vector3f& surfaceLocation) override
	{
		float distance = (_location - surfaceLocation).norm();
		return _intensity / (distance * distance);
	}

	virtual Type getType() override
	{
		return Type::POINT;
	}

	virtual Eigen::Vector3f getDirection(const Eigen::Vector3f& surfaceLocation) override
	{
		return (surfaceLocation - _location).normalized();
	}

	virtual Eigen::Vector3f getLightLocation() override
	{
		return _location;
	}
};

//spot lights have a location in the world, and their intensity falls off with the 
//inverse square law, just like point lights
//however they also point in a specified direction, and their intensity falls off as you move 
//away from this direction
//this spot light implementation has a hard edge, and intensity drops to zero if the angle
//between the surface direction and spot light direction is greater than _angle
class SpotLight : public Light 
{
private:
	Eigen::Vector3f _location;
	Eigen::Vector3f _direction;
	float _cosAngle;

public:
	SpotLight(const Eigen::Vector3f& intensity, const Eigen::Vector3f& location,
		const Eigen::Vector3f& direction, float angle)
		:Light(intensity), _location(location), _direction(direction), _cosAngle(cosf(angle))
	{
	};

	virtual Eigen::Vector3f getIntensityAt(const Eigen::Vector3f& surfaceLocation) override
	{
		auto surfaceDir = (surfaceLocation - _location).normalized();
		if (surfaceDir.dot(_direction) < _cosAngle) {
			return Eigen::Vector3f::Zero();
		}

		float distance = (_location - surfaceLocation).norm();
		return _intensity / (distance * distance);
	}

	virtual Type getType() override
	{
		return Type::SPOT;
	}

	virtual Eigen::Vector3f getDirection(const Eigen::Vector3f& surfaceLocation) override
	{
		return (surfaceLocation - _location).normalized();
	}

	virtual Eigen::Vector3f getLightLocation() override
	{
		return _location;
	}
};