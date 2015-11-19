/**
 * @file NTP.hh
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2014-2015, Mikael Patel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * This file is part of the Arduino Che Cosa project.
 */

#ifndef COSA_NTP_HH
#define COSA_NTP_HH

#include "Cosa/Types.h"
#include "Cosa/Socket.hh"
#include "Cosa/Time.hh"

/**
 * Network Time Protocol client; access time from server on network.
 * Used for clock synchronization between computer systems over
 * packet-switched, variable-latency data networks. Note: this
 * implementation does not adjust for clock drift or network latency.
 *
 * @section References
 * 1. Network Time Protocol Version 4: Protocol and Algorithms
 * Specification, June 2010, http://www.ietf.org/rfc/rfc5905.txt.
 */
class NTP {
public:
  /**
   * Construct NTP client with given socket, server name and time zone
   * adjustment.
   * @param[in] sock communications socket (UDP::PORT).
   * @param[in] server network address.
   * @param[in] zone time zone adjustment (Default GMT).
   */
  NTP(Socket* sock, uint8_t server[4], int8_t zone = 0);

  /**
   * Destruct NTP client. Close socket.
   */
  ~NTP();

  /**
   * Get current time as seconds from NTP Epoch. Returns zero if fails.
   * @return clock.
   */
  clock_t time();

  /**
   * Get current time as year, month, hours, minutes and seconds.
   * @param[out] time structure.
   * @return zero if successful otherwise negative error code.
   */
  int gettimeofday(time_t& time);

protected:
  /**
   * NTP Timestamp Format, fig. 3, pp. 13.
   */
  struct timestamp_t {
    uint32_t seconds;
    uint32_t fraction;
  };

  /**
   * NTP UDP datagram packet, fig. 8, pp. 18.
   */
  struct ntp_t {
    uint8_t mode:3;		//!< Association Mode (fig. 10).
    uint8_t version:3;		//!< Version Number (currently 4).
    uint8_t leap:2;		//!< Leap Indicator (fig. 9).
    uint8_t stratum;		//!< Packet Stratum (fig. 11).
    int8_t poll;		//!< Poll interval in log2 seconds (6..10).
    int8_t precision;		//!< Precision in log2 seconds (-20 is us).
    uint32_t rootdelay;		//!< Root delay.
    uint32_t rootdisp;		//!< Root dispersion.
    uint32_t refid;		//!< Reference ID.
    timestamp_t ref;		//!< Reference timestamp.
    timestamp_t org;		//!< Origin timestamp.
    timestamp_t rec;		//!< Receive timestamp.
    timestamp_t xmt;		//!< Transmit timestamp.

    ntp_t()
    {
      memset(this, 0, sizeof(ntp_t));
    }
  };

  /** NTP server port. */
  static const uint16_t PORT = 123;

  /** Timeout period for response from time server (milli-seconds). */
  static const uint16_t TIMEOUT = 32;

  /** Network address of server. */
  uint8_t m_server[4];

  /** Socket for communication with server. */
  Socket* m_sock;

  /** Time zone adjustment (hours). */
  int8_t m_zone;
};

#endif
