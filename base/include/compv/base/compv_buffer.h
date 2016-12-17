/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#if !defined(_COMPV_BASE_BUFFER_H_)
#define _COMPV_BASE_BUFFER_H_

#include "compv/base/compv_config.h"
#include "compv/base/compv_obj.h"
#include "compv/base/compv_common.h"

COMPV_NAMESPACE_BEGIN()

COMPV_OBJECT_DECLARE_PTRS(Buffer)

class COMPV_BASE_API CompVBuffer : public CompVObj
{
protected:
    CompVBuffer(const void* pcPtr = NULL, int32_t size = 0);
public:
    virtual ~CompVBuffer();
	COMPV_OBJECT_GET_ID(CompVBuffer);

    COMPV_ERROR_CODE copyData(const void* pcPtr, int32_t size);
    COMPV_ERROR_CODE refData(const void* pcPtr, int32_t size);
    COMPV_ERROR_CODE takeData(void** ppPtr, int32_t size);
    COMPV_INLINE const void* ptr() {
        return m_pPtr;
    }
    COMPV_INLINE int32_t size() {
        return m_nSize;
    }
    COMPV_INLINE bool isEmpty() {
        return !size() || !ptr();
    }
    static COMPV_ERROR_CODE newObj(const void* pcPtr, int32_t size, CompVBufferPtrPtr buffer);
    static COMPV_ERROR_CODE newObjAndNullData(CompVBufferPtrPtr buffer);
    static COMPV_ERROR_CODE newObjAndTakeData(void** ppPtr, int32_t size, CompVBufferPtrPtr buffer);
    static COMPV_ERROR_CODE newObjAndCopyData(const void* pcPtr, int32_t size, CompVBufferPtrPtr buffer);
    static COMPV_ERROR_CODE newObjAndRefData(const void* pcPtr, int32_t size, CompVBufferPtrPtr buffer);

private:
    void* m_pPtr;
    int32_t m_nSize;
    bool m_bOweMem;
};

COMPV_NAMESPACE_END()

#endif /* _COMPV_BASE_BUFFER_H_ */