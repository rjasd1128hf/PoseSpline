#ifndef QUATERNIONSPLINEUTILITY_H
#define QUATERNIONSPLINEUTILITY_H


#include "pose-spline/Quaternion.hpp"

typedef  Eigen::Matrix3d Jacobian_Quat;


/*
 * Continuous-Time Estimation of attitude
 * using B-splines on Lie groups
 */


class QSUtility{

public:

    template<typename T>
    static Eigen::Matrix<T, 3, 1> Phi(const Eigen::Matrix<T,4,1> & Q_k_1, const Eigen::Matrix<T,4,1> &Q_k) {
        Eigen::Matrix<T,4,1> invQ_k_1 = quatInv(Q_k_1);
        Eigen::Matrix<T,4,1> tmp  = quatMult(invQ_k_1,Q_k);
        return quatLog(tmp);
    }


    template<typename T>
    static Eigen::Matrix<T, 4, 1> r(const T beta_t,const Eigen::Matrix<T,3,1> Phi) {
            return quatExp<double>(beta_t*Phi);
    }
    static std::pair<Jacobian_Quat,Jacobian_Quat>
                    Jcobian_Phi_Quat(Quaternion &q_k_1, Quaternion &q_k);


    /*
     *  bpline basis fuctions
     */
    template<typename T>
    static T cubicBasisFun0(T u){
        return T((1 - u)*(1 - u)*(1 - u)/6.0);
    }

    template<typename T>
    static T cubicBasisFun1(T u){
        return T((3*u*u*u - 6*u*u + 4)/6.0);
    }

    template<typename T>
    static T cubicBasisFun2(T u){
        return T((-3*u*u*u + 3*u*u + 3*u +1)/6.0);
    }

    template<typename T>
    static T cubicBasisFun3(T u){
        return T(u*u*u /6.0);
    }

    // An alternative form
    template<typename T >
    static Eigen::Matrix<T,4,4> M(){
        return T((1.0/6.0)*(Eigen::Matrix4d()<<1,4,1,0,
                                           -3,0,3,0,
                                           3,-6,3,0,
                                           -1,3,-3,1).finished());
    }

    /*
     * bspline basis Cumulative functions
     */
    // todo: need move to base class

    template<typename T >
    static T beta0(T u){
        return T(1.0);
    }
    template<typename T >
    static T beta1(T u){

        //cubicBasisFun1(u) + cubicBasisFun2(u) + cubicBasisFun3(u)
        return T((u*u*u - 3*u*u + 3*u +5)/6.0);
    }
    template<typename T >
    static T beta2(T u){
        // cubicBasisFun2(u) + cubicBasisFun3(u)
        return T((-2*u*u*u + 3*u*u + 3*u +1)/6.0);
    }
    template<typename T >
    static T beta3(T u){
        return  cubicBasisFun3(u);
    }


    // An anternative matrix form
    // A Spline-Based Trajectory Representation for Sensor Fusion
    // and Rolling Shutter Cameras
    // But in form: beta = C*[1 u u^2 u^3]^T
    template<typename T >
    static Eigen::Matrix<T,4,4> C(){

        Eigen::Matrix<T,4,4> res;
        res <<  6,5,1,0,
                0,3,3,0,
                0,-3,3,0,
                0,1,-2,1;
        res = T(1.0/6.0)*res;


        return res;
    }
    /*
     * First order derivation of basis Cumulative functions
     * Here, we take dt into consideration.
     */

    //TODO: simplify
    template<typename T >
    static T dot_beta1(const T dt, const T u){

        Eigen::Matrix<T,4,1> uu(0.0, 1, 2*u ,3*u*u);
        return T(1/dt)*uu.transpose()*C<T>().col(1);
    }

    template<typename T >
    static T dot_beta2(const T dt, const T u){

        Eigen::Matrix<T,4,1> uu(0.0, 1, 2*u ,3*u*u);
        return (1/dt)*uu.transpose()*C<T>().col(2);
    }

    template<typename T >
    static T dot_beta3(const T dt, const T u){

        Eigen::Matrix<T,4,1> uu(0.0, 1, 2*u ,3*u*u);
        return (1/dt)*uu.transpose()*C<T>().col(3);
    }

    /*
     * Second order derivation
     */
    template<typename T >
    static T dot_dot_beta1(T dt, T u){
        Eigen::Matrix<T,4,1> uu(0.0, 0.0, 2.0 ,6*u);
        return (1/(dt*dt))*uu.transpose()*C<T>().col(1);
    }

    template<typename T >
    static T dot_dot_beta2(T dt, T u){

        Eigen::Matrix<T,4,1> uu(0.0, 0.0, 2.0 ,6*u);
        return (1/(dt*dt))*uu.transpose()*C<T>().col(2);
    }

    template<typename T >
    static T dot_dot_beta3(T dt, T u){

        Eigen::Matrix<T,4,1> uu(0.0, 0.0, 2.0 ,6*u);
        return (1/(dt*dt))*uu.transpose()*C<T>().col(3);
    }

/*
 * We follow Pose estimation using linearized rotations and quaternion algebra.
 * The exp and log function is a little different from Hannes Sommer's
 * Continuous-Time Estimation paper.
 * Thus, the dr_dt and d2r_dt2 are not equal to ones in this paper.
 */
    template<typename T>
    static Eigen::Matrix<T, 4, 1> dr_dt(T dot_beta, T beta,
                            const Eigen::Matrix<T, 4, 1> & Q_k_1, const Eigen::Matrix<T, 4, 1> &Q_k){

        Eigen::Matrix<T, 4, 1> phi_ext;
        Eigen::Matrix<T, 3, 1> phi = Phi(Q_k_1,Q_k);

        phi_ext << phi,0.0;

        return 0.5*dot_beta*quatLeftComp(phi_ext)*r(beta,phi);
    }

    template<typename T>
    static Eigen::Matrix<T, 4, 1> dr_dt(T dot_beta, T beta,const Eigen::Matrix<T, 3, 1>& phi){

        Eigen::Matrix<T, 4, 1> phi_ext;
        phi_ext << phi,0.0;

        return 0.5*dot_beta*quatLeftComp(phi_ext)*r(beta,phi);
    }


    template<typename T>
    static Eigen::Matrix<T, 4, 1> d2r_dt2(T dot_dot_beta, T dot_beta, T beta,
                              const Eigen::Matrix<T, 4, 1> & Q_k_1, const Eigen::Matrix<T, 4, 1> &Q_k){

        Eigen::Matrix<T, 4, 1> phi_ext;
        Eigen::Matrix<T, 3, 1> phi = Phi(Q_k_1,Q_k);

        phi_ext << phi,0.0;

        return 0.5*quatLeftComp<double>(
                (0.5*dot_beta*dot_beta*phi_ext + dot_dot_beta*unitQuat<double>()))
               *quatLeftComp(phi_ext)*r(beta,phi);
    }

    template<typename T>
    static Eigen::Matrix<T, 4, 1> d2r_dt2(T dot_dot_beta, T dot_beta, T beta,
                              const Eigen::Matrix<T, 3, 1>& phi){

        Eigen::Matrix<T, 4, 1> phi_ext;
        phi_ext << phi,0.0;

        return 0.5*quatLeftComp<double>(
                (0.5*dot_beta*dot_beta*phi_ext + dot_dot_beta*unitQuat<double>()))
               *quatLeftComp(phi_ext)*r(beta,phi);
    }

    static Quaternion EvaluateQS(double u,
                                 const Quaternion& Q0,
                                 const Quaternion& Q1,
                                 const Quaternion& Q2,
                                 const Quaternion& Q3);


    static Quaternion Evaluate_dot_QS(double dt,
                                      double u,
                                      const Quaternion& Q0,
                                      const Quaternion& Q1,
                                      const Quaternion& Q2,
                                      const Quaternion& Q3);

    static Quaternion Evaluate_dot_dot_QS(double dt,
                                      double u,
                                      const Quaternion& Q0,
                                      const Quaternion& Q1,
                                      const Quaternion& Q2,
                                      const Quaternion& Q3);

    template<typename T>
    static Eigen::Matrix<T,4,3> V(){
        Eigen::Matrix<T,4,3> M;
        M<< 0.5,   0,   0,
                0, 0.5,   0,
                0,   0, 0.5,
                0,   0,   0;
        return M;
    };

    template<typename T>
    static Eigen::Matrix<T,3,4> W(){
        Eigen::Matrix<T,3,4> M;
        M<< 2.0,   0,   0, 0,
                0, 2.0,   0, 0,
                0,   0, 2.0, 0;

        return M;
    };
    /*
 *  using Indrict Kalman filter for 3D attitude estimation.
 */
    template<typename T>
    static Eigen::Matrix<T,3,1> w(const Eigen::Matrix<T,4,1> Q_ba,const Eigen::Matrix<T,4,1> dot_Q_ba){
        return 2.0*(quatLeftComp(dot_Q_ba)*quatInv(Q_ba)).head(3);
    }

    template<typename T>
    static Eigen::Matrix<T,3,1> alpha(const Eigen::Matrix<T,4,1> Q_ba,
                                          const Eigen::Matrix<T,4,1> dot_dot_Q_ba){
        return 2.0*(quatLeftComp(dot_dot_Q_ba)*quatInv(Q_ba)).head(3);
    }
    static Quaternion Jacobian_dotQinvQ_t(const Quaternion& Q,
                                         const Quaternion& dQ,
                                         const Quaternion& ddQ);

    static Eigen::Vector3d Jacobian_omega_t(const Quaternion& Q,
                                    const Quaternion& dQ,
                                    const Quaternion& ddQ,
                                    const Quaternion& extrinsicQ);

    static Eigen::Matrix3d Jacobian_omega_extrinsicQ(const Quaternion& Q,
                                             const Quaternion& dQ,
                                             const Quaternion& extrinsicQ);
    template<typename T>
    Eigen::Matrix<T,4,3> Jac_Exp(const Eigen::Matrix<T,3,1> phi){
        return quatRightComp(quatExp(phi))*V<T>()*quatS(phi);
    };

};

#endif