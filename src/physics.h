#pragma once
#include "game.h"

class cPhysics : public Component {
public:
  cPhysics();
  ~cPhysics();
  glm::vec3 position;
  glm::vec3 prev_position;
  glm::dvec3 velocity;
  glm::dvec3 forces;
  //gravity is set in the particle so it can be modified
  glm::dvec3 gravity = glm::dvec3(0, -10.0, 0);
  double mass;
  bool fixed = false;
  virtual void Update(double delta);
  virtual void SetParent(Entity *p);
  virtual void AddImpulse(const glm::vec3 &i);
  virtual float getX();
  virtual float getY();
  virtual float getZ();
  virtual void Render() {};
private:
};

class cParticle : public cPhysics {
public:
  cParticle();
  ~cParticle();
  void Update(double delta);
  virtual void Render() {};
private:
};

//spring class
class cSpring
{
	//first particle
	cPhysics *a;
	//second particle
	cPhysics *b;
	//coloour of the spring
	phys::RGBAInt32 col;
	//spring stiffness
	float springConstant;
	//damper force
	float dampingFactor;
	//rest length of the spring
	float restLength;
public:
	//spring constructor passing second particle and first particle, with all the constants and the colour
	cSpring(cPhysics *particle, cPhysics *other, float springConstant, float restLength, float damping, phys::RGBAInt32 col);
	//method to update the spring that does all the spring physics calculations
	virtual void update();
	//for testing - renders the spring as a line
	void Render();
};

class cRigidBody : public cPhysics {
public:
  cRigidBody();
  ~cRigidBody();
  void Update(double delta);

private:
};

class cCollider : public Component {
public:
  cCollider(const std::string &tag);
  ~cCollider();
  void Update(double delta);

private:
};

class cSphereCollider : public cCollider {

public:
  double radius;
  cSphereCollider();
  ~cSphereCollider();

private:
};

class cPlaneCollider : public cCollider {
public:
  glm::dvec3 normal;
  cPlaneCollider();
  ~cPlaneCollider();

private:
};

struct collisionInfo {
  const cCollider *c1;
  const cCollider *c2;
  const glm::dvec3 position;
  const glm::dvec3 normal;
  const double depth;
};

void InitPhysics();
void ShutdownPhysics();
void UpdatePhysics(const double t, const double dt);
