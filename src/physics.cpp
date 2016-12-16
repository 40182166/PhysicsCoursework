#include "physics.h"
#include "collision.h"
#include <glm/glm.hpp>
using namespace std;
using namespace glm;
static vector<cPhysics *> physicsScene;
static vector<cCollider *> colliders;

static dvec3 initialV = dvec3(0.0, 0.0, 0.0);
//flag to determine if a particle has to be rendered and updated
bool flag = false;

void Resolve(const collisionInfo &ci) {

	//Coefficient of return has been removed since cloth particles don't have to bounce
  auto a = ci.c1->GetParent()->GetComponents("Physics");
  if (a.size() == 1) {
    const auto p = static_cast<cPhysics *>(a[0]);
    p->position += ci.normal * (ci.depth * 0.5);
    const double currentSpeed = glm::length(p->position - p->prev_position);
    p->prev_position = p->position;
  }
  auto b = ci.c2->GetParent()->GetComponents("Physics");
  if (b.size() == 1) {
    const auto p = static_cast<cPhysics *>(b[0]);
    p->position += -ci.normal * (ci.depth * 0.5 * 0.1);
    const double currentSpeed = glm::length(p->position - p->prev_position);
    p->prev_position = p->position;
  }
}

//Default mass is 1.0
cPhysics::cPhysics() : forces(dvec3(0)), mass(1.0), Component("Physics") { physicsScene.push_back(this); }

cPhysics::~cPhysics() {
  auto position = std::find(physicsScene.begin(), physicsScene.end(), this);
  if (position != physicsScene.end()) {
    physicsScene.erase(position);
  }
}

void cPhysics::Update(double delta) {
  for (auto &e : physicsScene) {
    e->GetParent()->SetPosition(e->position);
  }
}

void cPhysics::SetParent(Entity *p) {
  Component::SetParent(p);
  position = Ent_->GetPosition();
  prev_position = position;
}

void cPhysics::AddImpulse(const glm::vec3 &i) { forces += i; }

float cPhysics::getX()
{
	return this->position.x;
}

float cPhysics::getY()
{
	return this->position.y;
}

float cPhysics::getZ()
{
	return this->position.z;
}


void UpdatePhysics(const double t, const double dt) {
  std::vector<collisionInfo> collisions;
  // check for collisions
  {
    dvec3 pos;
    dvec3 norm;
    double depth;
    for (size_t i = 0; i < colliders.size(); ++i) {
      for (size_t j = i + 1; j < colliders.size(); ++j) {
        if (collision::IsColliding(*colliders[i], *colliders[j], pos, norm, depth)) {
          collisions.push_back({colliders[i], colliders[j], pos, norm, depth});
        }
      }
    }
  }
  // handle collisions
  {
    for (auto &c : collisions) {
      Resolve(c);
    }
  }
  // Integrating using Verlet method
  for (auto &e : physicsScene) {
	  if (e->fixed == false)
	  {
		  e->Render();
	  }
	  else
	  {
		  continue;
	  }
    
	
    // calcualte velocity from current and previous position
    e->velocity = e->position - e->prev_position;
	if (flag)
	{
		e->velocity += initialV;
		flag = false;
	}
    // set previous position to current position
    e->prev_position = e->position;
	//force multiplied by inverse mass to get realism of interaction between cloth mass, gravity and forces
	e->position += e->velocity + ((e->forces * (1.0 / e->mass)) + e->gravity) * pow(dt, 2);
    
    e->forces = dvec3(0);
    if (e->position.y <= 0.0f) {
    }
  }
}

void InitPhysics() {}

void ShutdownPhysics() {}
//----------------------

cParticle::cParticle() {}

cParticle::~cParticle() {}

void cParticle::Update(double delta) {}

//----------------------
cRigidBody::cRigidBody() {}

cRigidBody::~cRigidBody() {}

void cRigidBody::Update(double delta) {}

cCollider::cCollider(const std::string &tag) : Component(tag) { colliders.push_back(this); }

cCollider::~cCollider() {
  auto position = std::find(colliders.begin(), colliders.end(), this);
  if (position != colliders.end()) {
    colliders.erase(position);
  }
}

void cCollider::Update(double delta) {}

cSphereCollider::cSphereCollider() : radius(0.3), cCollider("SphereCollider") {}

cSphereCollider::~cSphereCollider() {}

cPlaneCollider::cPlaneCollider() : normal(dvec3(0, 1.0, 0)), cCollider("PlaneCollider") {}

cPlaneCollider::~cPlaneCollider() {}

//Spring constructor
cSpring::cSpring(cPhysics *other, cPhysics *p, float sc, float rl, float damper, phys::RGBAInt32 c) : b(p), a(other), springConstant(sc), restLength(rl), dampingFactor(damper), col(c)
{
}

//Spring update method, calculates all the physics
void cSpring::update()
{

	//force is position of the second particle - position of the first particle
	vec3 force = b->position;
	force -= a->position;

	//calculation force magnitude
	float magnitude = length(force);
	//magnitude of the force - the rest length of the spring
	magnitude = magnitude - restLength;
	//magnitude * the spring constant
	magnitude *= springConstant;

	//normalizing force
	force = normalize(force);
	//force * -magnitude of the force
	force *= -magnitude;


	//calculating damper force asa velocity of second particle - velocity of first particle
	vec3 dampForce = b->velocity - a->velocity;
	//damp force vector * the damping factor
	dampForce *= dampingFactor;
	//contrasting force of spring with damper : force - damper force
	force -= dampForce;

	//adding forces to both particles
	b->AddImpulse(force);
	a->AddImpulse(-force);

}

//For testing - renders all the springs and makes them visible and coloured
void cSpring::Render()
{
	//spring is rendered as a line
	phys::DrawLine(this->a->position, this->b->position, false, this->col);
}
