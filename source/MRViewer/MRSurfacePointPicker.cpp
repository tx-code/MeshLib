#include "MRSurfacePointPicker.h"
#include "MRViewport.h"
#include "MRViewer.h"
#include "MRMesh/MRObjectMesh.h"
#include "MRMesh/MRSphereObject.h"
#include "MRMesh/MRMesh.h"
#include "MRMesh/MRRingIterator.h"

#include "MRMesh/MRObjectPointsHolder.h"
#include "MRMesh/MRObjectLines.h"

#include "MRMesh/MRMeshTriPoint.h"
#include "MRMesh/MREdgePoint.h"
#include "MRMesh/MRPointOnObject.h"
#include "MRMesh/MRPointCloud.h"
#include "MRMesh/MRMatrix3Decompose.h"
#include "MRViewer/MRMenu.h"

#include <variant>

namespace MR
{

SurfacePointWidget::~SurfacePointWidget()
{
    reset();
}

const PickedPoint& SurfacePointWidget::create( const std::shared_ptr<VisualObject>& surface, const PointOnObject& startPos )
{
    if ( !surface )
    {
        currentPos_ = {};
        return currentPos_;
    }
    return create( surface, pointOnObjectToPickedPoint( surface.get(), startPos ) );
}

const PickedPoint& SurfacePointWidget::create( const std::shared_ptr<VisualObject>& surface, const PickedPoint& startPos )
{
    reset();
    if ( !surface )
    {
        currentPos_ = {};
        return currentPos_;
    }
    baseObject_ = surface;

    pickSphere_ = std::make_shared<SphereObject>();
    pickSphere_->setName( "Pick Sphere" );
    pickSphere_->setAncillary( true );
    setSphereColor_();
    pickSphere_->setGlobalAlpha( 255 );
    pickSphere_->setMainFeatureAlpha( 1.f );
    pickSphere_->setVisualizeProperty( false, DimensionsVisualizePropertyType::diameter, ViewportMask::all() );
    pickSphere_->setDecorationsColor( Color::transparent(), false );

    baseObject_->addChild( pickSphere_ );
    currentPos_ = startPos;
    updatePositionAndRadius_();

    // update the point radius on every base object's re-scale
    onBaseObjectWorldXfChanged_ = baseObject_->worldXfChangedSignal.connect( [this]
    {
        setPointRadius_();
    } );

    // 10 group to imitate plugins behavior
    connect( &getViewerInstance(), 10, boost::signals2::at_front );
    return currentPos_;
}


void SurfacePointWidget::reset()
{
    if ( !pickSphere_ )
        return;

    onBaseObjectWorldXfChanged_.disconnect();

    disconnect();
    pickSphere_->detachFromParent();
    pickSphere_.reset();

    baseObject_.reset();

    params_ = Parameters();
    isOnMove_ = false;
    isHovered_ = false;
    autoHover_ = true;
    startMove_ = {};
    onMove_ = {};
    endMove_ = {};
}

void SurfacePointWidget::setSphereColor_()
{
    if ( !pickSphere_ )
        return;
    if ( isOnMove_ )
        pickSphere_->setFrontColor( params_.activeColor, false );
    else if ( isHovered_ )
        pickSphere_->setFrontColor( params_.hoveredColor, false );
    else
        pickSphere_->setFrontColor( params_.baseColor, false );
    pickSphere_->setBackColor( pickSphere_->getFrontColor( false ) );
}

void SurfacePointWidget::setParameters( const Parameters& params )
{
    if ( pickSphere_ )
    {
        if ( params.positionType != params_.positionType ||
             params.radius != params_.radius )
        {
            updatePositionAndRadius_();
        }
    }
    params_ = params;
    setSphereColor_();
}

void SurfacePointWidget::setBaseColor( const Color& color )
{
    if ( params_.baseColor == color )
        return;
    params_.baseColor = color;
    setSphereColor_();
}

void SurfacePointWidget::updateParameters( const std::function<void( Parameters& )>& visitor )
{
    auto params = params_;
    visitor( params );
    setParameters( params );
}

void SurfacePointWidget::setHovered( bool on )
{
    if ( !isOnMove_ && isHovered_ != on )
    {
        isHovered_ = on;
        setSphereColor_();
    }
}

void SurfacePointWidget::startDragging()
{
    assert( !isOnMove_ );
    pickSphere_->setPickable( false );
    isOnMove_ = true;
    setSphereColor_();
    if ( startMove_ )
        startMove_( *this, currentPos_ );
}

bool SurfacePointWidget::onMouseDown_( Viewer::MouseButton button, int mod )
{
    if ( button != MouseButton::Left || !isHovered_ )
        return false;

    // check if modifier present and if there are exception for it.
    if ( ( mod != 0 ) && ( ( mod & params_.customModifiers ) != mod ) )
        return false;

    startDragging();
    return true;
}

bool SurfacePointWidget::onMouseUp_( Viewer::MouseButton button, int )
{
    if ( button != MouseButton::Left || !isOnMove_ )
        return false;
    isOnMove_ = false;
    pickSphere_->setPickable( true );
    setSphereColor_();
    if ( endMove_ )
        endMove_( *this, currentPos_ );
    return true;
}

bool SurfacePointWidget::onMouseMove_( int, int )
{
    if ( isOnMove_ )
    {
        auto [obj, pick] = getViewerInstance().viewport().pickRenderObject( { .exactPickFirst = params_.pickInBackFaceObject } );
        if ( obj != baseObject_ )
            return false;

        if ( ( params_.pickInBackFaceObject == false ) && ( isPickIntoBackFace( obj, pick, getViewerInstance().viewport().getCameraPoint() ) ) )
            return false;

        currentPos_ = pointOnObjectToPickedPoint( obj.get(), pick );
        updatePositionAndRadius_();
        if ( onMove_ )
            onMove_( *this, currentPos_ );
        return true;
    }
    else
    {
        if ( !autoHover_ )
            return false;

        auto [obj, pick] = getViewerInstance().viewport().pick_render_object();
        setHovered( obj == pickSphere_ );
    }
    return false;
}

void SurfacePointWidget::updatePositionAndRadiusMesh_( MeshTriPoint mtp )
{
    assert( pickSphere_ );
    auto baseSurface = std::dynamic_pointer_cast<ObjectMeshHolder>( baseObject_ );
    assert( baseSurface );
    assert( baseSurface->mesh() );
    const auto& mesh = *baseSurface->mesh();

    const auto f = mesh.topology.left( mtp.e );
    switch ( params_.positionType )
    {
        case PositionType::Faces:
            // nothing to change
            break;
        case PositionType::FaceCenters:
            currentPos_ = mesh.toTriPoint( f, mesh.triCenter( f ) );
            break;
        case PositionType::Edges:
            if ( !mtp.onEdge( mesh.topology ) )
            {
                const auto p = mesh.triPoint( mtp );
                EdgeId e = mesh.getClosestEdge( PointOnFace{ f, p } );
                if ( mesh.topology.left( e ) != f )
                    e = e.sym();
                const auto ep = mesh.edgePoint( mesh.toEdgePoint( e, p ) );
                currentPos_ = mesh.toTriPoint( f, ep );
            }
            break;
        case PositionType::EdgeCenters:
        {
            auto closestEdge = EdgeId( mesh.getClosestEdge( PointOnFace{ f, mesh.triPoint( mtp ) } ) );
            if ( mesh.topology.left( closestEdge ) != f )
                closestEdge = closestEdge.sym();
            mtp.e = closestEdge;
            mtp.bary.a = 0.5f;
            mtp.bary.b = 0.0f;
            currentPos_ = mtp;
            break;
        }
        case PositionType::Verts:
            if ( !mtp.inVertex() )
            {
                auto closestVert = mesh.getClosestVertex( PointOnFace{ f, mesh.triPoint( mtp ) } );
                for ( auto e : orgRing( mesh.topology, closestVert ) )
                {
                    if ( mesh.topology.left( e ) == f )
                    {
                        mtp.e = e;
                        mtp.bary.a = 0.0f;
                        mtp.bary.b = 0.0f;
                        currentPos_ = mtp;
                        break;
                    }
                }
            }
            break;
    }
}

void SurfacePointWidget::updatePositionAndRadius_()
{
    if ( const MeshTriPoint* triPoint = std::get_if<MeshTriPoint>( &currentPos_ ) )
        updatePositionAndRadiusMesh_( *triPoint );

    if ( auto p = findCoords() )
    {
        pickSphere_->setCenter( *p );
        setPointRadius_();
    }
}

void SurfacePointWidget::setPointRadius_()
{
    auto radius = 0.f;
    switch ( params_.radiusSizeType )
    {
        case Parameters::PointSizeType::Metrical:
            radius = params_.radius <= 0.0f ? baseObject_->getBoundingBox().diagonal() * 5e-3f : params_.radius;
            break;

        case Parameters::PointSizeType::Pixel:
        {
            const auto baseObjectWorldXf = baseObject_->worldXf();

            // This assertion should probably be true always, not only here. But here I rely on them being the same (for the scale calculation), so better check.
            assert( baseObject_.get() == pickSphere_->parent() );
            float cameraScale = getViewerInstance().viewport().getPixelSizeAtPoint( baseObjectWorldXf( pickSphere_->getCenter( getViewerInstance().viewport().id ) ) );

            Matrix3f r, s;
            decomposeMatrix3( baseObjectWorldXf.A, r, s );
            const auto baseObjectScale = ( s.x.x + s.y.y + s.z.z ) / 3.f;

            radius = params_.radius;
            if ( radius <= 0.f )
                radius = 5.f;

            radius *= cameraScale / baseObjectScale;

            if ( auto menu = getViewerInstance().getMenuPlugin().get() )
                radius *= menu->menu_scaling();
        }
            break;
    }
    pickSphere_->setRadius( radius );
}

void SurfacePointWidget::preDraw_()
{
    setPointRadius_();
}

Vector3f SurfacePointWidget::getCoords() const
{
    return pickSphere_ ? pickSphere_->getCenter() : Vector3f{};
}

void SurfacePointWidget::setCurrentPosition( const PointOnObject& pos )
{
    currentPos_ = pointOnObjectToPickedPoint( baseObject_.get(), pos );
    updatePositionAndRadius_();
}

void SurfacePointWidget::setCurrentPosition( const PickedPoint& pos )
{
    currentPos_ = pos;
    updatePositionAndRadius_();
}

void SurfacePointWidget::swapCurrentPosition( PickedPoint& pos )
{
    std::swap( currentPos_, pos );
    updatePositionAndRadius_();
}

bool SurfacePointWidget::isPickIntoBackFace( const std::shared_ptr<MR::VisualObject>& obj, const MR::PointOnObject& pick, const Vector3f& cameraEye )
{
    const auto& xf = obj->worldXf();

    if ( auto objMesh = std::dynamic_pointer_cast< const ObjectMeshHolder >( obj ) )
    {
        const auto& n = objMesh->mesh()->dirDblArea( pick.face );
        if ( dot( xf.A * n, cameraEye ) < 0 )
            return true;
        else
            return false;
    }


    if ( auto objPoints = std::dynamic_pointer_cast< const ObjectPointsHolder >( obj ) )
    {
        if ( objPoints->pointCloud()->normals.size() > static_cast< int > ( pick.vert ) )
        {
            const auto& n = objPoints->pointCloud()->normals[pick.vert];
            auto dt = dot( xf.A * n, cameraEye );
            if ( dt < 0 )
                return true;
            else
                return false;
        }
    }

    return false;
}

}
