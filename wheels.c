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

/* used by DFP
 * 
 * Credits go to MadCatX for finding out correct formula
 * See http://www.lfsforum.net/showthread.php?p=1593389#post1593389
 * 
 */
int get_range_cmd2(cmdstruct *c, int range)
{
    //Table of valid values for 6th byte
    char valid_byte6[] = {
        0xe0,
        0xa4,
        0x86,
        0x68,
        0x4a,
        0x2c,
        0x0e,
    };

    //Which segment to use
    int segment = (900 - range) / 7;   //Yes, integer division is what we need

    //Values for 3rd and 4th byte
    char byte4 = 255 - segment;
    char byte3 = 255 - byte4;

    //Set value of 6th byte
    int byte6_idx = 899 - (segment * 7) - range;

    c->cmds[0][0] = 0xf8;
    if (range > 200)
        c->cmds[0][1] = 0x03;
    else 
        c->cmds[0][1] = 0x02;
    c->cmds[0][2] = 0x00;
    c->cmds[0][3] = 0x00;
    c->cmds[0][4] = 0x00;
    c->cmds[0][5] = 0x00;
    c->cmds[0][6] = 0x00;
    c->cmds[0][7] = 0x00;

    c->cmds[1][0] = 0x81;
    c->cmds[1][1] = 0x0b;
    c->cmds[1][2] = byte3;
    c->cmds[1][3] = byte4;
    c->cmds[1][4] = 0xff;
    c->cmds[1][5] = valid_byte6[byte6_idx];
    c->cmds[1][6] = 0xff;
    c->cmds[1][7] = 0x00;   //Logitech driver does not send the 8th byte.

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

