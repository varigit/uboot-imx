/*
 * (c) Copyright 2015 - Variscite LTD
 *
 */

#include <common.h>
#include <command.h>

DECLARE_GLOBAL_DATA_PTR;

int do_codec_reset(cmd_tbl_t *cmd_tbl, int flag, int argc, char* argv[])
{
    extern void codec_reset(int rst);

    if (argc != 2 ) {
usage:
        cmd_usage(cmd_tbl);
        return 1;
    }

    if (!strcmp(argv[1], "on"))
        codec_reset(0);
    else if (!strcmp(argv[1], "reset"))
        codec_reset(1);
    else
        goto usage;

    return 0;
}

U_BOOT_CMD(codec_reset, 2, 0, do_codec_reset,
    "Audio codec reset command",
    "codec_reset <on|reset>\n"
    ""
);

int do_phy_ctl(cmd_tbl_t *cmd_tbl, int flag, int argc, char* argv[])
{
    extern void enet_board_reset(int rst);

    if (argc != 2 ) {
usage:
        cmd_usage(cmd_tbl);
        return 1;
    }

    if (!strcmp(argv[1], "on"))
        enet_board_reset(0);
    else if (!strcmp(argv[1], "reset"))
        enet_board_reset(1);
    else
        goto usage;

    return 0;
}

U_BOOT_CMD(phy_ctl, 2, 0, do_phy_ctl,
    "Ethernet PHY reset.",
    "phy_ctl <on|reset>\n"
    ""
);
