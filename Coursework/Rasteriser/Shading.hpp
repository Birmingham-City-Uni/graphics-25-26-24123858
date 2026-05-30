#pragma once
#include <Eigen/Dense>

//Reflect function to find the reflected vector
//reflect an incoming vector in a normal
//<param name="incoming">Incoming direction unit vector, pointing into surface.</param>
//<param name="normal">Surface normal</param>
//<returns>Reflected vector, pointing out from surface point.</returns>
Eigen::Vector3f reflect(const Eigen::Vector3f& incoming, const Eigen::Vector3f& normal)
{
	//reflection formula: R = I - 2(N·I)N
	//incoming vector points into the surface, reflected points out
	//I (incomingLightDir) points from the light to the surface (into the surface)
	//so - version of the formula used, as R = I + 2(N·I)N assumes I points out of the surface
	Eigen::Vector3f reflected = incoming - 2.0f * incoming.dot(normal) * normal;
	return reflected;
}

//Phong specular term to find the intensity of specular reflection
//Returns the Phong specular term for a given lighting direction, normal, view direction and specular exponent
//All input vectors must be normalised unit vectors.
//<param name="incomingLightDir">Unit direction vector from light towards surface point.</param>
//<param name="normal">Normal at surface point.</param>
//<param name="viewDir">Direction unit vector from surface point towards viewing camera.</param>
//<param name="exponent">Specular exponent (higher=shinier)</param>
//<returns>Specular term (number from 0 to 1)</returns>
float phongSpecularTerm(const Eigen::Vector3f& incomingLightDir, const Eigen::Vector3f& normal, const Eigen::Vector3f& viewDir, float exponent)
{
	//reflected direction using the reflect function
	Eigen::Vector3f reflectionDir = reflect(incomingLightDir, normal);

	//dot product between reflected and view directions
	float reflectDotNorm = reflectionDir.dot(viewDir);

	//making sure dot product is non-negative (if it's less than 0, set it to 0)
	reflectDotNorm = std::max(reflectDotNorm, 0.0f);

	//raise to specular exponent and return
	return powf(reflectDotNorm, exponent);
}

//Implementing the Blinn-Phong reflection model 
//Returns the Blinn-Phong specular term for a given lighting direction, normal, view direction and specular exponent
//All input vectors must be normalised unit vectors
//<param name="incomingLightDir">Unit direction vector from light towards surface point.</param>
//<param name="normal">Normal at surface point.</param>
//<param name="viewDir">Direction unit vector from surface point towards viewing camera.</param>
//<param name="exponent">Specular exponent (higher=shinier)</param>
//<returns>Specular term (number from 0 to 1)</returns>
float blinnPhongSpecularTerm(const Eigen::Vector3f& incomingLightDir, const Eigen::Vector3f& normal, const Eigen::Vector3f& viewDir, float exponent)
{
	//Finding the half-vector (average of view dir and light dir)
	//Blinn-Phong uses a halfway vector instead of a reflection vector which
	//is the vector exactly halfway between the view direction and the incoming 
	//light direction (negated)
	//Its cheaper to compute than a full reflection
	Eigen::Vector3f halfVec = (-incomingLightDir + viewDir).normalized();

	//Find dot product of half-vector and normal
	//When this is high (vectors align)= looking at the specular highlight
	float halfDotNorm = halfVec.dot(normal);
	//Force the dot product to be non-negative (if <0, set to 0)
	//clamp to 0 - negative means light is on wrong side of surface
	halfDotNorm = std::max(halfDotNorm, 0.0f);

	//Return the dot product raised to the exponent
	//return 0.0f;
	return powf(halfDotNorm, exponent);
}