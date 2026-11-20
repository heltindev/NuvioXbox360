// Winsock/XNet shim for testing production Http.cpp on Windows. Never used by XEX.
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <cstring>
struct XNetStartupParams {unsigned char cfgSizeOfStruct,cfgFlags;};
struct XNADDR {IN_ADDR ina;};
struct XNDNS {int iStatus,cina;IN_ADDR aina[1];};
enum { XNET_STARTUP_BYPASS_SECURITY=1,XNET_GET_XNADDR_PENDING=0,XNET_GET_XNADDR_NONE=1,XNET_ETHERNET_LINK_ACTIVE=1 };
inline int XNetStartup(XNetStartupParams*){return 0;}
inline DWORD XNetGetTitleXnAddr(XNADDR *a){a->ina.s_addr=inet_addr("127.0.0.1");return 2;}
inline DWORD XNetGetEthernetLinkStatus(){return 1;}
inline int XNetRandom(unsigned char *b,unsigned long n){std::memset(b,17,n);return 0;}
inline int XNetDnsLookup(const char*,HANDLE,XNDNS**){return -1;}
inline void XNetDnsRelease(XNDNS*){}
