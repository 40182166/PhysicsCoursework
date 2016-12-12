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
  double mass;
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

class cSpring
{
	cPhysics *a;
	cPhysics *b;
	float springConstant;
	float restLength;
public:
	cSpring(cPhysics *particle, cPhysics *other, float springConstant, float restLength);
	virtual void update( double delta);
	void Render();
};

//class ParticleForceGenerator
//{
//public:
//	virtual void Update(Entity *particle, double delta) = 0;
//};
//
//class ParticleForceRegistry
//{
//protected:
//	struct ParticleForceRegistration
//	{
//		Entity *particle;
//		ParticleForceGenerator *fg;
//	};
//
//	typedef std::vector<ParticleForceRegistration> Registry;
//	Registry registrations;
//
//public:
//	void add(Entity *particle, ParticleForceGenerator *fg);
//	void remove(Entity *particle, ParticleForceGenerator *fg);
//	void clear();
//	void Update(double delta);
//};

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
