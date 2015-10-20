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




#include "fdv.h"




namespace fdv
{



    //////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////
    // SerialConsole
        
#if (FDV_INCLUDE_SERIALCONSOLE == 1)


    typedef void (SerialConsole::*Handler)();

    struct Cmd
    {
        char const* cmd;
        char const* syntax;
        char const* description;
        Handler     handler;
    };


    Cmd const* FUNC_FLASHMEM cmdInfo(uint32_t i)
    {
        static const Cmd cmds[] =
        {
            // example:
            //   help
            //   help ifconfig
            {FSTR("help"),	     
             FSTR("[COMMAND]"), 
             FSTR("Show help"), 
             &SerialConsole::cmd_help},
             
             // example:
             //   reboot
             //   reboot 500
            {FSTR("reboot"),	 
             FSTR("[MS]"), 
             FSTR("Restart system in [MS] milliseconds"), 
             &SerialConsole::cmd_reboot},
             
            {FSTR("restore"),	 
             STR_,
             FSTR("Erase Flash stored settings"), 
             &SerialConsole::cmd_restore},
             
            {FSTR("free"),       
             STR_, 
             FSTR("Display amount of free and used memory"), 
             &SerialConsole::cmd_free},
             
             // example:
             //  ifconfig
             //  ifconfig static 192.168.1.99 255.255.255.0 192.168.1.1
             //  ifconfig dhcp
             //  ifconfig ap 192.168.5.1 255.255.255.0 192.168.5.1
             //  ifconfig dns 8.8.8.8 8.8.4.4
            {FSTR("ifconfig"),   
             FSTR("[static IP NETMASK GATEWAY] | [dhcp] | [ap IP NETMASK GATEWAY] | [dns DNS1 DNS2]"), 
             FSTR("No params: Display network info\r\n\tstatic: Set Client Mode static IP\r\n\tdhcp: Set Client Mode DHCP\r\n\tap: Set Access Point static IP\r\n\tdns: Set primary and seconday DNS server"), 
             &SerialConsole::cmd_ifconfig},
             
             // example:
             //   iwlist
             //   iwlist scan
            {FSTR("iwlist"),     
             FSTR("[scan]"), 
             FSTR("Display or scan for available wireless networks"), 
             &SerialConsole::cmd_iwlist},
             
            {STR_date,       
             STR_, 
             FSTR("Display current date/time"), 
             &SerialConsole::cmd_date},
             
             // example:
             //   ntpdate 192.168.1.10
             //   ntpdate ntp1.inrim.it
            {FSTR("ntpdate"),             
             FSTR("[SERVER]"), 
             FSTR("Display date/time from NTP server"), 
             &SerialConsole::cmd_ntpdate},
             
             // example:
             //   nslookup www.google.com
            {FSTR("nslookup"),   
             FSTR("NAME"), 
             FSTR("Query DNS"), 
             &SerialConsole::cmd_nslookup},    
             
            {STR_uptime,
             STR_, 
             FSTR("Display how long the system has been running"), 
             &SerialConsole::cmd_uptime},
             
            {FSTR("ping"),
             FSTR("SERVER"),
             FSTR("Sends ICMP ECHO_REQUEST and waits for ECHO_RESPONSE"),
             &SerialConsole::cmd_ping},
             
             // example:
             //   router on
             //   router off
            {FSTR("router"),
             FSTR("on | off"),
             FSTR("Enable/disable routing between networks"),
             &SerialConsole::cmd_router},
             
            {FSTR("test"),       
             FSTR(""), FSTR(""), 
             &SerialConsole::cmd_test},
        };
        static uint32_t const cmdCount = sizeof(cmds) / sizeof(Cmd);
        if (i < cmdCount)
            return &cmds[i];
        else
            return NULL;
    }
        


        
    void MTD_FLASHMEM SerialConsole::exec()
    {
        m_serial = HardwareSerial::getSerial(0);
        cmd_help();
        m_serial->writeNewLine();
        while (true)
        {
            m_receivedChunks.clear();
            m_serial->write(FSTR("$ "));
            if (m_serial->readLine(true, &m_receivedChunks))
            {
                m_serial->writeNewLine();
                separateParameters();
                routeCommand();
                /*
                debug("params = %d\r\n", m_paramsCount);
                for (int32_t i = 0; i != m_paramsCount; ++i)
                {
                    debug("  %d = ", i);
                    for (CharChunksIterator it = m_params[i]; *it; ++it)
                        debug(*it);
                    debug("   ");
                    for (CharChunksIterator it = m_params[i]; *it; ++it)
                        debug("%x ", (int)*it);
                    debug("\r\n");
                }
                */
            }
        }
    }


    void MTD_FLASHMEM SerialConsole::separateParameters()
    {
        m_paramsCount = 0;
        bool quote = false;
        CharChunksIterator start = m_receivedChunks.getIterator();
        for (CharChunksIterator it = start; it.isValid(); ++it)
        {
            if (!quote && *it == '"')
            {
                quote = true;
            }
            else if (quote && *it == '"')
            {
                m_params[m_paramsCount++] = start + 1;
                *it = 0x00;
                ++it;
                start = it + 1;
                quote = false;
                
            }
            else if ((!quote && *it == ' ') || *it == 0x00 || it.isLast())
            {
                if (it != start)
                {
                    *it = 0x00;
                    m_params[m_paramsCount++] = start;
                }
                start = it + 1;
            }
        }			
    }


    void MTD_FLASHMEM SerialConsole::routeCommand()
    {    
        if (m_paramsCount > 0)
        {
            for (uint32_t i = 0; cmdInfo(i); ++i)
            {
                if (hasParameter(0, cmdInfo(i)->cmd))
                {
                    (this->*(cmdInfo(i)->handler))();
                    return;
                }
            }
            m_serial->writeln(FSTR("Unknown command"));
        }
    }
    
    
    bool MTD_FLASHMEM SerialConsole::hasParameter(uint32_t paramIndex, char const* str)
    {
        return t_strcmp(m_params[paramIndex], CharIterator(str)) == 0;
    }


    void MTD_FLASHMEM SerialConsole::cmd_help()
    {
        if (m_paramsCount == 1)
        {
            for (uint32_t i = 0; cmdInfo(i); ++i)
                m_serial->printf(FSTR("%s %s\r\n"), cmdInfo(i)->cmd, cmdInfo(i)->syntax);
        }
        else if (m_paramsCount == 2)
        {
            for (uint32_t i = 0; cmdInfo(i); ++i)
                if (hasParameter(1, cmdInfo(i)->cmd))
                {
                    m_serial->printf(FSTR("Syntax:\r\n\t%s %s\r\n"), cmdInfo(i)->cmd, cmdInfo(i)->syntax);
                    m_serial->printf(FSTR("Description:\r\n\t%s\r\n"), cmdInfo(i)->description);
                    break;
                }
        }
    }


    void MTD_FLASHMEM SerialConsole::cmd_restore()
    {
        m_serial->write(FSTR("Are you sure [y/N]? "));
        m_receivedChunks.clear();
        if (m_serial->readLine(true, &m_receivedChunks))
        {
            if (*m_receivedChunks.getIterator() == 'y')
            {
                FlashDictionary::eraseContent();
                m_serial->write(FSTR("\r\nFlash settings restored"));
            }
        }
        m_serial->writeNewLine();
    }


    void MTD_FLASHMEM SerialConsole::cmd_reboot()
    {
        uint32_t ms = 50;
        if (m_paramsCount == 2)
            ms = t_strtol(m_params[1], 10);
        reboot(ms);
        m_serial->writeln(FSTR("rebooting...\r\n"));
    }


    void MTD_FLASHMEM SerialConsole::cmd_free()
    {
        uint32_t const totHeap  = 0x14000;
        uint32_t const freeHeap = getFreeHeap();
        uint32_t const flashDictUsedSpace = FlashDictionary::getUsedSpace();
        m_serial->printf(FSTR("                      total        used        free\r\n"));
        m_serial->printf(FSTR("Heap           :    %7d     %7d     %7d\r\n"), totHeap, totHeap - freeHeap, freeHeap);
        m_serial->printf(FSTR("Flash          :    %7d\r\n"), getFlashSize());
        m_serial->printf(FSTR("Flash settings :    %7d     %7d     %7d\r\n"), 4096, flashDictUsedSpace, 4096 - flashDictUsedSpace);
    }


    void MTD_FLASHMEM SerialConsole::cmd_ifconfig()
    {
        if (m_paramsCount == 5 && hasParameter(1, FSTR("static")))
        {
            // set static IP
            APtr<char> strIP( t_strdup(m_params[2]) );
            APtr<char> strMSK( t_strdup(m_params[3]) );
            APtr<char> strGTY( t_strdup(m_params[4]) );
            ConfigurationManager::setClientIPParams(true, strIP.get(), strMSK.get(), strGTY.get());
            ConfigurationManager::applyClientIP();
        }
        else
        if (m_paramsCount == 2 && hasParameter(1, FSTR("dhcp")))
        {
            // set dyncamic IP (DHCP)
            ConfigurationManager::setClientIPParams(false, STR_, STR_, STR_);
            ConfigurationManager::applyClientIP();
        }
        else
        if (m_paramsCount == 5 && hasParameter(1, FSTR("ap")))
        {
            // set Access Point static IP
            APtr<char> strIP( t_strdup(m_params[2]) );
            APtr<char> strMSK( t_strdup(m_params[3]) );
            APtr<char> strGTY( t_strdup(m_params[4]) );
            ConfigurationManager::setAccessPointIPParams(strIP.get(), strMSK.get(), strGTY.get());
            ConfigurationManager::applyAccessPointIP();
        }        
        else
        if (m_paramsCount == 4 && hasParameter(1, FSTR("dns")))
        {
            // set DNS1 and DNS2
            APtr<char> strDNS1( t_strdup(m_params[2]) );
            APtr<char> strDNS2( t_strdup(m_params[3]) );
            ConfigurationManager::setDNSParams(IPAddress(strDNS1.get()), IPAddress(strDNS2.get()));
            ConfigurationManager::applyDNS();
        }        
        else
        {
            // show info
            for (int32_t i = 0; i < 2; ++i)
            {
                m_serial->printf(i == 0? FSTR("Client Network:\r\n") : FSTR("Access Point Network:\r\n"));
                uint8_t mac[6];
                WiFi::getMACAddress((WiFi::Network)i, mac);
                m_serial->printf(FSTR("   ether %02x:%02x:%02x:%02x:%02x:%02x\r\n"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                uint8_t IP[4];
                uint8_t netmask[4];
                uint8_t gateway[4];
                IP::getIPInfo((WiFi::Network)i, IP, netmask, gateway);
                m_serial->printf(FSTR("   inet %d.%d.%d.%d netmask %d.%d.%d.%d gateway %d.%d.%d.%d\r\n"), 
                                 IP[0], IP[1], IP[2], IP[3],
                                 netmask[0], netmask[1], netmask[2], netmask[3],
                                 gateway[0], gateway[1], gateway[2], gateway[3]);
                if (i == 0)
                {
                    // In client mode show status
                    char const* connectionStatus = FSTR("");
                    switch (WiFi::getClientConnectionStatus())
                    {
                        case WiFi::ClientConnectionStatus_Idle:
                            connectionStatus = FSTR("Idle");
                            break;
                        case WiFi::ClientConnectionStatus_Connecting:
                            connectionStatus = FSTR("Connecting");
                            break;
                        case WiFi::ClientConnectionStatus_WrongPassword:
                            connectionStatus = FSTR("Wrong Password");
                            break;
                        case WiFi::ClientConnectionStatus_NoAPFound:
                            connectionStatus = FSTR("No AP Found");
                            break;
                        case WiFi::ClientConnectionStatus_Fail:
                            connectionStatus = FSTR("Fail");
                            break;
                        case WiFi::ClientConnectionStatus_GotIP:
                            connectionStatus = FSTR("Connected");
                            break;
                    }
                    m_serial->printf(FSTR("   status <%s>\r\n"), connectionStatus);
                }
            }
            m_serial->printf(FSTR("DNS1 %s DNS2 %s\r\n"), (char const*)NSLookup::getDNSServer(0).get_str(), 
                                                          (char const*)NSLookup::getDNSServer(1).get_str());
        }
    }

        
    void MTD_FLASHMEM SerialConsole::cmd_iwlist()
    {
        m_serial->printf(FSTR("Cells found:\r\n"));
        uint32_t count = 0;
        bool scan = (m_paramsCount == 2 && hasParameter(1, FSTR("scan")));
        WiFi::APInfo* infos = WiFi::getAPList(&count, scan);
        for (uint32_t i = 0; i != count; ++i)
        {
            m_serial->printf(FSTR("  %2d - Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n"), i, infos[i].BSSID[0], infos[i].BSSID[1], infos[i].BSSID[2], infos[i].BSSID[3], infos[i].BSSID[4], infos[i].BSSID[5]);
            m_serial->printf(FSTR("       SSID: %s\r\n"), infos[i].SSID);
            m_serial->printf(FSTR("       Channel: %d\r\n"), infos[i].Channel);
            m_serial->printf(FSTR("       RSSI: %d\r\n"), infos[i].RSSI);
            m_serial->printf(FSTR("       Security: %s\r\n"), WiFi::convSecurityProtocolToString(infos[i].AuthMode));
        }
    }
        
        
    void MTD_FLASHMEM SerialConsole::cmd_date()
    {
        char buf[30];
        DateTime::now().format(buf, FSTR("%c"));
        m_serial->writeln(buf);
    }


    void MTD_FLASHMEM SerialConsole::cmd_ntpdate()
    {
        IPAddress serverIP; // default is 0.0.0.0, accepted by getFromNTPServer()
        
        // is there the SERVER parameter?
        if (m_paramsCount > 1)
        {
            APtr<char> server( t_strdup(m_params[1]) );
            serverIP = NSLookup::lookup(server.get());
        }
        
        char buf[30];
        DateTime dt;
        if (dt.getFromNTPServer(serverIP))
        {                
            dt.format(buf, FSTR("%c"));
            m_serial->writeln(buf);
        }
        else
            m_serial->printf(FSTR("fail\r\n"));
    }

        
    void MTD_FLASHMEM SerialConsole::cmd_nslookup()
    {
        if (m_paramsCount != 2)
        {
            m_serial->writeln(FSTR("Error\r\n"));
            return;
        }
        APtr<char> name( t_strdup(m_params[1]) );
        m_serial->writeln(NSLookup::lookup(name.get()).get_str());
    }
        
        
    void MTD_FLASHMEM SerialConsole::cmd_uptime()
    {
        char uptimeStr[22];
        ConfigurationManager::getUpTimeStr(uptimeStr);
        m_serial->write(uptimeStr);
    }
    
    
    void MTD_FLASHMEM SerialConsole::cmd_ping()
    {
        if (m_paramsCount != 2)
        {
            m_serial->writeln(FSTR("Error\r\n"));
            return;
        }
        APtr<char> server( t_strdup(m_params[1]) );
        IPAddress serverIP = NSLookup::lookup(server.get());
        ICMP icmp;
        while (true)
        {
            float r = icmp.ping(serverIP) / 1000.0;
            m_serial->printf(FSTR("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\r\n"), icmp.receivedBytes(), (char const*)serverIP.get_str(), icmp.receivedSeq(), icmp.receivedTTL(), r);
            
            if (m_serial->available() > 0 && m_serial->read() == 27)
                break;
            
            Task::delay(1000);
        }
    }

    
    void MTD_FLASHMEM SerialConsole::cmd_router()
    {
        if (m_paramsCount == 2)
        {
            ConfigurationManager::setRouting(hasParameter(1, FSTR("on")));
            ConfigurationManager::applyRouting();
        }
    }

            
    void MTD_FLASHMEM SerialConsole::cmd_test()
    {
    }

        
        
#endif




	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// SerialBinary

#if (FDV_INCLUDE_SERIALBINARY == 1)
    



	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// SerialBinary::Message


    MTD_FLASHMEM SerialBinary::Message::Message()
        : valid(false), ID(0), command(0), dataSize(0), data(NULL)
    {
    }
    
    
    MTD_FLASHMEM SerialBinary::Message::Message(uint8_t ID_, uint8_t command_, uint16_t dataSize_)
        : valid(true), ID(ID_), command(command_), dataSize(dataSize_), data(dataSize_? new uint8_t[dataSize_] : NULL)
    {
    }
    

    void MTD_FLASHMEM SerialBinary::Message::freeData()
    {
        if (data != NULL)
        {
            delete[] data;
            data = NULL;
        }
    }



	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Message_ACK
    
    // base for ACK messages 
    struct Message_ACK
    {
        static uint16_t const SIZE = 1;
        
        uint8_t& ackID;
        
        // used to decode message
        Message_ACK(SerialBinary::Message* msg)
            : ackID(msg->data[0])
        {
        }
        // used to encode message
        Message_ACK(SerialBinary::Message* msg, uint8_t ackID_)
            : ackID(msg->data[0])
        {
            ackID = ackID_;
        }
    };
		
			

	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Message_CMD_READY
            
    struct Message_CMD_READY
    {
        static uint16_t const SIZE = 12;
        
        uint8_t& protocolVersion;
        uint8_t& platform;
        char*    magicString;
        
        // used to decode message
        Message_CMD_READY(SerialBinary::Message* msg)
            : protocolVersion(msg->data[0]), 
              platform(msg->data[1]), 
              magicString((char*)msg->data + 2)
        {
        }
        // used to encode message
        Message_CMD_READY(SerialBinary::Message* msg, uint8_t protocolVersion_, uint8_t platform_, char const* magicString_)
            : protocolVersion(msg->data[0]), 
              platform(msg->data[1]), 
              magicString((char*)msg->data + 2)
        {
            protocolVersion = protocolVersion_;
            platform        = platform_;
            f_strcpy(magicString, magicString_);
        }
        
    };
		
        
        
	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Message_CMD_READY_ACK        
		
    struct Message_CMD_READY_ACK : Message_ACK
    {
        static uint16_t const SIZE = Message_ACK::SIZE + 12;
        
        uint8_t& protocolVersion;
        uint8_t& platform;
        char*    magicString;
        
        // used to decode message
        Message_CMD_READY_ACK(SerialBinary::Message* msg)
            : Message_ACK(msg), 
              protocolVersion(msg->data[Message_ACK::SIZE + 0]), 
              platform(msg->data[Message_ACK::SIZE + 1]), 
              magicString((char*)msg->data + Message_ACK::SIZE + 2)
        {
        }
        // used to encode message
        Message_CMD_READY_ACK(SerialBinary::Message* msg, uint8_t ackID_, uint8_t protocolVersion_, uint8_t platform_, char const* magicString_)
            : Message_ACK(msg, ackID_), 
              protocolVersion(msg->data[Message_ACK::SIZE + 0]), 
              platform(msg->data[Message_ACK::SIZE + 1]), 
              magicString((char*)msg->data + Message_ACK::SIZE + 2)
        {
            protocolVersion = protocolVersion_;
            platform        = platform_;
            f_strcpy(magicString, magicString_);
        }			
    };


    
	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Message_CMD_IOCONF        
    
    struct Message_CMD_IOCONF
    {
        static uint16_t const SIZE = 2;
        
        uint8_t& pin;
        uint8_t& flags;
        
        // used to decode message
        Message_CMD_IOCONF(SerialBinary::Message* msg)
            : pin(msg->data[0]), 
              flags(msg->data[1])
        {
        }
        // used to encode message
        Message_CMD_IOCONF(SerialBinary::Message* msg, uint8_t pin_, uint8_t flags_)
            : pin(msg->data[0]), 
              flags(msg->data[1])
        {
            pin   = pin_;
            flags = flags_;
        }			
    };


    
	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Message_CMD_IOSET        
    
    struct Message_CMD_IOSET
    {
        static uint16_t const SIZE = 2;
        
        uint8_t& pin;
        uint8_t& state;
        
        // used to decode message
        Message_CMD_IOSET(SerialBinary::Message* msg)
            : pin(msg->data[0]), 
              state(msg->data[1])
        {
        }
        // used to encode message
        Message_CMD_IOSET(SerialBinary::Message* msg, uint8_t pin_, uint8_t state_)
            : pin(msg->data[0]), 
              state(msg->data[1])
        {
            pin   = pin_;
            state = state_;
        }			
    };

    
    
	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Message_CMD_IOGET        
    
    struct Message_CMD_IOGET
    {
        static uint16_t const SIZE = 1;
        
        uint8_t& pin;
        
        // used to decode message
        Message_CMD_IOGET(SerialBinary::Message* msg)
            : pin(msg->data[0])
        {
        }
        // used to encode message
        Message_CMD_IOGET(SerialBinary::Message* msg, uint8_t pin_)
            : pin(msg->data[0])
        {
            pin   = pin_;
        }			
    };


    
	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Message_CMD_IOGET_ACK        
    
    struct Message_CMD_IOGET_ACK : Message_ACK
    {
        static uint16_t const SIZE = Message_ACK::SIZE + 1;
        
        uint8_t& state;
        
        // used to decode message
        Message_CMD_IOGET_ACK(SerialBinary::Message* msg)
            : Message_ACK(msg), 
              state(msg->data[Message_ACK::SIZE + 0])
        {
        }
        // used to encode message
        Message_CMD_IOGET_ACK(SerialBinary::Message* msg, uint8_t ackID_, uint8_t state_)
            : Message_ACK(msg, ackID_), 
              state(msg->data[Message_ACK::SIZE + 0])
        {
            state = state_;
        }			
    };

    
    
	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Message_CMD_IOASET        
    
    struct Message_CMD_IOASET
    {
        static uint16_t const SIZE = 3;
        
        uint8_t&  pin;
        uint16_t& state;
        
        // used to decode message
        Message_CMD_IOASET(SerialBinary::Message* msg)
            : pin(msg->data[0]), 
              state(*(uint16_t*)(msg->data + 1))
        {
        }
        // used to encode message
        Message_CMD_IOASET(SerialBinary::Message* msg, uint8_t pin_, uint16_t state_)
            : pin(msg->data[0]), 
              state(*(uint16_t*)(msg->data + 1))
        {
            pin   = pin_;
            state = state_;
        }			
    };


    
	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Message_CMD_IOAGET        
    
    struct Message_CMD_IOAGET
    {
        static uint16_t const SIZE = 1;
        
        uint8_t& pin;
        
        // used to decode message
        Message_CMD_IOAGET(SerialBinary::Message* msg)
            : pin(msg->data[0])
        {
        }
        // used to encode message
        Message_CMD_IOAGET(SerialBinary::Message* msg, uint8_t pin_)
            : pin(msg->data[0])
        {
            pin   = pin_;
        }			
    };
		

        
	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Message_CMD_IOAGET_ACK        
        
    struct Message_CMD_IOAGET_ACK : Message_ACK
    {
        static uint16_t const SIZE = Message_ACK::SIZE + 2;
        
        uint16_t& state;
        
        // used to decode message
        Message_CMD_IOAGET_ACK(SerialBinary::Message* msg)
            : Message_ACK(msg), 
              state(*(uint16_t*)(msg->data + Message_ACK::SIZE + 0))
        {
        }
        // used to encode message
        Message_CMD_IOAGET_ACK(SerialBinary::Message* msg, uint8_t ackID_, uint16_t state_)
            : Message_ACK(msg, ackID_), 
              state(*(uint16_t*)(msg->data + Message_ACK::SIZE + 0))
        {
            state = state_;
        }			
    };

    

	//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////
    
    MTD_FLASHMEM SerialBinary::SerialBinary()
        : m_serial(HardwareSerial::getSerial(0)), 
          m_recvID(255), 
          m_sendID(0), 
          m_recvACKQueue(ACKMSG_QUEUE_LENGTH), 
          m_receiveTask(this, false, 256),
          m_isReady(false),
          m_platform(PLATFORM_BASELINE),
          m_HTTPRoutes(NULL)
    {
    }
    
    
    MTD_FLASHMEM SerialBinary::~SerialBinary()
    {
        m_receiveTask.terminate();
        delete m_HTTPRoutes;
        // todo: free pending messages
    }


    bool MTD_FLASHMEM SerialBinary::isReady()
    {
        MutexLock lock(&m_mutex);
        return m_isReady;
    }
    
    
    bool MTD_FLASHMEM SerialBinary::checkReady()
    {
        if (!isReady())
            send_CMD_READY();
        return isReady();
    }
    

    uint8_t MTD_FLASHMEM SerialBinary::getPlatform()
    {
        checkReady();
        MutexLock lock(&m_mutex);
        return m_platform;
    }
    
    
    StringList* MTD_FLASHMEM SerialBinary::getHTTPRoutes()
    {
        MutexLock lock(&m_himutex);
        checkReady();
        if (m_HTTPRoutes == NULL)   // already requested?
        {
            // request handled pages
            m_HTTPRoutes = new StringList;
            if (!send_CMD_GETHTTPHANDLEDPAGES())
            {
                // get a chance to retry            
                delete m_HTTPRoutes;    
                m_HTTPRoutes = NULL;
            }
        }
        return m_HTTPRoutes;
    }
    
    
    SerialBinary::Message MTD_FLASHMEM SerialBinary::receive()
    {
        Message msg;
        SoftTimeOut timeout(WAIT_MSG_TIMEOUT);
        while (!timeout)
        {
            // ID
            int16_t r = m_serial->read(INTRA_MSG_TIMEOUT);
            if (r < 0)
                continue;
            msg.ID = r;

            // Command
            r = m_serial->read(INTRA_MSG_TIMEOUT);
            if (r < 0)
                continue;
            msg.command = r;

            // Data Size Low
            r = m_serial->read(INTRA_MSG_TIMEOUT);
            if (r < 0)
                continue;
            msg.dataSize = r;

            // Data Size High
            r = m_serial->read(INTRA_MSG_TIMEOUT);
            if (r < 0)
                continue;
            msg.dataSize |= r << 8;

            // Data			
            if (msg.dataSize > 0 && msg.dataSize < (Task::getFreeHeap() >> 1))
            {
                msg.data = new uint8_t[msg.dataSize];
                if (m_serial->read(msg.data, msg.dataSize, INTRA_MSG_TIMEOUT) < msg.dataSize)
                {
                    msg.freeData();
                    continue;
                }
            }
            
            // check ID
            if (msg.ID == m_recvID)
            {
                msg.freeData();
                continue;
            }
            m_recvID = msg.ID;
            
            msg.valid = true;
            return msg;
        }			
        return msg;
    }
    
    
    uint8_t MTD_FLASHMEM SerialBinary::getNextID()
    {
        MutexLock lock(&m_mutex);
        return ++m_sendID;
    }
    

    void MTD_FLASHMEM SerialBinary::send(Message* msg)
    {
        MutexLock lock(&m_mutex);
        m_serial->write(msg->ID);
        m_serial->write(msg->command);
        m_serial->write(msg->dataSize & 0xFF);
        m_serial->write((msg->dataSize >> 8) & 0xFF);
        if (msg->dataSize > 0)
            m_serial->write(msg->data, msg->dataSize);
    }
    
    
    // send ACK without parameters
    void MTD_FLASHMEM SerialBinary::sendNoParamsACK(uint8_t ackID)
    {
        Message msgContainer(getNextID(), CMD_ACK, Message_ACK::SIZE);
        Message_ACK msgACK(&msgContainer, ackID);
        send(&msgContainer);
        msgContainer.freeData();			
    }
    
    
    SerialBinary::Message MTD_FLASHMEM SerialBinary::waitACK(uint8_t ackID)
    {
        Message msgContainer;
        SoftTimeOut timeout(GETACK_TIMEOUT);
        while (!timeout)
        {
            if (m_recvACKQueue.receive(&msgContainer, GETACK_TIMEOUT))
            {
                Message_ACK msgACK(&msgContainer);
                if (msgACK.ackID == ackID)
                    return msgContainer;
                msgContainer.freeData();	// discard this ACK
            }
        }
        msgContainer.valid = false;
        return msgContainer;
    }
    

    bool MTD_FLASHMEM SerialBinary::waitNoParamsACK(uint8_t ackID)
    {
        Message msgContainer = waitACK(ackID);
        if (msgContainer.valid)
        {
            msgContainer.freeData();
            return true;
        }
        return false;
    }		
    
    
    void MTD_FLASHMEM SerialBinary::receiveTask()
    {
        while (true)
        {
            Message msg = receive();
            if (msg.valid)
            {
                if (msg.command == CMD_ACK)
                    // if message is an ACK then put it into the ACK message queue, another task will handle it
                    m_recvACKQueue.send(msg, PUTACK_TIMEOUT);
                else
                    processMessage(&msg);
            }
        }
    }
    
    
    // must not process CMD_ACK messages
    void MTD_FLASHMEM SerialBinary::processMessage(SerialBinary::Message* msg)
    {
        switch (msg->command)
        {
            case CMD_READY:
                handle_CMD_READY(msg);
                break;
            case CMD_IOCONF:
                handle_CMD_IOCONF(msg);
                break;
            case CMD_IOSET:
                handle_CMD_IOSET(msg);
                break;
            case CMD_IOGET:
                handle_CMD_IOGET(msg);
                break;
            case CMD_IOASET:
                handle_CMD_IOASET(msg);
                break;
            case CMD_IOAGET:
                handle_CMD_IOAGET(msg);
                break;
        }			
        msg->freeData();
    }
    
    
    void MTD_FLASHMEM SerialBinary::handle_CMD_READY(Message* msg)
    {
        // process message
        Message_CMD_READY msgCMDREADY(msg);
        m_mutex.lock();
        m_isReady  = (msgCMDREADY.protocolVersion == PROTOCOL_VERSION && f_strcmp(msgCMDREADY.magicString, STR_BINPRORDY) == 0);
        m_platform = msgCMDREADY.platform;
        m_mutex.unlock();
        
        // send ACK with parameters
        Message msgContainer(getNextID(), CMD_ACK, Message_CMD_READY_ACK::SIZE);
        Message_CMD_READY_ACK msgCMDREADYACK(&msgContainer, msg->ID, PROTOCOL_VERSION, PLATFORM_THIS, STR_BINPRORDY);
        send(&msgContainer);
        msgContainer.freeData();
    }
    
    
    void MTD_FLASHMEM SerialBinary::handle_CMD_IOCONF(Message* msg)
    {
        // process message
        Message_CMD_IOCONF msgIOCONF(msg);
        if (msgIOCONF.flags & PIN_CONF_OUTPUT)
            GPIOX(msgIOCONF.pin).modeOutput();
        else
            GPIOX(msgIOCONF.pin).modeInput();
        GPIOX(msgIOCONF.pin).enablePullUp(msgIOCONF.flags & PIN_CONF_PULLUP);
                    
        // send simple ACK
        sendNoParamsACK(msg->ID);
    }
    

    void MTD_FLASHMEM SerialBinary::handle_CMD_IOSET(Message* msg)
    {
        // process message
        Message_CMD_IOSET msgIOSET(msg);
        GPIOX(msgIOSET.pin).write(msgIOSET.state);
        
        // send simple ACK
        sendNoParamsACK(msg->ID);
    }
    
    
    void MTD_FLASHMEM SerialBinary::handle_CMD_IOGET(Message* msg)
    {
        // process message
        Message_CMD_IOGET msgIOGET(msg);
        bool state = GPIOX(msgIOGET.pin).read();
        
        // send ACK with parameters
        Message msgContainer(getNextID(), CMD_ACK, Message_CMD_IOGET_ACK::SIZE);
        Message_CMD_IOGET_ACK msgCMDIOGETACK(&msgContainer, msg->ID, state);
        send(&msgContainer);
        msgContainer.freeData();
    }
    
    
    // not implemented
    void MTD_FLASHMEM SerialBinary::handle_CMD_IOASET(Message* msg)
    {
        // process message
        // not implemented
        
        // send simple ACK
        sendNoParamsACK(msg->ID);
    }
    
    
    // not implemented
    void MTD_FLASHMEM SerialBinary::handle_CMD_IOAGET(Message* msg)
    {
        // process message
        // not implemented
        
        // send ACK with parameters
        Message msgContainer(getNextID(), CMD_ACK, Message_CMD_IOAGET_ACK::SIZE);
        Message_CMD_IOAGET_ACK msgCMDIOGETACK(&msgContainer, msg->ID, 0);	// always returns 0!
        send(&msgContainer);
        msgContainer.freeData();
    }		
    

    bool MTD_FLASHMEM SerialBinary::send_CMD_READY()
    {
        m_isReady = false;
        for (uint32_t i = 0; i != MAX_RESEND_COUNT; ++i)
        {
            // send message
            uint8_t msgID = getNextID();
            Message msgContainer(msgID, CMD_READY, Message_CMD_READY::SIZE);
            Message_CMD_READY msgCMDREADY(&msgContainer, PROTOCOL_VERSION, PLATFORM_THIS, STR_BINPRORDY);
            send(&msgContainer);
            msgContainer.freeData();
            
            // wait for ACK
            msgContainer = waitACK(msgID);
            if (msgContainer.valid)
            {
                Message_CMD_READY_ACK msgCMDREADYACK(&msgContainer);
                MutexLock lock(&m_mutex);
                m_isReady  = (msgCMDREADYACK.protocolVersion == PROTOCOL_VERSION && f_strcmp(msgCMDREADYACK.magicString, STR_BINPRORDY) == 0);
                m_platform = msgCMDREADYACK.platform;
                msgContainer.freeData();
                return true;
            }
        }
        return false;
    }
    
    
    bool MTD_FLASHMEM SerialBinary::send_CMD_IOCONF(uint8_t pin, uint8_t flags)
    {
        if (checkReady())
        {
            for (uint32_t i = 0; i != MAX_RESEND_COUNT; ++i)
            {
                // send message
                uint8_t msgID = getNextID();
                Message msgContainer(msgID, CMD_IOCONF, Message_CMD_IOCONF::SIZE);
                Message_CMD_IOCONF msgCMDIOCONF(&msgContainer, pin, flags);
                send(&msgContainer);
                msgContainer.freeData();
                
                // wait for ACK
                if (waitNoParamsACK(msgID))
                    return true;
            }
            m_isReady = false;	// no more ready
        }
        return false;
    }
    
    
    bool MTD_FLASHMEM SerialBinary::send_CMD_IOSET(uint8_t pin, uint8_t state)
    {
        if (checkReady())
        {
            for (uint32_t i = 0; i != MAX_RESEND_COUNT; ++i)
            {
                // send message
                uint8_t msgID = getNextID();
                Message msgContainer(msgID, CMD_IOSET, Message_CMD_IOSET::SIZE);
                Message_CMD_IOSET msgCMDIOSET(&msgContainer, pin, state);
                send(&msgContainer);
                msgContainer.freeData();
                
                // wait for ACK
                if (waitNoParamsACK(msgID))
                    return true;
            }
            m_isReady = false;	// no more ready
        }
        return false;
    }
    
    
    bool MTD_FLASHMEM SerialBinary::send_CMD_IOGET(uint8_t pin, uint8_t* state)
    {
        if (checkReady())
        {
            for (uint32_t i = 0; i != MAX_RESEND_COUNT; ++i)
            {
                // send message
                uint8_t msgID = getNextID();
                Message msgContainer(msgID, CMD_IOGET, Message_CMD_IOGET::SIZE);
                Message_CMD_IOGET msgCMDIOGET(&msgContainer, pin);
                send(&msgContainer);
                msgContainer.freeData();
                
                // wait for ACK
                msgContainer = waitACK(msgID);
                if (msgContainer.valid)
                {
                    Message_CMD_IOGET_ACK msgCMDIOGETACK(&msgContainer);
                    *state = msgCMDIOGETACK.state;
                    msgContainer.freeData();
                    return true;
                }
            }
            m_isReady = false;	// no more ready
        }
        return false;
    }

    
    bool MTD_FLASHMEM SerialBinary::send_CMD_IOASET(uint8_t pin, uint16_t state)
    {
        if (checkReady())
        {
            for (uint32_t i = 0; i != MAX_RESEND_COUNT; ++i)
            {
                // send message
                uint8_t msgID = getNextID();
                Message msgContainer(msgID, CMD_IOASET, Message_CMD_IOASET::SIZE);
                Message_CMD_IOASET msgCMDIOASET(&msgContainer, pin, state);
                send(&msgContainer);
                msgContainer.freeData();
                
                // wait for ACK
                if (waitNoParamsACK(msgID))
                    return true;
            }
            m_isReady = false;	// no more ready
        }
        return false;
    }
    
    
    bool MTD_FLASHMEM SerialBinary::send_CMD_IOAGET(uint8_t pin, uint16_t* state)
    {
        if (checkReady())
        {
            for (uint32_t i = 0; i != MAX_RESEND_COUNT; ++i)
            {
                // send message
                uint8_t msgID = getNextID();
                Message msgContainer(msgID, CMD_IOAGET, Message_CMD_IOAGET::SIZE);
                Message_CMD_IOAGET msgCMDIOAGET(&msgContainer, pin);
                send(&msgContainer);
                msgContainer.freeData();
                
                // wait for ACK
                msgContainer = waitACK(msgID);
                if (msgContainer.valid)
                {
                    Message_CMD_IOAGET_ACK msgCMDIOAGETACK(&msgContainer);
                    *state = msgCMDIOAGETACK.state;
                    msgContainer.freeData();
                    return true;
                }
            }
            m_isReady = false;	// no more ready
        }
        return false;
    }
    
    
    bool MTD_FLASHMEM SerialBinary::send_CMD_GETHTTPHANDLEDPAGES()
    {
        m_HTTPRoutes->clear();
        if (checkReady())
        {
            for (uint32_t i = 0; i != MAX_RESEND_COUNT; ++i)
            {
                // send message
                uint8_t msgID = getNextID();
                Message msgContainer(msgID, CMD_GETHTTPHANDLEDPAGES, 0);
                send(&msgContainer);
                msgContainer.freeData();
                
                // wait for ACK
                msgContainer = waitACK(msgID);
                if (msgContainer.valid)
                {
                    char const* rpos = (char const*)(msgContainer.data + Message_ACK::SIZE);
                    uint8_t itemsCount = (uint8_t)(*rpos++);
                    for (uint8_t j = 0; j != itemsCount; ++j)
                    {
                        m_HTTPRoutes->add(rpos, StringList::Heap);
                        rpos += f_strlen(rpos) + 1;
                    }                    
                    msgContainer.freeData();
                    return true;
                }
            }
            m_isReady = false;	// no more ready
        }
        return false;        
    }
    
    



#endif


} // end of "fdv" namespace

