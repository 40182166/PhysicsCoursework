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

//defining physics_tick
#define physics_tick 1.0f / 60.0f

//Camera variables
double xpos = 0.0f;
double ypos = 0.0f;
free_camera free_cam;
//boolean to determine if free camera is active
bool isCam = false;

//Vectors containing particles and springs of the cloth
static vector<unique_ptr<Entity>> ClothParticles;
static vector<cSpring> springList;

//floor entity
static unique_ptr<Entity> floorEnt;

//cloth is composed of 15x15 particles
int rows = 15;
//Springs and damper constants 
float stretchConstant = 95.0f;
float shearConstant = 90.0f;
float bendingConstant = 80.0f;
float diagonalBendingConstant = 20.0f;
float dampingFactor = 90.0f;
//natural length of the cloth set to distance between particles
float naturalLength = 0.3f;

//default wind direction is 0
vec3 windDir = vec3(0.0f, 0.0f, 0.0f);
//boolean to determine if wind is active
bool isWindActive = false;

//boolean to determine if the particles are rendered - for graphical purposes only
bool isRendered = false;
//vector containing positions to create cloth grid
vector<glm::vec3> grid;


//method to create the cloth particles at a given position and with a given mass 
unique_ptr<Entity> CreateParticle(float xPos, float yPos, float zPos, double myMass) {
	//creates new entity
	unique_ptr<Entity> ent(new Entity());
	//set position at passed values to the method
	ent->SetPosition(vec3(xPos, yPos, zPos));
	//creating new cPhysics, that will contain all the particle physics
	cPhysics *phys = new cPhysics();
	//set mass to given mass
	phys->mass = myMass;
	//creating new component using cPhysics created before
	unique_ptr<Component> physComponent(phys);
	//rendering the particles as spheres
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	//choosing a random colour for particle
	renderComponent->SetColour(phys::RandomColour());
	//adding the component created to the entity
	ent->AddComponent(physComponent);
	//creating a new sphere collider for the particle
	cSphereCollider *coll = new cSphereCollider();
	//setting collider radius to 0.3
	coll->radius = 0.03f;
	//adding remaining components to the entity
	ent->AddComponent(unique_ptr<Component>(coll));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	//returning entity
	return ent;
}

//Method to get specific particle in the list of Cloth Particles, based on its cartesian coordinates 
cPhysics *getParticle(int x, int z)
{
	//casting particles from Entity to cPhysics (to get particles attributes) and converting its coordinates into correct index in the list like:
	//(rows = 16) if coordinates of the particle are (1 , 3), its index will be : 1 * 15 + 3 = 18. 
	auto p = static_cast<cPhysics *>(ClothParticles[x * rows + z]->GetComponents("Physics")[0]);
	return p;
}

//Method to get specific spring in the list of Cloth springs, based on its cartesian coordinates
cSpring getSpring(int x, int z)
{
	//converts spring's coordinate into its index inside the list of springs
	auto spring = springList[x * rows + z];
	return spring;
}

//Method to generate the cloth, creating particles and putting them in a grid layout nad inserting them into the list of particles
void Cloth()
{
	//looping through x axis
	for (int x = 0; x < rows; x++)
	{
		//looping through z axys
		for (int z = 0; z < rows; z++)
		{
			//creating particle based on x and z value (using a distance of 0.3)
			unique_ptr<Entity> particle = CreateParticle(x*0.3f, 10.0f, (z - 10.0f)*0.3f, 1.0f);
			//pushing it into the vector containing all particles
			ClothParticles.push_back(move(particle));
		}
	}
}

//method to update the cloth, creating springs between particles
void updateCloth()
{
	//clearing the list of spring every time update is called
	springList.clear();

	//looping through x and z axis
	for (int x = 0; x < rows; x++)
	{
		for (int z = 0; z < rows; z++)
		{
			//*********Structural (horizontal and vertical) and Shear (diagonal) springs*********//

			//if the particle is at a margin of the z axis
			if (z == (rows - 1))
			{
				//if x + 1 is different from the maximum number allowed (rows number)
				if (x + 1 != rows)
				{
					//create only horizontal structural spring
					//get the current particle and link it to the next one, using appropriate spring constants and setting colour
					cSpring aa = cSpring(getParticle(x + 1, z), getParticle(x, z), stretchConstant, naturalLength, dampingFactor, BLUE);
					//updating the spring
					aa.update();
					//
					springList.push_back(aa);
				}
			}
			//if the particle is at a margin of the x axis
			else if (x == (rows - 1))
			{
				//if z + 1 is different from the maximum number allowed (rows number)
				if (z + 1 != rows)
				{
					//vertical structural spring
					//get the current particle and link it to the next one, using appropriate spring constants and setting colour
					cSpring aa = cSpring(getParticle(x, z + 1), getParticle(x, z), stretchConstant, naturalLength, dampingFactor, BLUE);
					aa.update();
					springList.push_back(aa);
				}
			}
			//if z + 1 and x + 1 are different from the maximum number allowed (rows number)
			else if (x != (rows - 1) && z != (rows - 1))
			{
				//then every type of spring can be created..

				//creating diagonal shear spring
				//get the current particle and link it to the next one. Since it is diagonal, to keep the same distance, the natural length of the spring
				//is set to the diagonal of the square created by the 4 particles where the current particle is
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
			//if x is more than 0 and z is less than the last row
			if (x > 0 && z < (rows - 1))
			{
			//create reverse diagonal shear spring
			cSpring aa = cSpring(getParticle(x - 1, z + 1), getParticle(x, z), shearConstant, (naturalLength * sqrt(2.0)), dampingFactor, GREEN);
			aa.update();
			springList.push_back(aa);
			}

			//Bending springs (vertical, horizontal and diagonal springs every two particle)

			//if z + 2 doesn't go outside the cloth
				if (z + 2 < rows)
				{
					//vertical bending spring
					cSpring aa = cSpring(getParticle(x, z + 2), getParticle(x, z), bendingConstant, naturalLength * 2, dampingFactor, RED);
					aa.update();
					springList.push_back(aa);
				}
				//if x + 2 doesn't go outside the cloth
				if (x + 2 < rows)
				{
					//horizontal bending spring
					cSpring	aa = cSpring(getParticle(x + 2, z), getParticle(x, z), bendingConstant, naturalLength * 2.0, dampingFactor, RED);
					aa.update();
					springList.push_back(aa);
				}
				//check if both x + 2 and z + 2 don't go outside the cloth
				if (x + 2 < rows && z + 2 < rows)
				{
					//diagonal bending spring, using diagonal of the square of paricles, multiplied by 2 since bending spring is created every 2 particle
					cSpring aa = cSpring(getParticle(x + 2, z + 2), getParticle(x, z), diagonalBendingConstant, (naturalLength * sqrt(2.0)) * 2, dampingFactor, RED);
					aa.update();
					springList.push_back(aa);
				}
				//check if both x - 2 and z + 2 don't go outside the cloth
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

//Method to generate the wind in a given direction
void generateWind(const vec3 direction)
{
	//calculate wind for every particle
	for (auto &e : ClothParticles) {
		//add a randomness to every particle to make wind more realistic (multiplied by 4)
		float random = ((float)rand() / RAND_MAX) * 4.0f;
		//casting particles from Entity to cPhysics (to get particles attributes)
		auto p = static_cast<cPhysics *>(e->GetComponents("Physics")[0]);
		//windforce is the wind direction by the random factor
		vec3 windForce = direction  * random;
		//adding wind to particle as a force
		p->AddImpulse(windForce);
	}
}

//for testing - Fixes top row of the cloth, setting those particles position to their previous (starting) position and make them fixed, setting the bool to true
void fixTopRow()
{
	//Starting from (0, 4) position
	int z = rows - 1;
	int x = 0;
	//loops the particle position until the row is finished
	while (x < rows)
	{
		//set particle position to prev position
		getParticle(x, z)->position = getParticle(x, z)->prev_position;
		//set bool to true, so the particles are not rendered anymore
		getParticle(x, z)->fixed = true;
		//increment x axis
		x++;
	}
}

//for testing - Fixes bottom row of the cloth, setting those particles position to their previous (starting) position and make them fixed, setting the bool to true
void fixBottomRow()
{
	//Starting from (0, 0) position
	int z = 0;
	int x = 0;
	//loops the particle position until the row is finished
	while (x < rows)
	{
		//set particle position to prev position
		getParticle(x, z)->position = getParticle(x, z)->prev_position;
		//set bool to true, so the particles are not rendered anymore
		getParticle(x, z)->fixed = true;
		//increment x axis
		x++;
	}
}

//Method to fix the corners of the cloth - works like fixBottomRow and fixTopRow
void fixCorners()
{
	//get the particle at (0, 0) coordinates and set it to previous position, to make it fixed and to not render itanymore
	getParticle(0, 0)->position = getParticle(0, 0)->prev_position;
	getParticle(0, 0)->fixed = true;

	getParticle(0, rows - 1)->position = getParticle(0, rows - 1)->prev_position;
	getParticle(0, rows - 1)->fixed = true;

	getParticle(rows - 1, 0)->position = getParticle(rows - 1, 0)->prev_position;
	getParticle(rows - 1, 0)->fixed = true;

	getParticle(rows - 1, rows - 1)->position = getParticle(rows - 1, rows - 1)->prev_position;
	getParticle(rows - 1, rows - 1)->fixed = true;
}

//FPS Counter in the top bar of the window, using GLFM | from http://r3dux.org/  --> I'm keeping almost all original comments of the creator
double calcFPS(double theTimeInterval = 1.0, std::string windowTitle = "NONE")
{
	// Static values which only get initialised the first time the function runs
	static double t0Value = glfwGetTime();		// Set the initial time to now
	static double fpsFrameCount = 0.0;            // Set the initial FPS frame count to 0
	static double fps = 0.0;					// Set the initial FPS value to 0.0
									   
	double currentTime = glfwGetTime();		// Get the current time in seconds when the program starts

	// Calculate and display the FPS every specified time interval
	if ((currentTime - t0Value) > theTimeInterval)
	{
		// Calculate the FPS as the number of frames divided by the interval in seconds
		fps = fpsFrameCount / (currentTime - t0Value);

		// If the user specified a window title to append the FPS value to it
		if (windowTitle != "NONE")
		{
			// Convert the fps value into a string using an output stringstream
			std::ostringstream stream;
			stream << fps;
			std::string fpsString = stream.str();

			// Append the FPS value to the window title details
			windowTitle += " | FPS: " + fpsString;

			// Convert the new window title to a c_str and set it
			const char* title = windowTitle.c_str();
			glfwSetWindowTitle(renderer::get_window(), title);
		}
		// Reset the FPS frame counter and set the initial time to be now
		fpsFrameCount = 0;
		t0Value = glfwGetTime();
	}
	else // Increment the FPS frame counter if FPS calculation time interval hasn't elapsed yet
	{
		fpsFrameCount++;
	}

	// Return the current FPS
	return fps;
}

//Method to add mass to every particle in the cloth
void addMass()
{
	//Particle
	cPhysics *p;

	//for every Entity in ClothParticles
	for (auto &e : ClothParticles) {
		//casting particles from Entity to cPhysics (getting particles attributes)
		p = static_cast<cPhysics *>(e->GetComponents("Physics")[0]);
		//checking if mass doesn't go over safe value (30)
		if (p->mass < 30.0)
			//increase mass
			p->mass += 0.05;
	}

	//for testing only - can be commented out
	p = static_cast<cPhysics *>(ClothParticles[15]->GetComponents("Physics")[0]); //
	cout << "Cloth mass is: " << p->mass;										  //
}

//Method to remove mass to every particle in the cloth
void removeMass()
{
	//Particle
	cPhysics *p;

	//for every Entity in ClothParticles
	for (auto &e : ClothParticles) {
		//casting particles from Entity to cPhysics (getting particles attributes)
		p = static_cast<cPhysics *>(e->GetComponents("Physics")[0]);
		//checking if mass doesn't go ower than safe value (1)
		if (p->mass > 1.0)
			//decrease mass
			p->mass -= 0.05;
	}

	//for testing only - can be commented out
	p = static_cast<cPhysics *>(ClothParticles[15]->GetComponents("Physics")[0]); //
	cout << "Cloth mass is: " << p->mass;
}

//Method to increase gravity
void increaseGravity()
{
	//Particle
	cPhysics *p;

	//for every Entity in ClothParticles
	for (auto &e : ClothParticles) {
		//casting particles from Entity to cPhysics (getting particles attributes)
		p = static_cast<cPhysics *>(e->GetComponents("Physics")[0]);
		//checking if mass doesn't go over safe value (-40)
		if (p->gravity.y > -40.0)
			//increase gravity (in a negative direction)
			p->gravity.y -= 0.1;
	}

	//for testing only - can be commented out
	p = static_cast<cPhysics *>(ClothParticles[15]->GetComponents("Physics")[0]);
	cout << "Gravity is: " << p->gravity.y;
}

//Method to decrease gravity
void decreaseGravity()
{
	//Particle
	cPhysics *p;

	//for every Entity in ClothParticles
	for (auto &e : ClothParticles) {
		//casting particles from Entity to cPhysics (getting particles attributes)
		p = static_cast<cPhysics *>(e->GetComponents("Physics")[0]);
		//checking if mass doesn't go lower than safe value (-1)
		if (p->gravity.y < -1.0)
			//decrease gravity (in a positive direction)
			p->gravity.y += 0.1;
	}
	//for testing only - can be commented out
	p = static_cast<cPhysics *>(ClothParticles[15]->GetComponents("Physics")[0]);
	cout << "Gravity is: " << p->gravity.y;
}

//Get stiffness average of the cloth, to show the value to the user
double getAverageStiffness()
{
	//initializing average
	double average = 0.0;
	//adding all spring constants to average
	average += stretchConstant;
	average += shearConstant;
	average += bendingConstant;
	average += diagonalBendingConstant;
	//dividing the average by 4 (number of spring constants defined)
	average /= 4.0;

	//return average
	return average;
}

//Method to increase stiffness of the cloth, modifing springs constants
void increaseStiffness()
{
	//increment constant of 0.3 only if the highest constant is less than 95.0 (safe value tested)
	if (stretchConstant < 95.0f)
	{
		//incrementing constants and damping factor
		stretchConstant += 0.3f;
		shearConstant += 0.3f;
		bendingConstant += 0.3f;
		diagonalBendingConstant += 0.3f;
		dampingFactor += 0.3f;
	}
	else if (stretchConstant < 97.0 && stretchConstant > 93.0f)
	{
		stretchConstant = 95.0f;
		shearConstant = 90.0f;
		bendingConstant = 80.0f;
		diagonalBendingConstant = 20.0f;
		dampingFactor = 90.0f;
	}
}

//Method to decrease stiffness of the cloth, modifing springs constants
void decreaseStiffness()
{
	//checking that every spring constand and damper doesn't go lower than  agiven safe value

	if (diagonalBendingConstant > 1.0f)
	{
		//decreasing the spring constant if not lower than 1
		diagonalBendingConstant -= 0.3f;
	}
	if (stretchConstant > 20.0f)
	{
		//decreasing the spring constant if not lower than 20
		stretchConstant -= 0.3f;
		//damping factos should ideally be really similar to stretch constrant
		dampingFactor -= 0.3f;
	}
	if (stretchConstant > 20.0f)
	{
		shearConstant -= 0.3f;
	}
	if (bendingConstant > 20.0f)
	{
		bendingConstant -= 0.3f;
	}
}

//Method to set the title of the window and updating information about the simulation
void setTitle()
{
	auto p = static_cast<cPhysics *>(ClothParticles[15]->GetComponents("Physics")[0]);
	//string to be concatenated
	stringstream ss;
	string wind = "";
	//if wind is active, set string as yes, otherwise is no
	if (isWindActive)
	{
		wind = "Yes";
	}
	else
	{
		wind = "No";
	}
	//concatenation info for the title, updating in real time
	ss << "Physics Simulation Cloth ---> (M) Cloth mass is now: " << p->mass << " | (G) Gravity is: " << p->gravity.y << " | (S) Average Stiffnes is: " 
		<< getAverageStiffness() << " | (Z-X) Wind activated: " << wind << " | (C) Wind force: " 
		<< windDir.y << " | (C) Wind direction: " << windDir.x;
	//casting the stringstream to string
	string s = ss.str();
	calcFPS(1.0, s);        //updates window title with fps and other values

}

//update method
bool update(float delta_time) {
	static float t = 0.0;
	static float accumulator = 0.0;
	accumulator += delta_time;

	//definind time for executing all the physics calculations
	while (accumulator > physics_tick) {
		UpdatePhysics(t, physics_tick);
		accumulator -= physics_tick;
		t += physics_tick;
	}

	//update every particle in the cloth
	for (auto &e : ClothParticles) {
		e->Update(delta_time);
	}

	//calling the method to fix the corners of the cloth
	fixCorners();
	//calling method to update the springs of the cloth
	updateCloth();
	
	//***********************************************Camera Controls***********************************************//

	//if button for free camera is pressed, set boolean to true
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
		
		//update camera
		free_cam.update(delta_time);
	}
	//if button is not pressed, set cursor to normal (visible)
	else 
	{
		glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	//***********************************************Interactions Controls***********************************************//

	//****WIND****//

	//if wind is active, keep the flow of window active
	if (isWindActive == true && glfwGetKey(renderer::get_window(), GLFW_KEY_Z) == GLFW_RELEASE)
	{
		//calling generate wind method 
		generateWind(windDir);
	}

	//button to activate wind
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_Z))
	{
		//if button to activate is pressed, set boolean to true
		isWindActive = true;
		//setting the wind direction to a default value, taking also a random value to make it more realistic
		windDir = vec3(0.0f, ((float)rand()) / ((float)RAND_MAX) + 5.0f, 0.0f);
	}
	//if the button to stop wind is pressed
	else if (glfwGetKey(renderer::get_window(), GLFW_KEY_X))
	{
		//wind is not active
		isWindActive = false;
		//wind direction is set to 0
		windDir.y = 0.0f;
		windDir.x = 0.0f;
	}

	//button to increase-decrease strength (y axis) and direction (x axis)
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_C))
	{
		//if the wind is active
		if (isWindActive)
		{
			//if UP is pressed while C is down
			if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP))
			{
				//increasing wind y direction if in safe range
				if (windDir.y <= 35.0f)
				{
					windDir.y += 0.1f;
				}
			}
			//if DOWN is pressed while C is down
			else if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN))
			{
				//decreasing wind y direction if in safe range
				if (windDir.y >= 1.2f)
				{
					windDir.y -= 0.1f;
				}
			}
			//if LEFT is pressed while C is down
			else if (glfwGetKey(renderer::get_window(), GLFW_KEY_LEFT))
			{
				//increasing wind x direction if in safe range
				if (windDir.x <= 20.0f)
				{
					windDir.x += 0.01f;
				}
			}
			//if DOWN is pressed while C is down
			else if (glfwGetKey(renderer::get_window(), GLFW_KEY_RIGHT))
			{
				//decreasing wind x direction if in safe range
				if (windDir.x >= -20.0)
				{
					windDir.x -= 0.01f;
				}
			}
		}
	}

	//****STRUCTURE****//

	//button to render the particles to show cloth structure
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_SPACE))
	{
		//switching between the structure mode and normal mode
		//if the boolean is false, set it to true
		if (isRendered == false)
		{
			isRendered = true;
		}
		//if boolean is true, set it to false
		else
		{
			isRendered = false;
		}
	}

	//****MASS****//

	//if M is pressed
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_M))
	{
		//if UP is pressed while M is down
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP))
		{
			//calling method to add mass
			addMass();
		}
		//if DOWN is pressed while M is down
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN))
		{
			//calling method to remove mass
			removeMass();
		}
	}

	//****GRAVITY****//

	//if G is pressed
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_G))
	{
		//if UP is pressed while G is down
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP))
		{
			//calling method to increase gravity
			increaseGravity();
		}
		//if DOWN is pressed while G is down
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN))
		{
			//calling method to decrease gravity
			decreaseGravity();
		}
	}

	//****STIFFNESS****//

	//if S is pressed
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_S))
	{
		//if UP is pressed while S is down
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP))
		{
			//calling method to increase the stiffness
			increaseStiffness();
		}
		//if DOWN is pressed while S is down
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN))
		{
			//calling method to decrease the stiffness
			decreaseStiffness();
		}
	}

	//calling set title to update values
	setTitle();

	//set camera to false when F is not pressed
	isCam = false;

	//setting default camera to 
	phys::SetCameraTarget(free_cam.get_target());
	phys::SetCameraPos(free_cam.get_position());

	//updating physics
	phys::Update(delta_time);
	return true;
}

//load content method to load all entities
bool load_content() {
	phys::Init();

	//calling method to create cloth particles
	Cloth();

	//creating new entity plane
	floorEnt = unique_ptr<Entity>(new Entity());
	//adding planeCollider to plane
	floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));

	//setting cameras' position and target
	free_cam.set_target(vec3(1.0, 10.0, 8.0));
	free_cam.set_position(vec3(3.0f, 15.0f, -30.0f));

	phys::SetCameraPos(vec3(10.0f, 20.0f, -30.0f));
	phys::SetCameraTarget(vec3(0.0f, 15.0f, 0));

	InitPhysics();

	return true;
}

bool render() {

	//if space bar is pressed, boolean is true and then renders the particles
	if (isRendered)
	{
		for (auto &e : ClothParticles) {
			e->Render();
		}
	}

	//drawing the scene
	phys::DrawScene();

	//clearing the grid positions to update it in real time
	grid.clear();

	//setting grid position as cloth particles positions
	for (auto &e : ClothParticles) {
		grid.push_back(e->GetPosition());
	}

	//drawing the grid as a wireframe
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

