/* -*- mode: c++ -*- */
/****************************************************************************
 *****                                                                  *****
 *****                   Classification: UNCLASSIFIED                   *****
 *****                    Classified By:                                *****
 *****                    Declassify On:                                *****
 *****                                                                  *****
 ****************************************************************************
 *
 *
 * Developed by: Naval Research Laboratory, Tactical Electronic Warfare Div.
 *               EW Modeling & Simulation, Code 5773
 *               4555 Overlook Ave.
 *               Washington, D.C. 20375-5339
 *
 * License for source code at https://simdis.nrl.navy.mil/License.aspx
 *
 * The U.S. Government retains all rights to use, duplicate, distribute,
 * disclose, or release this software.
 *
 */
#include "osgEarth/MapNodeObserver"
#include "osgEarth/ElevationPool"
#include "osgEarth/ElevationQuery"
#include "osgEarth/ThreadingUtils"
#include "simVis/ElevationQueryProxy.h"

namespace simVis
{

/// Wrapper around the osgEarth::Future class
struct ElevationQueryProxy::PrivateData
{
#ifdef HAVE_ELEVQUERY_0317API
 /// Future object that monitors the status of the elevation query result
  osgEarth::Threading::Future<osgEarth::ElevationSample> elevationResult_;
#endif
};

/**
 * Empty node that implements the MapNodeVisitor interface.  As it gets notifications
 * that the MapNode changes, it passes those notifications to its parent Elevation
 * Query Proxy instance.
 */
class ElevationQueryProxy::MapChangeListener : public osg::Node, public osgEarth::MapNodeObserver
{
public:
  /** Constructor that takes an Elevation Query Proxy reference */
  explicit MapChangeListener(ElevationQueryProxy& queryProxy)
    : queryProxy_(queryProxy)
  {
  }

  /** Override setMapNode() from MapNodeObserver to inform the MousePositionManipulator. */
  virtual void setMapNode(osgEarth::MapNode* mapNode)
  {
    mapNode_ = mapNode;
    queryProxy_.setMapNode(mapNode);
  }

  /** Override getMapNode() from MapNodeObserver. */
  virtual osgEarth::MapNode* getMapNode()
  {
    return mapNode_.get();
  }

protected:
  /** osg::Referenced-derived */
  virtual ~MapChangeListener()
  {
  }

private:
  ElevationQueryProxy& queryProxy_;
  osg::observer_ptr<osgEarth::MapNode> mapNode_;
};

///////////////////////////////////////////////////////////////////////////////////

ElevationQueryProxy::ElevationQueryProxy(const osgEarth::Map* map, osg::Group* scene)
  : lastElevation_(NO_DATA_VALUE),
    lastResolution_(NO_DATA_VALUE),
    query_(NULL),
    map_(map),
    scene_(scene)
{
  data_ = new PrivateData();
  mapf_.setMap(map);
  query_ = new osgEarth::ElevationQuery(map);

#ifndef HAVE_ELEVQUERY_1016API
  // Prior to the 10/2016 API, this had to be set to true; now that's the default
  query_->setFallBackOnNoData(true);
#endif

  if (scene_.valid())
  {
    mapChangeListener_ = new MapChangeListener(*this);
    scene_->addChild(mapChangeListener_);
  }
}

ElevationQueryProxy::~ElevationQueryProxy()
{
  if (scene_.valid())
    scene_->removeChild(mapChangeListener_);
  delete query_;
  query_ = NULL;
  delete data_;
  data_ = NULL;
}

osgEarth::ElevationQuery* ElevationQueryProxy::q() const
{
  return query_;
}

bool ElevationQueryProxy::getPendingElevation(double& out_elevation, double* out_actualResolution)

{
#ifdef HAVE_ELEVQUERY_0317API
  // if result hasn't returned yet, return early
  if (!data_->elevationResult_.isAvailable())
    return false;

  osg::ref_ptr<osgEarth::ElevationSample> sample = data_->elevationResult_.release();
  getElevationFromSample_(sample.get(), out_elevation, out_actualResolution);
  return true;
#else
  return false;
#endif
}

bool ElevationQueryProxy::getElevationFromSample_(osgEarth::ElevationSample* sample, double& out_elevation, double* out_actualResolution)
{
  if (sample != NULL)
  {
    out_elevation = sample->elevation;
    if (out_elevation ==  NO_DATA_VALUE)
      out_elevation = 0.0;

    if (out_actualResolution)
      *out_actualResolution = sample->resolution;

    // cache values
    lastElevation_ = out_elevation;
    lastResolution_ = sample->resolution;
    return true;
  }

  out_elevation = 0.0;
  return false;
}

bool ElevationQueryProxy::getElevationFromPool_(const osgEarth::GeoPoint& point, double& out_elevation, double desiredResolution, double* out_actualResolution, bool blocking)
{
// ElevationPool::getElevation was introduced in 3/2017 to the osgEarth API
#ifdef HAVE_ELEVQUERY_0317API

  unsigned int lod = 23u; // use reasonable default value, same as osgEarth::ElevationQuery
  if (desiredResolution > 0.0)
  {
    int level = mapf_.getProfile()->getLevelOfDetailForHorizResolution(desiredResolution, 257);
    if ( level > 0 )
        lod = level;
  }

  data_->elevationResult_ = mapf_.getElevationPool()->getElevation(point, lod);
  // if blocking, get elevation result immediately
  if (blocking)
  {
    osg::ref_ptr<osgEarth::ElevationSample> sample = data_->elevationResult_.get();
    return getElevationFromSample_(sample.get(), out_elevation, out_actualResolution);
  }

  // return cached values while waiting for query to return
  out_elevation = lastElevation_;
  if (out_actualResolution)
    *out_actualResolution = lastResolution_;

  return out_elevation == NO_DATA_VALUE ? false : true;
#else
  return false;
#endif
}

bool ElevationQueryProxy::getElevation(const osgEarth::GeoPoint& point, double& out_elevation, double desiredResolution, double* out_actualResolution, bool blocking)
{
// ElevationPool got the getElevation() method in the 3/2017 osgEarth API. If we have it, use it
#ifdef HAVE_ELEVQUERY_0317API
  return getElevationFromPool_(point, out_elevation, desiredResolution, out_actualResolution, blocking);
#else
  if (query_)
  {
#ifndef HAVE_ELEVQUERY_1016API
    // Older API used a double value and returned a boolean, like our method
    return query_->getElevation(point, out_elevation, desiredResolution, out_actualResolution);
#else
    const float value = query_->getElevation(point, desiredResolution, out_actualResolution);
    if (value ==  NO_DATA_VALUE)
    {
      out_elevation = 0.0;
      return false; // fail
    }
    out_elevation = value;
    return true;
#endif
  }
  return false; // failure
#endif
}

void ElevationQueryProxy::setMaxTilesToCache(int value)
{
#ifndef HAVE_ELEVQUERY_1016API
  // After 10/2016 API, now uses map's elevation tile pool and this method is gone.
  // If you really want to change it from default of 128, you can do so by
  // calling map->getElevationPool()->setMaxEntries().
  if (query_)
    query_->setMaxTilesToCache(value);
#endif
}

void ElevationQueryProxy::setMap(const osgEarth::Map* map)
{
  // Avoid expensive operations on re-do of same map
  if (map == map_.get())
    return;

  delete query_;
  query_ = new osgEarth::ElevationQuery(map);
  mapf_.setMap(map);
}

void ElevationQueryProxy::setMapNode(const osgEarth::MapNode* mapNode)
{
  if (mapNode == NULL)
    setMap(NULL);
  else
    setMap(mapNode->getMap());
}

}
