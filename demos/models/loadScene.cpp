#include "App.h"

void App::loadScene() {
    const std::string path = "";

    sky = Sky::fromFile(System::findDataFile("sky"));

    const Matrix3 rot180 = Matrix3::fromAxisAngle(Vector3::unitY(), toRadians(180));
    const Matrix3 rot270 = Matrix3::fromAxisAngle(Vector3::unitY(), toRadians(270));

    const float groundY = -1.0f;
    float x = -2;

    // MD2
    if (true) {
        MD2Model::Ref model = MD2Model::fromFile(pathConcat(dataDir, "md2/pknight/tris.md2"));
        Texture::Ref texture = Texture::fromFile(pathConcat(dataDir, "md2/pknight/knight.pcx"), ImageFormat::AUTO(),
                                                 Texture::DIM_2D, Texture::Settings::defaults(), 
                                                 Texture::PreProcess::quake());
        entityArray.append(Entity::create(model, texture, CoordinateFrame(rot180, Vector3(x, groundY + 0.95f, 0))));
        x += 2;
    }

    // MD3
    if (false) {
//        const std::string path = "D:/morgan/data/md3/chaos-marine/models/players/Chaos-Marine/";
        const std::string path = "D:/morgan/data/md3/dragon/models/players/dragon/";
        MD3Model::Ref model = MD3Model::fromDirectory(path);
        entityArray.append(Entity::create(model, CFrame(rot180, Vector3(x, groundY + 0.8f, 0))));
        x += 2;
    }

    // 3DS
    if (true) {
        CoordinateFrame xform;

        xform.rotation[0][0] = xform.rotation[1][1] = xform.rotation[2][2] = 0.009f;
        xform.rotation = xform.rotation * rot270;
        xform.translation = Vector3(0, -1.0f, 0);

        std::string filename = pathConcat(dataDir, "3ds/weapon/cannon/cannon.3ds");
        ArticulatedModel::Ref model = ArticulatedModel::fromFile(filename, xform);
        entityArray.append(Entity::create(model, CoordinateFrame(rot180, Vector3(x, groundY + 1.0f,0))));
        x += 2;
    }

    // IFSModel as ArticulatedModel
    if (true) {
        ArticulatedModel::Ref model = ArticulatedModel::fromFile(System::findDataFile("teapot.ifs"));
        entityArray.append(Entity::create(model, CoordinateFrame(rot180, Vector3(x,groundY + 1.0f - 0.3f,0))));
        x += 2;
    }

    // IFSModel (note that IFS files can be loaded with ArticulatedModel and will render better)
    if (false) {
        IFSModelRef model = IFSModel::fromFile(System::findDataFile("cow.ifs"));
        entityArray.append(Entity::create(model, GMaterial(), CoordinateFrame(rot180, Vector3(x,0,2))));
        x += 2;
    }

    // Reflective object
    if (false) {
        std::string filename = System::findDataFile("cow.ifs");
        ArticulatedModel::Ref model = ArticulatedModel::fromFile(filename);
    
        Material::Settings spec;
        spec.setLambertian(Color3::zero());
        spec.setSpecular(Color3::white() * 0.5f);
        spec.setShininess(SuperBSDF::packedSpecularMirror());

        model->partArray[0].triList[0]->material = Material::create(spec);
        entityArray.append(Entity::create(model, CoordinateFrame(rot180, Vector3(x,0.05f,0))));
        x += 2;
    }

    // Textured ground plane generated mathematically on the fly
    if (true) {
        ArticulatedModel::Ref model = ArticulatedModel::createEmpty();

        model->name = "Ground Plane";
        ArticulatedModel::Part& part = model->partArray.next();
        part.cframe = CoordinateFrame();
        part.name = "root";
    
        ArticulatedModel::Part::TriList::Ref triList = part.newTriList();

        MeshAlg::generateGrid(part.geometry.vertexArray, part.texCoordArray,
                              triList->indexArray, 7, 7, Vector2(10, 10), true, 
                              false, Matrix3::identity() * 10);

        triList->twoSided = true;

        Material::Settings mat;
        mat.setEmissive(Color3::black());
        mat.setLambertian("stone.jpg", 0.8f);        
        BumpMap::Settings s;
        s.iterations = 1;
        mat.setBump("stone-bump.png", s);
        triList->material = Material::create(mat);
        
        triList->computeBounds(part);

        part.indexArray = triList->indexArray;

        model->updateAll();

        entityArray.append(Entity::create(model, CoordinateFrame(Vector3(0, groundY, 0))));
    }

    lighting = Lighting::create();
    {
        skyParameters = SkyParameters(G3D::toSeconds(1, 00, 00, PM));

        skyParameters.skyAmbient = Color3::white();

        if (sky.notNull()) {
            lighting->environmentMap = sky->getEnvironmentMap();
            lighting->environmentMapColor = skyParameters.skyAmbient;
        } else {
            lighting->environmentMapColor = Color3::black();
        }

        lighting->ambientTop = Color3(0.7f, 0.7f, 1.0f) * skyParameters.diffuseAmbient;
        lighting->ambientBottom = Color3(0.3f, 0.4f, 0.5f) * skyParameters.diffuseAmbient;

        lighting->emissiveScale = skyParameters.emissiveScale;

        lighting->lightArray.clear();
        lighting->shadowedLightArray.clear();

        GLight L = skyParameters.directionalLight();
        // Decrease the blue since we're adding blue ambient
        L.color *= Color3(1.2f, 1.2f, 1) * 0.5f;
        L.position = Vector4(Vector3(0,1,1).direction(), 0);

        /*
        L.position = Vector4(0,10,0,1);
        L.spotCutoff = 45;
        L.spotDirection = -Vector3::unitY();
        */

        lighting->shadowedLightArray.append(L);
        lighting->lightArray.append(GLight::point(Vector3(-1.5f,-0.6f,2.5f), Color3::blue() * 0.7f, 0.1f, 0, 1.5f, true, true));
        lighting->lightArray.append(GLight::point(Vector3(1.5f,-0.6f,2.5f), Color3::purple() * 0.7f, 0.1f, 0, 1.5f, true, true));
        lighting->lightArray.append(GLight::point(Vector3(-1.5f,-0.6f,1), Color3::green() * 0.7f, 0.1f, 0, 1.5f, true, true));
        lighting->lightArray.append(GLight::point(Vector3(0,-0.6f,1.5f), Color3::yellow() * 0.7f, 0.1f, 0, 1.5f, true, true));
        lighting->lightArray.append(GLight::point(Vector3(1.5f,-0.6f,1), Color3::red() * 0.7f, 0.1f, 0, 1.5f, true, true));
    }
}


