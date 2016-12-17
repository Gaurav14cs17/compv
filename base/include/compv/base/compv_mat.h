/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#if !defined(_COMPV_BASE_MAT_H_)
#define _COMPV_BASE_MAT_H_

#include "compv/base/compv_config.h"
#include "compv/base/compv_buffer.h"
#include "compv/base/compv_mem.h"
#include "compv/base/image/compv_image_utils.h"

COMPV_NAMESPACE_BEGIN()
COMPV_GCC_DISABLE_WARNINGS_BEGIN("-Warray-bounds")
COMPV_GCC_DISABLE_WARNINGS_BEGIN("-Wc++11-extensions")

COMPV_OBJECT_DECLARE_PTRS(Mat)

class COMPV_BASE_API CompVMat : public CompVObj
{
protected:
    CompVMat();
public:
    virtual ~CompVMat();
	COMPV_OBJECT_GET_ID(CompVMat);

    COMPV_INLINE COMPV_MAT_TYPE type()const {
        return m_eType;
    }

    COMPV_INLINE COMPV_SUBTYPE subType()const {
        return m_eSubType;
    }

    COMPV_INLINE bool isEmpty()const {
        return !ptr() || !rows() || !cols();
    }

	COMPV_INLINE bool isPacked()const {
		return m_bPlanePacked;
	}

	COMPV_INLINE bool isPlanar()const {
		return !m_bPlanePacked;
	}

    COMPV_INLINE size_t planeCount()const {
        return static_cast<size_t>(m_nPlaneCount);
    }

	COMPV_INLINE size_t dataSizeInBytes()const {
		return m_nDataSize;
	}

    template<class ptrType = void>
    COMPV_INLINE const ptrType* ptr(size_t row = 0, size_t col = 0, int32_t planeId = 0)const {
        if (planeId >= 0 && planeId < m_nPlaneCount) {
            return (row > m_nPlaneRows[planeId] || col > m_nPlaneCols[planeId]) ? NULL : static_cast<const ptrType*>(static_cast<const uint8_t*>(m_pCompPtr[planeId]) + (row * m_nPlaneStrideInBytes[planeId]) + (col * m_nElmtInBytes));
        }
        COMPV_DEBUG_ERROR("Invalid parameter");
        return NULL;
    }

    COMPV_INLINE size_t rows(int32_t planeId = -1)const { // In samples
        if (planeId < 0) {
            return m_nRows;
        }
        else if (planeId < m_nPlaneCount) {
            return m_nPlaneRows[planeId];
        }
        COMPV_DEBUG_ERROR("Invalid parameter");
        return 0;
    }

    COMPV_INLINE size_t cols(int32_t planeId = -1)const { // In samples
        if (planeId < 0) {
            return m_nCols;
        }
        else if (planeId < m_nPlaneCount) {
            return m_nPlaneCols[planeId];
        }
        COMPV_DEBUG_ERROR("Invalid parameter");
        return 0;
    }

    COMPV_INLINE size_t elmtInBytes() {
        return m_nElmtInBytes;
    }

    COMPV_INLINE size_t rowInBytes(int32_t planeId = -1)const { // in bytes
        if (planeId < 0) {
            return m_nCols * m_nElmtInBytes;
        }
        else if (planeId < m_nPlaneCount) {
            return m_nPlaneCols[planeId] * m_nElmtInBytes;
        }
        COMPV_DEBUG_ERROR("Invalid parameter");
        return 0;
    }

    COMPV_INLINE size_t strideInBytes(int32_t planeId = -1)const { // in bytes
        if (planeId < 0) {
            return m_nStrideInBytes;
        }
        else if (planeId < m_nPlaneCount) {
            return m_nPlaneStrideInBytes[planeId];
        }
        COMPV_DEBUG_ERROR("Invalid parameter");
        return 0;
    }

    COMPV_INLINE size_t stride(int32_t planeId = -1)const { // in samples
        if (planeId < 0) {
            return m_nStrideInElts;
        }
        else if (planeId < m_nPlaneCount) {
            return m_nPlaneStrideInElts[planeId];
        }
        COMPV_DEBUG_ERROR("Invalid parameter");
        return 0;
    }

    template<class elmType = uint8_t, COMPV_MAT_TYPE dataType = COMPV_MAT_TYPE_RAW, COMPV_SUBTYPE dataSubType = COMPV_SUBTYPE_RAW>
    static COMPV_ERROR_CODE newObj(CompVMatPtrPtr mat, size_t rows, size_t cols, size_t alignv, size_t stride = 0) {
        COMPV_CHECK_EXP_RETURN(!mat || !alignv, COMPV_ERROR_CODE_E_INVALID_PARAMETER);
        CompVMatPtr mat_ = *mat;
        if (!mat_) {
            mat_ = new CompVMat();
            COMPV_CHECK_EXP_RETURN(!mat_, COMPV_ERROR_CODE_E_OUT_OF_MEMORY);
            *mat = mat_;
        }
        COMPV_CHECK_CODE_RETURN((mat_->alloc<elmType, dataType, dataSubType>(rows, cols, alignv, stride)));
        return COMPV_ERROR_CODE_S_OK;
    }

    template<class elmType = uint8_t, COMPV_MAT_TYPE dataType = COMPV_MAT_TYPE_RAW, COMPV_SUBTYPE dataSubType = COMPV_SUBTYPE_RAW>
    static COMPV_ERROR_CODE newObjStrideless(CompVMatPtrPtr mat, size_t rows, size_t cols) {
        return CompVMat::newObj<elmType, dataType, dataSubType>(mat, rows, cols, 1);
    }

    template<class elmType = uint8_t, COMPV_MAT_TYPE dataType = COMPV_MAT_TYPE_RAW, COMPV_SUBTYPE dataSubType = COMPV_SUBTYPE_RAW>
    static COMPV_ERROR_CODE newObjAligned(CompVMatPtrPtr mat, size_t rows, size_t cols) {
        return CompVMat::newObj<elmType, dataType, dataSubType>(mat, rows, cols, COMPV_SIMD_ALIGNV_DEFAULT);
    }

protected:
    template<class elmType = uint8_t, COMPV_MAT_TYPE dataType = COMPV_MAT_TYPE_RAW, COMPV_SUBTYPE dataSubType = COMPV_SUBTYPE_RAW>
    COMPV_ERROR_CODE alloc(size_t rows, size_t cols, size_t alignv = 1, size_t stride = 0) {
        COMPV_CHECK_EXP_RETURN(!alignv || (stride && stride < cols), COMPV_ERROR_CODE_E_INVALID_PARAMETER);
        COMPV_ERROR_CODE err_ = COMPV_ERROR_CODE_S_OK;

        size_t nNewDataSize;
        size_t nElmtInBytes;
        bool bPlanePacked;
        size_t nPlaneCount;
        size_t nBestStrideInBytes;
        size_t nPlaneStrideInBytes[COMPV_MAX_PLANE_COUNT];
        size_t nPlaneCols[COMPV_MAX_PLANE_COUNT];
        size_t nPlaneRows[COMPV_MAX_PLANE_COUNT];
        size_t nPlaneSizeInBytes[COMPV_MAX_PLANE_COUNT];
        void* pMem = NULL;

        nElmtInBytes = sizeof(elmType);
        nBestStrideInBytes = stride ? (stride * nElmtInBytes) : static_cast<size_t>(CompVMem::alignForward((cols * nElmtInBytes), (int)alignv));

        if (dataType == COMPV_MAT_TYPE_RAW) {
            nPlaneCount = 1;
            bPlanePacked = false;
            nNewDataSize = nBestStrideInBytes * rows;
            nPlaneCols[0] = cols;
            nPlaneRows[0] = rows;
            nPlaneStrideInBytes[0] = nBestStrideInBytes;
            nPlaneSizeInBytes[0] = nNewDataSize;
        }
        else if (dataType == COMPV_MAT_TYPE_PIXELS) {
            COMPV_CHECK_CODE_RETURN(CompVImageUtils::planeCount(dataSubType, &nPlaneCount)); // get number of comps
            COMPV_CHECK_EXP_RETURN(!nPlaneCount || nPlaneCount > COMPV_MAX_PLANE_COUNT, COMPV_ERROR_CODE_E_INVALID_PIXEL_FORMAT); // check the number of comps
            COMPV_CHECK_CODE_RETURN(CompVImageUtils::isPlanePacked(dataSubType, &bPlanePacked)); // check whether the comps are packed
            COMPV_CHECK_CODE_RETURN(CompVImageUtils::sizeForPixelFormat(dataSubType, nBestStrideInBytes, rows, &nNewDataSize)); // get overal size in bytes
            size_t nSumPlaneSizeInBytes = 0;
            for (size_t planeId = 0; planeId < nPlaneCount; ++planeId) {
                COMPV_CHECK_CODE_RETURN(CompVImageUtils::planeSizeForPixelFormat(dataSubType, planeId, cols, rows, &nPlaneCols[planeId], &nPlaneRows[planeId])); // get 'cols' and 'rows' for the comp
                COMPV_CHECK_CODE_RETURN(CompVImageUtils::planeSizeForPixelFormat(dataSubType, planeId, nBestStrideInBytes, rows, &nPlaneStrideInBytes[planeId], &nPlaneRows[planeId])); // get 'stride' and 'rows' for the comp
                COMPV_CHECK_CODE_RETURN(CompVImageUtils::planeSizeForPixelFormat(dataSubType, planeId, nBestStrideInBytes, rows, &nPlaneSizeInBytes[planeId])); // get comp size in bytes
                nSumPlaneSizeInBytes += nPlaneSizeInBytes[planeId];
            }
            // Make sure that sum(comp sizes) = data size
            COMPV_CHECK_EXP_BAIL(nSumPlaneSizeInBytes != nNewDataSize, err_ = COMPV_ERROR_CODE_E_INVALID_PIXEL_FORMAT);
        }
        else {
            COMPV_CHECK_CODE_RETURN(COMPV_ERROR_CODE_E_NOT_IMPLEMENTED);
        }

        if (nNewDataSize > m_nDataCapacity) {
            pMem = CompVMem::malloc(nNewDataSize);
            COMPV_CHECK_EXP_BAIL(!pMem, (err_ = COMPV_ERROR_CODE_E_OUT_OF_MEMORY));
            m_nDataCapacity = nNewDataSize;
            // realloc()
            if (m_bOweMem) {
                CompVMem::free((void**)&m_pDataPtr);
            }
            m_pDataPtr = pMem;
            m_bOweMem = true;
        }

        //!\\ Do not update capacity unless shorter (see above)

        m_eType = dataType;
        m_eSubType = dataSubType;
        m_nCols = cols;
        m_nRows = rows;
        m_nStrideInBytes = nBestStrideInBytes;
        m_nStrideInElts = (nBestStrideInBytes / nElmtInBytes);
        m_nDataSize = nNewDataSize;
        m_nAlignV = alignv;
        m_nElmtInBytes = nElmtInBytes;
        m_bPlanePacked = bPlanePacked;
        m_nPlaneCount = static_cast<int32_t>(nPlaneCount);
        m_pCompPtr[0] = m_pDataPtr;
        m_nPlaneSizeInBytes[0] = nPlaneSizeInBytes[0];
        m_nPlaneCols[0] = nPlaneCols[0];
        m_nPlaneRows[0] = nPlaneRows[0];
        m_nPlaneStrideInBytes[0] = nPlaneStrideInBytes[0];
        m_nPlaneStrideInElts[0] = (nPlaneStrideInBytes[0] / nElmtInBytes);
        m_bPlaneStrideInEltsIsIntegral[0] = !(nPlaneStrideInBytes[0] % nElmtInBytes);
        for (size_t planeId = 1; planeId < nPlaneCount; ++planeId) {
            m_pCompPtr[planeId] = static_cast<const uint8_t*>(m_pCompPtr[planeId - 1]) + nPlaneSizeInBytes[planeId - 1];
            m_nPlaneSizeInBytes[planeId] = nPlaneSizeInBytes[planeId];
            m_nPlaneCols[planeId] = nPlaneCols[planeId];
            m_nPlaneRows[planeId] = nPlaneRows[planeId];
            m_nPlaneStrideInBytes[planeId] = nPlaneStrideInBytes[planeId];
            m_nPlaneStrideInElts[planeId] = (nPlaneStrideInBytes[planeId] / nElmtInBytes);
            m_bPlaneStrideInEltsIsIntegral[planeId] = !(nPlaneStrideInBytes[planeId] % nElmtInBytes);
        }

bail:
        if (COMPV_ERROR_CODE_IS_NOK(err_)) {
            CompVMem::free((void**)&pMem);
        }
        return err_;
    }

private:
    void* m_pDataPtr;
    int32_t m_nPlaneCount;
    bool m_bPlanePacked;
    size_t m_nCols;
    size_t m_nRows;
    size_t m_nStrideInBytes;
    size_t m_nStrideInElts;
    size_t m_nPlaneCols[COMPV_MAX_PLANE_COUNT];
    size_t m_nPlaneRows[COMPV_MAX_PLANE_COUNT];
    const void* m_pCompPtr[COMPV_MAX_PLANE_COUNT];
    size_t m_nPlaneSizeInBytes[COMPV_MAX_PLANE_COUNT];
    size_t m_nPlaneStrideInBytes[COMPV_MAX_PLANE_COUNT];
    size_t m_nPlaneStrideInElts[COMPV_MAX_PLANE_COUNT];
    bool m_bPlaneStrideInEltsIsIntegral[COMPV_MAX_PLANE_COUNT];
    size_t m_nElmtInBytes;
    size_t m_nAlignV;
    size_t m_nDataSize;
    size_t m_nDataCapacity;
    bool m_bOweMem;
    COMPV_MAT_TYPE m_eType;
    COMPV_SUBTYPE m_eSubType;
};

COMPV_GCC_DISABLE_WARNINGS_END()
COMPV_GCC_DISABLE_WARNINGS_END()

COMPV_NAMESPACE_END()

#endif /* _COMPV_BASE_MAT_H_ */