/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_COMPILER_XLA_ARRAY4D_H_
#define TENSORFLOW_COMPILER_XLA_ARRAY4D_H_

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "array2d.h"
#include "types.h"
#include "array_slice.h"
#include "str_util.h"
#include "strcat.h"
#include "stringprintf.h"
#include "logging.h"
#include "macros.h"
#include "base.h"

#include "tensor_array.h"

//#include "tensorflow/compiler/xla/array2d.h"
//#include "tensorflow/compiler/xla/types.h"
//#include "tensorflow/core/lib/gtl/array_slice.h"
//#include "tensorflow/core/lib/strings/str_util.h"
//#include "tensorflow/core/lib/strings/strcat.h"
//#include "tensorflow/core/lib/strings/stringprintf.h"
//#include "tensorflow/core/platform/logging.h"
//#include "tensorflow/core/platform/macros.h"
//#include "tensorflow/core/platform/types.h"

namespace xla {

// Simple 4D array structure, similar in form to Array2D, for use primarily in
// testing and describing to XLA APIs values in the 4D array structures used
// in convolutions.
//
// The data layout is, in order from major to minor:
//
//    First dimension: plane, batch, n1
//   Second dimension: depth, feature, z, n2
//    Third dimension: height, y, n3
//   Fourth dimension: width, x, n4
//
// These dimensions are referred to by various names, so that is why
// more than one name is given above. See operator() for the exact
// calculation of 1d indices from 4d indices.
template <typename T>
class Array4D : public TensorArray
{
 public:
  // Creates a 4D array, unitialized values.
  Array4D(int64 planes, int64 depth, int64 height, int64 width)
      : TensorArray(planes, depth, height, width)
      , planes_(planes),
        depth_(depth),
        height_(height),
        width_(width),
        values_(planes * depth * height * width) {}

  // Creates a 4D array, initalized to value.
  Array4D(int64 planes, int64 depth, int64 height, int64 width, T value)
     : TensorArray(planes, depth, height, width)
     , planes_(planes),
        depth_(depth),
        height_(height),
        width_(width),
        values_(planes * depth * height * width, value) {}

  // Creates a 4D array, initalized by specified array.
  // precondition: array.size = planes*depth*height*width
  // tf.reshape(channel, z, y, x)
  Array4D(int64 planes, int64 depth, int64 height, int64 width, const std::vector<T>& input_array)
     : TensorArray(planes, depth, height, width)
     , planes_(planes),
     depth_(depth),
     height_(height),
     width_(width),
     values_(input_array) 
  {
     CHECK_EQ(planes * depth * height * width, static_cast<int64>(input_array.size()));
  }

  // Creates a 4D array, filled with values.
  //
  // We need to set a default type for Container so that code like
  // Array4D(1, 1, 1, 1, {1}) will work. The template cannot infer the
  // initializer_list type in that case without this default.
  template <typename Container = std::initializer_list<T>>
  Array4D(int64 planes, int64 depth, int64 height, int64 width,
          const Container& values)
      : Array4D(planes, depth, height, width) {
    SetValues(values);
  }

  // Construct an Array4D with the given nested initializer list.
  Array4D(std::initializer_list<std::initializer_list<
              std::initializer_list<std::initializer_list<T>>>>
              values)
      : Array4D(values.size(), values.begin()->size(),
                values.begin()->begin()->size(),
                values.begin()->begin()->begin()->size()) {
    int64 plane = 0;
    for (const auto values_in_plane : values) {
      DCHECK_EQ(values_in_plane.size(), depth_);
      int64 depth = 0;
      for (const auto values_in_depth : values_in_plane) {
        DCHECK_EQ(values_in_depth.size(), height_);
        int64 height = 0;
        for (const auto values_in_height : values_in_depth) {
          DCHECK_EQ(values_in_height.size(), width_);
          int64 width = 0;
          for (const auto element_value : values_in_height) {
            (*this)(plane, depth, height, width) = element_value;
            ++width;
          }
          ++height;
        }
        ++depth;
      }
      ++plane;
    }
  }

  T& operator()(int64 plane, int64 depth, int64 height, int64 width) {
    CHECK_LT(plane, planes_);
    CHECK_LT(depth, depth_);
    CHECK_LT(height, height_);
    CHECK_LT(width, width_);
    //const int64 id = (((plane * size(3) + depth) * size(2) + height) * size(1)) + width;
    return values_[plane * (depth_ * height_ * width_) +
                   depth * (height_ * width_) + height * (width_) + width];

  }
  const T& operator()(int64 plane, int64 depth, int64 height,
                      int64 width) const {
    return const_cast<Array4D*>(this)->operator()(plane, depth, height, width);
  }

  bool operator == (const Array4D<T>& rhs) const
  {
     bool result = (num_elements() == rhs.num_elements());
     result = result 
        && (planes() == rhs.planes()) 
        && (depth() == rhs.depth()) 
        && (height() == rhs.height()) 
        && (width() == rhs.width());

     if (result)
     {
        //CHECK_EQ(num_elements(), rhs.num_elements());
        for (size_t i = 0; i < values_.size() && result; i++)
        {
           result = (std::abs(values_[i] - rhs.values_[i]) < 0.000001f);
        }
     }
     return result;
  }

  int64 width() const { return width_; }
  int64 height() const { return height_; }
  int64 depth() const { return depth_; }
  int64 planes() const { return planes_; }

  // Numerically-named aliases for the various dimensions. This matches the
  // dimension names used in array3d.
  int64 n4() const { return width_; }
  int64 n3() const { return height_; }
  int64 n2() const { return depth_; }
  int64 n1() const { return planes_; }
  int64 num_elements() const { return values_.size(); }

  const T* data() const { return const_cast<Array4D*>(this)->values_.data(); }

  // Sets all the values in the array to values.
  template <typename Container = std::initializer_list<T>>
  void SetValues(const Container& container) {
    CHECK_EQ(std::distance(std::begin(container), std::end(container)),
             num_elements());
    values_.assign(std::begin(container), std::end(container));
  }

  // Fills the array with the given value.
  void Fill(const T& value) {
    std::fill(values_.begin(), values_.end(), value);
  }

  // Fills the array with iota.
  void FillIota(const T& value) {
    std::iota(values_.begin(), values_.end(), value);
  }

  // Fills the array with random variable with a deviation of value and a mean
  // of mean.
  void FillRandom(const T& value, const double mean = 0.0,
                  const int seed = 12345) {
    std::mt19937 g(seed);
    std::normal_distribution<double> distribution(mean,
                                                  static_cast<double>(value));
    for (auto& v : values_) {
      v = static_cast<T>(distribution(g));
    }
  }

  // Fills values with the sequence i*multiplier for i=0,1,...
  void FillWithMultiples(float multiplier) {
    for (int64 i = 0; i < num_elements(); ++i) {
      values_[i] = i * multiplier;
    }
  }

  void mul(float multiplier)
  {
     for (int64 i = 0; i < num_elements(); ++i) 
     {
        values_[i] *= multiplier;
     }
  }

  // Invokes a callback with the (indices, value_ptr) for each cell in the 4D
  // array.
  void Each(std::function<void(tensorflow::gtl::ArraySlice<int64>, T*)> f) {
    for (int64 plane = 0; plane < planes(); ++plane) {
      for (int64 depth = 0; depth < this->depth(); ++depth) {
        for (int64 height = 0; height < this->height(); ++height) {
          for (int64 width = 0; width < this->width(); ++width) {
            auto& value = (*this)(plane, depth, height, width);
            f({plane, depth, height, width}, &value);
          }
        }
      }
    }
  }

  // Fills all of the {p,z} with the array provided, which specifies {y,x}.
  void FillWithYX(const Array2D<T>& value) {
    CHECK_EQ(value.height(), height());
    CHECK_EQ(value.width(), width());
    for (int64 plane = 0; plane < planes(); ++plane) {
      for (int64 depth = 0; depth < this->depth(); ++depth) {
        for (int64 height = 0; height < this->height(); ++height) {
          for (int64 width = 0; width < this->width(); ++width) {
            (*this)(plane, depth, height, width) = value(height, width);
          }
        }
      }
    }
  }

  // Fills all of the {x,y} with the array provided, which specifies {p,z}.
  void FillWithPZ(const Array2D<T>& value) {
    CHECK_EQ(value.height(), planes());
    CHECK_EQ(value.width(), depth());
    for (int64 height = 0; height < this->height(); ++height) {
      for (int64 width = 0; width < this->width(); ++width) {
        for (int64 plane = 0; plane < planes(); ++plane) {
          for (int64 depth = 0; depth < this->depth(); ++depth) {
            (*this)(plane, depth, height, width) = value(plane, depth);
          }
        }
      }
    }
  }

  // Fills each of the minor-dim matrices with a number designating which minor
  // dim matrix is enclosed by the shape.
  void FillWithMinorDimNum() {
    LOG(INFO) << "width: " << this->width();
    LOG(INFO) << "height: " << this->height();
    LOG(INFO) << "depth: " << this->depth();
    LOG(INFO) << "planes: " << this->planes();
    for (int64 height = 0; height < this->height(); ++height) {
      for (int64 width = 0; width < this->width(); ++width) {
        for (int64 plane = 0; plane < planes(); ++plane) {
          for (int64 depth = 0; depth < this->depth(); ++depth) {
            float this_val = plane * this->depth() + depth;
            (*this)(plane, depth, height, width) = this_val;
          }
        }
      }
    }
  }

  // Returns a string representation of the 4D array suitable for debugging.
  string ToString() const {
    std::vector<string> pieces = {
        tensorflow::strings::Printf("p=%lld,z=%lld,y=%lld,x=%lld\n[\n", planes(),
                                    depth(), height(), width())};
    for (int64 plane = 0; plane < planes_; ++plane) {
      pieces.push_back("  {\n");
      for (int64 depth = 0; depth < depth_; ++depth) {
        pieces.push_back("    {\n");
        for (int64 height = 0; height < height_; ++height) {
          pieces.push_back("      {");
          for (int64 width = 0; width < width_; ++width) {
            pieces.push_back(tensorflow::strings::StrCat(
                (*this)(plane, depth, height, width), ", "));
          }
          pieces.push_back("},\n");
        }
        pieces.push_back("    },\n");
      }
      pieces.push_back("  },\n");
    }
    pieces.push_back("]");
    return tensorflow::str_util::Join(pieces, "");
  }

  const std::vector<T>& flatten() const
  {
     return values_;
  }

  std::vector<T>& flatten()
  {
     return values_;
  }

  template<typename U>
  std::unique_ptr<xla::Array4D<U>> convert() const
  {
     std::unique_ptr<xla::Array4D<U>> result(new xla::Array4D<U>(size(0), size(1), size(2), size(3)));

     std::vector<U>& to = result->flatten();

     for (size_t i = 0; i < values_.size(); i++)
     {
        to[i] = U(values_[i]);
     }

     return result;
  }

  void operator * (const Array4D<T>& rhs)
  {
     if (  (rhs.size(0) == this->size(0))
        && (rhs.size(1) == this->size(1))
        && (rhs.size(2) == this->size(2))
        && (rhs.size(3) == this->size(3))
        )
     {
        for (size_t i = 0; i < values_.size(); i++)
        {
           values_[i] *= rhs.values_[i];
        }
     }
     else
     {
        assert(false);
     }
  }

 private:
  int64 planes_;
  int64 depth_;
  int64 height_;
  int64 width_;
  std::vector<T> values_;
};


template <typename T>
void MatrixMul(const xla::Array4D<T>& lhs, const xla::Array4D<T>& rhs, xla::Array4D<T>& result)
{
   // multiply lsh(b, d, p, r) * rhs(b, d, r, q) = result(b, d, p, q)

   CHECK_EQ(lhs.Width(), rhs.Height());
   CHECK_EQ(lhs.Height(), result.Height());
   CHECK_EQ(rhs.Width(), result.Width());

   CHECK_EQ(lhs.Depth(), rhs.Depth());
   CHECK_EQ(rhs.Depth(), result.Depth());

   CHECK_EQ(lhs.Batch(), rhs.Batch());
   CHECK_EQ(rhs.Batch(), result.Batch());

   assert(lhs.Width() == rhs.Height());
   assert(lhs.Height() == result.Height());
   assert(rhs.Width() == result.Width());

   int64 i = 0;
   int64 j = 0;
   int64 r = 0;

   for (int64 b = 0; b < result.Batch(); b++)
   {
      for (int64 d = 0; d < result.Depth(); d++)
      {
         for (i = 0; i < lhs.Height(); i++)
         {
            for (j = 0; j < rhs.Width(); j++)
            {
               result(b, d, i, j) = 0.f;
               for (r = 0; r < lhs.Width(); r++)
               {
                  result(b, d, i, j) += lhs(b, d, i, r) * rhs(b, d, r, j);
               }
            }
         }
      }
   }
}

template <typename T>
std::unique_ptr<xla::Array4D<T>> MakeMatrixMul(const xla::Array4D<T>& lhs, const xla::Array4D<T>& rhs)
{
   assert(lhs.n1() == rhs.n1());
   assert(lhs.n2() == rhs.n2());

   std::unique_ptr<xla::Array4D<T>> result = xla::MakeUnique<xla::Array4D<T>>(lhs.n1(), lhs.n2(), lhs.n3(), rhs.n4());
   xla::MatrixMul(lhs, rhs, *result);

   return result;
}

}  // namespace xla

#endif  // TENSORFLOW_COMPILER_XLA_ARRAY4D_H_
