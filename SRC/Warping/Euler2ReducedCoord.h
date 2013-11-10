#ifndef _EULER2REDUCEDCOORD_H_
#define _EULER2REDUCEDCOORD_H_

#include <MapMA2RS.h>
#include <RSCoordComp.h>
#include <DefGradOperator.h>
#include <JsonFilePaser.h>
#include <MatrixIO.h>
#include <TetMesh.h>

namespace LSW_WARPING{
  
  /**
   * @class Euler2ReducedCoord given the coordinates in full space, compute the
   * reduced RS coordinates z.
   * 1) compute RS coordinates y = y(u).
   * 2) compute compute z by solving \|\hat{W}z-y\|_2^2.
   * 
   * Requirements:
   * 1) G the deformation gradient operator calculated by DefGradOperator.
   * 2) W modal basis, generated by MA method.
   */
  class Euler2ReducedCoord{
	
  public:
	Euler2ReducedCoord(const SparseMatrix<double> &G, const MatrixXd &W){
	  initialize(G,W);
	}
	void initialize(const SparseMatrix<double> &G, const MatrixXd &W){

	  assert_eq(G.cols(),W.rows());
	  assert_eq(G.cols()%3,0);
	  assert_gt(G.size(),0);
	  assert_gt(W.size(),0);
	  this->_G = G;
	  MapMA2RS::computeMapMatPGW(G,W,_hatW);
	}
	void compute(const VectorXd &u,VectorXd &z){
	  assert_eq(u.size(),_G.cols());
	  VectorXd y;
	  RSCoordComp::constructWithoutWarp(_G,u,y);
	  z = (_hatW.transpose()*_hatW).ldlt().solve(_hatW.transpose()*y);
	}
	void compute(const MatrixXd &u,MatrixXd &z){
	  assert_eq(u.rows(),_G.cols());
	  z.resize(_hatW.cols(),u.cols());
	  Eigen::LDLT<MatrixXd> solver(_hatW.transpose()*_hatW);
	  VectorXd y;
	  for (int i = 0; i < u.cols(); ++i){
		RSCoordComp::constructWithoutWarp(_G,u.col(i),y);
		z.col(i) = solver.solve(_hatW.transpose()*y);
	  }
	}
	
  protected:
	SparseMatrix<double> _G;
	MatrixXd _hatW;
  };
  typedef boost::shared_ptr<Euler2ReducedCoord> pEuler2ReducedCoord;
 
  class Euler2ReducedCoordExt{
	
  public:
	/**
	 * load data from init file, and save the resulting z:
	 * "vol_filename": "beam.abq",
	 * "eigen_vectors": "W.b",
	 * "input_U": "U.b",
	 * "output_Z": "Z.b",
	 * "output_oldU": "oldU.b",
	 * "output_newU": "newU.b"
	 */
	static bool compute(const string inifile){

	  UTILITY::JsonFilePaser jsonf;
	  bool succ = jsonf.open(inifile);
	  while(succ){

		string volfile, Wfile, inputU, outputZ, outputOldU, outputNewU;
		succ = jsonf.read("vol_filename",volfile);
		if(!succ) break;
		
		succ = jsonf.read("eigen_vectors",Wfile);
		if(!succ) break;

		succ = jsonf.read("input_U",inputU);
		if(!succ) break;
		
		succ = jsonf.read("output_Z",outputZ);
		if(!succ) break;

		succ = jsonf.read("output_oldU",outputOldU);
		if(!succ) break;

		succ = jsonf.read("output_newU",outputNewU);
		if(!succ) break;

		succ = compute(volfile,Wfile,inputU,outputZ,outputOldU,outputNewU);
		break;
	  }
	  return succ;
	}
	static bool compute(const string volfile,
						const string Wfile,
						const string inputU,
						const string outputZ,
						const string outputOldU,
						const string outputNewU){
	  MatrixXd U;
	  bool succ = EIGEN3EXT::load(inputU,U);
	  if (succ) return compute(volfile,Wfile,U,outputZ,outputOldU,outputNewU);
	  return succ;
	}
	
	template<typename VECTORINT>
	static bool compute(const string volfile,
						const string Wfile,
						const string inputU,
						const string outputZ,
						const VECTORINT &selectecU,
						const string outputOldU,
						const string outputNewU){
	  MatrixXd U;
	  bool succ = EIGEN3EXT::load(inputU,U);
	  MatrixXd subU(U.rows(),selectecU.size());
	  for (size_t i = 0; i < (size_t)selectecU.size(); ++i){
		const int j = selectecU[i];
		assert_in(j,0,U.cols()-1);
		subU.col(i) = U.col(j);
	  }
	  if (succ) return compute(volfile,Wfile,subU,outputZ,outputOldU,outputNewU);
	  return succ;
	}
	static bool compute(const string volfile,
						const string Wfile,
						const MatrixXd &U,
						const string outputZ,
						const string outputOldU,
						const string outputNewU){
	  UTILITY::TetMesh tetmesh;
	  bool succ = tetmesh.load(volfile);
	  while(succ){
		
		MatrixXd W;
		succ = EIGEN3EXT::load(Wfile,W);
		if (!succ) break;

		SparseMatrix<double> G;
		succ = DefGradOperator::compute(tetmesh,G);
		if (!succ) break;

		Euler2ReducedCoord euler2z(G,W);
		MatrixXd Z;
		euler2z.compute(U,Z);

		succ = EIGEN3EXT::write(outputZ,Z);
		break;
	  }
	  succ &= tetmesh.writeVTK(outputOldU,U);
	  return succ;
	}
  };
 
}//end of namespace

#endif /*_EULER2REDUCEDCOORD_H_*/
