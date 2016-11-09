#include "physics.h"
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <graphics_framework.h>
#include <phys_utils.h>
#include <thread>

using namespace std;
using namespace graphics_framework;
using namespace glm;


#define physics_tick 1.0 / 60.0

static vector<unique_ptr<Entity>> SceneList;
static unique_ptr<Entity> floorEnt;

static vector<unique_ptr<Entity>> balls;

float springConstant = 2.0f;
float naturalLength = 2.0f;


unique_ptr<Entity> CreateParticle() {
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(vec3(0, 5.0 + (double)(rand() % 200) / 20.0, 0));
	//ent->SetPosition(vec3(10.0, 10.0f, 10.0f));
	unique_ptr<Component> physComponent(new cPhysics());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->AddComponent(unique_ptr<Component>(new cSphereCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	return ent;
}


void CreateSpring(const vector<unique_ptr<Entity>> &particles, float originalLength, float k)
{
	vec3 force = particles[1]->GetPosition() - particles[0]->GetPosition();
	float length = glm::length(force);
	length = abs(length - originalLength) * k;

	vec3 springNormal = normalize(force);
	force = springNormal * (-length);
	auto b = particles[0]->GetComponents("Physics");
	if (b.size() == 1) {
		const auto p = static_cast<cPhysics *>(b[0]);
		p->forces -= force;
	}

	b = particles[1]->GetComponents("Physics");
	if (b.size() == 1) {
		const auto p = static_cast<cPhysics *>(b[0]);
		p->forces += force;
	}
}


bool update(double delta_time) {
	static double t = 0.0;
	static double accumulator = 0.0;
	accumulator += delta_time;

	while (accumulator > physics_tick) {
		UpdatePhysics(t, physics_tick);
		accumulator -= physics_tick;
		t += physics_tick;
	}

	for (auto &e : SceneList) {
		e->Update(delta_time);
	}

	CreateSpring(SceneList, naturalLength, springConstant);

	phys::Update(delta_time);
	return true;
}

bool load_content() {
	phys::Init();
	for (size_t i = 0; i < 2; i++) {
		SceneList.push_back(move(CreateParticle()));
	}

	CreateSpring(SceneList, naturalLength, springConstant);

	floorEnt = unique_ptr<Entity>(new Entity());
	floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));

	phys::SetCameraPos(vec3(10.0f, 40.0f, -50.0f));
	phys::SetCameraTarget(vec3(0.0f, 0.0f, 0));
	InitPhysics();
	return true;
}

bool render() {
	for (auto &e : SceneList) {
		e->Render();
	}
	phys::DrawScene();
	return true;
}

void main() {
	// Create application
	app application;
	// Set load content, update and render methods
	application.set_load_content(load_content);
	application.set_update(update);
	application.set_render(render);
	// Run application
	application.run();
}

