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
#include <vector2d.h>
#include <string>
#include <sstream>
#include <istream>
#include <fstream>

#include "GLF.hpp"
#include "rules.h"

using namespace irr::video;
using namespace irr::scene;

irr::IrrlichtDevice* device;

void render();

irr::scene::IAnimatedMeshSceneNode* node;
irr::scene::ISceneManager* smgr;
irr::scene::ICameraSceneNode* cameraNode;
irr::video::IVideoDriver* driver;

bool searchName(std::string name) {
	for (auto object : smgr->getRootSceneNode()->getChildren()) {
		if (object->getName() == name) {
			return true;
		}
	}
	return false;
}

std::string makeName() {
	std::stringstream ss;
	ss << "object_" << smgr->getRootSceneNode()->getChildren().size();
	int count = 1;
	while (searchName(ss.str())) {
		ss.clear();
		ss << "object_" << smgr->getRootSceneNode()->getChildren().size() + count;
		count++;
	}
	return ss.str();
}

static int bind(lua_State* L) {
	int args = lua_gettop(L);
	if (args != 2) {//arguments length check
		GLF::throwError(L, "ERROR: Wrong amount of arguments.");
		return 0;
	}

	std::string object = luaL_checkstring(L, 1);
	std::string texture = luaL_checkstring(L, 2);

	const irr::c8* object_p = object.c_str();

	if (!searchName(object)) {
		GLF::throwError(L, "Error: could not find node with that name!");
		return 0;
	}

	irr::io::path texture_p;
	texture_p.append(texture.c_str());

	irr::video::ITexture* tex = driver->getTexture(texture_p);

	if (tex == nullptr) {
		GLF::throwError(L, "Error: could not texture with that name!");
		return 0;
	}

	irr::scene::ISceneNode* node = smgr->getSceneNodeFromName(object_p);

	node->setMaterialTexture(0, tex);

	lua_settop(L, 0);//clear cache
	return 0;
}

static int addTexture(lua_State* L) {
	int args = lua_gettop(L);
	if (args != 2) {//arguments length check
		GLF::throwError(L, "ERROR: Wrong amount of arguments.");
		return 0;
	}

	int dim1 = GLF::EDC(L, 1);
	if ((dim1 & (dim1 - 1)) != 0) {
		GLF::throwError(L, "ERROR: the dimension is not a power of two.");
		return 0;
	}
	int* dim2 = new int[dim1];
	int start = 3;
	for (int i = 3; i < dim1 + 3; i++) {
		dim2[i - 3] = GLF::EDC(L, i);
	}
	for (int i = 0; i < dim1; i++) {
		if (dim2[i] != dim1) {
			GLF::throwError(L,"ERROR: dimension mismatch.");
			return 0;
		}
	}
	delete dim2;

	float** values = new float*[dim1 * dim1];
	irr::core::dimension2d<irr::u32> d2v(dim1, dim1);
	unsigned int* bf = new unsigned int[dim1 * dim1];
	unsigned int* bfc = new unsigned int[dim1 * dim1];
	for (int i = 0; i < dim1 * dim1; i++) {
		float* p = GLF::RTAV(L, -(i + 1), 3);
		irr::video::SColor c;
		for (int j = 0; j < 3; j++) {
			if (p[j] > 1) {
				p[j] = 1;
			}
		}
		float r = p[0];
		float g = p[1];
		float bb = p[2];
		c.setAlpha(255);
		c.setRed(p[0]*255);
		c.setBlue(p[1]*255);
		c.setGreen(p[2]*255);
		unsigned int* b = new unsigned int;
		c.getData(b, ECF_A8R8G8B8);
		//std::cout << *b << "\n";
		bf[i] = *b;
		//std::cout << v2[i][0] << " : " << v2[i][1] << " : " << v2[i][2] << "\n";
	}
	driver->convertColor(bf, ECF_A8R8G8B8, dim1 * dim1, bfc, driver->getColorFormat());

	std::string name = luaL_checkstring(L, 2);//set name
	irr::io::path p;
	p.append(name.c_str());

	irr::video::IImage* image = driver->createImageFromData(driver->getColorFormat(), d2v, bfc);
	driver->addTexture(p, image);

	lua_settop(L, 0);//clear cache
	return 1;
}

int _camera(irr::core::vector3df pos, irr::core::vector3df look_at) {
	cameraNode->setPosition(pos);
	cameraNode->setTarget(look_at);
	return 1;
}

static int camera(lua_State* L) {
	int args = lua_gettop(L);
	if (args != 2) {//arguments length check
		GLF::throwError(L, "ERROR: Wrong amount of arguments.");
		return 0;
	}
	float* p = GLF::RTAV(L, 1, 3);
	if (!p) {
		return 0;
	}

	float* la = GLF::RTAV(L, 2, 3);
	if (!la) {
		return 0;
	}
	
	_camera(irr::core::vector3df(p[0], p[1], p[2]), irr::core::vector3df(la[0], la[1], la[2]));
	return 1;
}
//camera({100,100,100},{0,0,0})

int _addBox(float x, float y, float z, float size, std::string name = "") {
	bool valid;
	std::string n_name;
	if (name == "") {
		valid = true;
		n_name = makeName();
	}
	else {
		valid = !searchName(name);
		n_name = name;
	}

	if (valid) {
		irr::scene::ISceneNode* mnode = smgr->addCubeSceneNode(size, NULL, -1, irr::core::vector3df(x, y, z));
		mnode->setID(smgr->getRootSceneNode()->getChildren().size());
		//objects.push_back(sceneNodeObject(mnode, n_name));
		mnode->setName(n_name.c_str());
		mnode->setMaterialFlag(irr::video::EMF_LIGHTING, false);
		mnode->setMaterialFlag(irr::video::EMF_BACK_FACE_CULLING, false);
		std::cout << "New object created\n";
		return 1;
	}
	else {
		return -1;//name already in use
	}
}

static int addBox(lua_State* L) {
	int args = lua_gettop(L);
	if (args != 3 && args != 2) {//arguments length check
		GLF::throwError(L, "ERROR: Wrong amount of arguments.");
		return 0;
	}

	//std::cout << p[0] << p[1] << p[2] << "\n";

	float size = luaL_checknumber(L, 2);//set size

	std::string name;
	if (args == 2) {
		name = makeName();//generate name
	}
	else {
		name = luaL_checkstring(L, 3);//set name
	}

	float* p = GLF::RTAV(L, 1, 3);
	if (!p) {
		return 0;
	}

	int r = _addBox(p[0], p[1], p[2], size, name);//create box

	if (r < 1) {
		delete p;
		GLF::throwError(L, "ERROR: name already used.");
		return 0;
	}
	delete p;
	lua_settop(L, 0);//clear cache
	return r;
}
//addBox({1,2,3},10,"Jared")

static int updatepos(lua_State* L) {
	if (lua_gettop(L) != 3) {
		GLF::throwError(L, "Wrong number of parameters, use this format: updatepos(x,y,z)");
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
		GLF::throwError(L, "Error: not a valid number of vertices.");
		return 0;
	}

	int subt = 2;
	//read all subtables in argument, stop when non-table is found
	while (lua_istable(L, -1)) {
		lua_rawgeti(L, 1, subt);
		subt++;
	}

	if ((subt - 2) % 3 != 0 || subt < 5) {
		GLF::throwError(L, "Error: not a valid number of vertices.");
		return 0;
	}

	std::vector<std::vector<float>> vertices;
	int index = 0; //sane index
	for (int i = 2; i < subt; i++) {
		//if too few coordinates are passed.
		if (!lua_rawgeti(L, i, 1) || !lua_rawgeti(L, i, 2) || !lua_rawgeti(L, i, 3)) {
			GLF::throwError(L, "Error: number of components.");
			return 0;
		}
		//check to make sure all elements are numbers
		//get all coordinates
		float zi = luaL_checknumber(L, -1);
		float yi = luaL_checknumber(L, -2);
		float xi = luaL_checknumber(L, -3);
		std::vector<float> vector;
		vector.push_back(xi); vector.push_back(yi); vector.push_back(zi);
		vertices.push_back(vector);
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
		buf->Vertices[i] = irr::video::S3DVertex(vertices[i][0], vertices[i][1], vertices[i][2], 0, 1, 0, irr::video::SColor(255, 0, 0, 0), 0, 1);
		buf->Indices[i] = i;
	}

	buf->Indices.reallocate(subt-2);
	buf->Indices.set_used(subt-2);

	buf->recalculateBoundingBox();


	IMeshSceneNode* myNode = device->getSceneManager()->addMeshSceneNode(mesh);


	std::string n_name = makeName();
	myNode->setName(n_name.c_str());
	myNode->setID(smgr->getRootSceneNode()->getChildren().size());

	myNode->setMaterialFlag(irr::video::EMF_BACK_FACE_CULLING, false);
	myNode->setMaterialFlag(irr::video::EMF_LIGHTING, false);
	myNode->setMaterialFlag(irr::video::EMF_NORMALIZE_NORMALS, false);
	
	return 0;
}

static int getNodes(lua_State* L) {
	irr::core::list<irr::scene::ISceneNode*>  children = smgr->getRootSceneNode()->getChildren();
	lua_newtable(L);
	int count = 1;
	for (irr::scene::ISceneNode* cnode : children) {
		lua_newtable(L);
		lua_pushstring(L, cnode->getName());
		lua_setfield(L, -2, "name");
		lua_pushnumber(L, cnode->getID());
		lua_setfield(L, -2, "id");

		std::string type = "unknown";
		switch (cnode->getType()) {
		case ESNT_CUBE:
			type = "CUBE";
			break;
		case ESNT_MESH:
			type = "MESH";
		}

		lua_pushstring(L, type.c_str());
		lua_setfield(L, -2, "type");

		lua_rawseti(L, -2, count);
		count++;
	}
	return 1;
}

static int snapshot(lua_State* L) {
	luaL_argcheck(L, lua_isstring(L, -1), 1, "<string> expected.");
	std::string output = luaL_checkstring(L,-1);

	//render();
	Sleep(2000);

	irr::video::IImage* ss = device->getVideoDriver()->createScreenShot();

	if (!device->getVideoDriver()->writeImageToFile(ss, output.c_str())) {
		GLF::throwError(L, "Error: failed to write file!");
	}
	return 0;
}

static int loadScene(lua_State* L) {
	luaL_argcheck(L, lua_isstring(L, -1), 1, "<string> expected.");
	std::string file = luaL_checkstring(L, -1);

	std::ifstream ifs(file, std::ifstream::in);
	std::string contents((std::istreambuf_iterator<char>(ifs)),
		std::istreambuf_iterator<char>());

	const char* chars = contents.c_str();

	if (contents.length() > 0) {
		Parser p = Parser(chars);
		Tree* tred;
		if (p.DESCRIPTOR(&tred)) {
			tred->dump();

			smgr->getRootSceneNode()->removeAll();
			cameraNode = smgr->addCameraSceneNodeFPS();

			std::vector<loadedmesh> meshes;
			if (!tred->generateScene(smgr, driver, device, &meshes)) {
				tred->dumpErrors();
				smgr->getRootSceneNode()->removeAll();
				cameraNode = smgr->addCameraSceneNodeFPS();
				GLF::throwError(L, "Errors in scene file!");
			}
		}
		else {
			GLF::throwError(L, "Error compiling scene file!");
		}
		
	}
	else {
		GLF::throwError(L, "Scene file not found!");
	}
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
	lua_pushcfunction(L, getNodes);
	lua_setglobal(L, "getNodes");
	lua_pushcfunction(L, snapshot);
	lua_setglobal(L, "snapshot");
	lua_pushcfunction(L, loadScene);
	lua_setglobal(L, "loadScene");
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

void render() {
	bool active = device->isWindowActive();
	cameraNode->setInputReceiverEnabled(active);
	cameraNode->setInputReceiverEnabled(active);

	device->getVideoDriver()->beginScene(true, true, irr::video::SColor(255, 90, 101, 140));
	smgr->drawAll();
	device->getGUIEnvironment()->drawAll();

	device->getVideoDriver()->endScene();
}

int main()
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	std::thread conThread(ConsoleThread, L);

	device = irr::createDevice(irr::video::EDT_SOFTWARE, irr::core::dimension2d<irr::u32>(640, 480), 16, false, false, true, 0);
	if(!device)
		return 1;

	device->setWindowCaption(L"LUA Project");
	driver	= device->getVideoDriver();

	smgr		= device->getSceneManager();
	irr::gui::IGUIEnvironment* guienv	= device->getGUIEnvironment();

	lua_register(L, "bind", bind);
	lua_register(L, "addBox", addBox);
	lua_register(L, "camera", camera);
	lua_register(L, "addTexture", addTexture);
	registerLuaFunctions(L);
	cameraNode = smgr->addCameraSceneNodeFPS();
	while(device->run()) {
		render();
	}

	device->drop();

	conThread.join();
	return 0;
}