#include "pose-spline/LinearAccelerateSampleError.hpp"
#include "pose-spline/QuaternionSplineUtility.hpp"
#include "pose-spline/PoseLocalParameter.hpp"


LinearAccelerateSampleError::LinearAccelerateSampleError(const double& t_meas,
                                                                 const double& time_interval,
                                                 const Eigen::Vector3d& a_meas):
        t_meas_(t_meas),time_interval_(time_interval), a_Meas_(a_meas){

};

LinearAccelerateSampleError::~LinearAccelerateSampleError(){

}
bool LinearAccelerateSampleError::Evaluate(double const* const* parameters,
                                           double* residuals,
                                           double** jacobians) const{

    return EvaluateWithMinimalJacobians(parameters,residuals,jacobians,NULL);

}

bool LinearAccelerateSampleError::EvaluateWithMinimalJacobians(double const* const * parameters,
                                                               double* residuals,
                                                               double** jacobians,
                                                               double** jacobiansMinimal) const {

    Eigen::Map<const Eigen::Vector3d> V0(parameters[0]);
    Eigen::Map<const Eigen::Vector3d> V1(parameters[1]);
    Eigen::Map<const Eigen::Vector3d> V2(parameters[2]);
    Eigen::Map<const Eigen::Vector3d> V3(parameters[3]);


    Eigen::Map<const Quaternion> Q0(parameters[0]+3);
    Eigen::Map<const Quaternion> Q1(parameters[1]+3);
    Eigen::Map<const Quaternion> Q2(parameters[2]+3);
    Eigen::Map<const Quaternion> Q3(parameters[3]+3);

    double  ddBeta1 = QSUtility::dot_dot_beta1(time_interval_, t_meas_);
    double  ddBeta2 = QSUtility::dot_dot_beta2(time_interval_, t_meas_);
    double  ddBeta3 = QSUtility::dot_dot_beta3(time_interval_, t_meas_);

//    std::cout<<"t_meas_: " << t_meas_<< std::endl;
//    std::cout<< "ddBeta1: "<< ddBeta1<<std::endl;
//    std::cout<< "ddBeta2: "<< ddBeta2<<std::endl;
//    std::cout<< "ddBeta3: "<< ddBeta3<<std::endl;

    double  Beta1 = QSUtility::beta1(t_meas_);
    double  Beta2 = QSUtility::beta2(t_meas_);
    double  Beta3 = QSUtility::beta3(t_meas_);

    Eigen::Vector3d phi1 = QSUtility::Phi<double>(Q0,Q1);
    Eigen::Vector3d phi2 = QSUtility::Phi<double>(Q1,Q2);
    Eigen::Vector3d phi3 = QSUtility::Phi<double>(Q2,Q3);

    Quaternion r_1 = QSUtility::r(Beta1,phi1);
    Quaternion r_2 = QSUtility::r(Beta2,phi2);
    Quaternion r_3 = QSUtility::r(Beta3,phi3);

    // define residual
    // For simplity, we define error  =  /hat - meas.
    Quaternion Q_WI = quatLeftComp<double>(Q0)*quatLeftComp(r_1)*quatLeftComp(r_2)*r_3;
    Eigen::Matrix3d R_WI = quatToRotMat(Q_WI);

    // define residual
    // For simplity, we define error  =  /hat - meas.
    // /hat = R_WI^T( W_a)
    Eigen::Vector3d Wa = ddBeta1*(V1 - V0) +  ddBeta2*(V2 - V1) + ddBeta3*(V3 - V2);
    Eigen::Vector3d a_hat
            = R_WI.transpose()*Wa;

    Eigen::Map<Eigen::Vector3d> error(residuals);

    squareRootInformation_ = Eigen::Matrix3d::Identity();
    error = squareRootInformation_*(a_hat - a_Meas_);

    if(jacobians != NULL){

        Eigen::Matrix<double,3,4,Eigen::RowMajor> lift;
        QuaternionLocalParameter::liftJacobian(Q_WI.data(),lift.data());

        Eigen::Matrix<double,3,4> J_1st;
        J_1st.setZero();
        J_1st = - R_WI.transpose()*crossMat<double>(Wa) * lift;

        Eigen::Matrix<double,4,3> Vee = QSUtility::V<double>();

        Eigen::Vector3d BetaPhi1 = Beta1*phi1;
        Eigen::Vector3d BetaPhi2 = Beta2*phi2;
        Eigen::Vector3d BetaPhi3 = Beta3*phi3;
        Eigen::Matrix3d S1 = quatS(BetaPhi1);
        Eigen::Matrix3d S2 = quatS(BetaPhi2);
        Eigen::Matrix3d S3 = quatS(BetaPhi3);


        Quaternion invQ0Q1 = quatLeftComp(quatInv<double>(Q0))*Q1;
        Quaternion invQ1Q2 = quatLeftComp(quatInv<double>(Q1))*Q2;
        Quaternion invQ2Q3 = quatLeftComp(quatInv<double>(Q2))*Q3;
        Eigen::Matrix3d L1 = quatL(invQ0Q1);
        Eigen::Matrix3d L2 = quatL(invQ1Q2);
        Eigen::Matrix3d L3 = quatL(invQ2Q3);

        Eigen::Matrix3d C0 = quatToRotMat<double>(Q0);
        Eigen::Matrix3d C1 = quatToRotMat<double>(Q1);
        Eigen::Matrix3d C2 = quatToRotMat<double>(Q2);


        Eigen::Matrix<double,4,3> temp0;
        Eigen::Matrix<double,4,3> temp01;
        Eigen::Matrix<double,4,3> temp12;
        Eigen::Matrix<double,4,3> temp23;


        temp0 = quatRightComp<double>(quatLeftComp<double>(r_1)*quatLeftComp<double>(r_2)*r_3)*quatRightComp<double>(Q0)*Vee;
        temp01 = quatLeftComp<double>(Q0)*quatRightComp<double>(quatLeftComp<double>(r_2)*r_3)*quatRightComp<double>(r_1)*Vee*S1*Beta1*L1*C0.transpose();
        temp12 = quatLeftComp<double>(Q0)*quatLeftComp<double>(r_1)*quatRightComp<double>(r_3)*quatRightComp<double>(r_2)*Vee*S2*Beta2*L2*C1.transpose();
        temp23 = quatLeftComp<double>(Q0)*quatLeftComp<double>(r_1)*quatLeftComp<double>(r_2)*quatRightComp<double>(r_3)*Vee*S3*Beta3*L3*C2.transpose();

        if(jacobians[0] != NULL){
            Eigen::Map<Eigen::Matrix<double,3,7,Eigen::RowMajor>> J0(jacobians[0]);
            Eigen::Matrix<double,3,6,Eigen::RowMajor> J0_minimal;

            J0_minimal.setIdentity();
            J0_minimal.topLeftCorner(3,3) = R_WI.transpose() * (- ddBeta1 );
            J0_minimal.topRightCorner(3,3) = J_1st *(temp0 - temp01);

            Eigen::Matrix<double, 6, 7, Eigen::RowMajor> lift;
            PoseLocalParameter::liftJacobian(parameters[0], lift.data());
            J0_minimal = squareRootInformation_*J0_minimal;
            J0 = J0_minimal*lift;

//            std::cout<<"J0_minimal: "<<std::endl<< J0_minimal<<std::endl;

            if(jacobiansMinimal != NULL && jacobiansMinimal[0] != NULL){
                Eigen::Map<Eigen::Matrix<double,3,6,Eigen::RowMajor>> J0_minimal_map(jacobiansMinimal[0]);
                J0_minimal_map = J0_minimal;
            }
        }
        if(jacobians[1] != NULL){
            Eigen::Map<Eigen::Matrix<double,3,7,Eigen::RowMajor>> J1(jacobians[1]);
            Eigen::Matrix<double,3,6,Eigen::RowMajor> J1_minimal;

            J1_minimal.setIdentity();
            J1_minimal.topLeftCorner(3,3) = R_WI.transpose() * (ddBeta1 - ddBeta2);
            J1_minimal.topRightCorner(3,3) = J_1st *(temp01 - temp12);

            Eigen::Matrix<double, 6, 7, Eigen::RowMajor> lift;
            PoseLocalParameter::liftJacobian(parameters[1], lift.data());
            J1_minimal = squareRootInformation_*J1_minimal;
            J1 = J1_minimal*lift;

//            std::cout<<"J1_minimal: "<<std::endl<< J1_minimal<<std::endl;


            if(jacobiansMinimal != NULL && jacobiansMinimal[1] != NULL){
                Eigen::Map<Eigen::Matrix<double,3,6,Eigen::RowMajor>> J1_minimal_map(jacobiansMinimal[1]);
                J1_minimal_map = J1_minimal;
            }

        }
        if(jacobians[2] != NULL){
            Eigen::Map<Eigen::Matrix<double,3,7,Eigen::RowMajor>> J2(jacobians[2]);
            Eigen::Matrix<double,3,6,Eigen::RowMajor> J2_minimal;

            J2_minimal.setIdentity();
            J2_minimal.topLeftCorner(3,3) = R_WI.transpose() * (ddBeta2 - ddBeta3);
            J2_minimal.topRightCorner(3,3) = J_1st *(temp12 - temp23);

            Eigen::Matrix<double, 6, 7, Eigen::RowMajor> lift;
            PoseLocalParameter::liftJacobian(parameters[2], lift.data());
            J2_minimal = squareRootInformation_*J2_minimal;
            J2 = J2_minimal*lift;

//            std::cout<<"J2_minimal: "<<std::endl<< J2_minimal<<std::endl;

            if(jacobiansMinimal != NULL &&  jacobiansMinimal[2] != NULL){
                Eigen::Map<Eigen::Matrix<double,3,6,Eigen::RowMajor>> J2_minimal_map(jacobiansMinimal[2]);
                J2_minimal_map = J2_minimal;
            }

        }
        if(jacobians[3] != NULL){

            Eigen::Map<Eigen::Matrix<double,3,7,Eigen::RowMajor>> J3(jacobians[3]);
            Eigen::Matrix<double,3,6,Eigen::RowMajor> J3_minimal;
            J3_minimal.setIdentity();
            J3_minimal.topLeftCorner(3,3) = R_WI.transpose() * (ddBeta3);
            J3_minimal.topRightCorner(3,3) = J_1st *(temp23);

            Eigen::Matrix<double, 6, 7, Eigen::RowMajor> lift;
            PoseLocalParameter::liftJacobian(parameters[2], lift.data());
            J3_minimal = squareRootInformation_*J3_minimal;
            J3 = J3_minimal*lift;

//            std::cout<<"J3_minimal: "<<std::endl<< J3_minimal<<std::endl;


            if(jacobiansMinimal != NULL &&  jacobiansMinimal[3] != NULL){

                Eigen::Map<Eigen::Matrix<double,3,6,Eigen::RowMajor>> J3_minimal_map(jacobiansMinimal[3]);
                J3_minimal_map = J3_minimal;
            }
        }
    }

    return true;
}


