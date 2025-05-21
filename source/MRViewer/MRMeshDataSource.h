#pragma once

#include "MRViewerFwd.h"
#include "MRMesh/MRMeshFwd.h"

#include <MeshVS_DataSource.hxx>
#include <MeshVS_EntityType.hxx>
#include <Standard_Handle.hxx>
#include <Standard_Type.hxx>
#include <TColStd_PackedMapOfInteger.hxx>
#include <TColStd_HArray2OfInteger.hxx>
#include <TColStd_HArray2OfReal.hxx>

//! Data source for mesh
//! Basically the same as XSDRAWSTLVRML_DataSource but it allows to be free of TKXSDRAW
class MRVIEWER_CLASS MeshDataSource : public MeshVS_DataSource
{
  DEFINE_STANDARD_RTTIEXT(MeshDataSource, MeshVS_DataSource)
public:
  using Mesh = MR::Mesh;
  /**
   * @brief Constructs a MeshDataSource for the given MR::Mesh.
   * @param theMesh The mesh data to be visualized.
   */
  MRVIEWER_API MeshDataSource(const Mesh& theMesh);

  /**
   * @brief Retrieves geometry information for a specific node or element.
   * @param ID The numerical identifier of the node or element.
   * @param IsElement True if ID refers to an element, false if it refers to a node.
   * @param[out] Coords An array to be filled with the coordinates.
   *                    For a node: X, Y, Z.
   *                    For an element: X1, Y1, Z1, X2, Y2, Z2, ... for each vertex.
   * @param[out] NbNodes The number of nodes (vertices) for the entity.
   * @param[out] Type The type of the entity (e.g., MeshVS_ET_Node, MeshVS_ET_Face).
   * @return True if geometry information was successfully retrieved, false otherwise.
   */
  MRVIEWER_API Standard_Boolean GetGeom(const Standard_Integer ID,
                                        const Standard_Boolean IsElement,
                                        TColStd_Array1OfReal&  Coords,
                                        Standard_Integer&      NbNodes,
                                        MeshVS_EntityType&     Type) const override;
  /**
   * @brief Retrieves the type of a specific node or element.
   * @param ID The numerical identifier of the node or element.
   * @param IsElement True if ID refers to an element, false if it refers to a node.
   * @param[out] Type The type of the entity.
   * @return True if the type was successfully retrieved, false otherwise.
   */
  MRVIEWER_API Standard_Boolean GetGeomType(const Standard_Integer ID,
                                            const Standard_Boolean IsElement,
                                            MeshVS_EntityType&     Type) const override;

  /**
   * @brief Returns a pointer to the underlying data structure for a node or element.
   *        In this implementation, it always returns nullptr as direct data structure access
   *        via this method is not utilized.
   * @param ID The numerical identifier of the node or element.
   * @param IsElement True if ID refers to an element, false if it refers to a node.
   * @return Always nullptr.
   */
  Standard_Address GetAddr(const Standard_Integer /*ID*/,
                           const Standard_Boolean /*IsElement*/) const override
  {
    return nullptr;
  }

  /**
   * @brief Retrieves the normal vector at a specific node of an element (face).
   *        Used for smooth shading.
   * @param ranknode The rank (index) of the node within the element (0, 1, or 2 for a triangle).
   * @param ElementId The numerical identifier of the element (face).
   * @param[out] nx The x-component of the normal vector.
   * @param[out] ny The y-component of the normal vector.
   * @param[out] nz The z-component of the normal vector.
   * @return True if the node normal was successfully retrieved, false otherwise.
   */
  MRVIEWER_API Standard_Boolean GetNodeNormal(const Standard_Integer ranknode,
                                              const Standard_Integer ElementId,
                                              Standard_Real&         nx,
                                              Standard_Real&         ny,
                                              Standard_Real&         nz) const override;

  /**
   * @brief Retrieves the identifiers of the nodes that form a given element.
   * @param ID The numerical identifier of the element.
   * @param[out] NodeIDs An array to be filled with the node identifiers.
   * @param[out] NbNodes The number of nodes in the element.
   * @return True if node information was successfully retrieved, false otherwise.
   */
  MRVIEWER_API Standard_Boolean GetNodesByElement(const Standard_Integer   ID,
                                                  TColStd_Array1OfInteger& NodeIDs,
                                                  Standard_Integer&        NbNodes) const override;

  /**
   * @brief Returns a map of all node identifiers in the mesh.
   * @return A constant reference to the packed map of node IDs.
   */
  const TColStd_PackedMapOfInteger& GetAllNodes() const override { return nodes_; }

  /**
   * @brief Returns a map of all element identifiers in the mesh.
   * @return A constant reference to the packed map of element IDs.
   */
  const TColStd_PackedMapOfInteger& GetAllElements() const override { return elements_; }

  /**
   * @brief Retrieves the normal vector of an element (face).
   *        Used for flat shading and other calculations.
   * @param Id The numerical identifier of the element (face).
   * @param Max (Unused in this implementation) Maximal number of nodes an element can consist of.
   * @param[out] nx The x-component of the normal vector.
   * @param[out] ny The y-component of the normal vector.
   * @param[out] nz The z-component of the normal vector.
   * @return True if the normal was successfully retrieved, false otherwise.
   */
  MRVIEWER_API Standard_Boolean GetNormal(const Standard_Integer Id,
                                          const Standard_Integer Max,
                                          Standard_Real&         nx,
                                          Standard_Real&         ny,
                                          Standard_Real&         nz) const override;

  /**
   * @brief Retrieves normals for all nodes of a given element, or a single normal for the element.
   * @param Id The numerical identifier of the element.
   * @param IsNodal If true, requests per-node normals. If false, requests a single per-element
   * normal (duplicated for all nodes if it's a face).
   * @param MaxNodes Maximal number of nodes an element can consist of (typically 3 for faces).
   * @param[out] Normals An array to be filled with the normal components (Nx1, Ny1, Nz1, Nx2, Ny2,
   * Nz2, ...).
   * @return True if normals were successfully retrieved, false otherwise.
   */
  MRVIEWER_API Standard_Boolean
    GetNormalsByElement(const Standard_Integer         Id,
                        const Standard_Boolean         IsNodal,
                        const Standard_Integer         MaxNodes,
                        Handle(TColStd_HArray1OfReal)& Normals) const override;

  /**
   * @brief Returns the bounding box of the entire mesh.
   * @return A Bnd_Box object representing the mesh's bounding box.
   */
  MRVIEWER_API Bnd_Box GetBoundingBox() const override;

private:
  const Mesh&                mesh_;     //!< Constant reference to the mesh data.
  TColStd_PackedMapOfInteger nodes_;    //!< Map of all node IDs.
  TColStd_PackedMapOfInteger elements_; //!< Map of all element (face) IDs.
};