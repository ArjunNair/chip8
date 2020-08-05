#include "CTexture.h"
#include <iostream>

CTexture::CTexture() {}

CTexture::~CTexture()
{
	free_texture();
}

void CTexture::free_texture()
{
	glBindTexture(GL_TEXTURE_2D, 0);

	//Delete texture
	if (texID != 0)
	{
		glDeleteTextures(1, &texID);
		texID = 0;
	}

	width = 0;
	height = 0;
}

bool CTexture::init(GLuint* pixels, GLfloat _width, GLfloat _height)
{
	glEnable(GL_TEXTURE_2D);

	//Free texture if it exists
	free_texture();

	//Get texture dimensions
	width = _width;
	height = _height;
	
	//Generate texture ID
	glGenTextures(1, &texID);

	//Bind texture ID
	glBindTexture(GL_TEXTURE_2D, texID);
	
	//Generate texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	
	//Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	//Unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);

	//Check for error
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::cout << "Error init texture: " << error;
		return false;
	}

	return true;
}

bool CTexture::update(GLuint* pixels)
{
	//Bind texture ID
	glBindTexture(GL_TEXTURE_2D, texID);

	//Generate texture
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	glBindTexture(GL_TEXTURE_2D, 0);

	//Check for error
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::cout << "Error init texture: " << error;
		return false;
	}
	return true;
}

void CTexture::render(GLfloat x, GLfloat y)
{
	//If the texture exists
	if (texID != 0)
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		//Move to rendering point
		glTranslatef(x, y, 0.f);

		//Set texture ID
		glBindTexture(GL_TEXTURE_2D, texID);

		//Render textured quad
		glBegin(GL_QUADS);
			glTexCoord2f(0.f, 0.f); glVertex2f(0.f, 0.f);
			glTexCoord2f(1.f, 0.f); glVertex2f(width, 0.f);
			glTexCoord2f(1.f, 1.f); glVertex2f(width, height);
			glTexCoord2f(0.f, 1.f); glVertex2f(0.f, height);
		glEnd();
	}
}

GLuint  CTexture::get_texture_id() { return texID; }
