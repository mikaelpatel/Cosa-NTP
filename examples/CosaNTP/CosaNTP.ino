/**
 * @file CosaNTP.ino
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
 * @section Description
 * W5100 Ethernet Controller device driver example code; NTP client
 * using DHCP, DNS and automatic local clock (RTT/Watchdog) calibration.
 *
 * @section Output
 * Prints server address, server time (seconds from epoch), days.seconds,
 * day number, server date, client date, diff, retries for latest
 * request, current calibration offset, and number of adjustments.
 *
 * This file is part of the Arduino Che Cosa project.
 */

#include <DNS.h>
#include <DHCP.h>
#include <NTP.h>
#include <W5X00.h>

#include "Cosa/Trace.hh"
#include "Cosa/UART.hh"
#include "Cosa/RTT.hh"
#include "Cosa/Watchdog.hh"

// Configuration: local clock, ethernet control and shield
// #define USE_RTT_CLOCK
#define USE_WATCHDOG_CLOCK
#define USE_W5100
// #define USE_W5200
#define USE_ETHERNET_SHIELD

// W5100/W5200 Ethernet Controller with default MAC address
#if defined(USE_W5100)
#include <W5100.h>
W5100 ethernet;
#endif
#if defined(USE_W5200)
#include <W5200.h>
W5200 ethernet;
#endif

// Disable SD on Ethernet Shield
#if defined(USE_ETHERNET_SHIELD)
#include "Cosa/OutputPin.hh"
OutputPin sd(Board::D4, 1);
#endif

// RTT/Watchdog wall-clock and calibration offset (Mega 2560)
#if defined(USE_RTT_CLOCK)
RTT::Clock clock;
#define OFFSET_INIT -3
#endif
#if defined(USE_WATCHDOG_CLOCK)
Watchdog::Clock clock;
#define OFFSET_INIT -61
#endif

// NTP server address (alt. se.pool.ntp.org, time.nist.gov, ntp.ubuntu.com)
#define NTP_SERVER "se.pool.ntp.org"

// Time-zone; GMT+1, Stockholm
#define ZONE 1

void setup()
{
  uart.begin(115200);
  trace.begin(&uart, PSTR("CosaNTP: started"));

#if defined(USE_RTT_CLOCK)
  RTT::begin();
  trace << PSTR("Using RTT::Clock") << endl;
#endif
#if defined(USE_WATCHDOG_CLOCK)
  Watchdog::begin();
  trace << PSTR("Using Watchdog::Clock") << endl;
#endif

  time_t::epoch_year(NTP_EPOCH_YEAR);
  time_t::epoch_weekday = NTP_EPOCH_WEEKDAY;
  time_t::pivot_year = 37;

  // Note: This could also use_fastest_epoch if the clock_t offset was
  // calculated when the RTT is initiated. NTP::gettimeofday would
  // need modification.

  ASSERT(ethernet.begin_P(PSTR("CosaNTPclient")));

  trace << PSTR("server-ip;time;days.seconds;day;server-date;client-date;")
	<< PSTR("diff;retries;offset;adjusts")
	<< endl;
}

void loop()
{
  static int16_t offset = OFFSET_INIT - 1;
  static uint8_t adjusts = 0;
  uint8_t server[4];
  DNS dns;

  // Use DNS to get the NTP server network address
  ethernet.dns_addr(server);
  if (!dns.begin(ethernet.socket(Socket::UDP), server)) return;
  if (dns.gethostbyname_P(PSTR(NTP_SERVER), server) != 0) return;

  // Connect to the NTP server using given socket
  NTP ntp(ethernet.socket(Socket::UDP), server, ZONE);

  // Get current time. Allow a number of retries
  const uint8_t RETRY_MAX = 4;
  uint8_t retry;
  clock_t now;
  for (retry = 0; retry < RETRY_MAX; retry++) {
    if ((now = ntp.time()) != 0L) break;
    sleep(1);
  }

  // Check if another server should be used
  if (now == 0L) return;

  // Check if the rtt should be adjusted
  const int32_t DIFF_MAX = 2;
  uint32_t sec = clock.time();
  int32_t diff = now - sec;
  if (diff > DIFF_MAX || diff < -DIFF_MAX) {
    delay(400);
    now += 1;
    clock.time(now);
    if (diff > 0) offset -= 1; else offset += 1;
    clock.calibration(offset);
    diff = 0;
    sec = now;
    adjusts += 1;
  }

  // Convert to time structure
  time_t rtc(sec);

  // Print server address and time
  INET::print_addr(trace, server);
  trace << ';' << now << ';';

  // Print in star date notation; day.seconds
  trace << (now / SECONDS_PER_DAY) << '.'
	<< (now % SECONDS_PER_DAY) << ';';

  // Convert to time structure and print dayno followed by date and time
  time_t daytime(now);
  trace << daytime.day << ';' << daytime << ';' << rtc << ';';

  // Print diff and number of retries, current offset and adjustments
  trace << diff << ';' << retry << ';'<< offset << ';' << adjusts << endl;

  // Take a nap for 10 seconds (this is not 10 seconds period)
  sleep(10);
}
