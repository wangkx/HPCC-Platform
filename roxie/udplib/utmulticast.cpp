/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#include "jthread.hpp"
#include "jsocket.hpp"
#include "jsem.hpp"
#include "jdebug.hpp"
#include "jlog.hpp"
#include <time.h>

void usage()
{
    printf("USAGE: utmulticast [OPTIION]\n");
    printf("   Options are:\n");
    printf("      -send\n"
           "      -ip IP default:239.1.1.2\n"
           "      -port nn default:7000\n"
           "      -packageSize nn(>=60) default:60\n"
           "      -numGroupsToSend nn default:10\n"
           "      -numPackagetsInGroup nn default:10\n"
           "      -delayBetweenGroups nn default:1000 millseconds\n"
        );
    ExitModuleObjects();
    releaseAtoms();
    exit(1);
}

const char *defaultIP = "239.1.1.2";
unsigned defaultPort = 7000;

void multicastRecvTest(const char* snifferIP, unsigned snifferPort, unsigned packageSize)
{
    class receive_sniffer : public Thread
    {
        ISocket     *sniffer_socket;
        IpAddress   snifferIP;
        unsigned    snifferPort;
        unsigned    packageSize;
        bool        running;

    public:
        receive_sniffer(unsigned _snifferPort, const IpAddress &_snifferIP, unsigned _packageSize)
          : Thread("udplib::receive_sniffer"), snifferPort(_snifferPort), snifferIP(_snifferIP), packageSize(_packageSize), running(false)
        {
            sniffer_socket = ISocket::multicast_create(snifferPort, snifferIP);

            StringBuffer ipStr;
            snifferIP.getIpText(ipStr);
            DBGLOG("UdpReceiver: receive_sniffer port open %s:%i", ipStr.str(), snifferPort);
        }

        ~receive_sniffer()
        {
            running = false;
            if (sniffer_socket) sniffer_socket->close();
            join();
            if (sniffer_socket) sniffer_socket->Release();
        }

        virtual int run()
        {
            DBGLOG("UdpReceiver: sniffer started");
            while (running)
            {
                try
                {
                    unsigned int res;
                    byte *buffer = new byte[packageSize+1];
                    sniffer_socket->read(buffer, 1, packageSize, res, 5);
                    buffer[packageSize] = 0;
                    DBGLOG("Received %d bytes: %s", packageSize, buffer);
                    delete buffer;
                }
                catch (IException *e)
                {
                    if (running && e->errorCode() != JSOCKERR_timeout_expired)
                    {
                        StringBuffer s;
                        DBGLOG("UdpReceiver: receive_sniffer::run read failed %s", e->errorMessage(s).toCharArray());
                        MilliSleep(1000);
                    }
                    e->Release();
                }
                catch (...)
                {
                    DBGLOG("UdpReceiver: receive_sniffer::run unknown exception");
                    if (sniffer_socket)
                    {
                        sniffer_socket->Release();
                        sniffer_socket = ISocket::multicast_create(snifferPort, snifferIP);
                    }
                    MilliSleep(1000);
                }
            }
            return 0;
        }

        virtual void start()
        {
            running = true;
            Thread::start();
        }
    };

    IpAddress ip(snifferIP);
    receive_sniffer* sniffer = new receive_sniffer(snifferPort, ip, packageSize);
    sniffer->start();
    MilliSleep(15);

    printf("\n\nEnter any key and <RETURN> to terminate program. \n");
    char sBuff[100];
    int len = scanf("%99s", sBuff);

    delete sniffer;
}

void sendTest(const char* ipString, unsigned port, unsigned numGroupsToSend, unsigned numPackagetsInGroup,
    unsigned delayBetweenGroups, unsigned packageSize)
{
    IpAddress ip(ipString);
    SocketEndpoint ep(port, ip);
    ISocket* sock = ISocket::udp_connect(ep);

    StringBuffer buffer;
    for (unsigned i = 0; i < numGroupsToSend; i++)
    {
        for (unsigned j = 0; j < numPackagetsInGroup; j++)
        {
            buffer.setf("Sent: Group %d package %d.", i+1, j+1);
            unsigned len = buffer.length();
            if (len < packageSize)
                buffer.appendN(packageSize-len, '.');
            sock->write(buffer, packageSize);
            printf("%s\n", buffer.str());
        }
        printf("Group %d sent...\n", i+1);
        MilliSleep(delayBetweenGroups);
    }

    MilliSleep(3000);
    sock->close();
    sock->Release();
}

int main(int argc, char * argv[] )
{
    InitModuleObjects();

    StringBuffer cmdline;
    int c;
    for (c = 0; c < argc; c++)
    {
        if (c)
            cmdline.append(' ');
        cmdline.append(argv[c]);
    }
    DBGLOG("Command:%s",cmdline.str());

    bool send = false;
    StringBuffer ip(defaultIP);
    unsigned port = defaultPort;
    unsigned numGroupsToSend = 10;
    unsigned numPackagetsInGroup = 10;
    unsigned delayBetweenGroups = 1000;
    unsigned packageSize = 60;
    for (c = 1; c < argc; c++)
    {
        const char *arg = argv[c];
        if (streq(arg, "-send"))
        {
            send = true;
        }
        else if (streq(arg, "-ip"))
        {
            c++;
            if (c==argc)
                usage();
            ip.set(argv[c]);
        }
        else if (streq(arg, "-port"))
        {
            c++;
            if (c==argc || !isdigit(*argv[c]))
                usage();
            port = atoi(argv[c]);
        }
        else if(streq(arg,"-numGroupsToSend"))
        {
            c++;
            if (c==argc || !isdigit(*argv[c]))
                usage();
            numGroupsToSend = atoi(argv[c]);
        }
        else if(streq(arg,"-numPackagetsInGroup"))
        {
            c++;
            if (c==argc || !isdigit(*argv[c]))
                usage();
            numPackagetsInGroup = atoi(argv[c]);
        }
        else if(streq(arg,"-delayBetweenGroups"))
        {
            c++;
            if (c==argc || !isdigit(*argv[c]))
                usage();
            delayBetweenGroups = atoi(argv[c]);
        }
        else if(streq(arg,"-packageSize"))
        {
            c++;
            if (c==argc || !isdigit(*argv[c]))
                usage();
            packageSize = atoi(argv[c]);
        }
    }

    if (send)
        sendTest(ip.str(), port, numGroupsToSend, numPackagetsInGroup, delayBetweenGroups, packageSize);
    else
        multicastRecvTest(ip.str(), port, packageSize);

    ExitModuleObjects();
    releaseAtoms();
    return 0;
}

