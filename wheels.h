/*
 *    ltwheelconf - configure logitech racing wheels
 *
 *    Copyright (C) 2011  Michael Bauer <michael@m-bauer.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifndef wheels_h
#define wheels_h

#define VID_LOGITECH 0x046d

#define PID_DRIVINGFORCE 0xc294
#define PID_MOMOFORCE 0xc295
#define PID_DRIVINGFORCEPRO 0xc298
#define PID_G25 0xc299
#define PID_DRIVINGFORCEGT 0xc29A
#define PID_G27 0xc29B


enum PIDIndex {
    DRIVINGFORCE = 0,
    MOMOFORCE,
    DRIVINGFORCEPRO,
    G25,
    DRIVINGFORCEGT,
    G27,
    
    PIDs_END
};


typedef struct {
    // reserve space for max. 4 command strings, 8 chars each
    unsigned char cmds[4][8];
    // how many command strings are actually used
    unsigned int numCmds;
} cmdstruct;

typedef struct {
    unsigned int idx;
    char shortname[255];
    char name[255];
    unsigned int restricted_pid;
    unsigned int native_pid;
    cmdstruct cmd_native;
    unsigned char cmd_range_prefix[2]; // FIXME - This is just a hack...
    unsigned char cmd_autocenter_prefix[2]; // FIXME - This is just a hack...
}wheelstruct;

static const wheelstruct wheels[] = {
    { 
        DRIVINGFORCE,
        "DF",
        "Driving Force", 
        0xc294,
        0xc294,
        { { {0} },0 },
        {0},
        {0}
    },
    { 
        MOMOFORCE,
        "MF",
        "Momo Force", 
        0xc294,
        0xc295,
        { { {0} },0 },
        {0},
        {0}
    }, 
    { 
        DRIVINGFORCEPRO,
        "DFP",
        "Driving Force Pro",
        0xc294,
        0xc298,
        {
            { 
                { 0xf8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 }, // switch to native mode
                { 0xf8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 }, // enable rotation range > 200 degree
            },
            2
        },
        { 0x81, 0x0b },
        { 0xfe, 0x0b }
    },
    { 
        G25,
        "G25",
        "G25", 
        0xc294,
        0xc299,
        {
            { 
                { 0xf8, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 },
            },
            1
        },
        { 0xf8, 0x81 },
        { 0xfe, 0x0d }
    }, 
    { 
        DRIVINGFORCEGT,
        "DFGT",
        "Driving Force GT", 
        0xc294,
        0xc29A,
        { { {0} },0 },
        {0},
        {0}
    },
    { 
        G27, 
        "G27", 
        "G27", 
        0xc294,
        0xc29B,
        {
            { 
                { 0xf8, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
                { 0xf8, 0x09, 0x04, 0x01, 0x00, 0x00, 0x00 }
            },
            2
        },
        { 0xf8, 0x81 },
        { 0xfe, 0x0b }
    },
    
    
    { 
        PIDs_END,
        "",
        "",
        0,
        0,
        { { {0} },0 },
        {0},
        {0}
    }
};

#endif
