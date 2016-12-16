#define GLM_ENABLE_EXPERIMENTAL
#include "physics.h"
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <graphics_framework.h>
#include <phys_utils.h>
#include <thread>
#include <sstream>

using namespace std;
using namespace graphics_framework;
using namespace glm;

#define physics_tick 1.0f / 60.0f

double xpos = 0.0f;
double ypos = 0.0f;
free_camera free_cam;
bool isCam = false;
bool isRendered = false;
bool isWindActive = false;

static vector<unique_ptr<Entity>> ClothParticles;
static vector<cSpring> springList;
static unique_ptr<Entity> floorEnt;

int rows = 17;
float stretchConstant = 100.0f;
float shearConstant = 90.0f;
float bendingConstant = 80.0f;
float diagonalBendingConstant = 20.0f;
float naturalLength = 0.3f;
float dampingFactor = 100.0f;
vec3 windDir = vec3(0.0f, 0.0f, 0.0f);
vector<glm::vec3> grid;

unique_ptr<Entity> CreateParticle(float xPos, float yPos, float zPos, double myMass) {
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(vec3(xPos, yPos, zPos));
	cPhysics *phys = new cPhysics();
	phys->mass = myMass;
	unique_ptr<Component> physComponent(phys);
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	cSphereCollider *coll = new cSphereCollider();
	coll->radius = 0.03f;
	ent->AddComponent(unique_ptr<Component>(coll));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	return ent;
}

cPhysics *getParticle(int x, int z)
{
	auto p = static_cast<cPhysics *>(ClothParticles[x * rows + z]->GetComponents("Physics")[0]);
	return p;
}

cSpring getSpring(int x, int z)
{
	auto p = springList[x * rows + z];
	return p;
}

void Cloth()
{
	for (int x = 0; x < rows; x++)
	{
		for (int z = 0; z < rows; z++)
		{
			unique_ptr<Entity> particle = CreateParticle(x*0.3f, 10.0f, (z - 10.0f)*0.3f, 1.0f);
			auto p = static_cast<cPhysics *>(particle->GetComponents("Physics")[0]);
			ClothParticles.push_back(move(particle));
		}
	}
}

void updateCloth()
{
	springList.clear();

	for (int x = 0; x < rows; x++)
	{
		for (int z = 0; z < rows; z++)
		{
			//structural and shear springs
			if (z == (rows - 1))
			{
				if (x + 1 != rows)
				{
					//horizontal structural spring
					cSpring aa = cSpring(getParticle(x + 1, z), getParticle(x, z), stretchConstant, naturalLength, dampingFactor, BLUE);
					aa.update();
					springList.push_back(aa);
				}
			}
			else if (x == (rows - 1))
			{
				if (z + 1 != rows)
				{
					//vertical structural spring
					cSpring aa = cSpring(getParticle(x, z + 1), getParticle(x, z), stretchConstant, naturalLength, dampingFactor, BLUE);
					aa.update();
					springList.push_back(aa);
				}
			}
			else if (x != (rows - 1) && z != (rows - 1))
			{

				//diagonal shear spring
				cSpring aa = cSpring(getParticle(x + 1, z + 1), getParticle(x, z), shearConstant, (naturalLength * sqrt(2.0)), dampingFactor, GREEN);
				aa.update();
				springList.push_back(aa);
				//vertical structural spring
				aa = cSpring(getParticle(x, z + 1), getParticle(x, z), stretchConstant, naturalLength, dampingFactor, BLUE);
				aa.update();
				springList.push_back(aa);
				//horizontal structural spring
				aa = cSpring(getParticle(x + 1, z), getParticle(x, z), stretchConstant, naturalLength, dampingFactor, BLUE);
				aa.update();
				springList.push_back(aa);
			}
			if (x > 0 && z < (rows - 1))
			{
			//reverse diagonal shear spring
			cSpring aa = cSpring(getParticle(x - 1, z + 1), getParticle(x, z), shearConstant, (naturalLength * sqrt(2.0)), dampingFactor, GREEN);
			aa.update();
			springList.push_back(aa);
			}

			//Bending springs
				if (z + 2 < rows)
				{
					//vertical bending spring
					cSpring aa = cSpring(getParticle(x, z + 2), getParticle(x, z), bendingConstant, naturalLength * 2, dampingFactor, RED);
					aa.update();
					springList.push_back(aa);
				}

				if (x + 2 < rows)
				{
					//horizontal bending spring
					cSpring	aa = cSpring(getParticle(x + 2, z), getParticle(x, z), bendingConstant, naturalLength * 2.0, dampingFactor, RED);
					aa.update();
					springList.push_back(aa);
				}

				if (x + 2 < rows && z + 2 < rows)
				{
					//diagonal bending spring
					cSpring aa = cSpring(getParticle(x + 2, z + 2), getParticle(x, z), diagonalBendingConstant, (naturalLength * sqrt(2.0)) * 2, dampingFactor, RED);
					aa.update();
					springList.push_back(aa);
				}

				if (x - 2 > 0 && z + 2 < rows)
				{
					//diagonal bending spring
					cSpring aa = cSpring(getParticle(x - 1, z + 1), getParticle(x, z), diagonalBendingConstant, (naturalLength * sqrt(2.0)) * 3, dampingFactor, RED);
					aa.update();
					springList.push_back(aa);
				}
		}
	}
}


void generateWind(const vec3 direction)
{
	for (auto &e : ClothParticles) {

		float random = ((float)rand() / RAND_MAX) * 4.0f;
		auto p = static_cast<cPhysics *>(e->GetComponents("Physics")[0]);
		vec3 particleN = normalize(p->position);
		vec3 windForce = direction  * random;
		p->AddImpulse(windForce);
	}
}

//for testing
void fixTopRow()
{
	int z = rows - 1;
	int x = 0;
	while (x < rows)
	{
		getParticle(x, z)->position = getParticle(x, z)->prev_position;
		getParticle(x, z)->fixed = true;
		x++;
	}
}

//for testing
void fixBottomRow()
{
	int z = 0;
	int x = 0;
	while (x < rows)
	{
		getParticle(x, z)->position = getParticle(x, z)->prev_position;
		getParticle(x, z)->fixed = true;
		x++;
	}
}

void fixCorners()
{
	getParticle(0, 0)->position = getParticle(0, 0)->prev_position;
	getParticle(0, 0)->fixed = true;

	getParticle(0, rows - 1)->position = getParticle(0, rows - 1)->prev_position;
	getParticle(0, rows - 1)->fixed = true;

	getParticle(rows - 1, 0)->position = getParticle(rows - 1, 0)->prev_position;
	getParticle(rows - 1, 0)->fixed = true;

	getParticle(rows - 1, rows - 1)->position = getParticle(rows - 1, rows - 1)->prev_position;
	getParticle(rows - 1, rows - 1)->fixed = true;
}

//FPS Counter from http://r3dux.org/  --> I'm keeping original comments of the creator
double calcFPS(double theTimeInterval = 1.0, std::string theWindowTitle = "NONE")
{
	// Static values which only get initialised the first time the function runs
	static double t0Value = glfwGetTime();		// Set the initial time to now
	static double fpsFrameCount = 0.0;            // Set the initial FPS frame count to 0
	static double fps = 0.0;					// Set the initial FPS value to 0.0
									   
	double currentTime = glfwGetTime();		// Get the current time in seconds since the program started (non-static, so executed every time)

	// Calculate and display the FPS every specified time interval
	if ((currentTime - t0Value) > theTimeInterval)
	{
		// Calculate the FPS as the number of frames divided by the interval in seconds
		fps = fpsFrameCount / (currentTime - t0Value);

		// If the user specified a window title to append the FPS value to...
		if (theWindowTitle != "NONE")
		{
			// Convert the fps value into a string using an output stringstream
			std::ostringstream stream;
			stream << fps;
			std::string fpsString = stream.str();

			// Append the FPS value to the window title details
			theWindowTitle += " | FPS: " + fpsString;

			// Convert the new window title to a c_str and set it
			const char* pszConstString = theWindowTitle.c_str();
			glfwSetWindowTitle(renderer::get_window(), pszConstString);
		}
		// Reset the FPS frame counter and set the initial time to be now
		fpsFrameCount = 0;
		t0Value = glfwGetTime();
	}
	else // FPS calculation time interval hasn't elapsed yet? Simply increment the FPS frame counter
	{
		fpsFrameCount++;
	}

	// Return the current FPS - doesn't have to be used if you don't want it!
	return fps;
}

void addMass()
{
	cPhysics *p;

	for (auto &e : ClothParticles) {
		p = static_cast<cPhysics *>(e->GetComponents("Physics")[0]);
		if (p->mass < 40.0)
			p->mass += 0.05;
	}

	p = static_cast<cPhysics *>(ClothParticles[15]->GetComponents("Physics")[0]);
	cout << "Cloth mass is: " << p->mass;
}

void removeMass()
{
	cPhysics *p;

	for (auto &e : ClothParticles) {
		p = static_cast<cPhysics *>(e->GetComponents("Physics")[0]);
		if (p->mass > 1.0)
			p->mass -= 0.05;
	}

	p = static_cast<cPhysics *>(ClothParticles[15]->GetComponents("Physics")[0]);
	cout << "Cloth mass is: " << p->mass;
}

void increaseGravity()
{
	cPhysics *p;

	for (auto &e : ClothParticles) {
		p = static_cast<cPhysics *>(e->GetComponents("Physics")[0]);
		if (p->gravity.y > -60.0)
			p->gravity.y -= 0.1;
	}

	p = static_cast<cPhysics *>(ClothParticles[15]->GetComponents("Physics")[0]);
	cout << "Gravity is: " << p->gravity.y;
}

void decreaseGravity()
{
	cPhysics *p;

	for (auto &e : ClothParticles) {
		p = static_cast<cPhysics *>(e->GetComponents("Physics")[0]);
		if (p->gravity.y < -1.0)
			p->gravity.y += 0.1;
	}

	p = static_cast<cPhysics *>(ClothParticles[15]->GetComponents("Physics")[0]);
	cout << "Gravity is: " << p->gravity.y;
}

double getAverageStiffness()
{
	double average = 0.0;
	average += stretchConstant;
	average += shearConstant;
	average += bendingConstant;
	average += diagonalBendingConstant;
	average /= 4.0;

	return average;
}

void increaseStiffness()
{
	float average = getAverageStiffness();
	if (stretchConstant < 100.0f)
	{
		stretchConstant += 1.0f;
		shearConstant += 1.0f;
		bendingConstant += 1.0f;
		diagonalBendingConstant += 1.0f;
		dampingFactor += 1.0f;
	}
	else if (stretchConstant = 100.0f)
	{
		stretchConstant = 100.0f;
		shearConstant = 90.0f;
		bendingConstant = 80.0f;
		diagonalBendingConstant = 20.0f;
		dampingFactor = 100.0f;
	}
}

void decreaseStiffness()
{
	if (diagonalBendingConstant >= 1.0f)
	{
		diagonalBendingConstant -= 1.0f;
	}
	if (stretchConstant >= 20.0f)
	{
		stretchConstant -= 1.0f;
		dampingFactor -= 1.0f;
	}
	if (stretchConstant >= 20.0f)
	{
		shearConstant -= 1.0f;
	}
	if (bendingConstant >= 20.0f)
	{
		bendingConstant -= 1.0f;
	}
	
	
	
}

void setTitle()
{
	auto p = static_cast<cPhysics *>(ClothParticles[15]->GetComponents("Physics")[0]);
	stringstream ss;
	string wind = "";
	if (isWindActive)
	{
		wind = "Yes";
	}
	else
	{
		wind = "No";
	}
	ss << "Physics Simulation Cloth ---> (M) Cloth mass is now: " << p->mass << " | (G) Gravity is: " << p->gravity.y << " | (S) Average Stiffnes is: " 
		<< getAverageStiffness() << " | (Z-X) Wind activated: " << wind << " | (C) Wind force: " 
		<< windDir.y << " | (C) Wind direction: " << windDir.x;
	string s = ss.str();
	calcFPS(1.0, s);        // Update the window title to include the FPS details once per second
							//glfwSetWindowTitle(renderer::get_window(), s); to set in case of low dps

}

bool update(float delta_time) {

	static float t = 0.0;
	static float accumulator = 0.0;
	accumulator += delta_time;

	while (accumulator > physics_tick) {
		UpdatePhysics(t, physics_tick);
		accumulator -= physics_tick;
		t += physics_tick;
	}

	for (auto &e : ClothParticles) {
		e->Update(delta_time);
	}

	if (glfwGetKey(renderer::get_window(), GLFW_KEY_Z))
	{
			isWindActive = true;
			windDir = vec3(0.0f, ((float)rand()) / ((float)RAND_MAX) + 5.0f, 0.0f);
	}
	else if(glfwGetKey(renderer::get_window(), GLFW_KEY_X))
	{
		isWindActive = false;
		windDir.y = 0.0f;
		windDir.x = 0.0f;
	}

	if (isWindActive == true && glfwGetKey(renderer::get_window(), GLFW_KEY_Z) == GLFW_RELEASE)
	{
		generateWind(windDir);
	}

	updateCloth();
	fixCorners();
	

	if (glfwGetKey(renderer::get_window(), GLFW_KEY_SPACE))
	{
		if (isRendered == false)
		{
			isRendered = true;
		}
		else
		{
			isRendered = false;
		}
	}

	if (glfwGetKey(renderer::get_window(), GLFW_KEY_F))
	{
		isCam = true;
	}

	//if freecam is active 
	if (isCam == true)
	{
		//Mouse cursor is disabled when freecam is active
		glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		//Get mouse position
		double deltax, deltay;
		glfwGetCursorPos(renderer::get_window(), &deltax, &deltay);
		double tempx = deltax;
		double tempy = deltay;
		deltax -= xpos;
		deltay -= ypos;
		double ratio_width = (double)renderer::get_screen_width() / (double)renderer::get_screen_height();
		double ratio_height = 1.0 / ratio_width;
		deltax *= ratio_width * delta_time / 10;
		deltay *= -ratio_height * delta_time / 10;

		//Rotate freecam based on the mouse coordinates
		free_cam.rotate(deltax, deltay);

		//Cursor position
		xpos = tempx;
		ypos = tempy;

		//Movement with buttons
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_W))
		{
			free_cam.move(vec3(0.0, 0.0, 20.0) * delta_time);
		}
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_S))
		{
			free_cam.move(vec3(0.0, 0.0, -20.0) * delta_time);
		}
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_A))
		{
			free_cam.move(vec3(-20.0, 0.0, 0.0) * delta_time);
		}
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_D))
		{
			free_cam.move(vec3(20.0, 0.0, 0.0) * delta_time);
		}
		
		free_cam.update(delta_time);
	}
	else 
	{
		glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	isCam = false;

	if (glfwGetKey(renderer::get_window(), GLFW_KEY_C))
	{
		if (isWindActive)
		{
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP))
			{
				if (windDir.y <= 40.0f)
				{
					windDir.y += 0.1f;
				}
			}

			if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN))
			{
				if (windDir.y >= 1.2f)
				{
					windDir.y -= 0.1f;
				}
			}

			if (glfwGetKey(renderer::get_window(), GLFW_KEY_LEFT))
			{
				if (windDir.x <= 20.0f)
				{
					windDir.x += 0.01f;
				}
			}

			if (glfwGetKey(renderer::get_window(), GLFW_KEY_RIGHT))
			{
				if (windDir.x >= -20.0)
				{
					windDir.x -= 0.01f;
				}
			}
		}
	}
	

	if (glfwGetKey(renderer::get_window(), GLFW_KEY_M))
	{
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP))
		{
			addMass();
		}

		if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN))
		{
			removeMass();
		}
	}
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_G))
	{
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP))
		{
			increaseGravity();
		}

		if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN))
		{
			decreaseGravity();
		}
	}

	if (glfwGetKey(renderer::get_window(), GLFW_KEY_S))
	{
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP))
		{
			increaseStiffness();
		}

		if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN))
		{
			decreaseStiffness();
		}
	}

	setTitle();

	phys::SetCameraTarget(free_cam.get_target());
	phys::SetCameraPos(free_cam.get_position());

	phys::Update(delta_time);
	return true;
}

bool load_content() {

	phys::Init();
	Cloth();
	floorEnt = unique_ptr<Entity>(new Entity());

	floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));

	free_cam.set_target(vec3(1.0, 10.0, 8.0));
	free_cam.set_position(vec3(3.0f, 15.0f, -30.0f));

	phys::SetCameraPos(vec3(10.0f, 20.0f, -30.0f));
	phys::SetCameraTarget(vec3(0.0f, 15.0f, 0));

	InitPhysics();

	return true;
}

bool render() {

	if (isRendered)
	{
		for (auto &e : ClothParticles) {
			e->Render();
		}
	}

	phys::DrawScene();

	grid.clear();

	for (auto &e : ClothParticles) {
		grid.push_back(e->GetPosition());
	}

	phys::DrawGrid(&grid[0], rows*rows, rows, phys::wireframe);

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

