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
#ifndef SIMVIS_MEASUREMENT_H
#define SIMVIS_MEASUREMENT_H

#include <string>
#include <vector>

#include "osg/Referenced"
#include "osgEarth/Units"
#include "simCore/Common/Common.h"
#include "simData/ObjectId.h"

namespace simCore { class DatumConvert; }

namespace simVis
{
struct RangeToolState;

/// Units for calculations that have no units
const osgEarth::Units UNITLESS = osgEarth::Units("nounits", "", osgEarth::Units::TYPE_INVALID, 1.0);
/// Units in db
const osgEarth::Units LOG10 = osgEarth::Units("log10", "dB", osgEarth::Units::TYPE_INVALID, 1.0);
/// Units for RF Power in dBW
const osgEarth::Units RF_POWER = osgEarth::Units("rf_power", "dBW", osgEarth::Units::TYPE_INVALID, 1.0);
/// Units for RF Power in dBsm (square meters)
const osgEarth::Units RF_POWER_SM = osgEarth::Units("rf_power_sm", "dBsm", osgEarth::Units::TYPE_INVALID, 1.0);
/// Units for %
const osgEarth::Units PERCENTAGE = osgEarth::Units("percentage", "%", osgEarth::Units::TYPE_INVALID, 1.0);

/// Default effective Earth radius scalar for optical horizon measurement
const double DEFAULT_OPTICAL_RADIUS = 1.06;

/// Default effective Earth radius scalar for RF horizon measurement
const double DEFAULT_RF_RADIUS = 4. / 3.;

/**
* Base class for formatting values into a string
*/
class SDKVIS_EXPORT ValueFormatter : public osg::Referenced
{
public:
  virtual ~ValueFormatter() {}
  /**
  * Formats the value into a string
  * @param value The value that needs to be converted into a string.
  * @param precision The number of digits after the decimal point
  * @return The value as a string
  */
  virtual std::string stringValue(double value, int precision) const;
};

/**
* Class for formatting Above/Below into a string.
* Intended for use with RadioHorizonMeasurement and OpticalHorizonMeasurement
*/
class SDKVIS_EXPORT HorizonFormatter : public ValueFormatter
{
public:
  virtual ~HorizonFormatter() {}
  /**
  * Formats the value into a string
  * @param value The value that needs to be converted into a string.
  * @param precision Ignored
  * @return The value as a string
  */
  virtual std::string stringValue(double value, int precision) const;
};

/**
* Base class for a Measurement. A Measurement is a value derived from the
* simulation state data (object positions, orientations, velocities, etc.).
* Each calculation consists of multiple Graphic primitives and a single
* Measurement, which is the value displayed in the text label.
*
* You can create custom measurements by sub-classing Measurement.
*/
class SDKVIS_EXPORT Measurement : public osg::Referenced
{
protected:
  /**
  * Constructs a new Measurement
  * @param typeName    Unique type name of this measurement.
  * @param typeAbbr    Prefix string for labeling.
  * @param units       Units of the measurement value.
  */
  Measurement(const std::string &typeName, const std::string &typeAbbr, const osgEarth::Units &units);

public:
  /**
  * Gets the unique type name of the measurement
  */
  const std::string& typeName() const { return typeName_; }

  /**
  * The abbreviation string to use for labeling/UI.
  * @return A short string
  */
  const std::string& typeAbbr() const { return typeAbbr_; }

  /**
  * The Units in which value() is expressed.
  * @return Units
  */
  const osgEarth::Units& units() const { return units_; }

  /**
  * Returns the calculated value of the measurement
  * @param state Context object
  * @return Calculated value
  */
  virtual double value(RangeToolState& state) const = 0;

  /**
  * Returns if the calculation is valid for the given types
  * @param state All the state information for both entities
  * @return True if the types are valid for the calculation otherwise false
  */
  virtual bool willAccept(const RangeToolState& state) const = 0;

  /**
  * Returns the calculated value converted to the specified units.
  * @param outputUnits units to which to convert
  * @param state Context object
  * @return Calculated value in target units
  */
  double value(const osgEarth::Units& outputUnits, RangeToolState& state) const;

  /**
  * Returns the formatter for the measurement
  * @return The formatter for the measurement
  */
  ValueFormatter* formatter() const { return formatter_.get(); }

protected:
  /// osg::Referenced-derived
  virtual ~Measurement() {}

  /// Returns true if the type is a beam, gate, laser or lob group
  bool isRaeObject_(simData::ObjectType type) const;
  /// Returns true if both types are either platform, beam, gate, laser or lob group
  bool isEntityToEntity_(simData::ObjectType fromType, simData::ObjectType toType) const;
  /// Returns true if both types are platforms
  bool isPlatformToPlatform_(simData::ObjectType fromType, simData::ObjectType toType) const;
  /// Returns true if both types are either platforms or custom rendering
  bool isLocationToLocation_(simData::ObjectType fromType, simData::ObjectType toType) const;
  /// Returns true if one type is a beam and the other is a non-beam
  bool isBeamToNonBeamAssociation_(simData::ObjectType fromType, simData::ObjectType toType) const;
  /// Returns true if the fromType is a beam and the toType is a valid entity
  bool isBeamToEntity_(simData::ObjectType fromType, simData::ObjectType toType) const;
  /// Returns true if the nodes are valid for a angle calculation
  bool isAngle_(simData::ObjectType fromType, simData::ObjectId fromHostId, simData::ObjectType toType, simData::ObjectId toHostId) const;
  /// Returns true if the nodes are valid for velocity angle calculation
  bool isVelocityAngle_(simData::ObjectType fromType, simData::ObjectId fromHostId, simData::ObjectType toType, simData::ObjectId toHostId) const;
  /// Returns the composite angle (rad) for the given angles (rad) for entities on the SAME platform
  double getCompositeAngle_(double bgnAz, double bgnEl, double endAz, double endEl) const;
  /// Returns the true angles (rad) for the given state
  void calculateTrueAngles_(const RangeToolState& state, double* az, double* el, double* cmp) const;
  /**
  * Calculates the relative angles between entities (for the given state)
  * @param state current range tool state upon to use for calculation
  * @param az Relative azimuth angle to calculate (rad)
  * @param el Relative elevation angle to calculate (rad)
  * @param cmp Relative composite angle to calculate (rad)
  */
  void calculateRelativeAngles_(const RangeToolState& state, double* az, double* el, double* cmp) const;
  /// Formats the double value to a string
  osg::ref_ptr< ValueFormatter > formatter_;

private:
  std::string     typeName_;
  std::string     typeAbbr_;
  osgEarth::Units units_;
};

/// a vector of Measurement pointers
typedef std::vector< osg::ref_ptr<Measurement> > MeasurementVector;


/// Ground Range
class SDKVIS_EXPORT GroundDistanceMeasurement : public Measurement
{
public:
  GroundDistanceMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~GroundDistanceMeasurement() {}
};

/// Slant Range
class SDKVIS_EXPORT SlantDistanceMeasurement : public Measurement
{
public:
  SlantDistanceMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~SlantDistanceMeasurement() {}
};

/// Altitude
class SDKVIS_EXPORT AltitudeDeltaMeasurement : public Measurement
{
public:
  AltitudeDeltaMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~AltitudeDeltaMeasurement() {}
};

/// Beam Ground Range
class SDKVIS_EXPORT BeamGroundDistanceMeasurement : public Measurement
{
public:
  BeamGroundDistanceMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~BeamGroundDistanceMeasurement() {}
};

/// Beam Slant Range
class SDKVIS_EXPORT BeamSlantDistanceMeasurement : public Measurement
{
public:
  BeamSlantDistanceMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~BeamSlantDistanceMeasurement() {}
};

/// Beam Altitude
class SDKVIS_EXPORT BeamAltitudeDeltaMeasurement : public Measurement
{
public:
  BeamAltitudeDeltaMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~BeamAltitudeDeltaMeasurement() {}
};

/// Down Range
class SDKVIS_EXPORT DownRangeMeasurement : public Measurement
{
public:
  DownRangeMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~DownRangeMeasurement() {}
};

/// Cross Range
class SDKVIS_EXPORT CrossRangeMeasurement : public Measurement
{
public:
  CrossRangeMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~CrossRangeMeasurement() {}
};

/// Down Value
class SDKVIS_EXPORT DownRangeCrossRangeDownValueMeasurement : public Measurement
{
public:
  DownRangeCrossRangeDownValueMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~DownRangeCrossRangeDownValueMeasurement() {}
};

/// Geodesic Down Range
class SDKVIS_EXPORT GeoDownRangeMeasurement : public Measurement
{
public:
  GeoDownRangeMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~GeoDownRangeMeasurement() {}
};

/// Geodesic Cross Range
class SDKVIS_EXPORT GeoCrossRangeMeasurement : public Measurement
{
public:
  GeoCrossRangeMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~GeoCrossRangeMeasurement() {}
};

// True angles

/// True Azimuth
class SDKVIS_EXPORT TrueAzimuthMeasurement : public Measurement
{
public:
  TrueAzimuthMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~TrueAzimuthMeasurement() {}
};

/// True Elevation
class SDKVIS_EXPORT TrueElevationMeasurement : public Measurement
{
public:
  TrueElevationMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~TrueElevationMeasurement() {}
};

/// True Composite Angle Calculation
class SDKVIS_EXPORT TrueCompositeAngleMeasurement : public Measurement
{
public:
  TrueCompositeAngleMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~TrueCompositeAngleMeasurement() {}
};

class SDKVIS_EXPORT MagneticAzimuthMeasurement : public Measurement
{
public:
  explicit MagneticAzimuthMeasurement(std::shared_ptr<simCore::DatumConvert> datumConvert);
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~MagneticAzimuthMeasurement() {}
private:
  std::shared_ptr<const simCore::DatumConvert> datumConvert_;
};

// Orientation-relative angles

/// helper class - internal
struct RelOriMeasurement : public Measurement
{
protected:
  /**
  * Constructor.
  * @param name The name of the type.
  * @param abbr The type abbr.
  * @param units The units.
  */
  RelOriMeasurement(const std::string& name, const std::string& abbr, const osgEarth::Units& units)
    : Measurement(name, abbr, units) { }

  /**
  * Calculates the angles for the given state
  * @param az If non-null, the azimuth (radians).
  * @param el If non-null, the elevation (radians).
  * @param cmp If non-null, the composite angle (radians).
  * @param state The state information for both the begin and end entities
  */
  void getAngles(double* az, double* el, double* cmp, RangeToolState& state) const;

  /// osg::Referenced-derived
  virtual ~RelOriMeasurement() {}
};

/// Orientation Relative Azimuth
class SDKVIS_EXPORT RelOriAzimuthMeasurement : public RelOriMeasurement
{
public:
  RelOriAzimuthMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~RelOriAzimuthMeasurement() {}
};

/// Orientation Relative Elevation
class SDKVIS_EXPORT RelOriElevationMeasurement : public RelOriMeasurement
{
public:
  RelOriElevationMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~RelOriElevationMeasurement() {}
};

/// Orientation Relative Composite Angle
class SDKVIS_EXPORT RelOriCompositeAngleMeasurement : public RelOriMeasurement
{
public:
  RelOriCompositeAngleMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~RelOriCompositeAngleMeasurement() {}
};

// Velocity vector-relative angles

/// helper class - internal
struct RelVelMeasurement : public Measurement
{
protected:

  /**
  * Constructor.
  * @param name The name of the type.
  * @param abbr The type abbr.
  * @param units The units.
  */
  RelVelMeasurement(const std::string& name, const std::string& abbr, const osgEarth::Units& units)
    : Measurement(name, abbr, units) { }

  /**
  * Calculates the angles for the given state
  * @param az If non-null, the azimuth (radians).
  * @param el If non-null, the elevation (radians).
  * @param cmp If non-null, the composite angle (radians).
  * @param state The state information for both the begin and end entities
  */
  void getAngles(double* az, double* el, double* cmp, RangeToolState& state) const;

  /// osg::Referenced-derived
  virtual ~RelVelMeasurement() {}
};

/// Velocity Relative Azimuth
class SDKVIS_EXPORT RelVelAzimuthMeasurement : public RelVelMeasurement
{
public:
  RelVelAzimuthMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~RelVelAzimuthMeasurement() {}
};

/// Velocity Relative Elevation
class SDKVIS_EXPORT RelVelElevationMeasurement : public RelVelMeasurement
{
public:
  RelVelElevationMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~RelVelElevationMeasurement() {}
};

/// Velocity Relative Composite Angle
class SDKVIS_EXPORT RelVelCompositeAngleMeasurement : public RelVelMeasurement
{
public:
  RelVelCompositeAngleMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

public:
  /// osg::Referenced-derived
  virtual ~RelVelCompositeAngleMeasurement() {}
};

// Velocity measures

/// Closing Velocity
class SDKVIS_EXPORT ClosingVelocityMeasurement : public Measurement
{
public:
  ClosingVelocityMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~ClosingVelocityMeasurement() {}
};

/// Separation Velocity
class SDKVIS_EXPORT SeparationVelocityMeasurement : public Measurement
{
public:
  SeparationVelocityMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~SeparationVelocityMeasurement() {}
};

/// Velocity Delta
class SDKVIS_EXPORT VelocityDeltaMeasurement : public Measurement
{
public:
  VelocityDeltaMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~VelocityDeltaMeasurement() {}
};

/// Velocity Azimuth Down Range
class SDKVIS_EXPORT VelAzimDownRangeMeasurement : public Measurement
{
public:
  VelAzimDownRangeMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~VelAzimDownRangeMeasurement() {}
};

/// Velocity Azimuth Cross Range
class SDKVIS_EXPORT VelAzimCrossRangeMeasurement : public Measurement
{
public:
  VelAzimCrossRangeMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~VelAzimCrossRangeMeasurement() {}
};

/// Velocity Azimuth Geodesic Down Range
class SDKVIS_EXPORT VelAzimGeoDownRangeMeasurement : public Measurement
{
public:
  VelAzimGeoDownRangeMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~VelAzimGeoDownRangeMeasurement() {}
};

/// Velocity Azimuth Geodesic Cross Range
class SDKVIS_EXPORT VelAzimGeoCrossRangeMeasurement : public Measurement
{
public:
  VelAzimGeoCrossRangeMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~VelAzimGeoCrossRangeMeasurement() {}
};

/// Aspect Angle
class SDKVIS_EXPORT AspectAngleMeasurement : public Measurement
{
public:
  AspectAngleMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

private:
  /// osg::Referenced-derived
  virtual ~AspectAngleMeasurement() {}
};

/// Base class for RF calculations
class SDKVIS_EXPORT RfMeasurement : public RelOriMeasurement
{
public:
  /**
  * Constructor.
  * @param name Name of the type.
  * @param abbr The type abbr.
  * @param units The units.
  */
  RfMeasurement(const std::string& name, const std::string& abbr, const osgEarth::Units& units);

protected:
  /// osg::Referenced-derived
  virtual ~RfMeasurement() {}
  /**
  * Calculates RF parameters from the given state
  * @param state State information for both the begin and end entities
  * @param azAbs The absolute true azimuth, in radians, between the begin entity and the end entity
  * @param elAbs The absolute elevation, in radians, between the begin entity and the end entity
  * @param hgtMeters The height, in meters, of the antenna
  * @param xmtGaindB The gain, in dB, of the transmit antenna
  * @param rcvGaindB The gain, in dB, of the receive antenna
  * @param rcsSqm RCS db if useDb is true or RCS dBsm if useDb is false;
  * @param useDb Flag for selecting type of rcs value to return
  * @param freqMHz The frequency, in MHz, of the RF signal
  * @param powerWatts The power, in watts, of the RF signal
  */
  void getRfParameters_(RangeToolState& state, double* azAbs, double* elAbs, double *hgtMeters, double* xmtGaindB, double* rcvGaindB, double* rcsSqm, bool useDb,
    double* freqMHz, double* powerWatts) const;
};

/// Antenna Gain
class SDKVIS_EXPORT RFGainMeasurement : public RfMeasurement
{
public:
  RFGainMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~RFGainMeasurement() {}
};

/// Received Power
class SDKVIS_EXPORT RFPowerMeasurement : public RfMeasurement
{
public:
  RFPowerMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~RFPowerMeasurement() {}
};

/// One-Way Power
class SDKVIS_EXPORT RFOneWayPowerMeasurement : public RfMeasurement
{
public:
  RFOneWayPowerMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~RFOneWayPowerMeasurement() {}
};

/// Base class for Horizon calculations
class SDKVIS_EXPORT HorizonMeasurement : public Measurement
{
public:

  /**
  * Constructor.
  * @param typeName Name of the type.
  * @param typeAbbr The type abbr.
  * @param units The units.
  */
  HorizonMeasurement(const std::string &typeName, const std::string &typeAbbr, const osgEarth::Units &units);
  virtual bool willAccept(const RangeToolState& state) const;

  /**
  * Set effective Earth radius scalars for optical and rf horizon measurement.
  * @param opticalRadius new Earth radius scalar for optical horizon calculations
  * @param rfRadius new Earth radius scalar for rf horizon calculations
  */
  void setEffectiveRadius(double opticalRadius, double rfRadius);

protected:
  /// osg::Referenced-derived
  virtual ~HorizonMeasurement() {}

  /**
  * Calculates if the end entity is above or below the horizon
  * @param state Information on both the begin entity and end entity
  * @param horizon Type of calculation
  * @return 0 = below horizon and 1 = above horizon
  */
  virtual double calcAboveHorizon_(RangeToolState& state, simCore::HorizonCalculations horizon) const;

private:
  double opticalEffectiveRadius_;
  double rfEffectiveRadius_;
};

/// Radio Horizon
class SDKVIS_EXPORT RadioHorizonMeasurement : public HorizonMeasurement
{
public:
  RadioHorizonMeasurement();
  virtual double value(RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~RadioHorizonMeasurement() {}
};

/// Optical Horizon
class SDKVIS_EXPORT OpticalHorizonMeasurement : public HorizonMeasurement
{
public:
  OpticalHorizonMeasurement();
  virtual double value(RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~OpticalHorizonMeasurement() {}
};

/// Probability of Detection (PoD)
class SDKVIS_EXPORT PodMeasurement : public RfMeasurement
{
public:
  PodMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~PodMeasurement() {}
};

/// Propagation Loss
class SDKVIS_EXPORT LossMeasurement : public RfMeasurement
{
public:
  LossMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~LossMeasurement() {}
};

/// Pattern Propagation Factor (PPF)
class SDKVIS_EXPORT PpfMeasurement : public RfMeasurement
{
public:
  PpfMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~PpfMeasurement() {}
};

/// Signal to Noise (SNR)
class SDKVIS_EXPORT SnrMeasurement : public RfMeasurement
{
public:
  SnrMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~SnrMeasurement() {}
};

/// Clutter to Noise (CNR)
class SDKVIS_EXPORT CnrMeasurement : public RfMeasurement
{
public:
  CnrMeasurement();
  //unlike other RF - related calculations, CNR doesn't have a height component
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~CnrMeasurement() {}
};

/// Radar Cross Section (RCS)
class SDKVIS_EXPORT RcsMeasurement : public RfMeasurement
{
public:
  RcsMeasurement();
  virtual double value(RangeToolState& state) const;
  virtual bool willAccept(const RangeToolState& state) const;

protected:
  /// osg::Referenced-derived
  virtual ~RcsMeasurement() {}
};

}

#endif
