/* 
 * A header-only version of RedSVD
 * 
 * Copyright (c) 2014 Nicolas Tessore
 * 
 * based on RedSVD
 * 
 * Copyright (c) 2010 Daisuke Okanohara
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above Copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above Copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the authors nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 */

#ifndef REDSVD_MODULE_H
#define REDSVD_MODULE_H

#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

#include <cstdlib>
#include <cmath>

namespace RedSVD
{
	template<typename Scalar>
	inline void sample_gaussian(Scalar& x, Scalar& y)
	{
		using std::sqrt;
		using std::log;
		using std::cos;
		using std::sin;
		
		const Scalar PI(3.1415926535897932384626433832795028841971693993751);
		
		Scalar v1 = (Scalar)(std::rand() + Scalar(1)) / ((Scalar)RAND_MAX+Scalar(2));
		Scalar v2 = (Scalar)(std::rand() + Scalar(1)) / ((Scalar)RAND_MAX+Scalar(2));
		Scalar len = sqrt(Scalar(-2) * log(v1));
		x = len * cos(Scalar(2) * PI * v2);
		y = len * sin(Scalar(2) * PI * v2);
	}
	
	template<typename MatrixType>
	inline void sample_gaussian(MatrixType& mat)
	{
		typedef typename MatrixType::Index Index;
		
		for(Index i = 0; i < mat.rows(); ++i)
		{
			for(Index j = 0; j+1 < mat.cols(); j += 2)
				sample_gaussian(mat(i, j), mat(i, j+1));
			if(mat.cols() % 2)
				sample_gaussian(mat(i, mat.cols()-1), mat(i, mat.cols()-1));
		}
	}
	
	template<typename MatrixType>
	inline void gram_schmidt(MatrixType& mat)
	{
		typedef typename MatrixType::Scalar Scalar;
		typedef typename MatrixType::Index Index;
		
		static const Scalar EPS(1E-4);
		
		for(Index i = 0; i < mat.cols(); ++i)
		{
			for(Index j = 0; j < i; ++j)
			{
				Scalar r = mat.col(i).dot(mat.col(j));
				mat.col(i) -= r * mat.col(j);
			}
			
			Scalar norm = mat.col(i).norm();
			
			if(norm < EPS)
			{
				for(Index k = i; k < mat.cols(); ++k)
					mat.col(k).setZero();
				return;
			}
			mat.col(i) /= norm;
		}
	}
	
	template<typename _MatrixType>
	class RedSVD
	{
	public:
		typedef _MatrixType MatrixType;
		typedef typename MatrixType::Scalar Scalar;
		typedef typename MatrixType::Index Index;
		typedef typename Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> DenseMatrix;
		typedef typename Eigen::Matrix<Scalar, Eigen::Dynamic, 1> ScalarVector;
		
		RedSVD() {}
		
		RedSVD(const MatrixType& A)
		{
			int r = (A.rows() < A.cols()) ? A.rows() : A.cols();
			compute(A, r);
		}
		
		RedSVD(const MatrixType& A, const Index rank)
		{
			compute(A, rank);
		}
		
		void compute(const MatrixType& A, const Index rank)
		{
		        DenseMatrix Z;
		        DenseMatrix Y;
		        const Eigen::BDCSVD<DenseMatrix> &svdOfC = this->compute_svd(A, rank, &Z, &Y);
			m_matrixU = std::move(Z * svdOfC.matrixU());
			m_vectorS = std::move(svdOfC.singularValues());
			m_matrixV = std::move(Y * svdOfC.matrixV());
		}
		
	        void compute_V(const MatrixType& A, const Index rank)
		{
		        DenseMatrix Z;
		        DenseMatrix Y;
		        const Eigen::BDCSVD<DenseMatrix> &svdOfC = this->compute_svd(A, rank, &Z, &Y);
			m_vectorS = std::move(svdOfC.singularValues());
			m_matrixV = std::move(Y * svdOfC.matrixV());
		}

	        void compute_U(const MatrixType& A, const Index rank)
		{
		        DenseMatrix Z;
			DenseMatrix Y;
		        const Eigen::BDCSVD<DenseMatrix> &svdOfC = this->compute_svd(A, rank, &Z, &Y);
			m_vectorS = std::move(svdOfC.singularValues());
			m_matrixU = std::move(Z * svdOfC.matrixU());
		}

	        void compute_singularValues(const MatrixType& A, const Index rank)
		{
		        const Eigen::BDCSVD<DenseMatrix> &svdOfC = this->compute_svd(A, rank);
			// C = USV^T
			// A = Z * U * S * V^T * Y^T()
			m_vectorS = std::move(svdOfC.singularValues());
		}

	        const DenseMatrix& matrixU() const { return m_matrixU; }
		const ScalarVector& singularValues() const { return m_vectorS; }
		const DenseMatrix& matrixV() const { return m_matrixV; }
	        DenseMatrix& matrixU() { return m_matrixU; }
		ScalarVector& singularValues() { return m_vectorS; }
		DenseMatrix& matrixV() { return m_matrixV; }
	private:
		DenseMatrix m_matrixU;
		ScalarVector m_vectorS;
		DenseMatrix m_matrixV;

	        Eigen::BDCSVD<DenseMatrix> compute_svd(const MatrixType& A, const Index rank, DenseMatrix *Z, DenseMatrix *Y)
		{
		        if(A.cols() == 0 || A.rows() == 0) {}
			    // TODO throw error;

			Index r = (rank < A.cols()) ? rank : A.cols();

			r = (r < A.rows()) ? r : A.rows();

			// Gaussian Random Matrix for A^T
			DenseMatrix O(A.rows(), r);
			sample_gaussian(O);

			// Compute Sample Matrix of A^T
			*Y = A.transpose() * O;

			// Orthonormalize Y
			gram_schmidt(*Y);

			// Range(B) = Range(A^T)
			DenseMatrix B = A * (*Y);

			// Gaussian Random Matrix
			DenseMatrix P(B.cols(), r);
			sample_gaussian(P);

			// Compute Sample Matrix of B
			*Z = B * P;

			// Orthonormalize Z
			gram_schmidt(*Z);

			// Range(C) = Range(B)
			DenseMatrix C = Z->transpose() * B;

			// C = USV^T
			// A = Z * U * S * V^T * Y^T()
			Eigen::BDCSVD<DenseMatrix> svdOfC(C, Eigen::ComputeThinU | Eigen::ComputeThinV);
			return svdOfC;

		}
	};
	
	template<typename _MatrixType>
	class RedSymEigen
	{
	public:
		typedef _MatrixType MatrixType;
		typedef typename MatrixType::Scalar Scalar;
		typedef typename MatrixType::Index Index;
		typedef typename Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> DenseMatrix;
		typedef typename Eigen::Matrix<Scalar, Eigen::Dynamic, 1> ScalarVector;
		
		RedSymEigen() {}
		
		RedSymEigen(const MatrixType& A)
		{
			int r = (A.rows() < A.cols()) ? A.rows() : A.cols();
			compute(A, r);
		}
		
		RedSymEigen(const MatrixType& A, const Index rank)
		{
			compute(A, rank);
		}  
		
		void compute(const MatrixType& A, const Index rank)
		{
			if(A.cols() == 0 || A.rows() == 0)
				return;
			
			Index r = (rank < A.cols()) ? rank : A.cols();
			
			r = (r < A.rows()) ? r : A.rows();
			
			// Gaussian Random Matrix
			DenseMatrix O(A.rows(), r);
			sample_gaussian(O);
			
			// Compute Sample Matrix of A
			DenseMatrix Y = A.transpose() * O;
			
			// Orthonormalize Y
			gram_schmidt(Y);
			
			DenseMatrix B = Y.transpose() * A * Y;
			Eigen::SelfAdjointEigenSolver<DenseMatrix> eigenOfB(B);
			
			m_eigenvalues = eigenOfB.eigenvalues();
			m_eigenvectors = Y * eigenOfB.eigenvectors();
		}
		
		ScalarVector eigenvalues() const
		{
			return m_eigenvalues;
		}
		
		DenseMatrix eigenvectors() const
		{
			return m_eigenvectors;
		}
		
	private:
		ScalarVector m_eigenvalues;
		DenseMatrix m_eigenvectors;
	};
	
	template<typename _MatrixType>
	class RedPCA
	{
	public:
		typedef _MatrixType MatrixType;
		typedef typename MatrixType::Scalar Scalar;
		typedef typename MatrixType::Index Index;
		typedef typename Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> DenseMatrix;
		typedef typename Eigen::Matrix<Scalar, Eigen::Dynamic, 1> ScalarVector;
		
		RedPCA() {}
		
		RedPCA(const MatrixType& A)
		{
			int r = (A.rows() < A.cols()) ? A.rows() : A.cols();
			compute(A, r);
		}
		
		RedPCA(const MatrixType& A, const Index rank)
		{
			compute(A, rank);
		}  
		
		void compute(const DenseMatrix& A, const Index rank)
		{
			RedSVD<MatrixType> redsvd(A, rank);
			
			ScalarVector S = redsvd.singularValues();
			
			m_components = redsvd.matrixV();
			m_scores = redsvd.matrixU() * S.asDiagonal();
		}
		
		DenseMatrix components() const
		{
			return m_components;
		}
		
		DenseMatrix scores() const
		{
			return m_scores;
		}
		
	private:
		DenseMatrix m_components;
		DenseMatrix m_scores;
	};
}

#endif
