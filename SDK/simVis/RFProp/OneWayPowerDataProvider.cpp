/* -*- mode: c++ -*- */
/***************************************************************************
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
#include "simCore/EM/Decibel.h"
#include "simCore/EM/Propagation.h"
#include "simVis/RFProp/OneWayPowerDataProvider.h"

namespace simRF
{
OneWayPowerDataProvider::OneWayPowerDataProvider(const ProfileDataProvider* templateProvider, const RadarParametersPtr radarParameters)
  : FunctionalProfileDataProvider(templateProvider),
  radarParameters_(radarParameters)
{
  setType_(ProfileDataProvider::THRESHOLDTYPE_ONEWAYPOWER);
}

double OneWayPowerDataProvider::getValueByIndex(unsigned int heightIndex, unsigned int rangeIndex) const
{
  double ppfdB = FunctionalProfileDataProvider::templateGetValueByIndex_(heightIndex, rangeIndex);
  return (ppfdB <= simCore::SMALL_DB_VAL) ?
    simCore::SMALL_DB_VAL : getOneWayPower_(ppfdB, FunctionalProfileDataProvider::getRange_(rangeIndex), radarParameters_->antennaGaindB, radarParameters_->antennaGaindB);
}

double OneWayPowerDataProvider::interpolateValue(double height, double range) const
{
  double ppfdB = FunctionalProfileDataProvider::templateInterpolateValue_(height, range);
  return (ppfdB <= simCore::SMALL_DB_VAL) ?
    simCore::SMALL_DB_VAL : getOneWayPower_(ppfdB, range, radarParameters_->antennaGaindB, radarParameters_->antennaGaindB);
}

double OneWayPowerDataProvider::getOneWayPower(double height, double range, double slantRangeM, double xmtGaindB, double rcvGaindB) const
{
  double ppfdB = FunctionalProfileDataProvider::templateInterpolateValue_(height, range);
  return (ppfdB <= simCore::SMALL_DB_VAL) ?
    simCore::SMALL_DB_VAL : getOneWayPower_(ppfdB, slantRangeM, xmtGaindB, rcvGaindB);
}

double OneWayPowerDataProvider::getOneWayPower_(double ppfdB, double slantRangeM, double xmtGaindB, double rcvGaindB) const
{
  return simCore::getRcvdPowerBlake(
    slantRangeM,
    radarParameters_->freqMHz,
    radarParameters_->xmtPowerW,
    xmtGaindB,
    rcvGaindB,
    0,          // rcs, only required for two-way propagation
    ppfdB,
    radarParameters_->systemLossdB,
    true);
}

}

