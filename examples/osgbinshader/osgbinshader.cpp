

// This is public domain software and comes with
// absolutely no warranty. Use of public domain software
// may vary between counties, but in general you are free
// to use and distribute this software for any purpose.


// Example: OSG using an OpenGL 3.1 context.
// The comment block at the end of the source describes building OSG
// for use with OpenGL 3.x.

#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osg/GraphicsContext>
#include <osg/Camera>
#include <osg/Viewport>
#include <osg/StateSet>
#include <osg/Program>
#include <osg/Shader>
#include "C:/Program Files/NVIDIA Corporation/NvToolsExt/include/nvToolsExt.h"
#include "osgGA/NodeTrackerManipulator"
#include "osg/MatrixTransform"
#include "osg/LineWidth"
#include "osg/ShapeDrawable"
#include "osg/Texture2D"


#pragma region  shaders


#define GLSL430(src) "#version 430\n" #src

#pragma region passthrough shaders

const char* passthroughVertSource = GLSL430(

struct LightSource
{
	vec3 position;
	vec3 color;
};

layout(location = 0) in vec4 osg_Vertex;
layout(location = 1) in vec3 osg_Normal;
layout(location = 2) in vec3 osg_Color;

layout(location = 0) out vec4 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 color;

layout(location = 0) uniform mat4 osg_ModelViewProjectionMatrix;
layout(location = 1) uniform mat3 osg_NormalMatrix;
layout(location = 2) uniform mat4 NormalMatrix;
layout(location = 3) uniform mat4 ModelMatrix;
layout(location = 4) uniform LightSource lightsource; // consumes 2 locations

void main(void)
{
	fragNormal = osg_Normal;
	color = vec4(osg_Color, 1.0f);

	gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;
}
);

const char* passthroughFragSource = GLSL430(

struct LightSource
{
	vec3 position;
	vec3 color;
};

layout(location = 0) in vec4 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 color;


layout(location = 0) uniform mat4 osg_ModelViewProjectionMatrix;
layout(location = 1) uniform mat3 osg_NormalMatrix;
layout(location = 2) uniform mat4 NormalMatrix;
layout(location = 3) uniform mat4 ModelMatrix;
layout(location = 4) uniform LightSource lightsource; // consumes 2 locations

layout(location = 0) out vec4 FragColor;
void main(void)
{
	FragColor = color;
}
);


#pragma endregion 

#pragma region model shaders

const char* modelVertSource = GLSL430(

struct LightSource
{
	vec3 position;
	vec3 color;
};

layout(location = 0) in vec4 osg_Vertex;
layout(location = 1) in vec3 osg_Normal;
layout(location = 2) in vec3 osg_Color;
layout(location = 3) in vec4 osg_MultiTexCoord0;
layout(location = 4) in vec4 osg_MultiTexCoord1;
layout(location = 5) in vec4 osg_MultiTexCoord2;
layout(location = 6) in vec4 osg_MultiTexCoord3;
layout(location = 7) in vec4 osg_MultiTexCoord4;
layout(location = 8) in vec4 osg_MultiTexCoord5;
layout(location = 9) in vec4 osg_MultiTexCoord6;
layout(location = 10) in vec4 osg_MultiTexCoord7;

layout(location = 0) out vec4 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 color;
layout(location = 3) out vec2 fragTexCoords;

uniform sampler2D baseTexture;
layout(location = 0) uniform mat4 osg_ModelViewProjectionMatrix;
layout(location = 1) uniform mat3 osg_NormalMatrix;
layout(location = 3) uniform mat4 NormalMatrix;
layout(location = 4) uniform int numOfLightSources;
layout(location = 5) uniform mat4 osg_ViewMatrix;
layout(location = 6) uniform mat4 ModelMatrix;
layout(location = 7) uniform LightSource[2] lightSource; // consumes 2 * 2 locations

void main(void)
{
	fragNormal = osg_Normal;
	fragPos = vec4(vec3(ModelMatrix * osg_Vertex), 1.0);
	color = vec4(osg_Color, 1.0f);
	fragTexCoords = osg_MultiTexCoord0.xy;

	gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;
}
);

const char* modelFragSource = GLSL430(

struct LightSource
{
	vec3 position;
	vec3 color;
};

layout(location = 0) in vec4 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 color;
layout(location = 3) in vec2 fragTexCoords;

uniform sampler2D baseTexture;
layout(location = 0) uniform mat4 osg_ModelViewProjectionMatrix;
layout(location = 1) uniform mat3 osg_NormalMatrix;
layout(location = 3) uniform mat4 NormalMatrix;
layout(location = 4) uniform int numOfLightSources;
layout(location = 5) uniform mat4 osg_ViewMatrix;
layout(location = 6) uniform mat4 ModelMatrix;
layout(location = 7) uniform LightSource[2] lightSource; // consumes 2 * 2 locations


layout(location = 0) out vec4 FragColor;


void main(void)
{
	for (int i = 0; i <= numOfLightSources - 1; i++)
	{
		// ambient
		float ambientStrength = 0.001;
		vec3 ambient = ambientStrength * texture(baseTexture, fragTexCoords).rgb * lightSource[i].color;
		
		//diffuse			
		vec4 norm = normalize(NormalMatrix * vec4(fragNormal, 1.0f));
		vec3 lightDir = normalize(lightSource[i].position - fragPos.xyz);
		float diff = max(dot(norm.xyz, lightDir), 0.0);
		vec3 diffuse = diff * lightSource[i].color * texture(baseTexture, fragTexCoords).rgb;

		vec3 finalColor = ambient + diffuse;
		FragColor += vec4(finalColor, 1.0f);

	}
}


);


#pragma endregion 


#pragma endregion 


#pragma uniform_callback

struct NormalMatrixCallback : public osg::Uniform::Callback {
	NormalMatrixCallback(osg::Camera* camera) :
		_camera(camera) {
	}

	virtual void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv) {		
		osg::Matrixd viewMatrix = _camera->getViewMatrix();
		osg::Matrixd modelMatrix = osg::computeLocalToWorld(nv->getNodePath());

		
		osg::Matrixd normalMatrix = osg::Matrixd::inverse(osg::Matrixd::transpose4x4(modelMatrix));
		uniform->set(normalMatrix);
	}

	osg::Camera* _camera;
};


struct ModelMatrixCallback : public osg::Uniform::Callback {
	ModelMatrixCallback(osg::Camera* camera) :
		_camera(camera) {
	}

	virtual void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv) {

		osg::Matrixd viewMatrix = _camera->getViewMatrix();
		osg::Matrixd modelMatrix = osg::computeLocalToWorld(nv->getNodePath());
		uniform->set(modelMatrix);

	}

	osg::Camera* _camera;
};

#pragma endregion 

struct LightSource
{
	osg::Vec3f position;
	osg::Vec3f color;
};


void loadShadersFiles(osg::StateSet* stateSet, std::string vertSourcePath, std::string fragSourcePath, bool isBinaryShader = true)
{

	osg::Program* program = new osg::Program;

	if (isBinaryShader)
	{
		program->addShader(new osg::Shader(osg::Shader::VERTEX,
			osg::ShaderBinary::readShaderBinaryFile(vertSourcePath.c_str())));

		program->addShader(new osg::Shader(osg::Shader::FRAGMENT,
			osg::ShaderBinary::readShaderBinaryFile(fragSourcePath.c_str())));
	}
	else 
	{
		program->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, vertSourcePath.c_str()));
		program->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, fragSourcePath.c_str()));
	}

	stateSet->setAttribute(program);

}



void loadSourceShaders(osg::StateSet* stateSet, const char* vertSource, const char* fragSource)
{

	osg::Shader* vShader = new osg::Shader(osg::Shader::VERTEX, vertSource);
	osg::Shader* fShader = new osg::Shader(osg::Shader::FRAGMENT, fragSource);

	osg::Program* program = new osg::Program;

	program->addShader(vShader);
	program->addShader(fShader);

	stateSet->setAttribute(program);

}

osg::Drawable* createAxis(const osg::Vec3& corner, const osg::Vec3& xdir, const osg::Vec3& ydir, const osg::Vec3& zdir)
{// set up the Geometry.
	osg::Geometry* geom = new osg::Geometry;

	osg::Vec3Array* coords = new osg::Vec3Array(6);
	(*coords)[0] = corner;
	(*coords)[1] = corner + xdir;
	(*coords)[2] = corner;
	(*coords)[3] = corner + ydir;
	(*coords)[4] = corner;
	(*coords)[5] = corner + zdir;

	geom->setVertexArray(coords);

	osg::Vec4 x_color(1.0f, 0.5f, 0.5f, 1.0f);
	osg::Vec4 y_color(0.5f, 1.0f, 0.5f, 1.0f);
	osg::Vec4 z_color(0.5f, 0.5f, 1.0f, 1.0f); // color

	osg::Vec4Array* color = new osg::Vec4Array(6);
	(*color)[0] = x_color;
	(*color)[1] = x_color;
	(*color)[2] = y_color;
	(*color)[3] = y_color;
	(*color)[4] = z_color;
	(*color)[5] = z_color;

	geom->setColorArray(color, osg::Array::BIND_PER_VERTEX);

	geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 6));

	osg::StateSet* stateset = new osg::StateSet;
	osg::LineWidth* linewidth = new osg::LineWidth();
	linewidth->setWidth(2.0f);
	stateset->setAttributeAndModes(linewidth, osg::StateAttribute::ON);
	stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	geom->setStateSet(stateset);

	return geom;
}

int main(int argc, char** argv)
{
	osg::ArgumentParser arguments(&argc, argv);
	osg::ref_ptr<osg::Group> root = new osg::Group;
	osgViewer::Viewer viewer;

	bool isBinaryShader = true;
	std::string passthroughVertSourcePath;
	std::string passthroughFragSourcePath;
	std::string modelVertSourcePath;
	std::string modelFragtSourcePath;

	osg::ref_ptr<osg::Node> loadedModel =
		osgDB::readRefNodeFile("../../../examples/osgbinshader/cube.obj");
	if (loadedModel == NULL)
	{
		osg::notify(osg::FATAL) << "Unable to load model from command line." << std::endl;
		return(1);
	}
	
	osg::Image* image = osgDB::readImageFile("../../../examples/osgbinshader/rockwall.png");
	if (image)
	{
		osg::Texture2D* txt = new osg::Texture2D;
		txt->setImage(image);
		txt->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
		txt->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
		txt->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
		txt->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
		loadedModel->getOrCreateStateSet()->setTextureAttributeAndModes(0, txt, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

		osg::Uniform* baseTextureSampler = new osg::Uniform("baseTexture", 0);
		loadedModel->getOrCreateStateSet()->addUniform(baseTextureSampler);
	}
	

	osg::ref_ptr<osg::MatrixTransform> modelTransform = new osg::MatrixTransform;
	modelTransform->setMatrix(osg::Matrix::scale(1.0f, 1.0f, 1.0f) * osg::Matrix::translate(0.0f, 0.0f, 0.0f));
	modelTransform->addChild(loadedModel);

	if (isBinaryShader)
	{
		passthroughVertSourcePath = "../../../examples/osgbinshader/pass_through_vert.spv";
		passthroughFragSourcePath = "../../../examples/osgbinshader/pass_through_frag.spv";
		modelVertSourcePath = "../../../examples/osgbinshader/shader_vert.spv";
		modelFragtSourcePath = "../../../examples/osgbinshader/shader_frag.spv";
	}
	else
	{
		// must have absulute path
		passthroughVertSourcePath = "../../../examples/osgbinshader/pass_through.vert";
		passthroughFragSourcePath = "../../../examples/osgbinshader/pass_through.frag";
		modelVertSourcePath = "../../../examples/osgbinshader/shader.vert";
		modelFragtSourcePath = "../../../examples/osgbinshader/shader.frag";
	}

	loadShadersFiles(loadedModel->getOrCreateStateSet(), modelVertSourcePath, modelFragtSourcePath, isBinaryShader);
	
	// axis
	osg::Geode* axis = new osg::Geode();
	axis->addDrawable(createAxis(
		osg::Vec3(0.0f, 0.0f, 0.0f),
		osg::Vec3(5.0f, 0.0f, 0.0f),
		osg::Vec3(0.0f, 5.0f, 0.0f),
		osg::Vec3(0.0f, 0.0f, 5.0f)));
	loadShadersFiles(axis->getOrCreateStateSet(), passthroughVertSourcePath, passthroughFragSourcePath, isBinaryShader);

	// light		
	LightSource lightSources[2]{};
	lightSources[0].position = osg::Vec3f(-3.0f, 2.0f, .0f);
	lightSources[0].color = osg::Vec3f(1.f, .5f, .0f);
	lightSources[1].position = osg::Vec3f(2.0f, -1.0f, 1.0f);
	lightSources[1].color = osg::Vec3f(.5f, .0f, 1.f);
	int numOfLightSources = sizeof(lightSources) / sizeof(LightSource);

	for (int i = 0; i <= numOfLightSources - 1; i++)
	{
		osg::Geode* lightGeode = new osg::Geode;
		osg::ShapeDrawable* drawable = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), .3f));
		drawable->setColor(osg::Vec4(lightSources[i].color, 1.0f));
		lightGeode->addDrawable(drawable);
		osg::ref_ptr<osg::MatrixTransform> lightTransform = new osg::MatrixTransform;
		lightTransform->setMatrix(osg::Matrix::scale(1.0f, 1.0f, 1.0f) * osg::Matrix::translate(lightSources[i].position));
		lightTransform->addChild(lightGeode);
		loadShadersFiles(lightTransform->getOrCreateStateSet(), passthroughVertSourcePath, passthroughFragSourcePath, isBinaryShader);

		std::string uniformPosName = "lightSource[" + std::to_string(i) + "].position";
		std::string uniformColorName = "lightSource[" + std::to_string(i) + "].color";
		root->getOrCreateStateSet()->addUniform(new osg::Uniform(uniformPosName.c_str(), lightSources[i].position));
		root->getOrCreateStateSet()->addUniform(new osg::Uniform(uniformColorName.c_str(), lightSources[i].color));

		root->addChild(lightTransform);
	}
	osg::Uniform* numOfLightSourcesUniform = new osg::Uniform(osg::Uniform::INT, "numOfLightSources");
	numOfLightSourcesUniform->set(numOfLightSources);
	root->getOrCreateStateSet()->addUniform(numOfLightSourcesUniform);

	const int width(800), height(450);
	const std::string version("4.3");
	osg::ref_ptr< osg::GraphicsContext::Traits > traits = new osg::GraphicsContext::Traits();
	traits->x = 20; traits->y = 30;
	traits->width = width; traits->height = height;
	traits->windowDecoration = true;
	traits->doubleBuffer = true;
	traits->glContextVersion = version;
	osg::ref_ptr< osg::GraphicsContext > gc = osg::GraphicsContext::createGraphicsContext(traits.get());
	if (!gc.valid())
	{
		osg::notify(osg::FATAL) << "Unable to create OpenGL v" << version << " context." << std::endl;
		return(1);
	}

	// Create a Camera that uses the above OpenGL context.
	osg::Camera* cam = viewer.getCamera();
	cam->setClearColor({});

	cam->setGraphicsContext(gc.get());
	// Must set perspective projection for fovy and aspect.
	cam->setProjectionMatrix(osg::Matrix::perspective(30., (double)width / (double)height, 1., 100.));
	// Unlike OpenGL, OSG viewport does *not* default to window dimensions.
	cam->setViewport(new osg::Viewport(0, 0, width, height));


	// adding camera manipulator
	osgGA::NodeTrackerManipulator* nodeTrackerManipulator = new osgGA::NodeTrackerManipulator;
	nodeTrackerManipulator->setTrackerMode(osgGA::NodeTrackerManipulator::NODE_CENTER_AND_ROTATION);
	viewer.setCameraManipulator(nodeTrackerManipulator);


	osg::Uniform* modelMatrix = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "ModelMatrix");
	modelMatrix->setUpdateCallback(new ModelMatrixCallback(cam));
	root->getOrCreateStateSet()->addUniform(modelMatrix);

	osg::Uniform* normalMatrix = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "NormalMatrix");
	normalMatrix->setUpdateCallback(new NormalMatrixCallback(cam));
	root->getOrCreateStateSet()->addUniform(normalMatrix);

	
	osg::Uniform* isUsePhongModel = new osg::Uniform(osg::Uniform::BOOL, "IsUsePhongModel");
	isUsePhongModel->set(false);
	root->getOrCreateStateSet()->addUniform(isUsePhongModel);

	// enable the osg_ uniforms that the shaders will use,	
	gc->getState()->setUseModelViewAndProjectionUniforms(true);
	gc->getState()->setUseVertexAttributeAliasing(true);



	root->addChild(axis);
	root->addChild(modelTransform);



	viewer.setSceneData(root);

	do
	{		
		viewer.frame();		

	} while (!viewer.done());
}
