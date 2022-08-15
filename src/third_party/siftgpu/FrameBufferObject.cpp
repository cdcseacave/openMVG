////////////////////////////////////////////////////////////////////////////
//	File:		FrameBufferObject.cpp
//	Author:		Changchang Wu
//	Description : Implementation of FrameBufferObject Class
//
//
//
//	Copyright (c) 2007 University of North Carolina at Chapel Hill
//	All Rights Reserved
//
//	Permission to use, copy, modify and distribute this software and its
//	documentation for educational, research and non-profit purposes, without
//	fee, and without a written agreement is hereby granted, provided that the
//	above copyright notice and the following paragraph appear in all copies.
//	
//	The University of North Carolina at Chapel Hill make no representations
//	about the suitability of this software for any purpose. It is provided
//	'as is' without express or implied warranty. 
//
//	Please send BUG REPORTS to ccwu@cs.unc.edu
//
////////////////////////////////////////////////////////////////////////////


#include "GlobalUtil.h"
#include "FrameBufferObject.h"
#include <stdlib.h>

//whether use only one FBO globally
int		FrameBufferObject::UseSingleFBO=1;
GLuint	FrameBufferObject::GlobalFBO=0;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FrameBufferObject::FrameBufferObject(int autobind)
{
	if(UseSingleFBO && GlobalFBO)
	{
		_fboID = GlobalFBO;
	}else
	{
		GL_ASSERT(glGenFramebuffers(1, &_fboID));
		if(UseSingleFBO) GlobalFBO = _fboID;
	}
	if(autobind) GL_ASSERT(glBindFramebuffer(GL_FRAMEBUFFER, _fboID));
}

FrameBufferObject::~FrameBufferObject()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if(!UseSingleFBO)
	{
		glDeleteFramebuffers(1, &_fboID);
	}
}

void FrameBufferObject::DeleteGlobalFBO()
{
	if(UseSingleFBO)
	{
		GL_ASSERT(glDeleteFramebuffers(1, &GlobalFBO));
		GlobalFBO = 0;
	}
}

void FrameBufferObject::AttachDepthTexture(GLenum textureTarget, GLuint texID)
{
	GL_ASSERT(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureTarget, texID, 0));
}

void FrameBufferObject::AttachTexture(GLenum textureTarget, GLenum attachment, GLuint texId)
{
	GL_ASSERT(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, textureTarget, texId, 0));
}

void FrameBufferObject::BindFBO()
{
	GL_ASSERT(glBindFramebuffer(GL_FRAMEBUFFER, _fboID));
}

void FrameBufferObject::UnbindFBO()
{
	GL_ASSERT(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void FrameBufferObject::UnattachTex(GLenum attachment)
{
	GL_ASSERT(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, 0, 0));
}

void FrameBufferObject::AttachRenderBuffer(GLenum attachment, GLuint buffID)
{
	GL_ASSERT(glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, buffID));
}

void FrameBufferObject:: UnattachRenderBuffer(GLenum attachment)
{
	GL_ASSERT(glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, 0));
}

