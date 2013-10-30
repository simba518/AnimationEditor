#include <MatrixIO.h>
#include <MatrixTools.h>
#include <ConMatrixTools.h>
#include <JsonFilePaser.h>
#include <Log.h>
#include "MtlOptInterpolator.h"
using namespace UTILITY;
using namespace LSW_ANI_EDITOR;

MtlOptInterpolator::MtlOptInterpolator(){

  warper = pRSWarperExt(new RSWarperExt());
  ctrlF = pCtrlForceEnergy(new CtrlForceEnergy());
  mtlOpt = pMtlOptEnergy(new MtlOptEnergy());
  ctrlFSolver = pNoConIpoptSolver(new NoConIpoptSolver(ctrlF));
  mtlOptSolver = pNoConIpoptSolver(new NoConIpoptSolver(mtlOpt));
  use_warp = true;
}

bool MtlOptInterpolator::init (const string init_filename){
  
  TRACE_FUN();
  JsonFilePaser json_f;
  bool succ = json_f.open(init_filename);
  if (succ){
	succ &= json_f.readMatFile("eigen_vectors",W);
	succ &= json_f.readMatFile("nonlinear_basis",B);
	use_warp = json_f.read("use_warp",use_warp) ? use_warp:false;
	succ &= loadUref(json_f,u_ref);
  }
  if (succ){
	delta_z.resize(getT());
	for (int i = 0; i < getT(); ++i){
	  delta_z[i].resize(linearSubDim());
	  delta_z[i].setZero();
	}
  }
  if (succ){

	double h, alpha_k, alpha_m, penalty;
	VectorXd lambda;
	succ &= json_f.read("h",h);
	succ &= json_f.read("alpha_k",alpha_k);
	succ &= json_f.read("alpha_m",alpha_m);
	succ &= json_f.read("penaltyCon",penalty);
	succ &= json_f.readVecFile("eigen_values",lambda);
	if (succ){

	  ctrlF->setTimestep(h);
	  ctrlF->setTotalFrames(getT());
	  ctrlF->setPenaltyCon(penalty);
	  const VectorXd D = alpha_m*VectorXd::Ones(lambda.size())+lambda*alpha_k;
	  ctrlF->setMtl(lambda,D);
	  ctrlF->precompute();

	  mtlOpt->setTimestep(h);
	  mtlOpt->setTotalFrames(getT());
	  mtlOpt->setMtl(lambda,D);
	}
  }
  if (succ){
	succ = initWarper(json_f);
	if (succ)  ctrlF->setRedWarper(nodeWarper);
  }
  modalDisplayer.initialize(init_filename);
  succ &= ctrlFSolver->initialize();
  succ &= mtlOptSolver->initialize();
  if (!succ) ERROR_LOG("failed to initialize MtlOptInterpolator.");
  return succ;
}

void MtlOptInterpolator::setUc(const int frame_id, const VectorXd &uc){

  assert_in (frame_id,0,(int)u_ref.size()-1);
  assert_eq (con_nodes.size(),con_frame_id.size());
  if(!validConstraints(frame_id)){
	ERROR_LOG("frame "<<frame_id<<" can not constrained.");
	return ;
  }
  int i = 0;
  for (i = 0; i < (int)con_frame_id.size(); ++i){
	if (frame_id == con_frame_id[i]){
	  this->uc[i] = uc;
	  break;
	}
  }
  ERROR_LOG_COND("failed to setUc!", (i < (int)con_frame_id.size())); 
}

void MtlOptInterpolator::removeAllPosCon(){
  
  con_frame_id.clear();
  con_nodes.clear();
  uc.clear();
  ctrlF->clearPartialCon();
}

bool MtlOptInterpolator::interpolate (){

  if (modalDisplayer.isShowModalModes()){
	return modalDisplayer.showModalModes(ctrlF->getLambda(),delta_z);
  }

  ctrlF->setPartialCon(con_frame_id,con_nodes,uc);
  const int MAX_IT = 100;
  const double TOL = 1e-3;
  bool succ = ctrlFSolver->solve();
  double objValue = ctrlF->getObjValue();

  static MatrixXd V,Z;
  for (int it = 0; it < MAX_IT && succ; ++it){

	ctrlF->forward(V,Z);
	mtlOpt->setVZ(V,Z);
	succ = mtlOptSolver->solve();
	if(succ){
	  ctrlF->setKD(mtlOpt->getK(),mtlOpt->getD());
	  succ = ctrlFSolver->solve();
	  if( fabs(ctrlF->getObjValue() - objValue) <= TOL )
		break;
	  objValue = ctrlF->getObjValue();
	}
  }

  ctrlF->forward(V,Z);
  assert_eq(Z.rows(),reducedDim());
  assert_eq(Z.cols(),getT());
  EIGEN3EXT::convert(Z,delta_z);
  return succ;
}

const VectorXd& MtlOptInterpolator::getInterpU(const int frame_id){

  assert_in (frame_id, 0, getT()-1);
  if (use_warp && nodeWarper != NULL){
	nodeWarper->warp(delta_z[frame_id],frame_id,full_u);
  }else{
	full_u = u_ref[frame_id] + W*delta_z[frame_id];
  }
  return full_u;
}

bool MtlOptInterpolator::initWarper(JsonFilePaser &inf){

  string vol_filename;
  bool succ = inf.readFilePath("vol_filename",vol_filename);
  pTetMesh tet_mesh = pTetMesh(new TetMesh());
  if (succ) succ = tet_mesh->load(vol_filename);
  if (succ){ warper->setTetMesh(tet_mesh); }

  vector<int> fixed_nodes;
  if(inf.readVecFile("fixed_nodes_RS2Euler",fixed_nodes,UTILITY::TEXT))
	warper->setFixedNodes(fixed_nodes);
  warper->setRefU(u_ref);
  if(succ) succ = warper->precompute();

  vector<int> cubP;
  vector<double> cubW;
  if (inf.readVecFile("cubaturePoints",cubP,UTILITY::TEXT)&&
	  inf.readVecFile("cubatureWeights",cubW)){
  	nodeWarper = pRedRSWarperExt(new RedRSWarperExt(warper,B,W,cubP,cubW));
  }else{
  	nodeWarper = pRedRSWarperExt(new RedRSWarperExt(warper,B,W));
  }
  return succ;
}

const VectorXd &MtlOptInterpolator::getWarpU(const int frame_id,
											 const vector<set<int> > &c_nodes,
											 const VectorXd &bcen_uc){
  assert_in (frame_id, 0, getT()-1);
  assert (warper != NULL);
  warper->setConNodes(c_nodes,bcen_uc);
  warper->warp(W*delta_z[frame_id], frame_id,full_u);
  vector<set<int> > c_nodes_empty;
  VectorXd bcen_uc_emtpy;
  warper->setConNodes(c_nodes_empty,bcen_uc_emtpy);
  return full_u;
}

void MtlOptInterpolator::setAllConGroups(const set<pConNodesOfFrame> &newCons){
  
  removeAllPosCon();
  BOOST_FOREACH(pConNodesOfFrame con, newCons){
  	if( !con->isEmpty() ){
  	  addConGroups(con->getFrameId(), con->getConNodesSet(), con->getBarycenterUc());
  	}
  }
}

void MtlOptInterpolator::addConGroups(const int frame_id,const vector<set<int> >&group,const VectorXd&uc){
  
  if(!validConstraints(frame_id)){
	ERROR_LOG("frame "<<frame_id<<" can not constrained.");
	return ;
  }
  assert_in (frame_id,0,getT()-1);
  assert_eq (uc.size()%3,0);
  assert_eq (uc.size(),group.size()*3);

  vector<int> con_nodes;
  for (size_t i = 0; i < group.size(); ++i){
	assert_gt(group[i].size() , 0);
	con_nodes.push_back(*(group[i].begin()));
  }

  if (con_nodes.size() <=0 ){
	removeConOfFrame(frame_id);
	return ;
  }

  // sort the constraints by frames numder.
  size_t i = 0;
  for (; i < con_frame_id.size(); ++i){
	if (frame_id == con_frame_id[i]){
	  // frame_id is in con_frame_id
	  this->con_nodes[i] = con_nodes;
	  this->uc[i] = uc;
	  break;
	}else if(frame_id < con_frame_id[i]){
	  this->con_nodes.insert(this->con_nodes.begin()+i,con_nodes);
	  this->uc.insert(this->uc.begin()+i,uc);
	  con_frame_id.insert(con_frame_id.begin()+i,frame_id);
	  break;
	}
  }
  if ( i >= con_frame_id.size()){
	this->con_nodes.push_back(con_nodes);
	this->uc.push_back(uc);
	con_frame_id.push_back(frame_id);
  }
}

void MtlOptInterpolator::removeConOfFrame(const int frame_id){
  
  assert_in(frame_id,0,getT()-1);
  assert_eq(con_frame_id.size(), con_nodes.size());
  assert_eq(con_nodes.size(),uc.size());
  
  const vector<int> con_frame_id_t = con_frame_id;   
  const vector<vector<int> > con_nodes_t = con_nodes;
  const vector<VectorXd> uc_t = uc; 
  con_frame_id.clear();
  con_nodes.clear();
  uc.clear();

  for (size_t i = 0; i < con_frame_id_t.size(); ++i){
	if (frame_id != con_frame_id_t[i]){
	  con_frame_id.push_back(con_frame_id_t[i]);
	  con_nodes.push_back(con_nodes_t[i]);
	  uc.push_back(uc_t[i]);
	}
  }
}

bool MtlOptInterpolator::loadUref(JsonFilePaser &json_f,vector<VectorXd> &u_ref)const{
  
  int zero_input_animation = 0;
  bool succ = true;
  if (json_f.read("zero_input_animation",zero_input_animation)&&zero_input_animation>0){
	u_ref.resize(zero_input_animation);
	for (size_t i = 0; i < u_ref.size(); ++i){
	  u_ref[i].resize(W.rows());
	  u_ref[i].setZero();
	}
  }else{
	succ = false;
	string fullinput;
	if(json_f.readFilePath("full_input_animation",fullinput)){
	  MatrixXd Uin;
	  succ = EIGEN3EXT::load(fullinput,Uin);
	  EIGEN3EXT::convert(Uin,u_ref);
	}
  }
  return succ;
}