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
 * Credits go to MadCatX and slim.one for finding out correct formula
 * See http://www.lfsforum.net/showthread.php?p=1593389#post1593389
 * and http://www.lfsforum.net/showthread.php?p=1595604#post1595604
 */
int get_range_cmd2(cmdstruct *c, int range)
{
    //Prepare command A
    c->cmds[0][0] = 0xf8;
    c->cmds[0][1] = 0x00; 	//Set later
    c->cmds[0][2] = 0x00;
    c->cmds[0][3] = 0x00;
    c->cmds[0][4] = 0x00;
    c->cmds[0][5] = 0x00;
    c->cmds[0][6] = 0x00;
    c->cmds[0][7] = 0x00;

    if (range == 900) {
        //Finish command A
        c->cmds[0][1] = 0x03;

        //Set command B
        c->cmds[1][0] = 0x83;
        c->cmds[1][1] = 0x00;
        c->cmds[1][2] = 0x00;
        c->cmds[1][3] = 0x00;
        c->cmds[1][4] = 0x00;
        c->cmds[1][5] = 0x00;
        c->cmds[1][6] = 0x00;
        c->cmds[1][7] = 0x00;
    } else if (range < 900 && range > 200) {
        //Finish command A
        c->cmds[0][1] = 0x03;

        unsigned char byte3, byte6;
        byte3 = 127 - (unsigned char)((range - 5) / 7);

        //Set 6th byte
        int rem = (range - 5) % 7;
        if (rem == 6)
            byte6 = 0xe0;
        else
            byte6 = rem * 30 + 14;

        //Set command B
        c->cmds[1][0] = 0x81;
        c->cmds[1][1] = 0x0b;
        c->cmds[1][2] = byte3;
        c->cmds[1][3] = 255 - byte3;
        c->cmds[1][4] = 0xff;
        c->cmds[1][5] = byte6;
        c->cmds[1][6] = 0xff;
        c->cmds[1][7] = 0x00; 	//Official Logitech Windows driver does not send the 8th byte.
    } else if (range == 200) {
        //Finish command A
        c->cmds[0][1] = 0x02;

        //Set command B
        c->cmds[1][0] = 0x83;
        c->cmds[1][1] = 0x00;
        c->cmds[1][2] = 0x00;
        c->cmds[1][3] = 0x00;
        c->cmds[1][4] = 0x00;
        c->cmds[1][5] = 0x00;
        c->cmds[1][6] = 0x00;
        c->cmds[1][7] = 0x00;
    } else
      return -1;

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

