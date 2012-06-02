/*
 * Copyright (c) 2011 Collabora, Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Collabora Ltd. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Collabora Ltd. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * COLLABORA LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL COLLABORA LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Pekka Paalanen <ppaalanen@gmail.com>
 */

#include <map>
#include <opengl/framebufferobject.h>
#include <opengl/texture.h>

struct PrivateGLFramebufferObject
{
    PrivateGLFramebufferObject () :
	fboId (0),
	glTex (NULL)
    {
    }

    void pushFBO ();
    void popFBO ();

    GLuint fboId;
    GLuint tmpId;
    GLuint rbStencilId;
    GLTexture *glTex;

    static std::map<GLuint, GLFramebufferObject *> idMap;
};

std::map<GLuint, GLFramebufferObject *> PrivateGLFramebufferObject::idMap;

void
PrivateGLFramebufferObject::pushFBO ()
{
    GLint id = 0;
    glGetIntegerv (GL_FRAMEBUFFER_BINDING, &id);
    tmpId = id;
    if (tmpId == fboId)
	return;

    (*GL::bindFramebuffer) (GL_FRAMEBUFFER, fboId);
}

void
PrivateGLFramebufferObject::popFBO ()
{
    if (tmpId != fboId)
	(*GL::bindFramebuffer) (GL_FRAMEBUFFER, tmpId);
}

GLFramebufferObject::GLFramebufferObject () :
    priv (new PrivateGLFramebufferObject)
{
    (*GL::genFramebuffers) (1, &priv->fboId);
    (*GL::genRenderbuffers) (1, &priv->rbStencilId);
    if (priv->fboId != 0)
	PrivateGLFramebufferObject::idMap[priv->fboId] = this;
}

GLFramebufferObject::~GLFramebufferObject ()
{
    if (priv->glTex)
	GLTexture::decRef (priv->glTex);

    PrivateGLFramebufferObject::idMap.erase (priv->fboId);
    (*GL::deleteFramebuffers) (1, &priv->fboId);
    (*GL::deleteRenderbuffers) (1, &priv->rbStencilId);

    delete priv;
}

bool
GLFramebufferObject::allocate (const CompSize &size, const char *image,
			       GLenum format, GLenum type)
{
    if (!priv->glTex ||
        size.width () != priv->glTex->width () ||
        size.height () != priv->glTex->height ())
    {
	if (priv->glTex)
	    GLTexture::decRef (priv->glTex);
	priv->glTex = NULL;

	GLTexture::List list = GLTexture::imageDataToTexture (image, size,
							      format, type);
	if (list.size () != 1 || list[0] == NULL)
	    return false;

	priv->glTex = list[0];
	GLTexture::incRef (priv->glTex);

	if (GL::stencilBuffer)
	{
	    (*GL::bindRenderbuffer) (GL::RENDERBUFFER, priv->rbStencilId);
	    (*GL::renderbufferStorage) (GL::RENDERBUFFER, GL::DEPTH24_STENCIL8, size.width (), size.height ());
	}
    }

    priv->pushFBO ();
    (*GL::framebufferTexture2D) (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 priv->glTex->target (),
                                 priv->glTex->name (), 0);
    priv->popFBO ();
    return true;
}

GLFramebufferObject *
GLFramebufferObject::bind ()
{
    GLFramebufferObject *old = NULL;
    GLint id = 0;

    glGetIntegerv (GL_FRAMEBUFFER_BINDING, &id);
    if (id != 0)
    {
	std::map<GLuint, GLFramebufferObject *>::iterator it;
	it = PrivateGLFramebufferObject::idMap.find (id);

	if (it != PrivateGLFramebufferObject::idMap.end ())
	    old = it->second;
	else
	    compLogMessage ("opengl", CompLogLevelError,
		"An FBO without GLFramebufferObject cannot be restored");
    }

    (*GL::bindFramebuffer) (GL::FRAMEBUFFER, priv->fboId);
    (*GL::framebufferRenderbuffer) (GL::FRAMEBUFFER, GL::DEPTH_ATTACHMENT, GL::RENDERBUFFER, priv->rbStencilId);
    (*GL::framebufferRenderbuffer) (GL::FRAMEBUFFER, GL::STENCIL_ATTACHMENT, GL::RENDERBUFFER, priv->rbStencilId);

    return old;
}

// static
void
GLFramebufferObject::rebind (GLFramebufferObject *fbo)
{
    GLuint id = fbo ? fbo->priv->fboId : 0;
    (*GL::bindFramebuffer) (GL_FRAMEBUFFER, id);
}

static const char *
getFboErrorString (GLint status)
{
    switch (status)
    {
	case        GL_FRAMEBUFFER_COMPLETE:
	    return "GL_FRAMEBUFFER_COMPLETE";
	case        GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
	    return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
	case        GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
	    return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
#ifdef USE_GLES
	case        GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
	    return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
#endif
	case        GL_FRAMEBUFFER_UNSUPPORTED:
	    return "GL_FRAMEBUFFER_UNSUPPORTED";
	default:
	    return "unexpected status";
    }
}

bool
GLFramebufferObject::checkStatus ()
{
    priv->pushFBO ();
    GLint status = (*GL::checkFramebufferStatus) (GL_FRAMEBUFFER);
    priv->popFBO ();

    if (status == GL_FRAMEBUFFER_COMPLETE)
	return true;

    compLogMessage ("opengl", CompLogLevelError,
                    "FBO is incomplete: %s (0x%04x)",
                    getFboErrorString (status), status);
    return false;
}

GLTexture *
GLFramebufferObject::tex ()
{
	return priv->glTex;
}
