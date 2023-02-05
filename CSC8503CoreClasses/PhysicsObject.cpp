#include "PhysicsObject.h"
#include "PhysicsSystem.h"
#include "Transform.h"
using namespace NCL;
using namespace CSC8503;

PhysicsObject::PhysicsObject(Transform* parentTransform, const CollisionVolume* parentVolume, bool sleep)	{
	transform	= parentTransform;
	initialPosition = parentTransform->GetPosition();
	lastPosition = initialPosition;
	volume		= parentVolume;

	inverseMass = 1.0f;
	elasticity	= 0.8f;
	friction	= 0.8f;

	this->lockedAxes = Vector3(1, 1, 1);

	this->sleep = sleep;
}

PhysicsObject::~PhysicsObject()	{

}

void PhysicsObject::ApplyAngularImpulse(const Vector3& force) {
	angularVelocity += inverseInteriaTensor * force * lockedAxes;
}

void PhysicsObject::ApplyLinearImpulse(const Vector3& force) {
	linearVelocity += force * inverseMass;
}

void PhysicsObject::AddForce(const Vector3& addedForce) {
	force += addedForce;
	SetNotSleeping();
}

void PhysicsObject::AddForceAtPosition(const Vector3& addedForce, const Vector3& position) {
	Vector3 localPos = position - transform->GetPosition();

	force  += addedForce;
	torque += Vector3::Cross(localPos, addedForce);
	SetNotSleeping();
}

void PhysicsObject::AddTorque(const Vector3& addedTorque) {
	torque += addedTorque;
	SetNotSleeping();
}

void PhysicsObject::ClearForces() {
	force				= Vector3();
	torque				= Vector3();
}

void PhysicsObject::AddSleepProgress(float dt) {
	float check_constant = 0.75;
	// Check if object has significantly moved since last check
	Vector3 currentPosition = transform->GetPosition();
	float distanceSinceLast = (currentPosition - lastPosition).Length();
	if (distanceSinceLast < dt * check_constant * (sleep ? 0.5 : 4)) {
		if (!sleep) {
			if (sleepProgress == 0) {
				initialPosition = currentPosition;
				sleepProgress = FLT_MIN;
			}
			else
				if (sleepProgress >= 1) {
					// Check if object has significantly moved since first check
					float distanceSinceFirst = (currentPosition - initialPosition).Length();
					if (distanceSinceFirst < (sleepProgress + dt) * check_constant)
						sleep = true;
					else sleepProgress = 0;
				}
				else sleepProgress += dt;
		}
	}
	else {
		sleepProgress = 0;
		SetNotSleeping();
	}
	lastPosition = currentPosition;
}

void PhysicsObject::InitCubeInertia() {
	Vector3 dimensions	= transform->GetScale();

	Vector3 fullWidth = dimensions * 2;

	Vector3 dimsSqr		= fullWidth * fullWidth;

	inverseInertia.x = (12.0f * inverseMass) / (dimsSqr.y + dimsSqr.z);
	inverseInertia.y = (12.0f * inverseMass) / (dimsSqr.x + dimsSqr.z);
	inverseInertia.z = (12.0f * inverseMass) / (dimsSqr.x + dimsSqr.y);
}

void PhysicsObject::InitSphereInertia() {
	float radius	= transform->GetScale().GetMaxElement();
	float i			= 2.5f * inverseMass / (radius*radius);

	inverseInertia	= Vector3(i, i, i);
}

void PhysicsObject::UpdateInertiaTensor() {
	Quaternion q = transform->GetOrientation();
	
	Matrix3 invOrientation	= Matrix3(q.Conjugate());
	Matrix3 orientation		= Matrix3(q);

	inverseInteriaTensor = orientation * Matrix3::Scale(inverseInertia) *invOrientation;
}