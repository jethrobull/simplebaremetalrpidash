#include "mcp2515.h"
#include "spi.h"
#include "timer.h"
#include "uart.h"

// MCP2515 registers (subset)
#define MCP_CANCTRL   0x0F
#define MCP_CANSTAT   0x0E
#define MCP_CNF1      0x2A
#define MCP_CNF2      0x29
#define MCP_CNF3      0x28
#define MCP_TXB0CTRL  0x30
#define MCP_TXB0SIDH  0x31
#define MCP_TXB0SIDL  0x32
#define MCP_TXB0DLC   0x35
#define MCP_TXB0D0    0x36
#define MCP_RXB0CTRL  0x60
#define MCP_RXB0SIDH  0x61
#define MCP_RXB0SIDL  0x62
#define MCP_RXB0DLC   0x65
#define MCP_RXB0D0    0x66
#define MCP_CANINTE   0x2B
#define MCP_CANINTF   0x2C

// SPI commands
#define MCP_CMD_RESET      0xC0
#define MCP_CMD_READ       0x03
#define MCP_CMD_WRITE      0x02
#define MCP_CMD_BITMOD     0x05
#define MCP_CMD_READSTATUS 0xA0
#define MCP_CMD_RTS_TXB0   0x81

static void mcp_write_reg(uint8_t addr, uint8_t val) {
    spi_cs_low();
    spi_transfer(MCP_CMD_WRITE);
    spi_transfer(addr);
    spi_transfer(val);
    spi_cs_high();
}

static uint8_t mcp_read_reg(uint8_t addr) {
    spi_cs_low();
    spi_transfer(MCP_CMD_READ);
    spi_transfer(addr);
    uint8_t v = spi_transfer(0x00);
    spi_cs_high();
    return v;
}

static void mcp_bit_modify(uint8_t addr, uint8_t mask, uint8_t data) {
    spi_cs_low();
    spi_transfer(MCP_CMD_BITMOD);
    spi_transfer(addr);
    spi_transfer(mask);
    spi_transfer(data);
    spi_cs_high();
}

static void mcp_reset(void) {
    spi_cs_low();
    spi_transfer(MCP_CMD_RESET);
    spi_cs_high();
    timer_delay_us(1000);
}

static void mcp_set_bit_timing(mcp_xtal_t xtal, mcp_bitrate_t br) {
    uint8_t cnf1 = 0, cnf2 = 0, cnf3 = 0;

    switch (xtal) {
    case MCP_XTAL_16MHZ:
        switch (br) {
        case MCP_BITRATE_125K:
            // 16MHz, 125k: BRP=7, PropSeg=1, PS1=3, PS2=2
            cnf1 = 0x07;
            cnf2 = 0x93;
            cnf3 = 0x01;
            break;
        case MCP_BITRATE_250K:
            // 16MHz, 250k: BRP=3
            cnf1 = 0x03;
            cnf2 = 0x93;
            cnf3 = 0x01;
            break;
        case MCP_BITRATE_500K:
            // 16MHz, 500k: BRP=1
            cnf1 = 0x01;
            cnf2 = 0x93;
            cnf3 = 0x01;
            break;
        case MCP_BITRATE_1000K:
            // 16MHz, 1M: BRP=0
            cnf1 = 0x00;
            cnf2 = 0x92; // shorter TSEG1
            cnf3 = 0x00; // PS2=1
            break;
        }
        break;

    case MCP_XTAL_8MHZ:
        switch (br) {
        case MCP_BITRATE_125K:
            // 8MHz, 125k: BRP=3
            cnf1 = 0x03;
            cnf2 = 0x93;
            cnf3 = 0x01;
            break;
        case MCP_BITRATE_250K:
            // 8MHz, 250k: BRP=1
            cnf1 = 0x01;
            cnf2 = 0x93;
            cnf3 = 0x01;
            break;
        case MCP_BITRATE_500K:
            // 8MHz, 500k: BRP=0
            cnf1 = 0x00;
            cnf2 = 0x93;
            cnf3 = 0x01;
            break;
        case MCP_BITRATE_1000K:
            // 8MHz, 1M: BRP=0, fewer TQ
            cnf1 = 0x00;
            cnf2 = 0x82; // shorter PS1+PropSeg
            cnf3 = 0x00; // PS2=1
            break;
        }
        break;
    }

    mcp_write_reg(MCP_CNF1, cnf1);
    mcp_write_reg(MCP_CNF2, cnf2);
    mcp_write_reg(MCP_CNF3, cnf3);
}

bool mcp2515_init(mcp_xtal_t xtal, mcp_bitrate_t br) {
    spi_init();
    mcp_reset();

    // Config mode
    mcp_write_reg(MCP_CANCTRL, 0x80);
    timer_delay_us(1000);

    mcp_set_bit_timing(xtal, br);

    // RX0: receive all
    mcp_write_reg(MCP_RXB0CTRL, 0x60);
    // RX0 interrupt
    mcp_write_reg(MCP_CANINTE, 0x01);

    // Normal mode
    mcp_write_reg(MCP_CANCTRL, 0x00);
    timer_delay_us(1000);

    uint8_t stat = mcp_read_reg(MCP_CANSTAT);
    if ((stat & 0xE0) != 0x00) {
        uart_puts("MCP2515: failed to enter normal mode\n");
        return false;
    }

    uart_puts("MCP2515: init OK\n");
    return true;
}



bool mcp2515_send(const can_frame_t *f) {
    // Standard ID only for now
    uint16_t sid = (uint16_t)f->id & 0x7FF;

    spi_cs_low();
    spi_transfer(MCP_CMD_WRITE);
    spi_transfer(MCP_TXB0SIDH);
    spi_transfer((sid >> 3) & 0xFF);
    spi_transfer((sid & 0x07) << 5);
    spi_transfer(0x00); // EID8
    spi_transfer(0x00); // EID0
    spi_transfer(f->dlc & 0x0F);
    for (uint8_t i = 0; i < f->dlc; i++) {
        spi_transfer(f->data[i]);
    }
    spi_cs_high();

    // Request to send TXB0
    spi_cs_low();
    spi_transfer(MCP_CMD_RTS_TXB0);
    spi_cs_high();

    return true;
}

bool mcp2515_recv(can_frame_t *f) {
    // Check RX0IF
    uint8_t intf = mcp_read_reg(MCP_CANINTF);
    if (!(intf & 0x01))
        return false;

    spi_cs_low();
    spi_transfer(MCP_CMD_READ);
    spi_transfer(MCP_RXB0SIDH);
    uint8_t sidh = spi_transfer(0x00);
    uint8_t sidl = spi_transfer(0x00);
    spi_transfer(0x00); // EID8
    spi_transfer(0x00); // EID0
    uint8_t dlc = spi_transfer(0x00);
    dlc &= 0x0F;

    uint16_t sid = ((uint16_t)sidh << 3) | (sidl >> 5);
    f->id = sid;
    f->dlc = dlc;
    for (uint8_t i = 0; i < dlc; i++) {
        f->data[i] = spi_transfer(0x00);
    }
    spi_cs_high();

    // Clear RX0IF
    mcp_bit_modify(MCP_CANINTF, 0x01, 0x00);

    return true;
}
