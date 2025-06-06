#pragma once

#include "MRMeshFwd.h"
#include "MRVectorTraits.h"

namespace MR
{

/// a ball = points surrounded by a sphere in arbitrary space with vector type V
template <typename V>
struct Ball
{
    using VTraits = VectorTraits<V>;
    using T = typename VTraits::BaseType;
    static constexpr int elements = VTraits::size;

    V center; ///< ball's center
    T radiusSq = 0; ///< ball's squared radius

    /// returns true if given point is strictly inside the ball (not on its spherical surface)
    bool inside( const V & pt ) const { return distanceSq( pt, center ) < radiusSq; }

    /// returns true if given point is strictly outside the ball (not on its spherical surface)
    bool outside( const V & pt ) const { return distanceSq( pt, center ) > radiusSq; }
};

} //namespace MR
