#include <LibPrintf.h>
#define TLED_CHIP LED_WS2811
#define TLED_ORDER ORDER_GRB
#include "tinyLED.h"
#include "FastLED.h"

#define PIN 13
#define NUM_LEDS 181

#define STATE_WAIT 0
#define STATE_CMD 1
#define STATE_RESPONSE 2

#define DEBUG false
#define XOR_CHECK true

// `r` in byte
#define CMD_RESET 114
// `s` in byte
#define CMD_SET 115
// `b` in byte
#define CMD_BRIGHTNESS 98
// `a` in byte
#define CMD_APPLY 97
// `t` in byte
#define CMD_TEST 116

tinyLED<13> strip;

void setup() {
  FastLED.addLeds<WS2811, PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );/* */
  pinMode(PIN, OUTPUT);

  Serial.begin(115200);
  delay(100);

  printf("r:%d\n", NUM_LEDS);
}

int packetPointer = 0;
int packetLen = 0;
int xorCheck = 0;
char packet[65] = {0};

int readDataToPacket() {
  int c = Serial.read();
  if (c == -1) {
    return -1;
  } else if (c == 0 || c == '\n') {
    packetLen = packetPointer - 1;

    // last is always xor, ignore it
    packet[packetPointer - 1] = 0;
    packetPointer = 0;

    return 0;
  } else {
    packet[packetPointer++] = c;
    xorCheck ^= c;
  }

  return 1;
}

void resetPacket() {
  packet[0] = 0;
  packetLen = 0;
  packetPointer = 0;
  xorCheck = 0;
}

void loop() {
  int res = readDataToPacket();
  while (res == 1) {
    res = readDataToPacket();
  }

  if (res == -1) {
    return;
  }

  if (DEBUG) {
    printf("l:Packet stat:%d:%d:%s\n", packetLen, xorCheck, packet);    
  }

  if (packetLen < 3 || packetLen > 128) {
    printf("e:Broken data:length\n");
    resetPacket();

    return;
  }

  if (XOR_CHECK && xorCheck != 0) {
    printf("e:Xor check failed\n");
    resetPacket();

    return;
  }


  char cmd = packet[packetPointer++];
  char subCmd = packet[packetPointer++];
  if (packet[packetPointer++] != ':') {
    printf("e:Broken data:first_sep\n");
    resetPacket();

    return;
  }

  if (DEBUG) {
    printf("l:Got cmd:%d:%d\n", cmd, subCmd);
  }

  if (cmd == CMD_RESET) {
    FastLED.clear();
    FastLED.show();
  } else if (cmd == CMD_SET) {
    int offset = readIntToDelimiterFromPacket();
    char c = packet[packetPointer++];

    while (c >= '0' && c <= '9') {
      if (DEBUG) {
        printf("l:CMD_SET:continue:%d:%l\n", offset, c);
      }

      leds[offset++] = data2color(c);

      c = packet[packetPointer++];
    }
  } else if (cmd == CMD_BRIGHTNESS) {
    int b = readIntToDelimiterFromPacket();
    if (b == -1) {
      printf("e:Broken data:brightness\n");
    } else if (b > 255) {
      printf("e:Broken data:brightness_threshold\n");
    } else {
      FastLED.setBrightness(b);
    }
  } else if (cmd == CMD_APPLY) {
    FastLED.show();
  } else {
    printf("e:Unknown command:%d:%d\n", cmd, subCmd);
  }

  resetPacket();
}

long readIntToDelimiterFromPacket() {
  long res = 0;
  int readCount = 0;

  char c;
  for (; readCount < 9; readCount++) {
    c = packet[packetPointer++];
    if (c < '0' || c > '9') {
      break;
    }

    res *= 10;
    res += c - '0';
  }

  if ((c == ':' || c == 0) && readCount > 0) {
    return res;
  }

  return -1;
}

long data2color(char c) {
  switch (c) {
    case '1': return 0xff0000;
    case '2': return 0x00ff00;
    case '3': return 0x0000ff;
    case '4': return 0xffffff;
    case '5': return 0xffff00;
    case '6': return 0xff00ff;
    case '0':
    default: return 0;
  }
}
