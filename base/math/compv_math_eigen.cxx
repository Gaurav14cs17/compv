/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#include "compv/base/math/compv_math_eigen.h"
#include "compv/base/math/compv_math_matrix.h"
#include "compv/base/math/compv_math_utils.h"

#define COMPV_THIS_CLASSNAME	"CompVMathEigen"

#if !defined(COMPV_MATH_EIGEN_EPSILON)
#	define COMPV_MATH_EIGEN_DOUBLE_EPSILON		1.192092896e-07 // FLT_EPSILON TODO(dmi): set to DBL_EPSILON
#	define COMPV_MATH_EIGEN_FLOAT_EPSILON		1.192092896e-04 // TODO(dmi): set to FLT_EPSILON
#endif /* COMPV_MATH_EIGEN_EPSILON */

#if !defined(COMPV_MATH_EIGEN_MAX_ROUNDS)
#	define COMPV_MATH_EIGEN_MAX_ROUNDS 30 // should be 30
#endif

COMPV_NAMESPACE_BEGIN()

// S: an (n x n) symmetric matrix
// D: a (n x n) diagonal matrix containing the eigenvalues
// Q: an (n x n) matrix containing the eigenvectors (columns unless transposed)
// rowVectors: true -> eigenvectors are rows, otherwise it's columns. True is faster.
// sort: Whether to sort the eigenvalues and eigenvectors (from higher to lower)
template <class T>
COMPV_ERROR_CODE CompVMathEigen<T>::findSymm(const CompVMatPtr &S, CompVMatPtrPtr D, CompVMatPtrPtr Q, bool sort COMPV_DEFAULT(true), bool rowVectors COMPV_DEFAULT(false), bool forceZerosInD COMPV_DEFAULT(true))
{
	COMPV_CHECK_EXP_RETURN(!S || !D || !Q || !S->rows() || S->rows() != S->cols(), COMPV_ERROR_CODE_E_INVALID_PARAMETER);
	COMPV_ERROR_CODE err_ = COMPV_ERROR_CODE_S_OK;

	COMPV_DEBUG_INFO_CODE_NOT_OPTIMIZED("No MT implementation");

#if defined(_DEBUG) || defined(DEBUG)
	if (S->cols() == 3 && S->rows() == 3) {
		// https://github.com/DoubangoTelecom/compv/issues/85
		COMPV_DEBUG_INFO_CODE_NOT_OPTIMIZED("No fast implementation for 3x3 matrix");
	}
#endif

	size_t row, col;
	T gcos_, gsin_;
	const T epsilon_ = CompVMathEigen<T>::epsilon();
	size_t ops = 0, maxops = S->rows() * S->cols() * COMPV_MATH_EIGEN_MAX_ROUNDS;
	T maxOffDiag;
	CompVMatPtr Qt;
	CompVMatPtr GD_2rows;
	CompVMatPtr D_ = *D;
	bool transpose = !rowVectors;

	// Qt = I
	if (rowVectors) {
		COMPV_CHECK_CODE_RETURN(CompVMatrix::identity<T>(Q, S->rows(), S->cols()));
		Qt = *Q;
	}
	else {
		COMPV_CHECK_CODE_RETURN(CompVMatrix::identity<T>(&Qt, S->rows(), S->cols()));
	}
	// D = S
	COMPV_CHECK_CODE_RETURN(CompVMatrix::copy(&D_, S));
	// Check is S is already diagonal or not
	COMPV_CHECK_CODE_RETURN(CompVMatrix::maxAbsOffDiag_symm<T>(S, &row, &col, &maxOffDiag));
	if (maxOffDiag < epsilon_) { // S already diagonal -> D = S, Q = I
		COMPV_DEBUG_INFO_EX(COMPV_THIS_CLASSNAME, "Symmetric matrix already diagonal -> do nothing");
		goto done;
	}

	// If matrix A is symmetric then, mulAG(c, s) = mulGA(c, -s)

	// TODO(dmi): For multithreading, change 'maxAbsOffDiag_symm' to add max rows and use it as guard

	// TODO(dmi): Add JacobiAngles_Left() function to be used in mulGA() only

	// Change D = GtDG :
	// D = GtDG = Gt(GtDt)t

	// TODO(dmi): add mulGA9x9, transposeA9x9
	// Homography and Fundamental matrices are 3x3 which means we will be frequently working with 9x9 eigenvectors/eigenvalues matrices (Qt and D)
	COMPV_DEBUG_INFO_CODE_NOT_OPTIMIZED("Homography and Fundamental matrices are 3x3 which means we will be frequently working with 9x9 eigenvectors/eigenvalues matrices (Qt and D)");

	// TODO(dmi): Moments, replace vpextrd r32, xmm, 0 with vmod r32, xmm

	// Instead of returning Q = QG, return Qt, Qt = GtQt

	COMPV_CHECK_CODE_RETURN(CompVMat::newObjAligned<T>(&GD_2rows, 2, D_->rows()));
	do {
		CompVMathEigen<T>::jacobiAngles(D_, row, col, &gcos_, &gsin_);
		// COMPV_DEBUG_INFO_EX(COMPV_THIS_CLASSNAME, "A(%d) = %d, %d, %f, %f", ops, row, col, gcos_, gsin_);
		// Qt = G*Qt
		COMPV_CHECK_CODE_RETURN(CompVMatrix::mulGA<T>(Qt, row, col, gcos_, -gsin_)); // Thread-safe
		// GtDt
		CompVMathEigen<T>::extract2Cols(D_, row, col, GD_2rows); // GD_2rows = Dt
		COMPV_CHECK_CODE_RETURN(CompVMatrix::mulGA<T>(GD_2rows, 0, 1, gcos_, -gsin_)); // Thread-safe
		// COMPV_DEBUG_INFO_EX(COMPV_THIS_CLASSNAME, "B0(%d) = %f, %f, %f", ops, *GD_2rows->ptr(0, 0), *GD_2rows->ptr(0, 1), *GD_2rows->ptr(0, 2));
		// COMPV_DEBUG_INFO_EX(COMPV_THIS_CLASSNAME, "B1(%d) = %f, %f, %f", ops, *GD_2rows->ptr(1, 0), *GD_2rows->ptr(1, 1), *GD_2rows->ptr(1, 2));
		// Gt(GtDt)t
		CompVMathEigen<T>::insert2Cols(GD_2rows, D_, row, col); // GD_2rows = (GtDt)t
		COMPV_CHECK_CODE_RETURN(CompVMatrix::mulGA<T>(D_, row, col, gcos_, -gsin_)); // Thread-safe
		// COMPV_DEBUG_INFO_EX(COMPV_THIS_CLASSNAME, "C0(%d) = %f, %f, %f", ops, *GD_2rows->ptr(0, 0), *GD_2rows->ptr(0, 1), *GD_2rows->ptr(0, 2));
		// COMPV_DEBUG_INFO_EX(COMPV_THIS_CLASSNAME, "C1(%d) = %f, %f, %f", ops, *GD_2rows->ptr(1, 0), *GD_2rows->ptr(1, 1), *GD_2rows->ptr(1, 2));
	} while (++ops < maxops &&  COMPV_ERROR_CODE_IS_OK(err_ = CompVMatrix::maxAbsOffDiag_symm<T>(D_, &row, &col, &maxOffDiag)) && maxOffDiag > epsilon_); // TODO(dmi): for MT, see above comment

	// Sort Qt (eigenvectors are rows)
	if (sort) {
		size_t eigenValuesCount = D_->cols(), index, oldIndex;
		CompVMatPtr Idx;
		COMPV_CHECK_CODE_RETURN(CompVMat::newObjAligned<size_t>(&Idx, 1, eigenValuesCount));
		size_t* indexes = Idx->ptr<size_t>();
		for (size_t i = 0; i < eigenValuesCount; ++i) {
			indexes[i] = i;
		}
		bool sorted, wasSorted = true;
		do {
			sorted = true;
			for (size_t i = 0; i < eigenValuesCount - 1; ++i) {
				index = indexes[i];
				if (*D_->ptr<T>(indexes[i], indexes[i]) < *D_->ptr<T>(indexes[i + 1], indexes[i + 1])) {
					oldIndex = indexes[i];
					indexes[i] = indexes[i + 1];
					indexes[i + 1] = oldIndex;
					sorted = false;
					wasSorted = false;
				}
			}
		} while (!sorted);

		if (!wasSorted) {
			COMPV_DEBUG_INFO_CODE_NOT_TESTED();
			CompVMatPtr Dsorted, Qsorted;
			COMPV_CHECK_CODE_RETURN(CompVMatrix::zero<T>(&Dsorted, D_->rows(), D_->cols()));
			COMPV_CHECK_CODE_RETURN(CompVMat::newObjAligned<T>(&Qsorted, S->rows(), S->cols()));
			for (size_t i = 0; i < eigenValuesCount; ++i) {
				*Dsorted->ptr<T>(i, i) = *D_->ptr<const T>(indexes[i], indexes[i]);
				COMPV_CHECK_CODE_RETURN(CompVMem::copy(Qsorted->ptr<T>(i), Qt->ptr<T>(indexes[i]), Qsorted->rowInBytes()));
			}
			D_ = Dsorted;
			if (transpose) {
				COMPV_CHECK_CODE_RETURN(CompVMatrix::transpose(Qsorted, Q));
				transpose = false; // to avoid transpose after the done
			}
			else {
				*Q = Qsorted;
			}
		}
	}

done:
	if (transpose) {
		COMPV_CHECK_CODE_RETURN(CompVMatrix::transpose(Qt, Q));
	}

	// Off-diagonal values in D contains epsilons which is close to zero but not equal to zero
	if (forceZerosInD) {
		T* row;
		for (size_t j = 0; j < D_->rows(); ++j) {
			row = D_->ptr<T>(j);
			for (size_t i = 0; i < D_->cols(); ++i) {
				if (i == j) {
					if (CompVMathEigen<T>::isCloseToZero(row[i])) {
						row[i] = 0;
					}
				}
				else {
					row[i] = 0;
				}
			}
		}
	}

	if (ops >= maxops) {
		COMPV_DEBUG_ERROR_EX(COMPV_THIS_CLASSNAME, "ops(%zu) >= maxops(%zu). Using 'double': %s", ops, maxops, std::is_same<T, compv_float64_t>::value ? "true" : "false");
	}

	*D = D_;

	return err_;
}

template <class T>
T CompVMathEigen<T>::epsilon()
{
	return (T)(std::is_same<T, compv_float64_t>::value ? COMPV_MATH_EIGEN_DOUBLE_EPSILON : COMPV_MATH_EIGEN_FLOAT_EPSILON);
}

template <class T>
bool CompVMathEigen<T>::isCloseToZero(T a)
{
	return (static_cast<T>(COMPV_MATH_ABS(a)) <= CompVMathEigen<T>::epsilon());
}

// Compute cos('c') and sin ('s')
template <class T>
void CompVMathEigen<T>::jacobiAngles(const CompVMatPtr &S, size_t ith, size_t jth, T *c, T *s)
{
	// From https://en.wikipedia.org/wiki/Jacobi_eigenvalue_algorithm#Description
	// https://github.com/DoubangoTelecom/compv/issues/86
#if 1
	T Sii = *S->ptr<T>(ith, ith);
	T Sjj = *S->ptr<T>(jth, jth);
	if (Sii == Sjj) {
		// theta = PI/4
		*c = static_cast<T>(0.70710678118654757); // :: cos(PI/4)
		*s = static_cast<T>(0.70710678118654757); // :: sin(PI/4)
	}
	else {
		T theta = static_cast<T>(0.5) * static_cast<T>(COMPV_MATH_ATAN2(2.0 * *S->ptr<T>(ith, jth), Sjj - Sii));
		*c = static_cast<T>(COMPV_MATH_COS(theta));
		*s = static_cast<T>(COMPV_MATH_SIN(theta));
	}
#else
	// Not correct
	// Using SQRT function isn't faster than using cos(), sin() - At least on Win64
	COMPV_DEBUG_INFO_CODE_FOR_TESTING();
	T Sii = *S->ptr(ith, ith);
	T Sjj = *S->ptr(jth, jth);
	T Sij = *S->ptr(ith, jth);
	T b = (Sjj - Sii) / Sij;
	T b2 = b*b;
	T tan0 = (T)(-b + sqrt(b2 + 4)) / 2;
	T tan1 = (T)(-b - sqrt(b2 + 4)) / 2;
	T tan = (T)(abs(tan0) > abs(tan1) ? tan0 : tan1);
	T tan2 = tan * tan;
	*c = (T)sqrt(1 / (1 + tan2));
	*s = (*c * tan);
#endif
}

// Compute cos('c') and sin ('s')
// c and s can only be used for left-multiply (mulGA). Cannot be used for right multiply (mulAG)
// To be used for QR decomposion (https://en.wikipedia.org/wiki/QR_decomposition)
template <class T>
void CompVMathEigen<T>::jacobiAngles_Left(const CompVMatPtr &S, size_t ith, size_t jth, T *c, T *s)
{
	COMPV_DEBUG_INFO_CODE_NOT_TESTED("Not tested and not used yet!!");
	// From https://en.wikipedia.org/wiki/Givens_rotation#Stable_calculation
	T a = *S->ptr<T>(ith);
	T b = *S->ptr<T>(jth);
	T r = CompVMathUtils::hypot<T>(a, b);
	*c = a / r;
	*s = -b / r;
}

// Extract 2 cols from A and insert as rows to R
template <class T>
void CompVMathEigen<T>::extract2Cols(const CompVMatPtr &A, size_t a_col0, size_t a_col1, CompVMatPtr &R)
{
	// Private function -> do not check input parameters
	T* r0 = R->ptr<T>(0);
	T* r1 = R->ptr<T>(1);
	const T* a0 = A->ptr<const T>(0, a_col0);
	const T* a1 = A->ptr<const T>(0, a_col1);
	size_t astrideInElts;
	COMPV_CHECK_CODE_ASSERT(A->strideInElts(astrideInElts));
	const size_t rows_ = A->rows();
	switch (rows_) {
	case 1:
		r0[0] = a0[0];
		r1[0] = a1[0];
		break;
	case 2:
		r0[0] = a0[0];
		r0[1] = a0[astrideInElts];
		r1[0] = a1[0];
		r1[1] = a1[astrideInElts];
		break;
	case 3:
		r0[0] = a0[0];
		r0[1] = a0[astrideInElts];
		r0[2] = a0[astrideInElts << 1];
		r1[0] = a1[0];
		r1[1] = a1[astrideInElts];
		r1[2] = a1[astrideInElts << 1];
		break;
	default:
		size_t row_, aidx_, rowminus3 = rows_ - 3;
		size_t astrideInEltsTimes2 = astrideInElts << 1;
		size_t astrideInEltsTimes3 = astrideInEltsTimes2 + astrideInElts;
		size_t astrideInEltsTimes4 = astrideInEltsTimes3 + astrideInElts;
		for (row_ = 0, aidx_ = 0; row_ < rowminus3; row_ += 4, aidx_ += astrideInEltsTimes4) {
			r0[row_] = a0[aidx_];
			r0[row_ + 1] = a0[aidx_ + astrideInElts];
			r0[row_ + 2] = a0[aidx_ + astrideInEltsTimes2];
			r0[row_ + 3] = a0[aidx_ + astrideInEltsTimes3];
			r1[row_] = a1[aidx_];
			r1[row_ + 1] = a1[aidx_ + astrideInElts];
			r1[row_ + 2] = a1[aidx_ + astrideInEltsTimes2];
			r1[row_ + 3] = a1[aidx_ + astrideInEltsTimes3];
		}
		for (; row_ < rows_; ++row_, aidx_ += astrideInElts) {
			r0[row_] = a0[aidx_];
			r1[row_] = a1[aidx_];
		}
		break;
	}
}

template <class T>
void CompVMathEigen<T>::insert2Cols(const CompVMatPtr &A, CompVMatPtr &R, size_t r_col0, size_t r_col1)
{
	// Private function -> do not check input parameters
	const T* a0 = A->ptr<const T>(0);
	const T* a1 = A->ptr<const T>(1);
	T* r0 = R->ptr<T>(0, r_col0);
	T* r1 = R->ptr<T>(0, r_col1);
	size_t rstrideInElts;
	COMPV_CHECK_CODE_ASSERT(R->strideInElts(rstrideInElts));
	const size_t rows_ = R->rows();
	switch (rows_) {
	case 1:
		r0[0] = a0[0];
		r1[0] = a1[0];
		break;
	case 2:
		r0[0] = a0[0];
		r0[rstrideInElts] = a0[1];
		r1[0] = a1[0];
		r1[rstrideInElts] = a1[1];
		break;
	case 3:
		r0[0] = a0[0];
		r0[rstrideInElts] = a0[1];
		r0[rstrideInElts << 1] = a0[2];
		r1[0] = a1[0];
		r1[rstrideInElts] = a1[1];
		r1[rstrideInElts << 1] = a1[2];
		break;
	default:
		size_t row_, ridx_, rowminus3 = rows_ - 3;
		size_t rstrideInEltsTimes2 = rstrideInElts << 1;
		size_t rstrideInEltsTimes3 = rstrideInEltsTimes2 + rstrideInElts;
		size_t rstrideInEltsTimes4 = rstrideInEltsTimes3 + rstrideInElts;
		for (row_ = 0, ridx_ = 0; row_ < rowminus3; row_ += 4, ridx_ += rstrideInEltsTimes4) {
			r0[ridx_] = a0[row_];
			r0[ridx_ + rstrideInElts] = a0[row_ + 1];
			r0[ridx_ + rstrideInEltsTimes2] = a0[row_ + 2];
			r0[ridx_ + rstrideInEltsTimes3] = a0[row_ + 3];

			r1[ridx_] = a1[row_];
			r1[ridx_ + rstrideInElts] = a1[row_ + 1];
			r1[ridx_ + rstrideInEltsTimes2] = a1[row_ + 2];
			r1[ridx_ + rstrideInEltsTimes3] = a1[row_ + 3];
		}
		for (; row_ < rows_; ++row_, ridx_ += rstrideInElts) {
			r0[ridx_] = a0[row_];
			r1[ridx_] = a1[row_];
		}
		break;
	}
}

#if 0
template class CompVMathEigen<int8_t>;
template class CompVMathEigen<uint8_t>;
template class CompVMathEigen<int16_t>;
template class CompVMathEigen<uint16_t>;
template class CompVMathEigen<int32_t>;
template class CompVMathEigen<uint32_t>;
#endif
template class CompVMathEigen<compv_float32_t>;
template class CompVMathEigen<compv_float64_t>;

COMPV_NAMESPACE_END()