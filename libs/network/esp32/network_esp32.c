/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * Implementation of JsNetwork for ESP32 - cloned from linux
 * ----------------------------------------------------------------------------
 */
#include <string.h> // for memset
#include <stdlib.h>

#include "network_esp32.h"
#include "network.h"
#include "jsinteractive.h"

#define INVALID_SOCKET ((SOCKET)(-1))
#define SOCKET_ERROR (-1)

 #include <sys/stat.h>
 #include <errno.h>
#ifdef WIN32
 #include <winsock.h>
#else
 #ifndef ESP_PLATFORM
  #include <sys/select.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <resolv.h>
 #endif
 #include <sys/socket.h>
 #include <netdb.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <stdio.h>

  typedef struct sockaddr_in sockaddr_in;
  typedef int SOCKET;
#endif

#define closesocket(SOCK) close(SOCK)

#if NET_DBG > 0
#include "jsinteractive.h"
#define DBG(format, ...) jsiConsolePrintf(format, ## __VA_ARGS__)
#else
#define DBG(format, ...) do { } while(0)
#endif

/// Get an IP address from a name. Sets out_ip_addr to 0 on failure
void net_esp32_gethostbyname(JsNetwork *net, char * hostName, uint32_t* out_ip_addr) {
  NOT_USED(net);
  struct hostent * host_addr_p = gethostbyname(hostName);
  if (host_addr_p) {
    *out_ip_addr = *(uint32_t*)*host_addr_p->h_addr_list;
  }
}

/// Called on idle. Do any checks required for this device
void net_esp32_idle(JsNetwork *net) {
  NOT_USED(net);
}

/// Call just before returning to idle loop. This checks for errors and tries to recover. Returns true if no errors.
bool net_esp32_checkError(JsNetwork *net) {
  NOT_USED(net);
  bool hadErrors = false;
  return hadErrors;
}

/// if host=0, creates a server otherwise creates a client (and automatically connects). Returns >=0 on success
int net_esp32_createsocket(JsNetwork *net, SocketType socketType, uint32_t host, unsigned short port, JsVar *options) {
  int ippProto = socketType & ST_UDP ? IPPROTO_UDP : IPPROTO_TCP;
  int scktType = socketType & ST_UDP ? SOCK_DGRAM : SOCK_STREAM;
  int sckt = -1;

  if (host!=0) { // ------------------------------------------------- host (=client)

    sckt = socket(AF_INET, scktType, ippProto);
    if (sckt<0) {
      jsError("Socket creation failed");
      return sckt; // error
    }

    // turn on non-blocking mode
    #ifdef WIN_OS
    u_long n = 1;
    ioctlsocket(sckt,FIONBIO,&n);
    #endif

    if (scktType == SOCK_DGRAM) { // only for UDP
      // set broadcast
      int optval = 1;
      if (setsockopt(sckt,SOL_SOCKET,SO_BROADCAST,(const char *)&optval,sizeof(optval))<0) {
        jsDebug(DBG_INFO, "setsockopt(SO_BROADCAST) failed\n");
      }
    } else {
      sockaddr_in       sin;
      sin.sin_family = AF_INET;
      sin.sin_addr.s_addr = (in_addr_t)host;
      sin.sin_port = htons( port );

      int res = connect(sckt,(struct sockaddr *)&sin, sizeof(sockaddr_in) );

      if (res == SOCKET_ERROR) {
      #ifdef WIN_OS
       int err = WSAGetLastError();
      #else
       int err = errno;
      #endif
       if (err != EINPROGRESS &&
           err != EWOULDBLOCK) {
          jsError("Connect failed (err %d)", err);
          closesocket(sckt);
          return -1;
       }
      }
    }

  } else { // ------------------------------------------------- no host (=server)

    sckt = socket(AF_INET, scktType, ippProto);
    if (sckt == INVALID_SOCKET) {
      jsError("Socket creation failed");
      return 0;
    }

    if (scktType != SOCK_DGRAM ||
        jsvObjectGetBoolChild(options, "reuseAddr")) {
      int optval = 1;
      if (setsockopt(sckt,SOL_SOCKET,SO_REUSEADDR,(const char *)&optval,sizeof(optval)) < 0) {
        jsDebug(DBG_INFO, "setsockopt(SO_REUSADDR) failed\n");
      }
#ifdef SO_REUSEPORT
      if (setsockopt(sckt,SOL_SOCKET,SO_REUSEPORT,(const char *)&optval,sizeof(optval)) < 0) {
        jsDebug(DBG_INFO, "setsockopt(SO_REUSPORT) failed\n");
      }
#endif
    }

    int nret;
    sockaddr_in serverInfo;
    memset(&serverInfo, 0, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    //serverInfo.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // allow only LOCAL clients to connect
    serverInfo.sin_addr.s_addr = INADDR_ANY; // allow anyone to connect
    serverInfo.sin_port = (unsigned short)htons((unsigned short)port); // port
    nret = bind(sckt, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
    if (nret == SOCKET_ERROR) {
      jsError("Socket bind failed");
      closesocket(sckt);
      return -1;
    }

    // multicast support
    // FIXME: perhaps extend the JsNetwork with addmembership/removemembership instead of using options
    JsVar *mgrpVar = jsvObjectGetChildIfExists(options, "multicastGroup");
    if (mgrpVar) {
        char ipStr[18];

        uint32_t grpip;
        jsvGetString(mgrpVar, ipStr, sizeof(ipStr));
        jsvUnLock(mgrpVar);
        net_esp32_gethostbyname(net, ipStr, &grpip);

        JsVar *ipVar = jsvObjectGetChildIfExists(options, "multicastIp");
        jsvGetString(ipVar, ipStr, sizeof(ipStr));
        jsvUnLock(ipVar);
        uint32_t ip;
        net_esp32_gethostbyname(net, ipStr, &ip);

        struct ip_mreq mreq;
        mreq.imr_multiaddr = *(struct in_addr *)&grpip;
        mreq.imr_interface = *(struct in_addr *)&ip;
        setsockopt (sckt, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    }

    if (scktType == SOCK_STREAM) { // only for TCP
      // Make the socket listen
      nret = listen(sckt, 10); // 10 connections (but this ignored on CC30000)
      if (nret == SOCKET_ERROR) {
        jsError("Socket listen failed");
        closesocket(sckt);
        return -1;
      }
    }
  }

  if (!(socketType & ST_UDP)) {
    int flags = fcntl(sckt, F_GETFL, 0);
    if (flags >= 0) {
      if (fcntl(sckt, F_SETFL, flags | O_NONBLOCK) < 0) {
        jsDebug(DBG_INFO, "fcntl(F_SETFL, O_NONBLOCK) failed on ESP32 socket\n");
      }
    } else {
      jsDebug(DBG_INFO, "fcntl(F_GETFL) failed on ESP32 socket\n");
    }
  }

#ifdef SO_RCVBUF
  int rcvBufSize = net->data.recvBufferSize;
  if (rcvBufSize > 0) {
    if (setsockopt(sckt,SOL_SOCKET,SO_RCVBUF,(const char *)&rcvBufSize,sizeof(rcvBufSize))<0) {
      jsDebug(DBG_INFO, "setsockopt(SO_RCVBUF) failed\n");
    }
  }
#endif
#ifdef SO_NOSIGPIPE
  // disable SIGPIPE
  int optval = 1;
  if (setsockopt(sckt,SOL_SOCKET,SO_NOSIGPIPE,(const char *)&optval,sizeof(optval))<0) {
    jsDebug(DBG_INFO, "setsockopt(SO_NOSIGPIPE) failed\n");
  }
#endif

  return sckt;
}

/// destroys the given socket
void net_esp32_closesocket(JsNetwork *net, int sckt) {
  NOT_USED(net);
  closesocket(sckt);
}

/// If the given server socket can accept a connection, return it (or return < 0)
int net_esp32_accept(JsNetwork *net, int sckt) {
  NOT_USED(net);
  // TODO: look for unreffed servers?
  fd_set s;
  FD_ZERO(&s);
  FD_SET(sckt,&s);
  // check for waiting clients
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  int n = select(sckt+1,&s,NULL,NULL,&timeout);
  if (n>0) {
    // we have a client waiting to connect... try to connect and see what happens
    int theClient = accept(sckt,0,0);
    return theClient;
  }
  return -1;
}

/// Receive data if possible. returns nBytes on success, 0 on no data, or -1 on failure
int net_esp32_recv(JsNetwork *net, SocketType socketType, int sckt, void *buf, size_t len) {
  NOT_USED(net);
  struct sockaddr_in fromAddr;
  socklen_t fromAddrLen = sizeof(fromAddr);
  int num = 0;
  fd_set s;
  FD_ZERO(&s);
  FD_SET(sckt,&s);
  // check for waiting clients
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  int n = select(sckt+1,&s,NULL,NULL,&timeout);
  if (n==SOCKET_ERROR) {
    // we probably disconnected
    return -1;
  } else if (n>0) {
    // receive data
    if (socketType & ST_UDP) {
      num = (int)recvfrom(sckt,buf+sizeof(JsNetUDPPacketHeader),len-sizeof(JsNetUDPPacketHeader),0,(struct sockaddr*)&fromAddr, &fromAddrLen);

      JsNetUDPPacketHeader *header = (JsNetUDPPacketHeader*)buf;
      *(in_addr_t*)&header->host = fromAddr.sin_addr.s_addr;
      header->port = ntohs(fromAddr.sin_port);
      header->length = num;

      DBG("Recv %d %x:%d", num, *(uint32_t*)&header->host, header->port);
      if (num==0) return -1; // select says data, but recv says 0 means connection is closed
      num += sizeof(JsNetUDPPacketHeader);
    } else {
      num = (int)recvfrom(sckt,buf,len,0,(struct sockaddr*)&fromAddr, &fromAddrLen);
      if (num==0) return -1; // select says data, but recv says 0 means connection is closed
    }
  }

  return num;
}

/// Send data if possible. returns nBytes on success, 0 on no data, or -1 on failure
int net_esp32_send(JsNetwork *net, SocketType socketType, int sckt, const void *buf, size_t len) {
  NOT_USED(net);

  int flags = 0;
#if !defined(SO_NOSIGPIPE) && defined(MSG_NOSIGNAL)
  flags |= MSG_NOSIGNAL;
#endif
#ifdef MSG_DONTWAIT
  flags |= MSG_DONTWAIT;
#endif

  if (socketType & ST_UDP) {
    JsNetUDPPacketHeader *header = (JsNetUDPPacketHeader*)buf;
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = *(in_addr_t*)&header->host;
    sin.sin_port = htons(header->port);

    int n = (int)sendto(sckt, buf + sizeof(JsNetUDPPacketHeader), header->length, flags,
                       (struct sockaddr *)&sin, sizeof(sockaddr_in));
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
      return -1;
    }
    return n + sizeof(JsNetUDPPacketHeader);
  }

  int n = (int)send(sckt, buf, len, flags);
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
    return -1;
  }
  return n;
}

void netSetCallbacks_esp32(JsNetwork *net) {
  net->idle = net_esp32_idle;
  net->checkError = net_esp32_checkError;
  net->createsocket = net_esp32_createsocket;
  net->closesocket = net_esp32_closesocket;
  net->accept = net_esp32_accept;
  net->gethostbyname = net_esp32_gethostbyname;
  net->recv = net_esp32_recv;
  net->send = net_esp32_send;
  net->chunkSize = 536;
}
