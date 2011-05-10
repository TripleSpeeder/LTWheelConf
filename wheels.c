#include "wheels.h"

int get_nativemode_cmd_DFP(cmdstruct *c)
{
    c->cmds[0][0] = 0xf8;
    c->cmds[0][1] = 0x01;
    c->cmds[0][2] = 0x00;
    c->cmds[0][3] = 0x00;
    c->cmds[0][4] = 0x00;
    c->cmds[0][5] = 0x00;
    c->cmds[0][6] = 0x00;
    c->cmds[0][7] = 0x00;
    c->numCmds = 1;
    return 0;
}

int get_nativemode_cmd_G25(cmdstruct *c)
{
    c->cmds[0][0] = 0xf8;
    c->cmds[0][1] = 0x10;
    c->cmds[0][2] = 0x00;
    c->cmds[0][3] = 0x00;
    c->cmds[0][4] = 0x00;
    c->cmds[0][5] = 0x00;
    c->cmds[0][6] = 0x00;
    c->cmds[0][7] = 0x00;
    c->numCmds = 1;
    return 0;
}

int get_nativemode_cmd_G27(cmdstruct *c)
{
    c->cmds[0][0] = 0xf8;
    c->cmds[0][1] = 0x0a;
    c->cmds[0][2] = 0x00;
    c->cmds[0][3] = 0x00;
    c->cmds[0][4] = 0x00;
    c->cmds[0][5] = 0x00;
    c->cmds[0][6] = 0x00;
    c->cmds[0][7] = 0x00;

    c->cmds[1][0] = 0xf8;
    c->cmds[1][1] = 0x09;
    c->cmds[1][2] = 0x04;
    c->cmds[1][3] = 0x01;
    c->cmds[1][4] = 0x00;
    c->cmds[1][5] = 0x00;
    c->cmds[1][6] = 0x00;
    c->cmds[1][7] = 0x00;

    c->numCmds = 2;
    return 0;
}

/* used by G25 and G27 */
int get_range_cmd(cmdstruct *c, int range)
{
    c->cmds[0][0] = 0xf8;
    c->cmds[0][1] = 0x81;
    c->cmds[0][2] = range & 0x00ff;
    c->cmds[0][3] = (range & 0xff00)>>8;
    c->cmds[0][4] = 0x00;
    c->cmds[0][5] = 0x00;
    c->cmds[0][6] = 0x00;
    c->cmds[0][7] = 0x00;
    c->numCmds = 1;
    return 0;
}

/* used by DFP */
int get_range_cmd2(cmdstruct *c, int range)
{
    /* According to MadCatXs investigation this additional
     * cmd is needed to enable rotation range > 200 degrees.
     */
    c->cmds[0][0] = 0xf8;
    c->cmds[0][1] = 0x03;
    c->cmds[0][2] = 0x00;
    c->cmds[0][3] = 0x00;
    c->cmds[0][4] = 0x00;
    c->cmds[0][5] = 0x00;
    c->cmds[0][6] = 0x00;
    c->cmds[0][7] = 0x00;

    // FIXME - This is not working, need to find out the correct formula...
    c->cmds[1][0] = 0xf8;
    c->cmds[1][1] = 0x81;
    c->cmds[1][2] = range & 0x00ff;
    c->cmds[1][3] = (range & 0xff00)>>8;
    c->cmds[1][4] = 0x00;
    c->cmds[1][5] = 0x00;
    c->cmds[1][6] = 0x00;
    c->cmds[1][7] = 0x00;

    c->numCmds = 2;
    return 0;
}

/* used by all wheels */
int get_autocenter_cmd(cmdstruct *c, int centerforce, int rampspeed)
{
    c->cmds[0][0] = 0xfe;
    c->cmds[0][1] = 0x0d;
    c->cmds[0][2] = rampspeed & 0x0f;
    c->cmds[0][3] = rampspeed & 0x0f;
    c->cmds[0][4] = centerforce & 0xff;
    c->cmds[0][5] = 0x00;
    c->cmds[0][6] = 0x00;
    c->cmds[0][7] = 0x00;
    c->numCmds = 1;
    return 0;
}

