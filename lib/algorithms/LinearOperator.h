#ifndef  GRID_ALGORITHM_LINEAR_OP_H
#define  GRID_ALGORITHM_LINEAR_OP_H

namespace Grid {

  /////////////////////////////////////////////////////////////////////////////////////////////
  // LinearOperators Take a something and return a something.
  /////////////////////////////////////////////////////////////////////////////////////////////
  //
  // Hopefully linearity is satisfied and the AdjOp is indeed the Hermitian conjugateugate (transpose if real):
  //SBase
  //   i)  F(a x + b y) = aF(x) + b F(y).
  //  ii)  <x|Op|y> = <y|AdjOp|x>^\ast
  //
  // Would be fun to have a test linearity & Herm Conj function!
  /////////////////////////////////////////////////////////////////////////////////////////////
    template<class Field> class LinearOperatorBase {
    public:
      virtual void Op     (const Field &in, Field &out) = 0; // Abstract base
      virtual void AdjOp  (const Field &in, Field &out) = 0; // Abstract base
      virtual void HermOpAndNorm(const Field &in, Field &out,double &n1,double &n2)=0;
    };


  /////////////////////////////////////////////////////////////////////////////////////////////
  // By sharing the class for Sparse Matrix across multiple operator wrappers, we can share code
  // between RB and non-RB variants. Sparse matrix is like the fermion action def, and then
  // the wrappers implement the specialisation of "Op" and "AdjOp" to the cases minimising
  // replication of code.
  //
  // I'm not entirely happy with implementation; to share the Schur code between herm and non-herm
  // while still having a "OpAndNorm" in the abstract base I had to implement it in both cases
  // with an assert trap in the non-herm. This isn't right; there must be a better C++ way to
  // do it, but I fear it required multiple inheritance and mixed in abstract base classes
  /////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////
    // Construct herm op from non-herm matrix
    ////////////////////////////////////////////////////////////////////
    template<class Matrix,class Field>
    class MdagMLinearOperator : public LinearOperatorBase<Field> {
      Matrix &_Mat;
    public:
    MdagMLinearOperator(Matrix &Mat): _Mat(Mat){};
      void Op     (const Field &in, Field &out){
	_Mat.M(in,out);
      }
      void AdjOp     (const Field &in, Field &out){
	_Mat.Mdag(in,out);
      }
      void HermOpAndNorm(const Field &in, Field &out,double &n1,double &n2){
	_Mat.MdagM(in,out,n1,n2);
      }
    };

    ////////////////////////////////////////////////////////////////////
    // Wrap an already herm matrix
    ////////////////////////////////////////////////////////////////////
    template<class Matrix,class Field>
    class HermitianLinearOperator : public LinearOperatorBase<Field> {
      Matrix &_Mat;
    public:
    HermitianLinearOperator(Matrix &Mat): _Mat(Mat){};
      void Op     (const Field &in, Field &out){
	_Mat.M(in,out);
      }
      void AdjOp     (const Field &in, Field &out){
	_Mat.M(in,out);
      }
      void HermOpAndNorm(const Field &in, Field &out,double &n1,double &n2){
	ComplexD dot;

	_Mat.M(in,out);
	
	dot= innerProduct(in,out);
	n1=real(dot);

	dot = innerProduct(out,out);
	n2=real(dot);
      }
    };

    //////////////////////////////////////////////////////////
    // Even Odd Schur decomp operators; there are several
    // ways to introduce the even odd checkerboarding
    //////////////////////////////////////////////////////////

    template<class Field>
      class SchurOperatorBase :  public LinearOperatorBase<Field> {
    public:
      virtual  RealD Mpc      (const Field &in, Field &out) =0;
      virtual  RealD MpcDag   (const Field &in, Field &out) =0;
      virtual void MpcDagMpc(const Field &in, Field &out,RealD &ni,RealD &no) {
	Field tmp(in._grid);
	ni=Mpc(in,tmp);
	no=MpcDag(tmp,out);
      }
      void HermOpAndNorm(const Field &in, Field &out,RealD &n1,RealD &n2){
	MpcDagMpc(in,out,n1,n2);
      }
      void Op     (const Field &in, Field &out){
	Mpc(in,out);
      }
      void AdjOp     (const Field &in, Field &out){ 
	MpcDag(in,out);
      }
    };
    template<class Matrix,class Field>
      class SchurDiagMooeeOperator :  public SchurOperatorBase<Field> {
      Matrix &_Mat;
    public:
      SchurDiagMooeeOperator (Matrix &Mat): _Mat(Mat){};
      virtual  RealD Mpc      (const Field &in, Field &out) {
	Field tmp(in._grid);

	_Mat.Meooe(in,tmp);
	_Mat.MooeeInv(tmp,out);
	_Mat.Meooe(out,tmp);

	_Mat.Mooee(in,out);
	return axpy_norm(out,-1.0,tmp,out);
      }
      virtual  RealD MpcDag   (const Field &in, Field &out){
	Field tmp(in._grid);

	_Mat.MeooeDag(in,tmp);
	_Mat.MooeeInvDag(tmp,out);
	_Mat.MeooeDag(out,tmp);

	_Mat.MooeeDag(in,out);
	return axpy_norm(out,-1.0,tmp,out);
      }
    };
    template<class Matrix,class Field>
      class SchurDiagOneOperator :  public SchurOperatorBase<Field> {
      Matrix &_Mat;
    public:
      SchurDiagOneOperator (Matrix &Mat): _Mat(Mat){};

      virtual  RealD Mpc      (const Field &in, Field &out) {
	Field tmp(in._grid);

	_Mat.Meooe(in,tmp);
	_Mat.MooeeInv(tmp,out);
	_Mat.Meooe(out,tmp);
	_Mat.MooeeInv(tmp,out);

	return axpy_norm(out,-1.0,tmp,in);
      }
      virtual  RealD MpcDag   (const Field &in, Field &out){
	Field tmp(in._grid);

	_Mat.MooeeInvDag(in,out);
	_Mat.MeooeDag(out,tmp);
	_Mat.MooeeInvDag(tmp,out);
	_Mat.MeooeDag(out,tmp);

	return axpy_norm(out,-1.0,tmp,in);
      }
    };

    /////////////////////////////////////////////////////////////
    // Base classes for functions of operators
    /////////////////////////////////////////////////////////////
    template<class Field> class OperatorFunction {
    public:
      virtual void operator() (LinearOperatorBase<Field> &Linop, const Field &in, Field &out) = 0;
    };

    // FIXME : To think about

    // Chroma functionality list defining LinearOperator
    /*
     virtual void operator() (T& chi, const T& psi, enum PlusMinus isign) const = 0;
     virtual void operator() (T& chi, const T& psi, enum PlusMinus isign, Real epsilon) const
     virtual const Subset& subset() const = 0;
     virtual unsigned long nFlops() const { return 0; }
     virtual void deriv(P& ds_u, const T& chi, const T& psi, enum PlusMinus isign) const
     class UnprecLinearOperator : public DiffLinearOperator<T,P,Q>
       const Subset& subset() const {return all;}
     };
    */


}

#endif
