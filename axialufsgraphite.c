#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ebur128.h>

int main(int argc, const char *argv[])
{
  struct sockaddr_in addr, graphiteAddr;
  int addrlen, sock, packetLength, graphiteSocket;
  struct ip_mreq mreq;
  uint8_t packet[1452]; 
  //uint16_t sequenceNumber;
  //uint32_t timestamp;
  double audioPayload[480]; 
  ebur128_state* state = NULL;
  double shortTermLoudness;
  uint32_t frameCounter = 0;
  char graphiteOutputBuffer[200];

  if (argc != 4) {
    fprintf(stderr, "Argument Error!\n");
    fprintf(stderr, "Correct usage: axialufsgraphite <MC Livewire IP> <Graphite Server IP> <Graphite Metric>\n");
    fprintf(stderr, "Example: axialufsgraphite 239.192.2.169 127.0.0.1 Studio442PGM\n");
    return 1;
  }

  const char *axiaLivewireIP = argv[1];
  const char *graphiteServerIP = argv[2];
  const char *graphiteMetric = argv[3];

  /* setup graphite socket */
  graphiteSocket = socket(AF_INET,SOCK_STREAM,0);
  memset(&graphiteAddr, 0, sizeof(graphiteAddr));
  graphiteAddr.sin_family = AF_INET;
  graphiteAddr.sin_port = htons(2003);
  inet_pton(AF_INET, graphiteServerIP, &(graphiteAddr.sin_addr));
  connect(graphiteSocket,(struct sockaddr *) &graphiteAddr, sizeof(graphiteAddr));
 
  /* setup axia socket (multicast UDP listener) */
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  int reuse = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1) {
      fprintf(stderr, "setsockopt: %d\n", errno);
      return 1;
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(5004);
  addr.sin_addr.s_addr = inet_addr(axiaLivewireIP);
  addrlen = sizeof(addr);
  if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
      fprintf(stderr, "bind: %d\n", errno);
      return 1;
  }
  mreq.imr_multiaddr.s_addr = inet_addr(axiaLivewireIP);         
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));

  /* init ebur128 state */
  state = malloc((size_t) sizeof(ebur128_state*));
  state = ebur128_init((unsigned) 2, (unsigned) 48000, EBUR128_MODE_S);

  /* main loop */
  while(1){
    packetLength = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *) &addr, (socklen_t *) &addrlen);
    
    //sequenceNumber = (packet[2] << 8 | packet[3]);
    //timestamp = (packet[4] << 24 | packet[5] << 16 | packet[6] << 8 | packet[7]);

    for (int i = 12; i < packetLength; i += 3) {
      int32_t audioPCM = ((packet[i] << 16) | (packet[i + 1] << 8) | (packet[i + 2]));
      if (audioPCM & 0x800000) audioPCM |= ~0xffffff; // Convert to signed PCM.
      double audioFloat = (audioPCM * (1.0 / 0x7fffff)); // Convert to float.
      audioPayload[((i - 12) / 3)] = audioFloat;
    }

    ebur128_add_frames_double(state, audioPayload, (size_t) ((packetLength - 12) / 6));

    frameCounter += ((packetLength - 12) / 6);
    if (frameCounter >= 47999) {
      frameCounter = 0;
      ebur128_loudness_shortterm(state, &shortTermLoudness);
      sprintf(graphiteOutputBuffer, "%s %f %d\n", graphiteMetric, shortTermLoudness, (int) time(NULL)); 
      send(graphiteSocket, graphiteOutputBuffer, (strlen(graphiteOutputBuffer)), 0);
    } 
  }
  free(state);
  return 0;
}
