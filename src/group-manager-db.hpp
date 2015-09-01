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

#ifndef GEP_GROUP_MANAGER_DB_HPP
#define GEP_GROUP_MANAGER_DB_HPP

#include "schedule.hpp"

namespace ndn {
namespace gep {

/**
 * @brief GroupManagerDB is a class to manage the database of group manager.
 *
 * It contains two tables to store Schedules and Members
 */
class GroupManagerDB
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

public:
  explicit
  GroupManagerDB(const std::string& dbDir);

  ~GroupManagerDB();

public:
  ////////////////////////////////////////////////////// schedule management

  /**
   * @brief Check if there is a schedule with @p name
   */
  bool
  hasSchedule(const std::string& name) const;

  /**
   * @brief List all the names of the schedules
   * @return A list of the name of all schedules.
   */
  std::list<std::string>
  listAllScheduleNames() const;

  /**
   * @brief Get a schedule with @p name.
   * @throw Error if the schedule does not exist
   */
  Schedule
  getSchedule(const std::string& name) const;

  /**
   * @brief Get member information of a schedule with @p name.
   * The member information include member name and certificate.
   */
  std::map<Name, Data>
  getScheduleMembers(const std::string& name) const;

  /**
   * @brief Add a @p schedule with @p name
   * @pre Name.length() != 0
   *
   * @throw Error if add operation fails, e.g., a schedule with the same name already exists
   */
  void
  addSchedule(const std::string& name, const Schedule& schedule);

  /**
   * @brief Delete the schedule with @p name
   */
  void
  deleteSchedule(const std::string& name);

  /**
   * @brief Rename a schedule with @p oldName to @p newName
   * @pre newName.length() != 0
   *
   * @throw Error if update operation fails, e.g., a schedule with @p newName already exists
   */
  void
  renameSchedule(const std::string& oldName, const std::string& newName);

  /**
   * @brief Update the schedule with @p name and replace the old object with @p schedule
   *
   * if no schedule with @p name exists, a new schedule
   * with @p name and @p schedule will be added to database
   */
  void
  updateSchedule(const std::string& name, const Schedule& schedule);

  ////////////////////////////////////////////////////// member management

  /**
   * @brief Check if there is a member with name @p identity
   */
  bool
  hasMember(const Name& identity) const;

  /**
   * @brief List all the members
   */
  std::list<Name>
  listAllMembers() const;

  /**
   * @brief Get the certificate of the member with name @p identity
   *
   * @throw Error if there is no member with name @p identity in database
   */
  Data
  getMemberCert(const Name& identity) const;

  /**
   * @brief Get the schedule name of a member with name @p identity
   *
   * @throw Error if there is no member with name @p identity in database
   */
  std::string
  getMemberSchedule(const Name& identity) const;

  /**
   * @brief Add a new member with @p certificate into a schedule with name @p scheduleName.
   *
   * @throw Error when there's no schedule named @p scheduleName
   * @throw Error if add operation fails, e.g., a member with the same name exists
   */
  void
  addMember(const std::string& scheduleName, const Data& certificate);

  /**
   * @brief Change the schedule of a member with name @p identity to a schedule with @p scheduleName
   *
   * @throw Error when there's no schedule named @p scheduleName
   */
  void
  updateMemberSchedule(const Name& identity, const std::string& scheduleName);

  /**
   * @brief Delete a member with name @p identity from database
   */
  void
  deleteMember(const Name& identity);

private:
  class Impl;
  unique_ptr<Impl> m_impl;
};

} // namespace gep
} // namespace ndn

#endif // GEP_GROUP_MANAGER_DB_HPP
