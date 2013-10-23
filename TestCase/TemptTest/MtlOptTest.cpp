#include <boost/test/unit_test.hpp>
#include <UnitTestAssert.h>
#include <eigen3/Eigen/Dense>
#include <DrawCurves.h>
#include "MtlOpt.h"
#include "MtlOptModel.h"
using namespace Eigen;
using namespace UTILITY;

BOOST_AUTO_TEST_SUITE(MtlOptTest)

BOOST_AUTO_TEST_CASE(Opt_Z_K_AkAm){

  MtlOptModel model;
  model.produceSimRlst();
  // model.extrangeKeyframes();
  model.lambda *= 0.25f;
  model.Kz.col(model.Kz.cols()-1).setZero();

  MtlDataModel dataM;
  model.initMtlData(dataM);

  // dataM.subZ.setZero();
  // dataM.K.setZero();
  // dataM.D.setZero();
  // dataM.Ak.setZero();
  // dataM.Am.setZero();

  ZOptimizer optZ(dataM);
  KOptimizer optK(dataM);
  KAtAmOptimizer optKAtAm(dataM);
  AkAmOptimizer optAkAm(dataM);

  for (int i = 0; i < 200; ++i){

	const MatrixXd oldZ = dataM.Z;
  	optZ.optimize();
  	optKAtAm.optimize();
  	// optk.optimize();
  	// optAkAm.optimize();

  	cout << "ITERATION " << i << "------------------------------------" << endl;
  	dataM.print();

	if(oldZ.size() == dataM.Z.size()){
	  const double err = (oldZ-dataM.Z).norm()/dataM.Z.norm();
	  cout << "err: " << err << endl;
	  if(err < 1e-3) break;
	}
  }

  model.saveMesh(dataM.Z,"Opt_Z_K_AkAm");
  model.saveMesh(model.Z,"input");

  PythonScriptDraw2DCurves<VectorXd> curves;
  const MatrixXd newZ = dataM.getRotZ();
  const MatrixXd newKZ = dataM.getUt()*model.Kz;
  for (int i = 0; i < model.Z.rows(); ++i){

	// const VectorXd &z = model.Z.row(i).transpose();
	// curves.add("z_input",z,1.0f,0);

	const VectorXd &z2 = newZ.row(i).transpose();
	curves.add(string("mode ")+TOSTR(i),z2,1.0f,0,"--");

	const VectorXd &kz = newKZ.row(i).transpose();
	VectorXd kid(model.Kid.size());
	for (int j = 0; j < kid.size(); ++j)
	  kid[j] = model.Kid[j];
	curves.add(string("keyframes of mode ")+TOSTR(i),kz,kid,"o");
  }
  TEST_ASSERT( curves.write("Opt_Z_K_AkAm") );
}

BOOST_AUTO_TEST_CASE(Opt_Z){

  MtlOptModel model;
  model.produceSimRlst();
  // model.extrangeKeyframes();
  model.lambda *= 0.25f;
  model.Kz.col(model.Kz.cols()-1).setZero();

  MtlDataModel dataM;
  model.initMtlData(dataM);

  ZOptimizer optZ(dataM);

  for (int i = 0; i < 50; ++i){

	const MatrixXd oldZ = dataM.Z;
  	optZ.optimize();
  	cout << "ITERATION " << i << "------------------------------------" << endl;
  	dataM.print();
	if(oldZ.size() == dataM.Z.size()){
	  const double err = (oldZ-dataM.Z).norm()/dataM.Z.norm();
	  cout << "err: " << err << endl;
	  if(err < 1e-3) break;
	}
  }

  model.saveMesh(dataM.Z,"Opt_Z");
  model.saveMesh(model.Z,"input");

  PythonScriptDraw2DCurves<VectorXd> curves;
  for (int i = 0; i < model.Z.rows(); ++i){

	// const VectorXd &z = model.Z.row(i).transpose();
	// curves.add("z_input",z,1.0f,0);

	const VectorXd &z2 = dataM.Z.row(i).transpose();
	curves.add(string("mode ")+TOSTR(i),z2,1.0f,0,"--");

	const VectorXd &kz = model.Kz.row(i).transpose();
	VectorXd kid(model.Kid.size());
	for (int j = 0; j < kid.size(); ++j)
	  kid[j] = model.Kid[j];
	curves.add(string("keyframes of mode ")+TOSTR(i),kz,kid,"o");
  }
  TEST_ASSERT( curves.write("Opt_Z") );
}

BOOST_AUTO_TEST_SUITE_END()