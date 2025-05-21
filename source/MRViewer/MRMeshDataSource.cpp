#include "MRMeshDataSource.h"
#include "MRMesh/MRMesh.h"
#include "MRMesh/MRMeshTopology.h"
#include "MRMesh/MRBox.h" // For MR::Box3f

// OCCT
#include <Bnd_Box.hxx> // For Bnd_Box
#include <gp_Pnt.hxx>  // For gp_Pnt

IMPLEMENT_STANDARD_RTTIEXT(MeshDataSource, MeshVS_DataSource);
using namespace MR;

MeshDataSource::MeshDataSource(const Mesh& theMesh)
    : mesh_(theMesh)
{
  // Fill the nodes_ and elements_ maps with all valid vertices and faces
  const auto& validVerts = mesh_.topology.getValidVerts();
  const auto& validFaces = mesh_.topology.getValidFaces();
  nodes_.ReSize((int)validVerts.size());
  elements_.ReSize((int)validFaces.size());
  for (auto v : validVerts)
  {
    nodes_.Add(v);
  }

  for (auto f : validFaces)
  {
    elements_.Add(f);
  }
}

Standard_Boolean MeshDataSource::GetGeom(const Standard_Integer ID,
                                         const Standard_Boolean IsElement,
                                         TColStd_Array1OfReal&  Coords,
                                         Standard_Integer&      NbNodes,
                                         MeshVS_EntityType&     Type) const
{
  if (IsElement)
  {
    // Handle element (face) geometry
    if (ID >= 0 && mesh_.topology.hasFace(FaceId(ID)))
    {
      Type    = MeshVS_ET_Face;
      NbNodes = 3; // Triangle mesh

      // Get the three vertices of the face
      Vector3f v0, v1, v2;
      mesh_.getTriPoints(FaceId(ID), v0, v1, v2);

      // Fill the coordinates array
      const int coordsLow   = Coords.Lower();
      Coords(coordsLow)     = v0.x;
      Coords(coordsLow + 1) = v0.y;
      Coords(coordsLow + 2) = v0.z;
      Coords(coordsLow + 3) = v1.x;
      Coords(coordsLow + 4) = v1.y;
      Coords(coordsLow + 5) = v1.z;
      Coords(coordsLow + 6) = v2.x;
      Coords(coordsLow + 7) = v2.y;
      Coords(coordsLow + 8) = v2.z;

      return Standard_True;
    }
  }
  else
  {
    // Handle node (vertex) geometry
    if (ID >= 0 && mesh_.topology.hasVert(VertId(ID)))
    {
      Type    = MeshVS_ET_Node;
      NbNodes = 1;

      // Get the vertex coordinates
      const Vector3f& point = mesh_.points[VertId(ID)];

      // Fill the coordinates array
      const int coordsLow   = Coords.Lower();
      Coords(coordsLow)     = point.x;
      Coords(coordsLow + 1) = point.y;
      Coords(coordsLow + 2) = point.z;

      return Standard_True;
    }
  }

  return Standard_False;
}

Standard_Boolean MeshDataSource::GetGeomType(const Standard_Integer ID,
                                             const Standard_Boolean IsElement,
                                             MeshVS_EntityType&     Type) const
{
  if (IsElement)
  {
    if (ID >= 0 && mesh_.topology.hasFace(FaceId(ID)))
    {
      Type = MeshVS_ET_Face;
      return Standard_True;
    }
  }
  else
  {
    if (ID >= 0 && mesh_.topology.hasVert(VertId(ID)))
    {
      Type = MeshVS_ET_Node;
      return Standard_True;
    }
  }

  return Standard_False;
}

Standard_Boolean MeshDataSource::GetNodeNormal(const Standard_Integer ranknode,
                                               const Standard_Integer ElementId,
                                               Standard_Real&         nx,
                                               Standard_Real&         ny,
                                               Standard_Real&         nz) const
{
  if (ElementId >= 0 && mesh_.topology.hasFace(FaceId(ElementId)) && ranknode >= 0 && ranknode < 3)
  {
    // Get the three vertices of the face
    VertId verts[3];
    mesh_.topology.getTriVerts(FaceId(ElementId), verts);

    // Get the normal at the specified vertex
    if (ranknode < 3 && verts[ranknode].valid())
    {
      Vector3f normal = mesh_.normal(verts[ranknode]);
      nx              = normal.x;
      ny              = normal.y;
      nz              = normal.z;
      return Standard_True;
    }
  }

  return Standard_False;
}

Standard_Boolean MeshDataSource::GetNodesByElement(const Standard_Integer   ID,
                                                   TColStd_Array1OfInteger& NodeIDs,
                                                   Standard_Integer&        NbNodes) const
{
  if (ID >= 0 && mesh_.topology.hasFace(FaceId(ID)))
  {
    // Get the three vertices of the face
    VertId verts[3];
    mesh_.topology.getTriVerts(FaceId(ID), verts);

    NbNodes = 3; // Triangle mesh

    // Fill the node IDs array
    const int nodesLow    = NodeIDs.Lower();
    NodeIDs(nodesLow)     = verts[0];
    NodeIDs(nodesLow + 1) = verts[1];
    NodeIDs(nodesLow + 2) = verts[2];

    return Standard_True;
  }

  return Standard_False;
}

Standard_Boolean MeshDataSource::GetNormal(const Standard_Integer Id,
                                           const Standard_Integer /*Max*/,
                                           Standard_Real& nx,
                                           Standard_Real& ny,
                                           Standard_Real& nz) const
{
  if (Id >= 0 && mesh_.topology.hasFace(FaceId(Id)))
  {
    // Get the normal of the face
    Vector3f normal = mesh_.normal(FaceId(Id));
    nx              = normal.x;
    ny              = normal.y;
    nz              = normal.z;
    return Standard_True;
  }

  return Standard_False;
}

Standard_Boolean MeshDataSource::GetNormalsByElement(
  const Standard_Integer Id,
  const Standard_Boolean IsNodal,
  const Standard_Integer /*MaxNodes*/, // MaxNodes is not strictly needed for triangles, but passed
                                       // by OCCT
  Handle(TColStd_HArray1OfReal)& Normals) const
{
  if (Id < 0 || !mesh_.topology.hasFace(FaceId(Id)))
  {
    return Standard_False;
  }

  const Standard_Integer nbElementNodes = 3; // Triangles
  Normals                               = new TColStd_HArray1OfReal(1, 3 * nbElementNodes);

  if (IsNodal)
  {
    // Get per-node normals
    VertId verts[3];
    mesh_.topology.getTriVerts(FaceId(Id), verts);
    bool allNormalsOk = true;
    for (Standard_Integer i = 0; i < nbElementNodes; ++i)
    {
      if (verts[i].valid())
      {
        Vector3f normal = mesh_.normal(verts[i]);
        Normals->SetValue(i * 3 + 1, normal.x);
        Normals->SetValue(i * 3 + 2, normal.y);
        Normals->SetValue(i * 3 + 3, normal.z);
      }
      else
      {
        allNormalsOk = false;
        break;
      }
    }
    return allNormalsOk ? Standard_True : Standard_False;
  }
  else
  {
    // Get a single per-element (face) normal and duplicate it
    Vector3f faceNormal = mesh_.normal(FaceId(Id));
    for (Standard_Integer i = 0; i < nbElementNodes; ++i)
    {
      Normals->SetValue(i * 3 + 1, faceNormal.x);
      Normals->SetValue(i * 3 + 2, faceNormal.y);
      Normals->SetValue(i * 3 + 3, faceNormal.z);
    }
    return Standard_True;
  }
}

Bnd_Box MeshDataSource::GetBoundingBox() const
{
  if (mesh_.topology.numValidVerts() == 0)
  {
    return {}; // Return an empty/invalid box if no vertices
  }

  // Try to get the bounding box from the AABB tree first (cached, faster if available)
  MR::Box3f mrBox = mesh_.getBoundingBox();

  if (!mrBox.valid())
  {
    // If AABB tree's box is not valid (e.g., tree not built or invalidated),
    // compute it directly by iterating over vertices (more robust, potentially slower)
    mrBox = mesh_.computeBoundingBox();
  }

  if (!mrBox.valid())
  {
    return {}; // Still invalid, return empty OCC box
  }

  return Bnd_Box(gp_Pnt(mrBox.min.x, mrBox.min.y, mrBox.min.z),
                 gp_Pnt(mrBox.max.x, mrBox.max.y, mrBox.max.z));
}