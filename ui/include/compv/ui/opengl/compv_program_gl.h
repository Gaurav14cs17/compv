/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#if !defined(_COMPV_UI_OPENGL_PROGRAM_GL_H_)
#define _COMPV_UI_OPENGL_PROGRAM_GL_H_

#include "compv/base/compv_config.h"
#if defined(HAVE_GL_GLEW_H)
#include "compv/base/compv_obj.h"
#include "compv/base/compv_common.h"
#include "compv/ui/opengl/compv_program.h"
#include "compv/ui/opengl/compv_headers_gl.h"

#include <vector>

#if defined(_COMPV_API_H_)
#error("This is a private file and must not be part of the API")
#endif

COMPV_NAMESPACE_BEGIN()

class CompVProgramGL;
typedef CompVPtr<CompVProgramGL* > CompVProgramGLPtr;
typedef CompVProgramGLPtr* CompVProgramGLPtrPtr;

class CompVProgramGL : public CompVProgram
{
protected:
	CompVProgramGL();
public:
	virtual ~CompVProgramGL();
	virtual COMPV_INLINE const char* getObjectId() {
		return "CompVProgramGL";
	};

	virtual COMPV_ERROR_CODE shadAttachVertexFile(const char* pcFilePath);
	virtual COMPV_ERROR_CODE shadAttachFragmentFile(const char* pcFilePath);
	virtual COMPV_ERROR_CODE shadAttachVertexData(const char* dataPtr, size_t dataLength);
	virtual COMPV_ERROR_CODE shadAttachFragmentData(const char* dataPtr, size_t dataLength);
	virtual COMPV_ERROR_CODE link();
	virtual COMPV_ERROR_CODE useBegin();
	virtual COMPV_ERROR_CODE useEnd();

	static COMPV_ERROR_CODE newObj(CompVProgramGLPtrPtr program);

private:
	COMPV_ERROR_CODE shaderAttachData(const char* dataPtr, size_t dataLength, bool vertexType);

private:
	COMPV_DISABLE_WARNINGS_BEGIN(4251 4267)
	GLuint m_uShadVertex;
	GLuint m_uShadFragment;
	GLuint m_uPrg;
	bool m_bLinked;
	bool m_bUsed;
	COMPV_DISABLE_WARNINGS_END()
};

COMPV_NAMESPACE_END()

#endif /* HAVE_GL_GLEW_H */

#endif /* _COMPV_UI_OPENGL_PROGRAM_GL_H_ */
