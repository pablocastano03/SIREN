/******************************************************************************
 *                                                                            *
 * This file is part of the simulation tool PROPOSAL.                         *
 *                                                                            *
 * Copyright (C) 2017 TU Dortmund University, Department of Physics,          *
 *                    Chair Experimental Physics 5b                           *
 *                                                                            *
 * This software may be modified and distributed under the terms of a         *
 * modified GNU Lesser General Public Licence version 3 (LGPL),               *
 * copied verbatim in the file "LICENSE".                                     *
 *                                                                            *
 * Modifcations to the LGPL License:                                          *
 *                                                                            *
 *      1. The user shall acknowledge the use of PROPOSAL by citing the       *
 *         following reference:                                               *
 *                                                                            *
 *         J.H. Koehne et al.  Comput.Phys.Commun. 184 (2013) 2070-2090 DOI:  *
 *         10.1016/j.cpc.2013.04.001                                          *
 *                                                                            *
 *      2. The user should report any bugs/errors or improvments to the       *
 *         current maintainer of PROPOSAL or open an issue on the             *
 *         GitHub webpage                                                     *
 *                                                                            *
 *         "https://github.com/tudo-astroparticlephysics/PROPOSAL"            *
 *                                                                            *
 ******************************************************************************/

#pragma once
#include <exception>
#include <functional>
#include <string>
#include <memory>
#include <earthmodel-service/Vector3D.h>
#include <earthmodel-service/Polynomial.h>

namespace earthmodel {

class DensityException : public std::exception {
   public:
    DensityException(const char* m) : message_(m){};
    const char* what() const throw() { return message_.c_str(); };

   private:
    std::string message_;
};

class DensityDistribution {
   public:
    DensityDistribution();
    DensityDistribution(const DensityDistribution&);

    virtual bool operator==(const DensityDistribution& dens_distr) const;
    virtual bool operator!=(const DensityDistribution& dens_distr) const;
    virtual bool compare(const DensityDistribution& dens_distr) const = 0;


    virtual DensityDistribution* clone() const = 0;
    virtual std::shared_ptr<const DensityDistribution> create() const = 0;

    virtual double Derivative(const Vector3D& xi,
                              const Vector3D& direction) const = 0;
    virtual double AntiDerivative(const Vector3D& xi,
                                  const Vector3D& direction) const = 0;
    virtual double Integral(const Vector3D& xi,
                            const Vector3D& direction,
                            double distance) const = 0;
    virtual double Integral(const Vector3D& xi,
                            const Vector3D& xj) const = 0;
    virtual double InverseIntegral(const Vector3D& xi,
                                   const Vector3D& direction,
                                   double integral,
                                   double max_distance) const =0;
    virtual double Evaluate(const Vector3D& xi) const = 0;
};

class Distribution1D {
public:
    Distribution1D();
    Distribution1D(const Distribution1D&);
    virtual bool operator==(const Distribution1D& dist) const;
    virtual bool operator!=(const Distribution1D& dist) const;
    virtual bool compare(const Distribution1D& dist) const = 0;
    virtual double Derivative(double x) const = 0;
    virtual double AntiDerivative(double x) const = 0;
    virtual double Evaluate(double x) const = 0;
};

class PolynomialDistribution1D : Distribution1D{
public:
    PolynomialDistribution1D(const PolynomialDistribution1D&);
    PolynomialDistribution1D(const Polynom&);
    PolynomialDistribution1D(const std::vector<double>&);
    bool compare(const Distribution1D& dist) const override;
    double Derivative(double x) const override;
    double AntiDerivative(double x) const override;
    double Evaluate(double x) const override;
protected:
    Polynom polynom_;
    Polynom Ipolynom_;
    Polynom dpolynom_;
};

class Axis1D {
   public:
    Axis1D();
    Axis1D(const Vector3D& fAxis, const Vector3D& fp0);
    Axis1D(const Axis1D&);

    virtual ~Axis1D() {};

    bool operator==(const Axis1D& axis) const;
    bool operator!=(const Axis1D& axis) const;

    virtual Axis1D* clone() const = 0;
    virtual std::shared_ptr<const Axis1D> create() const = 0;

    virtual double GetX(const Vector3D& xi) const = 0;
    virtual double GetdX(const Vector3D& xi, const Vector3D& direction) const = 0;

    Vector3D GetAxis() const { return fAxis_; };
    Vector3D GetFp0() const { return fp0_; };

   protected:
    Vector3D fAxis_;
    Vector3D fp0_;
};

class RadialAxis1D : public Axis1D {
   public:
    RadialAxis1D();
    RadialAxis1D(const Vector3D& fAxis, const Vector3D& fp0);

    Axis1D* clone() const override { return new RadialAxis1D(*this); };
    virtual std::shared_ptr<const Axis1D> create() const override {
        return std::shared_ptr<const Axis1D>(new RadialAxis1D(*this));
    };
    ~RadialAxis1D() {};

    double GetX(const Vector3D& xi) const override;
    double GetdX(const Vector3D& xi, const Vector3D& direction) const override;
};

class CartesianAxis1D : public Axis1D {
   public:
    CartesianAxis1D();
    CartesianAxis1D(const Vector3D& fAxis, const Vector3D& fp0);
    ~CartesianAxis1D() {};

    Axis1D* clone() const override { return new CartesianAxis1D(*this); };
    virtual std::shared_ptr<const Axis1D> create() const override {
        return std::shared_ptr<const Axis1D>(new CartesianAxis1D(*this));
    };

    double GetX(const Vector3D& xi) const override;
    double GetdX(const Vector3D& xi, const Vector3D& direction) const override;
};

class Density_homogeneous : public DensityDistribution {
   public:
    Density_homogeneous();
    Density_homogeneous(const Density_homogeneous&);
    Density_homogeneous(double correction_factor);

    // ~Density_homogeneous(){};

    bool compare(const DensityDistribution& dens_distr) const override;

    DensityDistribution* clone() const override {
        return new Density_homogeneous(*this);
    };

    std::shared_ptr<const DensityDistribution> create() const override {
        return std::shared_ptr<const DensityDistribution>(new Density_homogeneous(*this));
    };

    double Derivative(const Vector3D& xi,
                      const Vector3D& direction) const override;
    double AntiDerivative(const Vector3D& xi,
                          const Vector3D& direction) const override;
    double Integral(const Vector3D& xi,
                    const Vector3D& direction,
                    double distance) const override;
    double Integral(const Vector3D& xi,
                    const Vector3D& xj) const override;
    double InverseIntegral(const Vector3D& xi,
                           const Vector3D& direction,
                           double integral,
                           double max_distance) const override;
    double Evaluate(const Vector3D& xi) const override;

    double GetInverseIntegralionfactor() const { return correction_factor_; }

   private:
    double correction_factor_;
   protected:
    Axis1D* axis_;
};

class Density_polynomial : public DensityDistribution {
   public:
    Density_polynomial(const Axis1D&, const Polynom&);
    Density_polynomial(const Density_polynomial&);
    ~Density_polynomial();

    bool compare(const DensityDistribution& dens_distr) const override;

    DensityDistribution* clone() const override {
        return new Density_polynomial(*this);
    };
    std::shared_ptr<const DensityDistribution> create() const override { return std::shared_ptr<const DensityDistribution>( new Density_polynomial(*this) ); };

    double Derivative(const Vector3D& xi,
                      const Vector3D& direction) const override;
    double AntiDerivative(const Vector3D& xi,
                          const Vector3D& direction) const override;
    double Integral(const Vector3D& xi,
                    const Vector3D& direction,
                    double distance) const override;
    double Integral(const Vector3D& xi,
                    const Vector3D& xj) const override;
    double InverseIntegral(const Vector3D& xi,
                           const Vector3D& direction,
                           double integral,
                           double max_distance) const override;
    double Evaluate(const Vector3D& xi) const override;

    double Helper_function(const Vector3D& xi,
                           const Vector3D& direction,
                           double res,
                           double l) const;
    double helper_function(const Vector3D& xi,
                           const Vector3D& direction,
                           double res,
                           double l) const;

   protected:
    Polynom polynom_;
    Polynom Polynom_;

    std::function<double(double)> density_distribution;
    std::function<double(double)> antiderived_density_distribution;
   protected:
    Axis1D* axis_;
};

class Density_exponential : public DensityDistribution {
   public:
    Density_exponential(const Axis1D& axis, double sigma);

    ~Density_exponential(){};

    bool compare(const DensityDistribution& dens_distr) const override;

    DensityDistribution* clone() const override {
        return new Density_exponential(*this);
    };
    std::shared_ptr<const DensityDistribution> create() const override { return std::shared_ptr<const DensityDistribution>( new Density_exponential(*this) ); };

    double Derivative(const Vector3D& xi,
                      const Vector3D& direction) const override;
    double AntiDerivative(const Vector3D& xi,
                          const Vector3D& direction) const override;
    double Integral(const Vector3D& xi,
                    const Vector3D& direction,
                    double distance) const override;
    double Integral(const Vector3D& xi,
                    const Vector3D& xj) const override;
    double InverseIntegral(const Vector3D& xi,
                           const Vector3D& direction,
                           double integral,
                           double max_distance) const override;
    double Evaluate(const Vector3D& xi) const override;

    double GetDepth(const Vector3D& xi) const;
    double GetEffectiveDistance(const Vector3D& xi, const Vector3D& direction) const;

   private:
    double sigma_;
   protected:
    Axis1D* axis_;
};

}  // namespace earthmodel
