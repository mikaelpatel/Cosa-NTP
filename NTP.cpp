/**
 * @file NTP.cpp
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

#include "NTP.hh"
#include "Cosa/Errno.h"

NTP::NTP(Socket* sock, uint8_t server[4], int8_t zone) :
  m_sock(sock),
  m_zone(zone)
{
  memcpy(m_server, server, sizeof(m_server));
}

NTP::~NTP()
{
  if (UNLIKELY(m_sock == NULL)) return;
  m_sock->close();
}

clock_t
NTP::time()
{
  if (UNLIKELY(m_sock == NULL)) return (ENOTSOCK);
  ntp_t msg;
  int res;
  msg.leap = 3;			// Clock unsynchronized
  msg.version = 4;		// NTP version 4
  msg.mode = 3;			// Client mode
  msg.refid = 0x41534f43;	// COSA
  msg.poll = 6;			// 2**6 = 64 seconds interval
  msg.precision = -20;		// 2**-20 = 0.954 us precision
  res = m_sock->send(&msg, sizeof(msg), m_server, PORT);
  if (UNLIKELY(res != sizeof(msg))) return (0L);
  delay(TIMEOUT);
  uint8_t src[4];
  uint16_t port;
  res = m_sock->recv(&msg, sizeof(msg), src, port);
  if (UNLIKELY(res != sizeof(msg))) return (0L);
  return (ntoh(msg.xmt.seconds) + (m_zone * 3600L));
}

int
NTP::gettimeofday(time_t& time)
{
  clock_t clock = this->time();
  if (UNLIKELY(clock == 0L)) return (EINVAL);
  time = clock;
  return (0);
}
