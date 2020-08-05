/** A wrapper for texture related functionality **/

#pragma once
#include <SDL_opengl.h>

class CTexture
{
private:
	GLuint  texID;
	GLfloat  width;
	GLfloat  height;

public:
	CTexture();
	~CTexture();
	void free_texture();
	bool init(GLuint* pixels, GLfloat _width, GLfloat _height);
	bool update(GLuint* pixels);
	void render(GLfloat x, GLfloat y);
	GLuint get_texture_id();
};

