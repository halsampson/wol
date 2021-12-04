// wol.cpp

// send Wake on Lan packet

#define USAGE "Use: wol <MAC addr>"

#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib,"Ws2_32.lib")

typedef struct {
  BYTE addr[6];
} MacAddr;

const MacAddr Sync = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
const int MacRepeats = 16;

typedef struct {
  MacAddr sync;
  MacAddr mac[MacRepeats];
  // TODO: optional password: 0, 4 or 6 bytes
} MagicPacket;

#define REMOTE_PORT 9  // open on Windows, ...

void usage(int err = -1) {
  printf(USAGE "\n");
  exit(err);
}

#if 1 // parse MAC address w/o sscanf() for embedded

MacAddr getMacAddr(char* str) {
  MacAddr macAddr = { 0 };
  for (int bytePos = 0; bytePos < 6; ++bytePos) {
    for (int niblPos = 4; niblPos >= 0; niblPos -= 4, ++str) { // MS then LS nibble
      while (!isalnum(*str) && *str) ++str; // skip punctuation ':', '-', ...     
      *str = toupper(*str);
      if (!*str || *str > 'F') usage(bytePos);
      macAddr.addr[bytePos] |= (*str - (*str >= 'A' ? 'A' - 10 : '0')) << niblPos;
    }
  }
  return macAddr;
}

#else // parse using sscanf

MacAddr getMacAddr(const char* str) {
  MacAddr macAddr;
  for (int bytePos = 0; bytePos < 6; ++bytePos, str += 2) {
    while (!isalnum(*str) && *str) ++str; // skip punctuation ':', '-', ...
    if (!*str || sscanf_s(str, "%02hhx", &macAddr.addr[bytePos]) != 1) usage(bytePos);     
  }
  return macAddr;
}
#endif

int main(int argc, char** argv) {
  MagicPacket magicPacket = { Sync };
  MacAddr macAddr = getMacAddr(argv[1]);
  for (int i = 0; i < MacRepeats; i++)
    magicPacket.mac[i] = macAddr;

  WSADATA wsaData = { 0 };
  int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (res) return res;

  SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  char optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(REMOTE_PORT);
  addr.sin_addr.S_un.S_un_b = { 255, 255, 255, 255 };  // broadcast address

  if (sendto(sock, (char*)&magicPacket, sizeof(magicPacket), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "Can't send!: %s\n", strerror(errno));
    return errno;
  }
  
  closesocket(sock);

  // TODO: optional arp / ping until ready or timeout -- see pingwait
  // ping 192.168.1.255 broadcast blocked by most


  WSACleanup();
  return 0;
}
