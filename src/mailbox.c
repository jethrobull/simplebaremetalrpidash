#include "peripherals.h"
#include "mailbox.h"

#define MBOX_READ    (MBOX_BASE + 0x00)
#define MBOX_STATUS  (MBOX_BASE + 0x18)
#define MBOX_WRITE   (MBOX_BASE + 0x20)

#define MBOX_EMPTY   (1u << 30)
#define MBOX_FULL    (1u << 31)

bool mbox_call(uint8_t ch, volatile uint32_t *mbox) {
    uint32_t addr = (uint32_t)((uintptr_t)mbox);

    if (addr & 0xF)
        return false; // must be 16-byte aligned

    uint32_t msg = addr | (ch & 0xF);

    while (mmio_read(MBOX_STATUS) & MBOX_FULL) { }

    mmio_write(MBOX_WRITE, msg);

    for (;;) {
        while (mmio_read(MBOX_STATUS) & MBOX_EMPTY) { }

        uint32_t resp = mmio_read(MBOX_READ);

        if ((resp & 0xF) == (ch & 0xF) &&
            (resp & ~0xF) == addr) {

            return mbox[1] == 0x80000000;
        }
    }
}
