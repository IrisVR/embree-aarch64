// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "../common/ray.h"
#include "../common/scene_grid_mesh.h"
#include "../bvh/bvh.h"

namespace embree
{
    /* Stores M quads from an indexed face set */
      struct SubGrid
      {
        /* Virtual interface to query information about the quad type */
        struct Type : public PrimitiveType
        {
          Type();
          size_t size(const char* This) const;
        };
        static Type type;

      public:

        /* primitive supports multiple time segments */
        static const bool singleTimeSegment = false;

        /* Returns maximum number of stored quads */
        static __forceinline size_t max_size() { return 1; }

        /* Returns required number of primitive blocks for N primitives */
        static __forceinline size_t blocks(size_t N) { return (N+max_size()-1)/max_size(); }

      public:

        /* Default constructor */
        __forceinline SubGrid() {  }

        /* Construction from vertices and IDs */
        __forceinline SubGrid(const unsigned int x,
                              const unsigned int y,
                              const unsigned int geomID,
                              const unsigned int primID)
          : _x(x), _y(y), _geomID(geomID), _primID(primID)
        {
        }

        __forceinline bool invalid3x3X() const { return (unsigned int)_x & (1<<15); }
        __forceinline bool invalid3x3Y() const { return (unsigned int)_y & (1<<15); }

        /* Gather the quads */
        __forceinline void gather(Vec3vf4& p0,
                                  Vec3vf4& p1,
                                  Vec3vf4& p2,
                                  Vec3vf4& p3,
                                  const GridMesh* const mesh,
                                  const GridMesh::Grid &g) const
        {
          /* first quad always valid */
          const size_t vtxID00 = g.startVtxID + x() + y() * g.lineVtxOffset;
          const size_t vtxID01 = vtxID00 + 1;
          const vfloat4 vtx00  = vfloat4::loadu(mesh->vertexPtr(vtxID00));
          const vfloat4 vtx01  = vfloat4::loadu(mesh->vertexPtr(vtxID01));
          const size_t vtxID10 = vtxID00 + g.lineVtxOffset;
          const size_t vtxID11 = vtxID01 + g.lineVtxOffset;
          const vfloat4 vtx10  = vfloat4::loadu(mesh->vertexPtr(vtxID10));
          const vfloat4 vtx11  = vfloat4::loadu(mesh->vertexPtr(vtxID11));

          /* deltaX => vtx02, vtx12 */
          const size_t deltaX  = invalid3x3X() ? 0 : 1;
          const size_t vtxID02 = vtxID01 + deltaX;       
          const vfloat4 vtx02  = vfloat4::loadu(mesh->vertexPtr(vtxID02));
          const size_t vtxID12 = vtxID11 + deltaX;       
          const vfloat4 vtx12  = vfloat4::loadu(mesh->vertexPtr(vtxID12));

          /* deltaY => vtx20, vtx21 */
          const size_t deltaY  = invalid3x3Y() ? 0 : g.lineVtxOffset;
          const size_t vtxID20 = vtxID10 + deltaY;
          const size_t vtxID21 = vtxID11 + deltaY;
          const vfloat4 vtx20  = vfloat4::loadu(mesh->vertexPtr(vtxID20));
          const vfloat4 vtx21  = vfloat4::loadu(mesh->vertexPtr(vtxID21));

          /* deltaX/deltaY => vtx22 */
          const size_t vtxID22 = vtxID11 + deltaX + deltaY;       
          const vfloat4 vtx22  = vfloat4::loadu(mesh->vertexPtr(vtxID22));

          transpose(vtx00,vtx01,vtx11,vtx10,p0.x,p0.y,p0.z);
          transpose(vtx01,vtx02,vtx12,vtx11,p1.x,p1.y,p1.z);
          transpose(vtx11,vtx12,vtx22,vtx21,p2.x,p2.y,p2.z);
          transpose(vtx10,vtx11,vtx21,vtx20,p3.x,p3.y,p3.z);          
          
        }

        /* Gather the quads */
        __forceinline void gather(Vec3vf4& p0,
                                  Vec3vf4& p1,
                                  Vec3vf4& p2,
                                  Vec3vf4& p3,
                                  const Scene *const scene) const
        {
          const GridMesh* const mesh = scene->get<GridMesh>(geomID());
          const GridMesh::Grid &g    = mesh->grid(primID());
          gather(p0,p1,p2,p3,mesh,g);
        }

        /* Gather the quads */
        __forceinline void gather(Vec3fa vtx[16], const Scene *const scene) const
        {
          const GridMesh* mesh     = scene->get<GridMesh>(geomID());
          const GridMesh::Grid &g  = mesh->grid(primID());

          /* first quad always valid */
          const size_t vtxID00 = g.startVtxID + x() + y() * g.lineVtxOffset;
          const size_t vtxID01 = vtxID00 + 1;
          const Vec3fa vtx00  = Vec3fa::loadu(mesh->vertexPtr(vtxID00));
          const Vec3fa vtx01  = Vec3fa::loadu(mesh->vertexPtr(vtxID01));
          const size_t vtxID10 = vtxID00 + g.lineVtxOffset;
          const size_t vtxID11 = vtxID01 + g.lineVtxOffset;
          const Vec3fa vtx10  = Vec3fa::loadu(mesh->vertexPtr(vtxID10));
          const Vec3fa vtx11  = Vec3fa::loadu(mesh->vertexPtr(vtxID11));

          /* deltaX => vtx02, vtx12 */
          const size_t deltaX  = invalid3x3X() ? 0 : 1;
          const size_t vtxID02 = vtxID01 + deltaX;       
          const Vec3fa vtx02  = Vec3fa::loadu(mesh->vertexPtr(vtxID02));
          const size_t vtxID12 = vtxID11 + deltaX;       
          const Vec3fa vtx12  = Vec3fa::loadu(mesh->vertexPtr(vtxID12));

          /* deltaY => vtx20, vtx21 */
          const size_t deltaY  = invalid3x3Y() ? 0 : g.lineVtxOffset;
          const size_t vtxID20 = vtxID10 + deltaY;
          const size_t vtxID21 = vtxID11 + deltaY;
          const Vec3fa vtx20  = Vec3fa::loadu(mesh->vertexPtr(vtxID20));
          const Vec3fa vtx21  = Vec3fa::loadu(mesh->vertexPtr(vtxID21));

          /* deltaX/deltaY => vtx22 */
          const size_t vtxID22 = vtxID11 + deltaX + deltaY;       
          const Vec3fa vtx22  = Vec3fa::loadu(mesh->vertexPtr(vtxID22));

          vtx[ 0] = vtx00; vtx[ 1] = vtx01; vtx[ 2] = vtx11; vtx[ 3] = vtx10;
          vtx[ 4] = vtx01; vtx[ 5] = vtx02; vtx[ 6] = vtx12; vtx[ 7] = vtx11;
          vtx[ 8] = vtx10; vtx[ 9] = vtx11; vtx[10] = vtx21; vtx[11] = vtx20;
          vtx[12] = vtx11; vtx[13] = vtx12; vtx[14] = vtx22; vtx[15] = vtx21;

        }

        /* Calculate the bounds of the subgrid */
        __forceinline const BBox3fa bounds(const Scene *const scene, const size_t itime=0) const
        {
          BBox3fa bounds = empty;
          FATAL("not implemented yet");
          return bounds;
        }

        /* Calculate the linear bounds of the primitive */
        __forceinline LBBox3fa linearBounds(const Scene* const scene, const size_t itime)
        {
          return LBBox3fa(bounds(scene,itime+0),bounds(scene,itime+1));
        }

        __forceinline LBBox3fa linearBounds(const Scene *const scene, size_t itime, size_t numTimeSteps)
        {
          LBBox3fa allBounds = empty;
          //const GridMesh* mesh = scene->get<GridMesh>(geomID);
          //allBounds.extend(mesh->linearBounds(primID, itime, numTimeSteps));
          FATAL("not implemented yet");
          return allBounds;
        }

        __forceinline LBBox3fa linearBounds(const Scene *const scene, const BBox1f time_range)
        {
          LBBox3fa allBounds = empty;
          FATAL("not implemented yet");
          return allBounds;
        }


        friend std::ostream& operator<<(std::ostream& cout, const SubGrid& sg) {
          return cout << "SubGrid " << " ( x " << sg.x() << ", y = " << sg.y() << ", geomID = " << sg.geomID() << ", primID = " << sg.primID() << " )";
        }

        __forceinline unsigned int geomID() const { return _geomID; }
        __forceinline unsigned int primID() const { return _primID; }
        __forceinline unsigned int x() const { return (unsigned int)_x & 0x7fff; }
        __forceinline unsigned int y() const { return (unsigned int)_y & 0x7fff; }

      private:
        unsigned short _x;
        unsigned short _y;
        unsigned int _geomID;    // geometry ID of mesh
        unsigned int _primID;    // primitive ID of primitive inside mesh
      };

    /* Stores M quads from an indexed face set */
      template<int N>
      struct SubGridQBVHN
      {
        static const unsigned char MIN_QUAN = 0;
        static const unsigned char MAX_QUAN = 255;

        /* Virtual interface to query information about the quad type */
        struct Type : public PrimitiveType
        {
          Type();
          size_t size(const char* This) const;
        };
        static Type type;

      public:

        /* primitive supports multiple time segments */
        static const bool singleTimeSegment = false;

        /* Returns maximum number of stored quads */
        static __forceinline size_t max_size() { return 1; }

        /* Returns required number of primitive blocks for M primitives */
        //static __forceinline size_t blocks(size_t N) { PING; return (N+max_size()-1)/max_size(); }

      public:

        /* Default constructor */
        __forceinline SubGridQBVHN() {  }

        /* Construction from vertices and IDs */
        __forceinline SubGridQBVHN(const unsigned int x[N],
                                   const unsigned int y[N],
                                   const unsigned int primID[N],
                                   const unsigned int geomID,
                                   const unsigned int items)
        {
          _geomID = geomID;
          for (size_t i=0;i<items;i++)
            subgridIDs[i] = SubGridID(x[i],y[i],primID[i]);
          for (size_t i=items;i<N;i++)
            subgridIDs[i] = SubGridID(0,0,0);            
        }


        friend std::ostream& operator<<(std::ostream& cout, const SubGridQBVHN& sg) {
          cout << "SubGridQBVHN ";
          for (size_t i=0;i<N;i++)
            cout << " ( x = " << sg.subgridIDs[i].x << ", y = " << sg.subgridIDs[i].y << ", primID = " << sg.subgridIDs[i].primID << " ), ";
          cout << "geomID " << sg._geomID << " ";
          return cout;
        }

        __forceinline unsigned int geomID() const { return _geomID; }
        __forceinline unsigned int primID(const unsigned int i) const { assert(i < N); return subgridIDs[i].primID; }


      public:
        struct SubGridID {
          unsigned short x;
          unsigned short y;
          unsigned int primID;

          __forceinline SubGridID(const unsigned int x, const unsigned int y, const unsigned int primID) :
          x(x), y(y), primID(primID) {}

        } subgridIDs[N];
      private:
        unsigned int _geomID;    // geometry ID of mesh

        Vec3f start;
        Vec3f scale;

        union {
          struct {
            unsigned char lower_x[N]; //!< 8bit discretized X dimension of lower bounds of all N children
            unsigned char upper_x[N]; //!< 8bit discretized X dimension of upper bounds of all N children
            unsigned char lower_y[N]; //!< 8bit discretized Y dimension of lower bounds of all N children
            unsigned char upper_y[N]; //!< 8bit discretized Y dimension of upper bounds of all N children
            unsigned char lower_z[N]; //!< 8bit discretized Z dimension of lower bounds of all N children
            unsigned char upper_z[N]; //!< 8bit discretized Z dimension of upper bounds of all N children
          };
          unsigned char all_planes[6*N];
        };

        static __forceinline void init_dim(const vfloat<N> &lower,
                                           const vfloat<N> &upper,
                                           unsigned char lower_quant[N],
                                           unsigned char upper_quant[N],
                                           float &start,
                                           float &scale)
        {
          const vbool<N> m_valid = lower != vfloat<N>(pos_inf);
          const float minF = reduce_min(lower);
          const float maxF = reduce_max(upper);
          float diff = maxF - minF; //todo: add extracted difference here
          float scale_diff = diff / float(MAX_QUAN);

          /* accomodate floating point accuracy issues in 'diff' */
          size_t iterations = 0;
          while(minF + scale_diff * float(MAX_QUAN) < maxF)
          {
            diff = nextafter(diff, FLT_MAX);
            scale_diff = diff / float(MAX_QUAN);
            iterations++;
          }
          const float inv_diff   = float(MAX_QUAN) / diff;

          vfloat<N> floor_lower = floor(  (lower - vfloat<N>(minF)) * vfloat<N>(inv_diff) );
          vfloat<N> ceil_upper  = ceil (  (upper - vfloat<N>(minF)) * vfloat<N>(inv_diff) );
          vint<N> i_floor_lower( floor_lower );
          vint<N> i_ceil_upper ( ceil_upper  );

          i_ceil_upper = min(i_ceil_upper,(int)MAX_QUAN);

          /* lower/upper correction */
          vbool<N> m_lower_correction = ((madd(vfloat<N>(i_floor_lower),scale_diff,minF)) > lower) & m_valid;
          vbool<N> m_upper_correction = ((madd(vfloat<N>(i_ceil_upper),scale_diff,minF)) < upper) & m_valid;
          i_floor_lower  = select(m_lower_correction,i_floor_lower-1,i_floor_lower);
          i_ceil_upper   = select(m_upper_correction,i_ceil_upper +1,i_ceil_upper);

          /* disable invalid lanes */
          i_floor_lower = select(m_valid,i_floor_lower,MAX_QUAN);
          i_ceil_upper  = select(m_valid,i_ceil_upper ,0);

          /* store as uchar to memory */
          vint<N>::store(lower_quant,i_floor_lower);
          vint<N>::store(upper_quant,i_ceil_upper);
          start = minF;
          scale = scale_diff;

#if defined(DEBUG)
          vfloat<N> extract_lower( vint<N>::load(lower_quant) );
          vfloat<N> extract_upper( vint<N>::load(upper_quant) );
          vfloat<N> final_extract_lower = madd(extract_lower,scale_diff,minF);
          vfloat<N> final_extract_upper = madd(extract_upper,scale_diff,minF);
          assert( (movemask(final_extract_lower <= lower ) & movemask(m_valid)) == movemask(m_valid));
          assert( (movemask(final_extract_upper >= upper ) & movemask(m_valid)) == movemask(m_valid));
#endif
        }


        __forceinline vfloat<N> dequantizeLowerX() const { return madd(vfloat<N>(vint<N>::load(lower_x)),scale.x,vfloat<N>(start.x)); }

        __forceinline vfloat<N> dequantizeUpperX() const { return madd(vfloat<N>(vint<N>::load(upper_x)),scale.x,vfloat<N>(start.x)); }

        __forceinline vfloat<N> dequantizeLowerY() const { return madd(vfloat<N>(vint<N>::load(lower_y)),scale.y,vfloat<N>(start.y)); }

        __forceinline vfloat<N> dequantizeUpperY() const { return madd(vfloat<N>(vint<N>::load(upper_y)),scale.y,vfloat<N>(start.y)); }

        __forceinline vfloat<N> dequantizeLowerZ() const { return madd(vfloat<N>(vint<N>::load(lower_z)),scale.z,vfloat<N>(start.z)); }

        __forceinline vfloat<N> dequantizeUpperZ() const { return madd(vfloat<N>(vint<N>::load(upper_z)),scale.z,vfloat<N>(start.z)); }

        template <int M>
        __forceinline vfloat<M> dequantize(const size_t offset) const { return vfloat<M>(vint<M>::loadu(all_planes+offset)); }


      };

      typedef SubGridQBVHN<4> SubGridQBVH4;
}
