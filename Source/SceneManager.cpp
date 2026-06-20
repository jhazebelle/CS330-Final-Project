///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


void SceneManager::LoadSceneTextures()
{
	// Load textures for Milestone Four.
	// Load texture used for the sausage object.
	CreateGLTexture("textures/sausage.jpg", "sausage");

	// Load wood texture used for the table plane.
	CreateGLTexture("textures/wood.jpg", "wood");

	// Load rice texture used for the prism riceball.
	CreateGLTexture("textures/rice.jpg", "rice");

	// Load seaweed texture used for the seaweed plane for riceball.
	CreateGLTexture("textures/seaweed.jfif", "seaweed");

	// Load egg texture used for the egg blocks.
	CreateGLTexture("textures/egg.jpg", "egg");

	// Bind loaded textures to OpenGL texture slots.
	BindGLTextures();
}

// Defines object material properties used by Phong lighting.
void SceneManager::DefineObjectMaterials()
{
	// Wood material gives the textured plane a soft, slightly reflective surface.
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	woodMaterial.specularColor = glm::vec3(0.25f, 0.25f, 0.25f);
	woodMaterial.shininess = 8.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// Sausage material keeps the complex object mostly matte with a small highlight.
	OBJECT_MATERIAL sausageMaterial;
	sausageMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	sausageMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	sausageMaterial.shininess = 16.0f;
	sausageMaterial.tag = "sausage";
	m_objectMaterials.push_back(sausageMaterial);
}

// Sets up scene light sources used to illuminate the 3D world.
void SceneManager::SetupSceneLights()
{
	// Enable Phong lighting.
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Main warm point light acts like overhead kitchen lighting for the food scene.
	m_pShaderManager->setVec3Value("pointLights[0].position", 3.0f, 10.0f, 5.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.25f, 0.25f, 0.20f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 1.0f, 0.95f, 0.85f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 1.0f, 0.95f, 0.85f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Secondary fill light softens shadows so no object is completely dark.
	m_pShaderManager->setVec3Value("pointLights[1].position", -6.0f, 5.0f, -3.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.08f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.40f, 0.40f, 0.60f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.40f, 0.40f, 0.60f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	
	
	// Load scene textures before rendering textured meshes.
	LoadSceneTextures();
	// Define material properties so textured objects respond to Phong lighting.
	DefineObjectMaterials();
	// Add light sources before rendering the scene.
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply wood texture and wood material so the plane reflects light as a table surface.
	SetShaderTexture("wood");
	SetShaderMaterial("wood");
	// Tile the texture to avoid stretching across the large plane.
	SetTextureUVScale(4.0f, 4.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	/* Lettuce backdrop: flattened green spheres placed behind food */
	/****************************************************************/

	float lettuceScales[6] = { 5.5f, 3.0f, 5.8f, 3.2f, 5.7f, 3.1f };

	for (int i = 0; i < 6; i++)
	{
		scaleXYZ = glm::vec3(
			lettuceScales[i],
			3.8f + ((i % 2) * 0.4f),
			0.5f);

		positionXYZ = glm::vec3(
			-4.5f + (i * 2.2f),
			4.0f + ((i % 2) * 0.5f),
			-6.8f);

		SetTransformations(
			scaleXYZ,
			0.0f,
			0.0f,
			0.0f,
			positionXYZ);

		// Flat green spheres create a simple lettuce backdrop behind the bento items.
		SetShaderColor(0.15f, 0.55f, 0.12f, 1.0f);
		m_basicMeshes->DrawSphereMesh();
	}



	/****************************************************************/
	/* 3-3 MILESTONE TWO: Scene inspired by a Japanese style bento lunch box */
	/* Reusing 3-2 Assignment background floor and wall planes for scene setup and to create depth*/
	/****************************************************************/

	// Cylinder for sausage body: long reddish brown cylinder on the right side
	// Scaled vertically to create the long sausage shape
	// Slight Z rotation creates a more natural appearance
	scaleXYZ = glm::vec3(1.4f, 5.0f, 1.4f);

	// Rotate cylinder so it stands vertically facing the camera
	// Small tilt added for visual realism
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 8.0f;

	// Position sausage on the right side of the scene
	// Inspired by the reference bento image layout
	positionXYZ = glm::vec3(5.5f, 2.5f, 0.0f);

	// Apply transformations to sausage body
	// Order: scale -> rotate -> translate
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply sausage texture and material to the cylinder body of the complex object.
	SetShaderTexture("sausage");
	SetShaderMaterial("sausage");
	// Tile the texture along the sausage body to add detail
	// and satisfy the texturing requirement.
	SetTextureUVScale(3.0f, 1.0f);

	// Draw cylinder mesh for sausage body
	m_basicMeshes->DrawCylinderMesh();

	// Top sphere: rounded top end of sausage
	// Sphere overlaps slightly with cylinder to form one object
	scaleXYZ = glm::vec3(1.4f, 1.4f, 1.4f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.80f, 7.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use the same texture and material on the end sphere to keep the sausage cohesive.
	SetShaderTexture("sausage");
	SetShaderMaterial("sausage");
	SetTextureUVScale(1.0f, 1.0f);

	// Draw sphere mesh
	m_basicMeshes->DrawSphereMesh();

	// Bottom sphere: rounded bottom end of sausage
	// Positioned to connect smoothly with cylinder body
	scaleXYZ = glm::vec3(1.4f, 1.4f, 1.4f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.55f, 2.1f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use the same texture and material on the end sphere to keep the sausage cohesive.
	SetShaderTexture("sausage");
	SetShaderMaterial("sausage");
	SetTextureUVScale(1.0f, 1.0f);

	// Draw sphere mesh
	m_basicMeshes->DrawSphereMesh();

	/******************************************************************
	 * Transformation Notes:
	 *
	 * Scale:
	 * Changes the size of the object on the X, Y, and Z axes.
	 *
	 * Rotation:
	 * Rotates the object around the X, Y, and Z axes in degrees.
	 *
	 * Translation:
	 * Moves the object to a new position in 3D space.
	 *
	 * Combining these transformations allows simple primitive
	 * meshes to form more complex scene objects.
	 ******************************************************************/

	 // Egg block stack placed left of the sausage to match the bento reference.
	SetShaderColor(1.0f, 0.78f, 0.20f, 1.0f);

	for (int i = 0; i < 3; i++)
	{
		scaleXYZ = glm::vec3(5.2f, 1.9f, 1.4f);

		positionXYZ = glm::vec3(
			1.0f,
			1.0f + (i * 2.0f),
			0.0f);

		SetTransformations(
			scaleXYZ,
			0.0f,
			0.0f,
			0.0f,
			positionXYZ);

		SetShaderColor(1.0f, 0.78f, 0.20f, 1.0f);
		// Use egg texture for the stacked blocks to add visual interest and satisfy texturing requirement.
		SetShaderTexture("egg");
		m_basicMeshes->DrawBoxMesh();
	}

	// Second rice ball placed behind the front rice ball.
	scaleXYZ = glm::vec3(8.0f, 3.2f, 7.0f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	positionXYZ = glm::vec3(-4.0f, 5.8f, -4.4f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.92f, 0.92f, 0.86f, 1.0f);
	// Use rice texture for the prism rice ball to add detail and satisfy texturing requirement.
	SetShaderTexture("rice");
	m_basicMeshes->DrawPrismMesh();


	// Seaweed strip for the second rice ball.
	scaleXYZ = glm::vec3(3.0f, 3.2f, 0.15f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-4.0f, 4.0f, -2.8f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.05f, 0.12f, 0.09f, 1.0f);
	//Use seaweed texture for the seaweed plane to add detail and satisfy texturing requirement.
	SetShaderTexture("seaweed");
	m_basicMeshes->DrawBoxMesh();

	// First rice ball made from a prism to better resemble the triangular onigiri in the reference image.
	scaleXYZ = glm::vec3(8.0f, 3.2f, 7.0f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	positionXYZ = glm::vec3(-4.0f, 3.5f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.95f, 0.95f, 0.88f, 1.0f);
	// Use rice texture for the prism rice ball to add detail and satisfy texturing requirement.
	SetShaderTexture("rice");
	m_basicMeshes->DrawPrismMesh();


	// Seaweed strip placed in front of the first rice ball.
	scaleXYZ = glm::vec3(3.0f, 3.2f, 0.15f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-4.0f, 1.6f, 0.6f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.05f, 0.12f, 0.09f, 1.0f);
	//Use seaweed texture for the seaweed plane to add detail and satisfy texturing requirement.
	SetShaderTexture("seaweed");
	m_basicMeshes->DrawBoxMesh();
}
