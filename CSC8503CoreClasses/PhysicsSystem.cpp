#include "PhysicsSystem.h"
#include "PhysicsObject.h"
#include "GameObject.h"
#include "CollisionDetection.h"
#include "Quaternion.h"

#include "Constraint.h"

#include "Debug.h"
#include "Window.h"
#include <functional>
using namespace NCL;
using namespace CSC8503;

PhysicsSystem::PhysicsSystem(GameWorld& g) : gameWorld(g)	{
	applyGravity	= true;
	useBroadPhase	= true;
	dTOffset		= 0.0f;
	globalDamping	= 0.995f;
	g.SetPhysicsSystem(this);
	SetGravity(Vector3(0.0f, -9.8f, 0.0f));
	octTreePointer = new OctTree<GameObject>(Vector3(1024, 256, 1024), 7, 6);
}

PhysicsSystem::~PhysicsSystem()	{
	delete octTreePointer;
}

void PhysicsSystem::SetGravity(const Vector3& g) {
	gravity = g;
}

void PhysicsSystem::AddObject(GameObject* object) {
		Vector3 halfSizes;

		object->UpdateBroadphaseAABB();
		
		if (!object->GetBroadphaseAABB(halfSizes))
			return;
		Vector3 pos = object->GetTransform().GetPosition();
		octTreePointer->Insert(object, pos, halfSizes, object->GetPhysicsObject()->GetSleepState());
}

void PhysicsSystem::RemoveObject(GameObject* object) {
	std::set<CollisionDetection::CollisionInfo> tempCollisions;
	for (CollisionDetection::CollisionInfo collision : allCollisions) {
		if (collision.a != object && collision.b != object)
			tempCollisions.insert(collision);
	}
	allCollisions = tempCollisions;
}

/*

If the 'game' is ever reset, the PhysicsSystem must be
'cleared' to remove any old collisions that might still
be hanging around in the collision list. If your engine
is expanded to allow objects to be removed from the world,
you'll need to iterate through this collisions list to remove
any collisions they are in.

*/
void PhysicsSystem::Clear() {
	allCollisions.clear();
	delete octTreePointer;
	octTreePointer = new OctTree<GameObject>(Vector3(1024, 256, 1024), 7, 6);
}

/*

This is the core of the physics engine update

*/

bool useSimpleContainer = false;

int constraintIterationCount = 10;

//This is the fixed timestep we'd LIKE to have
const int   idealHZ = 120;
const float idealDT = 1.0f / idealHZ;

/*
This is the fixed update we actually have...
If physics takes too long it starts to kill the framerate, it'll drop the 
iteration count down until the FPS stabilises, even if that ends up
being at a low rate. 
*/
int realHZ		= idealHZ;
float realDT	= idealDT;

void PhysicsSystem::Update(float dt) {	
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::B)) {
		useBroadPhase = !useBroadPhase;
		std::cout << "Setting broadphase to " << useBroadPhase << std::endl;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::N)) {
		useSimpleContainer = !useSimpleContainer;
		std::cout << "Setting broad container to " << useSimpleContainer << std::endl;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::I)) {
		constraintIterationCount--;
		std::cout << "Setting constraint iterations to " << constraintIterationCount << std::endl;
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::O)) {
		constraintIterationCount++;
		std::cout << "Setting constraint iterations to " << constraintIterationCount << std::endl;
	}

	dTOffset += dt; //We accumulate time delta here - there might be remainders from previous frame!

	GameTimer t;
	t.GetTimeDeltaSeconds();

	if (useBroadPhase) {
		UpdateObjectAABBs();
	}
	int iteratorCount = 0;
	while(dTOffset > realDT) {
		IntegrateAccel(realDT); //Update accelerations from external forces
		if (useBroadPhase) {
			BroadPhase(iteratorCount == 0 ? true : false);
			NarrowPhase();
		}
		else {
			BasicCollisionDetection();
		}

		//This is our simple iterative solver - 
		//we just run things multiple times, slowly moving things forward
		//and then rechecking that the constraints have been met		
		float constraintDt = realDT /  (float)constraintIterationCount;
		for (int i = 0; i < constraintIterationCount; ++i) {
			UpdateConstraints(constraintDt);	
		}
		IntegrateVelocity(realDT); //update positions from new velocity changes

		dTOffset -= realDT;
		iteratorCount++;
	}
	CheckSleeping(dt);

	ClearForces();	//Once we've finished with the forces, reset them to zero

	UpdateCollisionList(); //Remove any old collisions

	t.Tick();
	float updateTime = t.GetTimeDeltaSeconds();

	//Uh oh, physics is taking too long...
	if (updateTime > realDT) {
		realHZ /= 2;
		realDT *= 2;
		std::cout << "Dropping iteration count due to long physics time...(now " << realHZ << ")\n";
	}
	else if (dt * 2 < realDT) { //we have plenty of room to increase iteration count!
		int temp = realHZ;
		realHZ *= 2;
		realDT /= 2;

		if (realHZ > idealHZ) {
			realHZ = idealHZ;
			realDT = idealDT;
		}
		if (temp != realHZ) {
			std::cout << "Raising iteration count due to short physics time...(now " << realHZ << ")\n";
		}
	}
}

void PhysicsSystem::CheckSleeping(float dt) {
	std::vector<GameObject*>::const_iterator first, last;
	gameWorld.GetObjectIterators(first, last);

	for (auto i = first; i != last; i++) {
		PhysicsObject* object = (*i)->GetPhysicsObject();
		if (object == nullptr)
			continue; // No physics object for this GameObject
		object->AddSleepProgress(dt);
	}
}

/*
Later on we're going to need to keep track of collisions
across multiple frames, so we store them in a set.

The first time they are added, we tell the objects they are colliding.
The frame they are to be removed, we tell them they're no longer colliding.

From this simple mechanism, we we build up gameplay interactions inside the
OnCollisionBegin / OnCollisionEnd functions (removing health when hit by a 
rocket launcher, gaining a point when the player hits the gold coin, and so on).
*/
void PhysicsSystem::UpdateCollisionList() {
	for (std::set<CollisionDetection::CollisionInfo>::iterator i = allCollisions.begin(); i != allCollisions.end(); ) {
		if ((*i).framesLeft == numCollisionFrames) {
			i->a->OnCollisionBegin(i->b);
			i->b->OnCollisionBegin(i->a);
		}

		CollisionDetection::CollisionInfo& in = const_cast<CollisionDetection::CollisionInfo&>(*i);
		in.framesLeft--;

		if ((*i).framesLeft < 0) {
			i->a->OnCollisionEnd(i->b);
			i->b->OnCollisionEnd(i->a);
			i = allCollisions.erase(i);
		}
		else {
			++i;
		}
	}
}

void PhysicsSystem::UpdateObjectAABBs() {
	std::vector<GameObject*>::const_iterator first, last;
	gameWorld.GetObjectIterators(first, last);
	for (auto i = first; i != last; i++)
		(*i)->UpdateBroadphaseAABB();
}

/*

This is how we'll be doing collision detection in tutorial 4.
We step thorugh every pair of objects once (the inner for loop offset 
ensures this), and determine whether they collide, and if so, add them
to the collision set for later processing. The set will guarantee that
a particular pair will only be added once, so objects colliding for
multiple frames won't flood the set with duplicates.
*/
void PhysicsSystem::BasicCollisionDetection() {
	std::vector<GameObject*>::const_iterator first, last;
	gameWorld.GetObjectIterators(first, last);

	int count = 0;
	int totalCount = 0;
	for (auto i = first; i != last; i++) {
		if ((*i)->GetPhysicsObject() == nullptr)
			continue;
		for (auto j = i + 1; j != last; j++) {
			if ((*j)->GetPhysicsObject() == nullptr)
				continue;
			CollisionDetection::CollisionInfo info;
			count++;
			totalCount++;
			if (CollisionDetection::ObjectIntersection(*i, *j, info)) {
				count--;
				ImpulseResolveCollision(*info.a, *info.b, info.point);
				info.framesLeft = numCollisionFrames;
				allCollisions.insert(info);
			}
		}
	}
}

/*

In tutorial 5, we start determining the correct response to a collision,
so that objects separate back out. 

*/
void PhysicsSystem::ImpulseResolveCollision(GameObject& a, GameObject& b, CollisionDetection::ContactPoint& p) const {
	PhysicsObject* physA = a.GetPhysicsObject();
	PhysicsObject* physB = b.GetPhysicsObject();

	Transform& transformA = a.GetTransform();
	Transform& transformB = b.GetTransform();

	float totalMass = physA->GetInverseMass() + physB->GetInverseMass();

	if (totalMass == 0)
		return; // Two static objects??

	// Seperate objects using projection
	transformA.SetPosition(transformA.GetPosition() - (p.normal * p.penetration * (physA->GetInverseMass() / totalMass)));

	transformB.SetPosition(transformB.GetPosition() + (p.normal * p.penetration * (physB->GetInverseMass() / totalMass)));

	// Calculate impulse
	Vector3 relativeA = p.localA;
	Vector3 relativeB = p.localB;

	Vector3 angVelocityA = Vector3::Cross(physA->GetAngularVelocity(), relativeA);
	Vector3 angVelocityB = Vector3::Cross(physB->GetAngularVelocity(), relativeB);

	Vector3 fullVelocityA = physA->GetLinearVelocity() + angVelocityA;
	Vector3 fullVelocityB = physB->GetLinearVelocity() + angVelocityB;

	Vector3 contactVelocity = fullVelocityB - fullVelocityA;

	float impulseForce = Vector3::Dot(contactVelocity, p.normal);

	// Work out effect of inertia
	Vector3 inertiaA = Vector3::Cross(physA->GetInertiaTensor() * Vector3::Cross(relativeA, p.normal), relativeA);

	Vector3 inertiaB = Vector3::Cross(physB->GetInertiaTensor() * Vector3::Cross(relativeB, p.normal), relativeB);

	float angularEffect = Vector3::Dot(inertiaA + inertiaB, p.normal);

	float cRestitution = (a.getCRest() + b.getCRest())/2.0f; // Disperse some kinetic energy

	float j = (-(1.0f + cRestitution) * impulseForce) / (totalMass + angularEffect);

	Vector3 fullImpulse = p.normal * j;

	physA->ApplyLinearImpulse(-fullImpulse);
	physB->ApplyLinearImpulse(fullImpulse);

	physA->ApplyAngularImpulse(Vector3::Cross(relativeA, -fullImpulse));
	physB->ApplyAngularImpulse(Vector3::Cross(relativeB, fullImpulse));

	// Friction, I think this is working right? In any case it seems to work
	Vector3 tangent = contactVelocity - (p.normal * impulseForce);

	Vector3 frictionA = Vector3::Cross(physA->GetInertiaTensor() * Vector3::Cross(relativeA, tangent), relativeA);

	Vector3 frictionB = Vector3::Cross(physB->GetInertiaTensor() * Vector3::Cross(relativeB, tangent), relativeB);

	float frictionEffect = Vector3::Dot(frictionA + frictionB, tangent);

	float tangentForce = Vector3::Dot(contactVelocity, tangent);

	float cFriction = (a.getCFric() + b.getCFric()) / 2.0f;

	float Jt = (-cFriction * tangentForce) / (totalMass + frictionEffect);

	Vector3 fullFriction = tangent * Jt;

	physA->ApplyAngularImpulse(Vector3::Cross(relativeA, -fullFriction));
	physB->ApplyAngularImpulse(Vector3::Cross(relativeB, fullFriction));
}

/*

Later, we replace the BasicCollisionDetection method with a broadphase
and a narrowphase collision detection method. In the broad phase, we
split the world up using an acceleration structure, so that we can only
compare the collisions that we absolutely need to. 

*/
void PhysicsSystem::BroadPhase(bool firstPass) {
	broadphaseCollisions.clear();

	std::vector<GameObject*>::const_iterator first, last;
	gameWorld.GetObjectIterators(first, last);

	Timepoint preIns = std::chrono::high_resolution_clock::now();

	for (auto i = first; i != last; i++) {
		(*i)->initOwnCount = (*i)->GetNodeCount();
	}

	/*	Loop through all non-sleeping objects, notify each node it is in for updating
	*/
	for (auto i = first; i != last; i++) {
		if (!(*i)->GetPhysicsObject()->GetSleepState()){
			(*i)->NotifyOwningNodes();
		}
	}

	// Get all object pairs within the same node
	//octTree.OperateOnContents(
	octTreePointer->OperateOnContents(
		[&](std::list<OctTreeEntry<GameObject>>& data) {
			CollisionDetection::CollisionInfo info;
			for (auto i = data.begin(); i != data.end(); i++)
				for (auto j = std::next(i); j != data.end(); j++) {
					// If both are sleeping, discard
					if ((*i).object->GetPhysicsObject()->GetSleepState() && (*j).object->GetPhysicsObject()->GetSleepState())
						continue;

					// Is this pair of items already in collision set
					// ie. Is the pair already in another QuadTree node together?

					// Give the two objects a fixed order: objects A and B will be in the info in the same order no matter if they are i or j
					info.a = std::min((*i).object, (*j).object);
					info.b = std::max((*i).object, (*j).object);

					// C++ sets do not permit duplicate values, Insert will prevent this for us
					broadphaseCollisions.insert(info);
				}
		}
	);
	if(firstPass){
		//octTreePointer->DebugDraw();
	}
}

/*

The broadphase will now only give us likely collisions, so we can now go through them,
and work out if they are truly colliding, and if so, add them into the main collision list
*/
void PhysicsSystem::NarrowPhase() {
	int count = 0;
	int totalCount = 0;
	for (std::set<CollisionDetection::CollisionInfo>::iterator i = broadphaseCollisions.begin(); i != broadphaseCollisions.end(); i++) {
		CollisionDetection::CollisionInfo info = *i;
		count++;
		totalCount++;
		if (CollisionDetection::ObjectIntersection(info.a, info.b, info)) {
			count--;
			info.framesLeft = numCollisionFrames;
			ImpulseResolveCollision(*info.a, *info.b, info.point);
			allCollisions.insert(info); // Insert into main set
		}
	}
}

/*
Integration of acceleration and velocity is split up, so that we can
move objects multiple times during the course of a PhysicsUpdate,
without worrying about repeated forces accumulating etc. 

This function will update both linear and angular acceleration,
based on any forces that have been accumulated in the objects during
the course of the previous game frame.
*/
void PhysicsSystem::IntegrateAccel(float dt) {
	std::vector<GameObject*>::const_iterator first, last;
	gameWorld.GetObjectIterators(first, last);

	for (auto i = first; i != last; i++) {
		PhysicsObject* object = (*i)->GetPhysicsObject();
		if (object == nullptr)
			continue; // No physics object for this GameObject

		if (object->GetSleepState())
			continue;
		
		float inverseMass = object->GetInverseMass();
		Vector3 linearVel = object->GetLinearVelocity();
		Vector3 force = object->GetForce();
		Vector3 accel = force * inverseMass;
		float test = force.Length();

		if (applyGravity && inverseMass > 0) // Check mass to not move infinitely heavy things
			accel += gravity;

		linearVel += accel * dt;
		object->SetLinearVelocity(linearVel);

		// Angular stuff
		Vector3 torque = object->GetTorque();
		Vector3 angVel = object->GetAngularVelocity();

		object->UpdateInertiaTensor(); // Update tensor vs orientation

		Vector3 angAccel = object->GetInertiaTensor() * torque;

		angVel += angAccel * dt; // Integrate anglular accel
		object->SetAngularVelocity(angVel);
	}
}

/*
This function integrates linear and angular velocity into
position and orientation. It may be called multiple times
throughout a physics update, to slowly move the objects through
the world, looking for collisions.
*/
void PhysicsSystem::IntegrateVelocity(float dt) {
	std::vector<GameObject*>::const_iterator first, last;
	gameWorld.GetObjectIterators(first, last);

	float frameLinearDamping = 1.0f - (0.4f * dt);

	for (auto i = first; i != last; i++) {
		PhysicsObject* object = (*i)->GetPhysicsObject();
		if (object == nullptr)
			continue;
		if (object->GetSleepState())
			continue;
		Transform& transform = (*i)->GetTransform();
		// Position
		Vector3 position = transform.GetPosition();
		Vector3 linearVel = object->GetLinearVelocity();
		float movement = Vector3(linearVel * dt).Length();
		position += linearVel * dt;
		transform.SetPosition(position);
		// Linear Damping
		linearVel = linearVel * frameLinearDamping;
		object->SetLinearVelocity(linearVel);

		// Orientation
		Quaternion orientation = transform.GetOrientation();
		Vector3 angVel = object->GetAngularVelocity();

		orientation = orientation + (Quaternion(angVel * dt * 0.5f, 0.0f) * orientation);
		orientation.Normalise();

		transform.SetOrientation(orientation);

		// Dampen the angular velocity too
		float frameAngularDamping = 1.0f - (0.4f * dt);
		angVel = angVel * frameAngularDamping;
		object->SetAngularVelocity(angVel);
	}
}

/*
Once we're finished with a physics update, we have to
clear out any accumulated forces, ready to receive new
ones in the next 'game' frame.
*/
void PhysicsSystem::ClearForces() {
	gameWorld.OperateOnContents(
		[](GameObject* o) {
			o->GetPhysicsObject()->ClearForces();
		}
	);
}


/*

As part of the final physics tutorials, we add in the ability
to constrain objects based on some extra calculation, allowing
us to model springs and ropes etc. 

*/
void PhysicsSystem::UpdateConstraints(float dt) {
	std::vector<Constraint*>::const_iterator first;
	std::vector<Constraint*>::const_iterator last;
	gameWorld.GetConstraintIterators(first, last);

	for (auto i = first; i != last; ++i) {
		(*i)->UpdateConstraint(dt);
	}
}