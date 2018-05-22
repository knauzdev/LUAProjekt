#pragma comment(lib, "Irrlicht.lib")
#ifdef _DEBUG
#pragma comment(lib, "LuaLibd.lib")
#else
#pragma comment(lib, "Lualib.lib")
#endif

#include <stdio.h>

#include <lua.hpp>
#include <Windows.h>
#include <iostream>
#include <thread>
#include "lua.hpp"
#include <irrlicht.h>
#include <vector>
#include <string>
#include <sstream>

using namespace irr::video;
using namespace irr::scene;

irr::IrrlichtDevice* device;

struct sceneNodeObject {
public:
	sceneNodeObject(irr::scene::ISceneNode * node, std::string name) {
		this->node = node;
		this->name = name;
	}
	irr::scene::ISceneNode * node;
	std::string name;
};

irr::scene::IAnimatedMeshSceneNode* node;
std::vector<sceneNodeObject> objects;
irr::scene::ISceneManager* smgr;
bool searchName(std::string name) {
	for (auto object : objects) {
		if (object.name == name) {
			return true;
		}
	}
	return false;
}
std::string makeName() {
	std::stringstream ss;
	ss << "object_" << objects.size();
	int count = 1;
	while (searchName(ss.str())) {
		ss.clear();
		ss << "object_" << objects.size() + count;
		count++;
	}
	return ss.str();
}
int _addBox(float x, float y, float z, float size, std::string name = "") {
	bool valid;
	std::string n_name;
	if (name == "") {
		valid = true;
		std::cout << "No name set, generating name...\n";
		n_name = makeName();
		std::cout << "New name: " << n_name << "\n";
	}
	else {
		valid = !searchName(name);
		n_name = name;
	}

	if (valid) {
		objects.push_back(sceneNodeObject(smgr->addCubeSceneNode(10, NULL, -1, irr::core::vector3df(x, y, z)), n_name));
		objects.at(objects.size() - 1).node->setMaterialFlag(irr::video::EMF_LIGHTING, false);
		std::cout << "New object created\n";
		return 1;
	}
	else {
		std::cout << "Name already in use!\n";
		return -1;
	}
}
static int addBox(lua_State* L) {
	//std::cout << lua_gettop(L) << "\n";
	//lua_settop(L, 1);
	//float x = std::atof(lua_tostring(L, -3));
	//std::cout << x << "\n";
	int args = lua_gettop(L);
	std::cout << args << "\n";
	if (args != 3 && args != 2) {
		luaL_loadstring(L, "print('Syntax error')");
		lua_pcall(L, 0, 0, 0);
		lua_pop(L, 1);
		return 0;
	}
	luaL_checktype(L, 1, LUA_TTABLE);
	//float * TC = lua_touserdata(L,1);
	//check table content
	float size = luaL_checknumber(L, 2);
	std::string name;
	if (args == 2) {
		name = makeName();
	}
	else {
		name = luaL_checkstring(L, 3);
	}
	std::cout << size << " : " << name << "\n";
	return 0;
}
//addBox({1,2,3},10,"Jared")
static int updatepos(lua_State* L) {
	if (lua_gettop(L) != 3) {
		luaL_loadstring(L, "print('Wrong number of parameters, use this format: updatepos(x,y,z)')");
		lua_pcall(L, 0, 0, 0);
		lua_pop(L, 1);
		return 0;
	}
	
	float x = std::atof(lua_tostring(L, -3));
	float y = std::atof(lua_tostring(L, -2));
	float z = std::atof(lua_tostring(L, -1));
	node->setPosition(irr::core::vector3df(x, y, z));
	return 0;
}

static int addMesh(lua_State* L) {
	luaL_argcheck(L, lua_istable(L, -1), 1, "<table> expected!");

	

	if (!lua_rawgeti(L, 1, 1)) {
		luaL_loadstring(L, "print('Error: not a valid number of vertices.')");
		lua_pcall(L, 0, 0, 0);
		lua_pop(L, 1);
		return 0;
	}

	int verti = -1;
	int subt = 2;
	while (lua_istable(L, verti)) {
		lua_rawgeti(L, 1, subt);
		subt++;
		//verti--;
	}

	//make sure all the 3 elements in the table are tables
	/*luaL_argcheck(L, lua_istable(L, -1), 1, "<table> expected in table!");
	luaL_argcheck(L, lua_istable(L, -2), 1, "<table> expected in table!");
	luaL_argcheck(L, lua_istable(L, -3), 1, "<table> expected in table!");*/
	std::vector<std::vector<float>> vertices;
	//float vertices[-verti][3];
	int index = 0; //sane index
	for (int i = 2; i < subt; i++) {
		//if too few coordinates are passed.
		if (!lua_rawgeti(L, i, 1) || !lua_rawgeti(L, i, 2) || !lua_rawgeti(L, i, 3)) {
			luaL_loadstring(L, "print('Error: number of components.')");
			lua_pcall(L, 0, 0, 0);
			lua_pop(L, 1);
			return 0;
		}
		//check to make sure all elements are numbers
		luaL_checknumber(L, -1); luaL_checknumber(L, -2); luaL_checknumber(L, -3);
		//get all coordinates
		float zi = lua_tonumber(L, -1);
		float yi = lua_tonumber(L, -2);
		float xi = lua_tonumber(L, -3);
		std::vector<float> vector;
		vector.push_back(xi); vector.push_back(yi); vector.push_back(zi);
		vertices.push_back(vector);
		//vertices[index][0] = xi; vertices[index][1] = yi; vertices[index][2] = zi;
		index++;
	}
	SMesh* mesh = new SMesh();

	SMeshBuffer *buf = 0;
	buf = new SMeshBuffer();
	mesh->addMeshBuffer(buf);
	buf->drop();

	buf->Vertices.reallocate(subt-2);
	buf->Vertices.set_used(subt-2);
	buf->Indices.reallocate(subt - 2);
	buf->Indices.set_used(subt - 2);

	for (int i = 0; i < vertices.size(); i++) {
		std::cout << "Table[" << i << "] coordinates: " << vertices[i][0] << " " << vertices[i][1] << " " << vertices[i][2] << "\n";
		buf->Vertices[i] = irr::video::S3DVertex(vertices[i][0], vertices[i][1], vertices[i][2], 0, 1, 0, irr::video::SColor(255, 0, 255, 255), 0, 1);
		buf->Indices[i] = i;
	}

	buf->Indices.reallocate(subt-2);
	buf->Indices.set_used(subt-2);

	
	/*buf->Indices[1] = 1;
	buf->Indices[2] = 2;*/

	buf->recalculateBoundingBox();


	IMeshSceneNode* myNode = device->getSceneManager()->addMeshSceneNode(mesh);

	myNode->setMaterialFlag(irr::video::EMF_BACK_FACE_CULLING, false);
	myNode->setMaterialFlag(irr::video::EMF_LIGHTING, false);
	myNode->setMaterialFlag(irr::video::EMF_NORMALIZE_NORMALS, false);
	
	return 0;
}

static int getpos(lua_State* L) {
	irr::core::vector3df pos = node->getPosition();
	lua_newtable(L);
	lua_pushnumber(L, pos.X);
	lua_rawseti(L, -2, 1);
	lua_pushnumber(L, pos.Y);
	lua_rawseti(L, -2, 2);
	lua_pushnumber(L, pos.Z);
	lua_rawseti(L, -2, 3);
	return 1;
}

void registerLuaFunctions(lua_State* L) {
	lua_pushcfunction(L, updatepos);
	lua_setglobal(L, "updatepos");
	lua_pushcfunction(L, getpos);
	lua_setglobal(L, "getpos");
	lua_pushcfunction(L, addMesh);
	lua_setglobal(L, "addMesh");
}


void ConsoleThread(lua_State* L) {
	char command[1000];
	while(GetConsoleWindow()) {
		memset(command, 0, 1000);
		std::cin.getline(command, 1000);
		if (luaL_loadstring(L, command) || lua_pcall(L, 0, 0, 0)) {
			std::cout << lua_tostring(L, -1) << '\n';
			lua_pop(L, 1);
		}
	}
}

int main()
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

		std::thread conThread(ConsoleThread, L);

	device = irr::createDevice(irr::video::EDT_SOFTWARE, irr::core::dimension2d<irr::u32>(640, 480), 16, false, false, true, 0);
	if(!device)
		return 1;

	device->setWindowCaption(L"Hello World! - Irrlicht Engine Demo");
	irr::video::IVideoDriver* driver	= device->getVideoDriver();
	smgr		= device->getSceneManager();
	irr::gui::IGUIEnvironment* guienv	= device->getGUIEnvironment();

	guienv->addStaticText(L"Hello World! This is the Irrlicht Software renderer!", irr::core::rect<irr::s32>(10, 10, 260, 22), true);

	irr::scene::IAnimatedMesh* mesh = smgr->getMesh("../Meshes/sydney.md2");
	if (!mesh)
	{
		device->drop();
		return 1;
	}
	node = smgr->addAnimatedMeshSceneNode(mesh);

	if (node)
	{
		node->setMaterialFlag(irr::video::EMF_LIGHTING, false);
		node->setMaterialFlag(irr::video::EMF_BACK_FACE_CULLING, false);
		node->setMD2Animation(irr::scene::EMAT_STAND);
		node->setMaterialTexture(0, driver->getTexture("../Meshes/sydney.bmp"));
	}

	node->setScale(irr::core::vector3df(0.5f, 0.5f, 0.5f));
	
	//smgr->addCameraSceneNode(0, irr::core::vector3df(0, 30, -40), irr::core::vector3df(0, 5, 0));
	_addBox(20, 20, 20, 10);
	_addBox(20, 20, 20, 10);
	_addBox(20, 20, 20, 10);
	_addBox(20, 20, 20, 10, "object_0");


	lua_register(L, "addBox", addBox);
	registerLuaFunctions(L);
	auto camera = smgr->addCameraSceneNodeFPS();
	while(device->run()) {
		bool active = device->isWindowActive();
		camera->setInputReceiverEnabled(active);
		camera->setInputReceiverEnabled(active);

		driver->beginScene(true, true, irr::video::SColor(255, 90, 101, 140));

		smgr->drawAll();
		guienv->drawAll();

		driver->endScene();
	}

	device->drop();

	conThread.join();
	return 0;
}