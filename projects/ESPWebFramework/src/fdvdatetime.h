/*
# Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com)
# Copyright (c) 2015 Fabrizio Di Vittorio.
# All rights reserved.

# GNU GPL LICENSE
#
# This module is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; latest version thereof,
# available at: <http://www.gnu.org/licenses/gpl.txt>.
#
# This module is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this module; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*/



#ifndef _FDVDATETIME_H
#define _FDVDATETIME_H

#include "fdv.h"




namespace fdv
{

    // Contains datetimes >= 01/01/2000
    // parts from JeeLabs and Rob Tillaart
    struct DateTime
    {

        uint8_t  seconds;
        uint8_t  minutes;
        uint8_t  hours;
        uint16_t year;
        uint8_t  month;
        uint8_t  day;


        DateTime()
            : seconds(0), minutes(0), hours(0), year(2000), month(1), day(1)
        {      
        }


        DateTime(uint8_t day_, uint8_t month_, uint16_t year_, uint8_t hours_, uint8_t minutes_, uint8_t seconds_)
            : seconds(seconds_), minutes(minutes_), hours(hours_), year(year_), month(month_), day(day_)
        {  
        }


        explicit DateTime(uint32_t unixTimeStamp)
        {
            setUnixDateTime(unixTimeStamp);
        }


        char const* monthStr() const;
        uint8_t dayOfWeek() const;
        DateTime& setUnixDateTime(uint32_t unixTime);
        uint32_t getUnixDateTime() const;
        DateTime& setNTPDateTime(uint8_t const* datetimeField, uint8_t timeZone);
        static DateTime now();
        static void adjustNow(DateTime const& currentDateTime);
        void format(char* outbuf, char const* format);

    private:

        static uint32_t const SECONDS_FROM_1970_TO_2000 = 946684800;
        static uint8_t daysInMonth(uint8_t month);
        static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s);
        static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d);
        static DateTime& lastDateTime();
        static uint32_t& lastMillis();

    };


    inline bool operator > (DateTime const& lhs, DateTime const& rhs)
    {
        return lhs.getUnixDateTime() > rhs.getUnixDateTime();
    }


    // dd/mm/yyyy hh:mm:ss
    // outbuf size must be 20 bytes
    inline void toString(char* outbuf, DateTime const& dt, bool date = true, bool time = true)
    {
        if (date)
            outbuf += sprintf(outbuf, "%02d/%02d/%d ", dt.day, dt.month, dt.year);
        if (time)
            outbuf += sprintf(outbuf, "%02d:%02d:%02d", dt.hours, dt.minutes, dt.seconds);
        *outbuf = 0;
    }



} // end of "fdv" namespace

#endif
