#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} can_frame_t;

typedef enum {
    MCP_XTAL_8MHZ,
    MCP_XTAL_16MHZ
} mcp_xtal_t;

typedef enum {
    MCP_BITRATE_125K,
    MCP_BITRATE_250K,
    MCP_BITRATE_500K,
    MCP_BITRATE_1000K
} mcp_bitrate_t;

bool mcp2515_init(mcp_xtal_t xtal, mcp_bitrate_t br);
bool mcp2515_send(const can_frame_t *f);
bool mcp2515_recv(can_frame_t *f);
