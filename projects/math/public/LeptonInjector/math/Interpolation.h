#ifndef LI_Interpolation_H
#define LI_Interpolation_H

#include <map>
#include <set>
#include <cmath>
#include <memory>
#include <vector>
#include <iterator>
#include <algorithm>
#include <functional>

#include <cereal/cereal.hpp>

namespace LI {
namespace math {

template<typename T>
class Transform {
public:
    virtual T Function(T x) const = 0;
    virtual T Inverse(T x) const = 0;
};

template<typename T>
class IdentityTransform : public Transform<T> {
public:
    IdentityTransform() {}
    virtual T Function(T x) const override {
        return x;
    }
    virtual T Inverse(T x) const override {
        return x;
    }
};

template<typename T>
class GenericTransform : public Transform<T> {
    std::function<T(T)> function;
    std::function<T(T)> inverse;
public:
    GenericTransform(std::function<T(T)> function, std::function<T(T)> inverse)
        : function(function), inverse(inverse) {}
    virtual T Function(T x) const override {
        return function(x);
    }
    virtual T Inverse(T x) const override {
        return inverse(x);
    }
};

template<typename T>
class LogTransform : public Transform<T> {
public:
    LogTransform() {}
    virtual T Function(T x) const override {
        return log(x);
    }
    virtual T Inverse(T x) const override {
        return exp(x);
    }
};

template<typename T>
class SymLogTransform : public Transform<T> {
    T min_x;
    T log_min_x;
public:
    SymLogTransform(T min_x) : min_x(std::abs(min_x)), log_min_x(log(std::abs(min_x))) {}
    virtual T Function(T x) const override {
        T abs_x = std::abs(x);
        if(abs_x < min_x) {
            return x;
        } else {
            return std::copysign(log(abs_x) - log_min_x + min_x, x);
        }
    }
    virtual T Inverse(T x) const override {
        T abs_x = std::abs(x);
        if(abs_x < min_x) {
            return x;
        } else {
            return std::copysign(exp(x - min_x + log_min_x), x);
        }
    }
};

template<typename T>
class RangeTransform : public Transform<T> {
    T min_x;
    T range;
public:
    RangeTransform(T min_x, T max_x) : min_x(min_x), range(max_x - min_x) {}
    virtual T Function(T x) const override {
        return (x - min_x) / range;
    }
    virtual T Inverse(T x) const override {
        return x * range + min_x;
    }
};

template<typename T>
class FunctionalRangeTransform : public Transform<T> {
    std::function<T(T)> min_function;
    std::function<T(T)> max_function;
public:
    FunctionalRangeTransform(std::function<T(T)> min_function, std::function<T(T)> max_function)
        : min_function(min_function), max_function(max_function) {}
    virtual T Function(T x) const override {
        T min_x = min_function(x);
        T range = max_function(x) - min_x;
        return (x - min_x) / range;
    }
    virtual T Inverse(T x) const override {
        T min_x = min_function(x);
        T range = max_function(x) - min_x;
        return x * range + min_x;
    }
};

template<typename T>
struct LinearInterpolationOperator {
public:
    LinearInterpolationOperator() {};
    virtual T operator()(T const & x0, T const & x1, T const & y0, T const & y1, T const & x) const {
        T delta_x = x1 - x0;
        T delta_y = y1 - y0;
        return (x - x0) * delta_y / delta_x;
    }
};

template<typename T>
struct DropLinearInterpolationOperator : public LinearInterpolationOperator<T> {
public:
    DropLinearInterpolationOperator() {}
    virtual T operator()(T const & x0, T const & x1, T const & y0, T const & y1, T const & x) const override {
        if(y0 == 0 or y1 == 0)
            return 0;
        T delta_x = x1 - x0;
        T delta_y = y1 - y0;
        return (x - x0) * delta_y / delta_x;
    }
};

template<typename T>
std::tuple<std::shared_ptr<Transform<T>>, std::shared_ptr<Transform<T>>> DetermineInterpolationSpace1D(
        std::vector<T> const & x,
        std::vector<T> const & y,
        std::shared_ptr<LinearInterpolationOperator<T>> interp) {
    std::vector<T> symlog_x(x.size());
    std::vector<T> symlog_y(y.size());
    T min_x = 1;
    T min_y = 1;
    bool have_x = false;
    bool have_y = false;
    for(size_t i=0; i<x.size(); ++i) {
        if(x[i] != 0) {
            if(have_x) {
                min_x = std::min(min_x, std::abs(x[i]));
                have_x = true;
            } else {
                min_x = std::abs(x[i]);
            }
        }
        if(y[i] != 0) {
            if(have_y) {
                min_y = std::min(min_y, std::abs(y[i]));
                have_y = true;
            } else {
                min_y = std::abs(y[i]);
            }
        }
    }
    SymLogTransform<T> symlog_t_x(min_x);
    SymLogTransform<T> symlog_t_y(min_y);
    for(size_t i=0; i<x.size(); ++i) {
        symlog_x[i] = symlog_t_x.Function(x[i]);
        symlog_y[i] = symlog_t_y.Function(y[i]);
    }

    T tot_00 = 0;
    T tot_01 = 0;
    T tot_10 = 0;
    T tot_11 = 0;

    for(size_t i=1; i<x.size()-1; ++i) {
        T y_i_estimate = interp->operator()(x[i-1], x[i+1], y[i-1], y[i+1], x[i]);
        T delta = y_i_estimate - y[i];
        tot_00 += delta * delta;

        y_i_estimate = interp->operator()(x[i-1], x[i+1], symlog_y[i-1], symlog_y[i+1], x[i]);
        delta = symlog_t_y.Inverse(y_i_estimate) - y[i];
        tot_01 += delta * delta;

        y_i_estimate = interp->operator()(symlog_x[i-1], symlog_x[i+1], y[i-1], y[i+1], symlog_x[i]);
        delta = y_i_estimate - y[i];
        tot_10 += delta * delta;

        y_i_estimate = interp->operator()(symlog_x[i-1], symlog_x[i+1], symlog_y[i-1], symlog_y[i+1], symlog_x[i]);
        delta = symlog_t_y.Inverse(y_i_estimate) - y[i];
        tot_11 += delta * delta;
    }

    std::vector<T> tots = {tot_00, tot_01, tot_10, tot_11};
    int min_combination = std::distance(tots.begin(), std::max_element(tots.begin(), tots.end()));

    switch(min_combination) {
        case 1:
            return {std::make_shared<IdentityTransform<T>>(), std::make_shared<SymLogTransform<T>>(min_y)};
        case 2:
            return {std::make_shared<SymLogTransform<T>>(min_x), std::make_shared<IdentityTransform<T>>()};
        case 3:
            return {std::make_shared<SymLogTransform<T>>(min_x), std::make_shared<SymLogTransform<T>>(min_y)};
        default:
            return {std::make_shared<IdentityTransform<T>>(), std::make_shared<IdentityTransform<T>>()};
    }
};

template<typename T>
class Indexer1D {
public:
    virtual std::tuple<int, int> operator()(T x) const = 0;
};

template<typename T>
class RegularIndexer1D : public Indexer1D<T> {
    T low;
    T high;
    T range;
    unsigned int n_points;
    T delta;

    RegularIndexer1D() {};
    RegularIndexer1D(std::vector<T> x) {
        n_points = x.size();
        low = x.front();
        high = x.back();
        range = high - low;
        delta = range / (n_points - 1);
    };

    template<class Archive>
    void serialize(Archive & archive, std::uint32_t const version) {
        if(version == 0) {
            archive(cereal::make_nvp("Low", low));
            archive(cereal::make_nvp("High", high));
            archive(cereal::make_nvp("Range", range));
            archive(cereal::make_nvp("NPoints", n_points));
            archive(cereal::make_nvp("Delta", delta));
        } else {
            throw std::runtime_error("IndexFinderRegular only supports version <= 0!");
        }
    }

    virtual std::tuple<int, int> operator()(T const & x) const override {
        int i = (int)alt_floor<T>()((x - low) / range * (n_points - 1));
        if(i < 0)
            i = 0;
        else if(i >= int(n_points) - 1)
            i = n_points - 2;
        return {x, x+1};
    }
};

template<typename T>
class IrregularIndexer1D : public Indexer1D<T> {
    std::vector<T> data;
    T low;
    T high;
    T range;
    unsigned int n_points;
public:
    IrregularIndexer1D() {};
    IrregularIndexer1D(std::vector<T> x): data(x.begin(), x.end()) {
        std::sort(data.begin(), data.end());
        low = data.front();
        high = data.back();
        range = high - low;
        n_points = data.size();
    };

    template<class Archive>
    void serialize(Archive & archive, std::uint32_t const version) {
        if(version == 0) {
            archive(cereal::make_nvp("Data", data));
            archive(cereal::make_nvp("Low", low));
            archive(cereal::make_nvp("High", high));
            archive(cereal::make_nvp("Range", range));
            archive(cereal::make_nvp("NPoints", n_points));
        } else {
            throw std::runtime_error("IndexFinderIrregular only supports version <= 0!");
        }
    }

    std::tuple<int, int> operator()(T const & x) const override {
        // Lower bound returns pointer to element that is greater than or equal to x
        // i.e. x \in (a,b] --> pointer to b, x \in (b,c] --> pointer to c
        // begin is the first element
        // distance(begin, pointer to y) --> y
        // therefore this function returns the index of the lower bin edge
        unsigned int index = std::distance(data.begin(), std::lower_bound(data.begin(), data.end(), x)) - 1;
        if(index < 0)
            index = 0;
        else if(index >= n_points - 1)
            index = n_points - 2;
        return {index, index + 1};
    }
};

template<typename T>
class Interpolator1D {
    virtual T operator()(T x) const = 0;
};

template<typename T>
class LinearInterpolator1D : public Interpolator1D<T> {
    // Data
    std::vector<T> t_x;
    std::vector<T> t_y;

    // Indexer
    std::shared_ptr<Indexer1D<T>> indexer;

    // Interpolation info
    std::shared_ptr<LinearInterpolationOperator<T>> interp;
    std::shared_ptr<Transform<T>> x_transform;
    std::shared_ptr<Transform<T>> y_transform;

    LinearInterpolator1D(std::vector<T> x, std::vector<T> y, std::shared_ptr<Indexer1D<T>> indexer, std::shared_ptr<LinearInterpolationOperator<T>> interp, std::shared_ptr<Transform<T>> x_transform, std::shared_ptr<Transform<T>> y_transform)
        : t_x(), t_y(), indexer(indexer), interp(interp), x_transform(x_transform), y_transform(y_transform) {
        t_x.reserve(x.size());
        t_y.reserve(y.size());
        for(size_t i=0; i<x.size(); ++i) {
            t_x.push_back(x_transform->operator()(x[i]));
            t_y.push_back(y_transform->operator()(y[i]));
        }
    }

    virtual T operator()(T x) const override {
        std::tuple<int, int> indices = indexer->operator()(x);
        T const & t_x0 = t_x[std::get<0>(indices)];
        T const & t_y0 = t_y[std::get<0>(indices)];
        T const & t_x1 = t_x[std::get<1>(indices)];
        T const & t_y1 = t_y[std::get<1>(indices)];

        T t_x = x_transform->Function(x);

        T t_res = interp->operator()(t_x0, t_y0, t_x1, t_y1, t_x);
        T res = y_transform->Inverse(t_res);
        return res;
    }
};

template<typename T>
struct BiLinearInterpolationOperator {
public:
    BiLinearInterpolationOperator() {}
    virtual T operator()(T const & x0, T const & x1, T const & y0, T const & y1, T const & z00, T const & z01, T const & z10, T const & z11, T const & x, T const & y) const {

        T delta_x = x1 - x0;
        T delta_y = y1 - y0;
        T dx0 = x - x0;
        T dx1 = x1 - x;
        T dy0 = y - y0;
        T dy1 = y1 - y;

        T res =
            dx1 * dy1 * z00 +
            dx1 * dy0 * z01 +
            dx0 * dy1 * z10 +
            dx0 * dy0 * z11;

        return res / (delta_x * delta_y);
    }
};

template<typename T>
struct DropBiLinearInterpolationOperator : public BiLinearInterpolationOperator<T> {
    DropBiLinearInterpolationOperator() {}
    virtual T operator()(T const & x0, T const & x1, T const & y0, T const & y1, T const & z00, T const & z01, T const & z10, T const & z11, T const & x, T const & y) const override {
        if((z00 == 0 and (z01 == 0 or z10 == 0)) or (z11 == 0 and (z01 == 0 or z10 == 0))) {
            return 0.0;
        }

        T delta_x = x1 - x0;
        T delta_y = y1 - y0;
        T dx0 = x - x0;
        T dx1 = x1 - x;
        T dy0 = y - y0;
        T dy1 = y1 - y;

        T res =
            dx1 * dy1 * z00 +
            dx1 * dy0 * z01 +
            dx0 * dy1 * z10 +
            dx0 * dy0 * z11;

        return res / (delta_x * delta_y);
    }
};

struct Index2D {
    int x0;
    int y0;
    int x1;
    int y1;
    int z00;
    int z01;
    int z10;
    int z11;
    Index2D(int x0, int y0, int x1, int y1, int z00, int z01, int z10, int z11)
        : x0(x0), y0(y0), x1(x1), y1(y1), z00(z00), z01(z01), z10(z10), z11(z11) {}
};

template<typename T>
class Indexer2D {
public:
    virtual Index2D operator()(T const & x, T const & y) const = 0;
};

template<typename T, typename IndexerT>
class GridIndexer2D : public Indexer2D<T> {
    std::map<std::tuple<int, int>, int> xy_to_z_map;
    IndexerT x_indexer;
    IndexerT y_indexer;
public:
    GridIndexer2D() {};
    GridIndexer2D(std::vector<T> x, std::vector<T> y) {
        std::set<T> unique_x(x.begin(), x.end());
        std::set<T> unique_y(y.begin(), y.end());
        std::vector<T> sorted_x(unique_x.begin(), unique_x.end());
        std::vector<T> sorted_y(unique_y.begin(), unique_y.end());
        std::sort(sorted_x.begin(), sorted_x.end());
        std::sort(sorted_y.begin(), sorted_y.end());
        x_indexer = IndexerT(sorted_x);
        y_indexer = IndexerT(sorted_y);
        std::map<T, size_t> x_map;
        std::map<T, size_t> y_map;
        for(size_t i=0; i<sorted_x.size(); ++i)
            x_map.insert({sorted_x[i], i});
        for(size_t i=0; i<sorted_y.size(); ++i)
            y_map.insert({sorted_y[i], i});
        for(size_t i=0; i<x.size(); ++i)
            xy_to_z_map.insert({{x_map[x[i]], y_map[y[i]]}, i});
    };

    virtual Index2D operator()(T const & x, T const & y) const override {
        std::tuple<int, int> x_indices = x_indexer(x);
        std::tuple<int, int> y_indices = y_indexer(y);
        int x0 = std::get<0>(x_indices);
        int y0 = std::get<0>(y_indices);
        int x1 = std::get<1>(x_indices);
        int y1 = std::get<1>(y_indices);
        std::tuple<int, int> xy00(x0, y0);
        std::tuple<int, int> xy01(x0, y1);
        std::tuple<int, int> xy10(x1, y0);
        std::tuple<int, int> xy11(x1, y1);
        int z00 = xy_to_z_map.at(xy00);
        int z01 = xy_to_z_map.at(xy01);
        int z10 = xy_to_z_map.at(xy10);
        int z11 = xy_to_z_map.at(xy11);
        return Index2D(x0, y0, x1, y1, z00, z01, z10, z11);
    }
};

template<typename T>
using RegularGridIndexer2D = GridIndexer2D<T, RegularIndexer1D<T>>;

template<typename T>
using IrregularGridIndexer2D = GridIndexer2D<T, IrregularIndexer1D<T>>;

template<typename T>
class Interpolator2D {
public:
    T operator()(T x, T y) const = 0;
};

template<typename T>
class GridLinearInterpolator2D : public Interpolator2D<T> {
    // Data
    std::vector<T> t_x;
    std::vector<T> t_y;
    std::vector<T> t_z;

    // Indexer
    IrregularGridIndexer2D<T> indexer;

    // Interpolation info
    std::shared_ptr<BiLinearInterpolationOperator<T>> interp;
    std::shared_ptr<Transform<T>> x_transform;
    std::shared_ptr<Transform<T>> y_transform;
    std::shared_ptr<Transform<T>> z_transform;


public:
    GridLinearInterpolator2D(std::vector<T> x, std::vector<T> y, std::vector<T> z, std::shared_ptr<BiLinearInterpolationOperator<T>> interp, std::shared_ptr<Transform<T>> x_transform, std::shared_ptr<Transform<T>> y_transform, std::shared_ptr<Transform<T>> z_transform)
        : t_x(), t_y(), t_z(), indexer(x, y), interp(interp), x_transform(x_transform), y_transform(y_transform), z_transform(z_transform) {
        t_x.reserve(x.size());
        t_y.reserve(y.size());
        t_z.reserve(y.size());
        for(size_t i=0; i<x.size(); ++i) {
            t_x.push_back(x_transform->operator()(x[i]));
            t_y.push_back(y_transform->operator()(y[i]));
            t_z.push_back(z_transform->operator()(z[i]));
        }
    }

    virtual T operator()(T x, T y) const override {
        Index2D indices = indexer(x);
        T const & t_x0 = t_x[indices.x0];
        T const & t_y0 = t_y[indices.y0];
        T const & t_x1 = t_x[indices.x1];
        T const & t_y1 = t_y[indices.x1];
        T const & t_z00 = t_z[indices.z00];
        T const & t_z01 = t_z[indices.z01];
        T const & t_z10 = t_z[indices.z10];
        T const & t_z11 = t_z[indices.z11];

        T t_x = x_transform->Function(x);
        T t_y = y_transform->Function(y);

        T t_res = interp->operator()(t_x0, t_y0, t_x1, t_y1, t_z00, t_z01, t_z10, t_z11, t_x, t_y);
        T res = z_transform->Inverse(t_res);
        return res;
    }
};

}
}

#endif // LI_Interpolation_H
