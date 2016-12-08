#define GLM_ENABLE_EXPERIMENTAL
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

static vector<unique_ptr<Entity>> ClothParticles;
static vector<cSpring> springList;
static unique_ptr<Entity> floorEnt;

static vector<unique_ptr<Entity>> balls;

float springConstant = 10.0f;
float naturalLength = 2.0f;


unique_ptr<Entity> CreateParticle(int xPos, int yPos, int zPos) {
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(vec3(xPos, yPos, zPos));
	unique_ptr<Component> physComponent(new cPhysics());
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	ent->AddComponent(unique_ptr<Component>(new cSphereCollider()));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	return ent;
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

	for (auto &e : ClothParticles) {
		e->Update(delta_time);
	}

	for (int i = 0; i < springList.size() - 1; i++)
	{
		auto b = ClothParticles[i+1]->GetComponents("Physics");
		const auto p = static_cast<cPhysics *>(b[0]);

		springList[i].update(p, 1.0);
	}
	

	phys::Update(delta_time);
	return true;
}

bool load_content() {
	phys::Init();

	for (int x = -4; x < 5; x++)
	{
		//for (int z = -4; z < 5; z++)
			
		//{
			unique_ptr<Entity> particle = CreateParticle(x * 5.0f, 1.0f, 5.0f);
			ClothParticles.push_back(move(particle));
		//}
	}
	
	for (auto &e : ClothParticles) {
		auto b = e->GetComponents("Physics");
		const auto p = static_cast<cPhysics *>(b[0]);

		cSpring spring = cSpring(p, springConstant, naturalLength);

		springList.push_back(spring);
	}


	floorEnt = unique_ptr<Entity>(new Entity());
	floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));

	phys::SetCameraPos(vec3(10.0f, 40.0f, -50.0f));
	phys::SetCameraTarget(vec3(0.0f, 0.0f, 0));
	InitPhysics();
	return true;
}

bool render() {
	for (auto &e : ClothParticles) {
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

