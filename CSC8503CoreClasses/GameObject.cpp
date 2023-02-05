#include "GameObject.h"
#include "CollisionDetection.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "NetworkObject.h"
#include "OctTreeNode.h"

using namespace NCL::CSC8503;

GameObject::GameObject(string objectName)	{
	name			= objectName;
	worldID			= -1;
	isActive		= true;
	boundingVolume	= nullptr;
	physicsObject	= nullptr;
	renderObject	= nullptr;
	networkObject	= nullptr;
	setCRest(0.66f);
	setCFric(0.66f);
}

GameObject::~GameObject()	{
	delete boundingVolume;
	delete physicsObject;
	delete renderObject;
	delete networkObject;
	RemoveFromTree();
}

bool GameObject::GetBroadphaseAABB(Vector3&outSize) const {
	if (!boundingVolume) {
		return false;
	}
	outSize = broadphaseAABB;
	return true;
}

void GameObject::UpdateBroadphaseAABB() {
	if (!boundingVolume) {
		return;
	}
	if (boundingVolume->type == VolumeType::AABB) {
		broadphaseAABB = ((AABBVolume&)*boundingVolume).GetHalfDimensions();
	}
	else if (boundingVolume->type == VolumeType::Sphere) {
		float r = ((SphereVolume&)*boundingVolume).GetRadius();
		broadphaseAABB = Vector3(r, r, r);
	}
	else if (boundingVolume->type == VolumeType::OBB) {
		Matrix3 mat = Matrix3(transform.GetOrientation());
		mat = mat.Absolute();
		Vector3 halfSizes = ((OBBVolume&)*boundingVolume).GetHalfDimensions();
		broadphaseAABB = mat * halfSizes;
	}
}

void GameObject::AddOwningNode(OctTreeNode<GameObject>* node) {
	owningNodes.push_back(node);
	int i = 1;
}

void GameObject::RemoveOwningNode(OctTreeNode<GameObject>* node) {
	int count = owningNodes.size();
	owningNodes.remove(node);
	if (owningNodes.size() == count) 
		throw new std::invalid_argument("Attempted to remove node that isn't listened to");
}

void GameObject::NotifyOwningNodes() {
	if (owningNodes.size() == 0) // Safety check: If object has fallen outside of tree then won't crash
		return;
	int nodesToRemove = owningNodes.size() - 1;
	for (int i = 0; i < nodesToRemove; i++) {
		owningNodes.back()->Delete(this);
		owningNodes.pop_back();
	}
	OctTreeNode<GameObject>* updatedNode = owningNodes.front();
	owningNodes.pop_front();
	updatedNode->Update(this);
}

void GameObject::RemoveFromTree() {
	while (!owningNodes.empty()) {
		owningNodes.back()->Delete(this);
		owningNodes.pop_back();
	}
}