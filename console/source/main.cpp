#include <3ds.h>
#include <arpa/inet.h>
#include <malloc.h>

#include <cstdio>

constexpr u32 kSocBufSize = 0x100000;

int main(int argc, char** argv) {
  gfxInitDefault();
  consoleInit(GFX_TOP, NULL);

  u32* soc_buf = static_cast<u32*>(memalign(0x1000, kSocBufSize));
  if (soc_buf == nullptr) {
    gfxExit();
    return 0;
  }

  auto res = socInit(soc_buf, kSocBufSize);

  if (R_FAILED(res)) {
    printf("socInit failed: 0x%08lX\n", res);
  } else {
    in_addr ip{};
    ip.s_addr = static_cast<in_addr_t>(gethostid());
    printf("soc ok ip %s\n", inet_ntoa(ip));
  }

  printf("Hello, vibrew!\n");
  printf("Press START to exit.\n");

  while (aptMainLoop()) {
    hidScanInput();
    if (hidKeysDown() & KEY_START) break;

    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
  }

  if (R_SUCCEEDED(res)) {
    socExit();
  }
  free(soc_buf);
  gfxExit();
  return 0;
}
