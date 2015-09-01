/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California
 *
 * This file is part of ndn-group-encrypt (Group-based Encryption Protocol for NDN).
 * See AUTHORS.md for complete list of ndn-group-encrypt authors and contributors.
 *
 * ndn-group-encrypt is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndn-group-encrypt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndn-group-encrypt, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Zhiyi Zhang <dreamerbarrychang@gmail.com>
 */

#include "schedule.hpp"
#include "tlv.hpp"

#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/util/concepts.hpp>

namespace ndn {
namespace gep {

BOOST_CONCEPT_ASSERT((WireEncodable<Schedule>));
BOOST_CONCEPT_ASSERT((WireDecodable<Schedule>));

Schedule::Schedule() = default;

Schedule::Schedule(const Block& block)
{
  wireDecode(block);
}

template<encoding::Tag TAG>
size_t
Schedule::wireEncode(EncodingImpl<TAG>& encoder) const
{
  size_t totalLength = 0;
  size_t blackLength = 0;
  size_t whiteLength = 0;

  // encode the blackIntervalList as an embed TLV structure
  for (const RepetitiveInterval& element : m_blackIntervalList) {
    blackLength += encoder.prependBlock(element.wireEncode());
  }
  blackLength += encoder.prependVarNumber(blackLength);
  blackLength += encoder.prependVarNumber(tlv::BlackIntervalList);

  // encode the whiteIntervalList as an embed TLV structure
  for (const RepetitiveInterval& element : m_whiteIntervalList) {
    whiteLength += encoder.prependBlock(element.wireEncode());
  }
  whiteLength += encoder.prependVarNumber(whiteLength);
  whiteLength += encoder.prependVarNumber(tlv::WhiteIntervalList);

  totalLength = whiteLength + blackLength;
  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::Schedule);

  return totalLength;
}

const Block&
Schedule::wireEncode() const
{
  if (m_wire.hasWire())
    return m_wire;

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  this->m_wire = buffer.block();
  return m_wire;
}

void
Schedule::wireDecode(const Block& wire)
{
  if (wire.type() != tlv::Schedule)
    BOOST_THROW_EXCEPTION(tlv::Error("Unexpected TLV type when decoding RepetitiveInterval"));

  m_wire = wire;
  m_wire.parse();

  if (m_wire.elements_size() != 2)
    BOOST_THROW_EXCEPTION(tlv::Error("RepetitiveInterval tlv does not have two sub-TLVs"));

  Block::element_const_iterator it = m_wire.elements_begin();

  if (it != m_wire.elements_end() && it->type() == tlv::WhiteIntervalList) {
    it->parse();
    Block::element_const_iterator tempIt = it->elements_begin();
    while (tempIt != it->elements_end() && tempIt->type() == tlv::RepetitiveInterval) {
      m_whiteIntervalList.insert(RepetitiveInterval(*tempIt));
      tempIt++;
    }
    it++;
  }
  else
    BOOST_THROW_EXCEPTION(tlv::Error("The first element must be WhiteIntervalList"));

  if (it != m_wire.elements_end() && it->type() == tlv::BlackIntervalList) {
    it->parse();
    Block::element_const_iterator tempIt = it->elements_begin();
    while (tempIt != it->elements_end() && tempIt->type() == tlv::RepetitiveInterval) {
      m_blackIntervalList.insert(RepetitiveInterval(*tempIt));
      tempIt++;
    }
    it++;
  }
  else
    BOOST_THROW_EXCEPTION(tlv::Error("The second element must be BlackIntervalList"));
}

Schedule&
Schedule::addWhiteInterval(const RepetitiveInterval& repetitiveInterval)
{
  m_wire.reset();
  m_whiteIntervalList.insert(repetitiveInterval);
  return *this;
}

Schedule&
Schedule::addBlackInterval(const RepetitiveInterval& repetitiveInterval)
{
  m_wire.reset();
  m_blackIntervalList.insert(repetitiveInterval);
  return *this;
}

std::tuple<bool, Interval>
Schedule::getCoveringInterval(const TimeStamp& tp) const
{
  Interval blackPositiveResult(true);
  Interval whitePositiveResult(true);

  Interval blackNegativeResult;
  Interval whiteNegativeResult;

  Interval tempInterval;
  bool isPositive;

  // get the blackResult
  for (const RepetitiveInterval& element : m_blackIntervalList) {
    std::tie(isPositive, tempInterval) = element.getInterval(tp);
    if (isPositive == true) {
      // tempInterval is covering the time stamp, || to the black negative result
      // get the union interval of all the black interval covering the timestamp
      // return false and the union interval
      blackPositiveResult || tempInterval;
    }
    else {
      // tempInterval is not covering the time stamp, && to the black positive result
      // get the intersection interval of all the black interval not covering the timestamp
      // return true if white positive result is not empty, false if white positive result is empty
      if (!blackNegativeResult.isValid())
        blackNegativeResult = tempInterval;
      else
        blackNegativeResult && tempInterval;
    }
  }

  // if black positive result is not full, the result must be false
  if (!blackPositiveResult.isEmpty())
    return std::make_tuple(false, blackPositiveResult);

  // get the whiteResult
  for (const RepetitiveInterval& element : m_whiteIntervalList) {
    std::tie(isPositive, tempInterval) = element.getInterval(tp);
    if (isPositive == true) {
      // tempInterval is covering the time stamp, || to the white positive result
      // get the union interval of all the white interval covering the timestamp
      // return true
      whitePositiveResult || tempInterval;
    }
    else {
      // tempInterval is not covering the time, && to the white negative result
      // get the intersection of all the white interval not covering the timestamp
      // return false if positive result is empty, return true if positive result is not empty
      if (!whiteNegativeResult.isValid())
        whiteNegativeResult = tempInterval;
      else
        whiteNegativeResult && tempInterval;
    }
  }

  // return false if positive result is empty, return true if positive result is not empty
  if (!whitePositiveResult.isEmpty())
    return std::make_tuple(true, whitePositiveResult && blackNegativeResult);
  else
    return std::make_tuple(false, whiteNegativeResult);
}

} // namespace gep
} // namespace ndn
