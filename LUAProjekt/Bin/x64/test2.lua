addMesh({{0,2,0},{0,2,10},{10,2,10},{-5,2,0},{-5,2,-5},{0,2,-5}}) addBox({0,0,0},1,"origin") addBox({1,0,0},1,"offx") addBox({0,0,1},1,"offz") addBox({0,0,2},1) addTexture({{{1.0,0.5,0.5},{0.5,1.0,0.5}},{{0.5,0.5,1.0},{0.0,0.0,0.0}}}, "yoyo") bind("offz","yoyo") camera({-10,8,0},{0,0,0}) snapshot("test2.png")