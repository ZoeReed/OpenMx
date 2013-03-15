/*
 *  Copyright 2007-2013 The OpenMx Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

/***********************************************************
 *
 *  omxMatrix.h
 *
 *  Created: Timothy R. Brick 	Date: 2008-11-13 12:33:06
 *
 *	Contains header information for the omxMatrix class
 *   omxDataMatrices hold necessary information to simplify
 * 	dealings between the OpenMX back end and BLAS.
 *
 **********************************************************/

#ifndef _OMXMATRIX_H_
#define _OMXMATRIX_H_

#include "R.h"
#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Rdynload.h>
#include <R_ext/BLAS.h>
#include <R_ext/Lapack.h>
#include "omxDefines.h"
#include "omxBLAS.h"

typedef struct omxMatrix omxMatrix;

#include "omxAlgebra.h"
#include "omxFitFunction.h"
#include "omxExpectation.h"
#include "omxState.h"


struct omxMatrix {						// A matrix
										//TODO: Improve encapsulation
/* Actually Useful Members */
	int rows, cols;						// Matrix size  (specifically, its leading edge)
	double* data;						// Actual Data Pointer
	unsigned short colMajor;			// and column-majority.
	unsigned short hasMatrixNumber;		// is this object in the matrix or algebra arrays?
	int matrixNumber;					// the offset into the matrices or algebras arrays

/* For Memory Administrivia */
	unsigned short localData;			// If data has been malloc'd, and must be freed.

/* For aliased matrices */				// Maybe this should be a subclass, as well.
	omxMatrix* aliasedPtr;				// For now, assumes outside data if aliased.
	unsigned short originalColMajor;	// Saved for reset of aliased matrix.
	unsigned short originalRows;		// Saved for reset of aliased matrix.
	unsigned short originalCols;		// Saved for reset of aliased matrix.

/* For BLAS Multiplication Speedup */ 	// TODO: Replace some of these with inlines or macros.
	const char* majority;				// Filled by compute(), included for speed
	const char* minority;				// Filled by compute(), included for speed
	int leading;						// Leading edge; depends on original majority
	int lagging;						// Non-leading edge.

/* Curent State */
	omxState* currentState;				// Optimizer State
	unsigned short isDirty;				// Whether or not the matrix requires recomputation.
	unsigned short isTemporary;			// Whether or not to destroy the omxMatrix Structure when omxFreeAllMatrixData is called.

/* For Algebra Functions */				// At most, one of these may be non-NULL.
	omxAlgebra* algebra;				// If it's not an algebra, this is NULL.
	omxFitFunction* fitFunction;		// If it's not a fit function, this is NULL.

/* For inclusion in(or of) other matrices */
	int numPopulateLocations;
	int *populateFrom;
	int *populateFromRow, *populateFromCol;
	int *populateToRow, *populateToCol;

	const char* name;
};

/* Initialize and Destroy */
	omxMatrix* omxInitMatrix(omxMatrix* om, int nrows, int ncols, unsigned short colMajor, omxState* os);			// Set up matrix 
	omxMatrix* omxInitTemporaryMatrix(omxMatrix* om, int nrows, int ncols, unsigned short colMajor, omxState* os);	// Set up matrix that can be freed
	void omxFreeMatrixData(omxMatrix* om);							// Release any held data.
	void omxFreeAllMatrixData(omxMatrix* om);						// Ditto, traversing argument trees

/* Matrix Creation Functions */
	omxMatrix* omxNewMatrixFromMxMatrix(SEXP matrix, omxState *state); 			// Create an omxMatrix from an R MxMatrix
	omxMatrix* omxNewMatrixFromRPrimitive(SEXP rObject, omxState *state,
	unsigned short hasMatrixNumber, int matrixNumber); 							// Create an omxMatrix from an R object
	omxMatrix* omxNewIdentityMatrix(int nrows, omxState* state);				// Creates an Identity Matrix of a given size
	extern omxMatrix* omxNewMatrixFromMxIndex(SEXP matrix, omxState* os);	// Create a matrix/algebra from a matrix pointer
	extern omxMatrix* omxNewMatrixFromIndexSlot(SEXP rObj, omxState* state, char* const slotName);	// Gets a matrix from an R SEXP slot
	omxMatrix* omxDuplicateMatrix(omxMatrix* src, omxState* newState);
	SEXP omxExportMatrix(omxMatrix *om);

/* Getters 'n Setters (static functions declared below) */
	// static OMXINLINE double omxMatrixElement(omxMatrix *om, int row, int col);
	// static OMXINLINE double omxVectorElement(omxMatrix *om, int index);
	// static OMXINLINE void omxSetMatrixElement(omxMatrix *om, int row, int col, double value);
	// static OMXINLINE void omxSetVectorElement(omxMatrix *om, int index, double value);

	double omxAliasedMatrixElement(omxMatrix *om, int row, int col);			// Element from unaliased form of the same matrix
	double* omxLocationOfMatrixElement(omxMatrix *om, int row, int col);
	void omxMarkDirty(omxMatrix *om);
	void omxMarkClean(omxMatrix *om);

/* Matrix Modification Functions */
	void omxZeroByZeroMatrix(omxMatrix *source);
	void omxResizeMatrix(omxMatrix *source, int nrows, int ncols,
							unsigned short keepMemory);									// Resize, with or without re-initialization
	omxMatrix* omxFillMatrixFromMxMatrix(omxMatrix* om, SEXP matrix, omxState *state); 	// Populate an omxMatrix from an R MxMatrix
	omxMatrix* omxFillMatrixFromRPrimitive(omxMatrix* om, SEXP rObject, omxState *state,
		unsigned short hasMatrixNumber, int matrixNumber); 								// Populate an omxMatrix from an R object
	void omxProcessMatrixPopulationList(omxMatrix *matrix, SEXP matStruct);
	void omxCopyMatrix(omxMatrix *dest, omxMatrix *src);								// Copy across another matrix.
	void omxTransposeMatrix(omxMatrix *mat);												// Transpose a matrix in place.
	void omxToggleRowColumnMajor(omxMatrix *mat);										// Transform row-major into col-major and vice versa 

/* Function wrappers that switch based on inclusion of algebras */
	void omxPrint(omxMatrix *source, char* d); 											// Pretty-print a (small) matrix
	unsigned short int omxNeedsUpdate(omxMatrix *matrix);								// Does this need to be recomputed?
	void omxRecompute(omxMatrix *matrix);												// Recompute the matrix if needed.
	void omxCompute(omxMatrix *matrix);													// Recompute the matrix no matter what.

/* Aliased Matrix Functions */
	void omxAliasMatrix(omxMatrix *alias, omxMatrix* const source);		// Allows aliasing for faster reset.
	void omxResetAliasedMatrix(omxMatrix *matrix);						// Reset to the original matrix
	void omxRemoveElements(omxMatrix *om, int numRemoved, int removed[]);
	void omxRemoveRowsAndColumns(omxMatrix* om, int numRowsRemoved, int numColsRemoved, int rowsRemoved[], int colsRemoved[]);

/* Matrix-Internal Helper functions */
	void omxMatrixLeadingLagging(omxMatrix *matrix);
	void omxPrintMatrix(omxMatrix *source, char* d);                    // Pretty-print a (small) matrix
	unsigned short int omxMatrixNeedsUpdate(omxMatrix *matrix);

/* OMXINLINE functions and helper functions */

void setMatrixError(omxMatrix *om, int row, int col, int numrow, int numcol);
void setVectorError(int index, int numrow, int numcol);
void matrixElementError(int row, int col, int numrow, int numcol);
void vectorElementError(int index, int numrow, int numcol);

static OMXINLINE int omxIsMatrix(omxMatrix *mat) {
    return (mat->algebra == NULL && mat->fitFunction == NULL);
}

/* BLAS Wrappers */

static OMXINLINE void omxSetMatrixElement(omxMatrix *om, int row, int col, double value) {
	if((row < 0) || (col < 0) || (row >= om->rows) || (col >= om->cols)) {
		setMatrixError(om, row + 1, col + 1, om->rows, om->cols);
		return;
	}
	int index = 0;
	if(om->colMajor) {
		index = col * om->rows + row;
	} else {
		index = row * om->cols + col;
	}
	om->data[index] = value;
}

static OMXINLINE void omxAccumulateMatrixElement(omxMatrix *om, int row, int col, double value) {
        if((row < 0) || (col < 0) || (row >= om->rows) || (col >= om->cols)) {
                setMatrixError(om, row + 1, col + 1, om->rows, om->cols);
                return;
        }
        int index = 0;
        if(om->colMajor) {
                index = col * om->rows + row;
        } else {
                index = row * om->cols + col;
        }
        om->data[index] += value;
}

static OMXINLINE double omxMatrixElement(omxMatrix *om, int row, int col) {
	int index = 0;
	if((row < 0) || (col < 0) || (row >= om->rows) || (col >= om->cols)) {
		matrixElementError(row + 1, col + 1, om->rows, om->cols);
        return (NA_REAL);
	}
	if(om->colMajor) {
		index = col * om->rows + row;
	} else {
		index = row * om->cols + col;
	}
	return om->data[index];
}

static OMXINLINE void omxAccumulateVectorElement(omxMatrix *om, int index, double value) {
	if (index < 0 || index >= (om->rows * om->cols)) {
		setVectorError(index + 1, om->rows, om->cols);
		return;
	} else {
		om->data[index] += value;
    }
}

static OMXINLINE void omxSetVectorElement(omxMatrix *om, int index, double value) {
	if (index < 0 || index >= (om->rows * om->cols)) {
		setVectorError(index + 1, om->rows, om->cols);
		return;
	} else {
		om->data[index] = value;
    }
}

static OMXINLINE double omxVectorElement(omxMatrix *om, int index) {
	if (index < 0 || index >= (om->rows * om->cols)) {
		vectorElementError(index + 1, om->rows, om->cols);
        return (NA_REAL);
	} else {
		return om->data[index];
    }
}

static OMXINLINE void omxUnsafeSetVectorElement(omxMatrix *om, int index, double value) {
	om->data[index] = value;
}

static OMXINLINE double omxUnsafeVectorElement(omxMatrix *om, int index) {
	return om->data[index];
}


static OMXINLINE void omxDGEMM(unsigned short int transposeA, unsigned short int transposeB,		// result <- alpha * A %*% B + beta * C
				double alpha, omxMatrix* a, omxMatrix *b, double beta, omxMatrix* result) {
	int nrow = (transposeA?a->cols:a->rows);
	int nmid = (transposeA?a->rows:a->cols);
	int ncol = (transposeB?b->rows:b->cols);

	F77_CALL(omxunsafedgemm)((transposeA?a->minority:a->majority), (transposeB?b->minority:b->majority), 
							&(nrow), &(ncol), &(nmid),
							&alpha, a->data, &(a->leading), 
							b->data, &(b->leading),
							&beta, result->data, &(result->leading));

	if(!result->colMajor) omxToggleRowColumnMajor(result);
}

static OMXINLINE void omxDGEMV(unsigned short int transposeMat, double alpha, omxMatrix* mat,	// result <- alpha * A %*% B + beta * C
				omxMatrix* vec, double beta, omxMatrix*result) {							// where B is treated as a vector
	int onei = 1;
	int nrows = mat->rows;
	int ncols = mat->cols;
	if(OMX_DEBUG_DEVELOPER) {
		int nVecEl = vec->rows * vec->cols;
		// Rprintf("DGEMV: %c, %d, %d, %f, 0x%x, %d, 0x%x, %d, 0x%x, %d\n", *(transposeMat?mat->minority:mat->majority), (nrows), (ncols), 
        	// alpha, mat->data, (mat->leading), vec->data, onei, beta, result->data, onei); //:::DEBUG:::
		if((transposeMat && nrows != nVecEl) || (!transposeMat && ncols != nVecEl)) {
			Rprintf("Mismatch in vector/matrix multiply: %s (%d x %d) * (%d x 1).\n", (transposeMat?"transposed":""), mat->rows, mat->cols, nVecEl); // :::DEBUG:::
		}
	}
	F77_CALL(omxunsafedgemv)((transposeMat?mat->minority:mat->majority), &(nrows), &(ncols), 
	        &alpha, mat->data, &(mat->leading), vec->data, &onei, &beta, result->data, &onei);
	if(!result->colMajor) omxToggleRowColumnMajor(result);
}

static OMXINLINE void omxDSYMV(double alpha, omxMatrix* mat,            // result <- alpha * A %*% B + beta * C
				omxMatrix* vec, double beta, omxMatrix* result) {       // only A is symmetric, and B is a vector
	char u='U';
    int onei = 1;

	if(OMX_DEBUG_DEVELOPER) {
		int nVecEl = vec->rows * vec->cols;
		// Rprintf("DSYMV: %c, %d, %f, 0x%x, %d, 0x%x, %d, %f, 0x%x, %d\n", u, (mat->cols),alpha, mat->data, (mat->leading), 
	                    // vec->data, onei, beta, result->data, onei); //:::DEBUG:::
		if(mat->cols != nVecEl) {
			Rprintf("Mismatch in symmetric vector/matrix multiply: %s (%d x %d) * (%d x 1).\n", "symmetric", mat->rows, mat->cols, nVecEl); // :::DEBUG:::
		}
	}

    F77_CALL(dsymv)(&u, &(mat->cols), &alpha, mat->data, &(mat->leading), 
                    vec->data, &onei, &beta, result->data, &onei);

    // if(!result->colMajor) omxToggleRowColumnMajor(result);
}

static OMXINLINE void omxDSYMM(unsigned short int symmOnLeft, double alpha, omxMatrix* symmetric, 		// result <- alpha * A %*% B + beta * C
				omxMatrix *other, double beta, omxMatrix* result) {	                            // One of A or B is symmetric

	char r='R', l = 'L';
	char u='U';
	F77_CALL(dsymm)((symmOnLeft?&l:&r), &u, &(result->rows), &(result->cols),
					&alpha, symmetric->data, &(symmetric->leading),
 					other->data, &(other->leading),
					&beta, result->data, &(result->leading));

	if(!result->colMajor) omxToggleRowColumnMajor(result);
}

static OMXINLINE int omxDGETRF(omxMatrix* mat, int* ipiv) {										// LUP decomposition of mat
	int info = 0;
	F77_CALL(dgetrf)(&(mat->rows), &(mat->cols), mat->data, &(mat->leading), ipiv, &info);
	return info;
}

static OMXINLINE int omxDGETRI(omxMatrix* mat, int* ipiv, double* work, int lwork) {				// Invert mat from LUP decomposition
	int info = 0;
	F77_CALL(dgetri)(&(mat->rows), mat->data, &(mat->leading), ipiv, work, &lwork, &info);
	return info;
}

static OMXINLINE void omxDAXPY(double alpha, omxMatrix* lhs, omxMatrix* rhs) {              // RHS += alpha*lhs  
    // N.B.  Not fully tested.                                                              // Assumes common majority or vectordom.
    if(lhs->colMajor != rhs->colMajor) { omxToggleRowColumnMajor(rhs);}
    int len = lhs->rows * lhs->cols;
    int onei = 1;
    F77_CALL(daxpy)(&len, &alpha, lhs->data, &onei, rhs->data, &onei);

}

static OMXINLINE double omxDDOT(omxMatrix* lhs, omxMatrix* rhs) {              // returns dot product, as if they were vectors
    // N.B.  Not fully tested.                                                  // Assumes common majority or vectordom.
    if(lhs->colMajor != rhs->colMajor) { omxToggleRowColumnMajor(rhs);}
    int len = lhs->rows * lhs->cols;
    int onei = 1;
    return(F77_CALL(ddot)(&len, lhs->data, &onei, rhs->data, &onei));
}

static OMXINLINE void omxDPOTRF(omxMatrix* mat, int* info) {										// Cholesky decomposition of mat
	// TODO: Add error checking, and/or adjustments for row vs. column majority.
	// N.B. Not fully tested.
	char u = 'U'; //l = 'L'; //U for storing upper triangle
	F77_CALL(dpotrf)(&u, &(mat->rows), mat->data, &(mat->cols), info);
}
static OMXINLINE void omxDPOTRI(omxMatrix* mat, int* info) {										// Invert mat from Cholesky
	// TODO: Add error checking, and/or adjustments for row vs. column majority.
	// N.B. Not fully tested.
	char u = 'U'; //l = 'L'; // U for storing upper triangle
	F77_CALL(dpotri)(&u, &(mat->rows), mat->data, &(mat->cols), info);
}

void omxShallowInverse(int numIters, omxMatrix* A, omxMatrix* Z, omxMatrix* Ax, omxMatrix* I );

#endif /* _OMXMATRIX_H_ */
