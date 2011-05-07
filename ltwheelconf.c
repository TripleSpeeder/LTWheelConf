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

#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include <linux/input.h>
#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "wheels.h"


#define VERSION "0.2.3"

#define TRANSFER_WAIT_TIMEOUT_MS 5000
#define CONFIGURE_WAIT_SEC 3
#define UDEV_WAIT_SEC 2

int verbose_flag = 0;

/*
 * Search and list all known/supported wheels
 */
void list_devices() {
    
    libusb_device_handle *handle = 0;
    libusb_device *dev = 0;
    struct libusb_device_descriptor desc;
    unsigned char descString[255];
    memset(&descString, 0, sizeof(descString));

    int numFound = 0;
    int PIDIndex = 0;
    wheelstruct w = wheels[PIDIndex];
    for (PIDIndex = 0; PIDIndex < PIDs_END; PIDIndex++) {
        w = wheels[PIDIndex];
        printf("Scanning for \"%s\": ", w.name);
        handle = libusb_open_device_with_vid_pid(NULL, VID_LOGITECH, w.native_pid);
        if (handle != 0) {
            dev = libusb_get_device(handle);
            if (dev != 0) {
                int ret = libusb_get_device_descriptor(dev, &desc);
                if (ret == 0) {
                    numFound++;
                    libusb_get_string_descriptor_ascii(handle, desc.iProduct, descString, 255);
                    printf("\t\tFound \"%s\", %04x:%04x (bus %d, device %d)", descString, desc.idVendor, desc.idProduct,
                           libusb_get_bus_number(dev), libusb_get_device_address(dev));
                    
                } else {
                    perror("Get device descriptor");
                }
            } else {
                perror ("Get device");
            }
        }
        printf("\n");
    }
    printf("Found %d devices.\n", numFound);
}

/*
 * Send custom command to USB device using interrupt transfer
 */
int send_command(libusb_device_handle *handle, cmdstruct command ) {
    
    if (command.numCmds == 0) {
        printf( "send_command: Empty command provided! Not sending anything...\n");
        return 0;
    }

    int stat;
    stat = libusb_detach_kernel_driver(handle, 0);
    if ((stat < 0 ) || verbose_flag) perror("Detach kernel driver");

    stat = libusb_claim_interface( handle, 0 );
    if ( (stat < 0) || verbose_flag) perror("Claiming USB interface");

    int transferred = 0;
    
    // send all command strings provided in command
    int cmdCount;
    for (cmdCount=0; cmdCount < command.numCmds; cmdCount++) {
        stat = libusb_interrupt_transfer( handle, 1, command.cmds[cmdCount], sizeof( command.cmds[cmdCount] ), &transferred, TRANSFER_WAIT_TIMEOUT_MS );
        if ( (stat < 0) || verbose_flag) perror("Sending USB command");
    }

    /* In case the command just sent caused the device to switch from restricted mode to native mode
     * the following two commands will fail due to invalid device handle (because the device changed
     * its pid on the USB bus). 
     * So it is not possible anymore to release the interface and re-attach kernel driver.
     * I am not sure if this produces a memory leak within libusb, but i do not think there is another 
     * solution possible...
     */
    stat = libusb_release_interface(handle, 0 );
    if (stat != LIBUSB_ERROR_NO_DEVICE) { // silently ignore "No such device" error due to reasons explained above.
        if ( (stat < 0) || verbose_flag) {
            perror("Releasing USB interface.");
        }
    }

    stat = libusb_attach_kernel_driver( handle, 0);
    if (stat != LIBUSB_ERROR_NO_DEVICE) { // silently ignore "No such device" error due to reasons explained above.
        if ( (stat < 0) || verbose_flag) {
            perror("Reattaching kernel driver");
        }
    }
    return 0;
}

/*
 * Logitech wheels are in a kind of restricted mode when initially connected via usb.
 * In this restricted mode 
 *  - axes for throttle and brake are always combined
 *  - rotation range is limited to 300 degrees
 *  - clutch pedal of G25/G27 does not work
 *  - H-gate shifter of G25/G27 does not work
 * 
 * In restricted mode the wheels register on USB with pid 0xc294.
 * In native mode they register on USB with pid 0xc298 (DFP) or 0xc299 (G25/G27)
 * 
 * This function takes care to switch the wheel to "native" mode with no restrictions.
 * 
 */
int set_native_mode(int wheelIndex) {
    
    wheelstruct w = wheels[wheelIndex];

    // first check if wheel has native mode at all
    if (w.native_pid == w.restricted_pid) {
        printf( "%s is always in native mode.\n", w.name);
        return 0;
    }
    
    // check if wheel is already in native mode
    libusb_device_handle *handle = libusb_open_device_with_vid_pid(NULL, VID_LOGITECH, w.native_pid);
    if ( handle != NULL ) {
        printf( "Found a %s already in native mode.\n", w.name);
        return 0;
    }
    
    // check if we know how to set native mode
    if (!w.cmd_native.numCmds) {
        printf( "Sorry, do not know how to set %s into native mode.\n", w.name);
        return -1;
    }
    
    // try to get handle to device in restricted mode
    handle = libusb_open_device_with_vid_pid(NULL, VID_LOGITECH, w.restricted_pid );
    if ( handle == NULL ) {
        printf( "Can not find %s in restricted mode (PID %x). This should not happen :-(\n", w.name, w.restricted_pid);
        return -1;
    }

    // finally send command to switch wheel to native mode
    send_command(handle, w.cmd_native);

    // wait until wheel reconfigures to new PID...
    sleep(CONFIGURE_WAIT_SEC);
    
    // If above command was successfully we should now find the wheel in extended mode
    handle = libusb_open_device_with_vid_pid(NULL, VID_LOGITECH, w.native_pid);
    if ( handle != NULL ) {
        printf ( "%s is now set to native mode.\n", w.name);
    } else {
        // this should not happen, just in case
        printf ( "Unable to set %s to native mode.\n", w.name );
        return -1;
    }

    return 0;
}
    

/*
 * Set maximum rotation range of wheel in degrees
 * G25/G27/DFP support up to 900 degrees.
 */
int set_range(int wheelIndex, unsigned short int range) {
    
    wheelstruct w = wheels[wheelIndex];
    
    libusb_device_handle *handle = libusb_open_device_with_vid_pid(NULL, VID_LOGITECH, w.native_pid );
    if ( handle == NULL ) {
        printf ( "%s not found. Make sure it is set to native mode (use --native).\n", w.name);
        return -1;
    }
    
    // Build up command to set range.
    
    // check if we know how to set range
    if (!w.cmd_range_prefix) {
        printf( "Sorry, do not know how to set rotation range for %s.\n", w.name);
        return -1;
    }
    
    // FIXME - This cmd_range_prefix stuff is really ugly for now...
    cmdstruct setrange = {
        { 
            { w.cmd_range_prefix[0], w.cmd_range_prefix[1], range & 0x00ff , (range & 0xff00)>>8, 0x00, 0x00, 0x00 }
        },
        1,
    };
    // send command to change range
    send_command(handle, setrange);
    printf ("Wheel rotation range of %s is now set to %d degrees.\n", w.name, range);
    return 0;
    
}


/*
 * Native method to set autcenter behaviour of LT wheels.
 * 
 * Based on a post by "anrp" on the vdrift forum:
 * http://vdrift.net/Forum/viewtopic.php?t=412&postdays=0&postorder=asc&start=60
 * 
 * fe0b0101ff - centering spring, slow spring ramp 
 * ____^^____ - left ramp speed 
 * ______^^__ - right ramp speed 
 * ________^^ - overall strength 
 * 
 * Rampspeed seems to be limited to 0-7 only.
 */
int set_autocenter(int wheelIndex, int centerforce, int rampspeed)
{
    if (verbose_flag) printf ( "Setting autocenter...");
    
    wheelstruct w = wheels[wheelIndex];
    
    libusb_device_handle *handle = libusb_open_device_with_vid_pid(NULL, VID_LOGITECH, w.native_pid );
    if ( handle == NULL ) {
        printf ( "%s not found. Make sure it is set to native mode (use --native).\n", w.name);
        return -1;
    }
    
    // Build up command to set range.
    
    // check if we know how to set native range
    if (!w.cmd_autocenter_prefix) {
        printf( "Sorry, do not know how to set autocenter force for %s. Please try generic implementation using --alt_autocenter.\n", w.name);
        return -1;
    }
    
    // FIXME - This cmd_autocenter_prefix stuff is really ugly for now...
    cmdstruct command = {
        { 
            { w.cmd_autocenter_prefix[0], w.cmd_autocenter_prefix[1], rampspeed & 0x0f , rampspeed & 0x0f, centerforce & 0xff, 0x00, 0x00, 0x00 }
        },
        1,
    };
    
    send_command(handle, command);
    
    printf ("Autocenter for %s is now set to %d with rampspeed %d.\n", w.name, centerforce, rampspeed);
    return 0;
}

/*
 * Generic method to set autocenter force of any wheel device recognized by kernel
 * This method does not allow to set the rampspeed and force individually.
 * 
 * Looking at hid-lgff.c i think the kernel driver actually might do the wrong thing by modifying 
 * the rampspeed instead of the general force...?
 */
int alt_set_autocenter(int centerforce, char *device_file_name, int wait_for_udev)
{
    if (verbose_flag) printf ( "Device %s: Setting autocenter force to %d.\n", device_file_name, centerforce );
    
    /* sleep UDEV_WAIT_SEC seconds to allow udev to set up device nodes due to kernel 
     * driver re-attaching while setting native mode or wheel range before
     */
    if (wait_for_udev) sleep(UDEV_WAIT_SEC);

    /* Open device */
    int fd = open(device_file_name, O_RDWR);
    if (fd == -1) {
        perror("Open device file");
        return -1;
    }

    if (centerforce >= 0 && centerforce <= 100) {
        struct input_event ie;
        ie.type = EV_FF;
        ie.code = FF_AUTOCENTER;
        ie.value = 0xFFFFUL * centerforce/100;
        if (write(fd, &ie, sizeof(ie)) == -1) {
            perror("set auto-center");
            return -1;
        }
    }
    printf ("Wheel autocenter force is now set to %d.\n", centerforce);
    return 0;
}

int set_gain(int gain, char *device_file_name, int wait_for_udev)
{
    if (verbose_flag) printf ( "Device %s: Setting FF gain to %d.\n", device_file_name, gain);
    
    /* sleep UDEV_WAIT_SEC seconds to allow udev to set up device nodes due to kernel 
     * driver re-attaching while setting native mode or wheel range before
     */
    if (wait_for_udev) sleep(UDEV_WAIT_SEC);
    
    /* Open device */
    int fd = open(device_file_name, O_RDWR);
    if (fd == -1) {
        perror("Open device file");
        return -1;
    }
    
    if (gain >= 0 && gain <= 100) {
        struct input_event ie;
        ie.type = EV_FF;
        ie.code = FF_GAIN;
        ie.value = 0xFFFFUL * gain / 100;
        if (write(fd, &ie, sizeof(ie)) == -1) {
            perror("set gain");
            return -1;
        }
    }
    printf ("Wheel forcefeedback gain is now set to %d.\n", gain);
    return 0;
}


void help() {
    printf ( "%s","\nltwheelconf " VERSION " - Configure Logitech Driving Force Pro, G25, G27 wheels\n\
\n\
ltwheelconf Copyright (C) 2011 Michael Bauer\n\
This program comes with ABSOLUTELY NO WARRANTY.\n\
\n\
General Options: \n\
-h, --help                  This help text\n\
-v, --verbose               Verbose output\n\
-l, --list                  List all found/supported devices\n\
\n\
Wheel configuration: \n\
-w, --wheel=shortname       Which wheel is connected. Supported values:\n\
                                -> 'DF'   (Driving Force)\n\
                                -> 'MF'   (Momo Force)\n\
                                -> 'DFP'  (Driving Force Pro)\n\
                                -> 'DFGT' (Driving Force GT)\n\
                                -> 'G25'  (G25)\n\
                                -> 'G27'  (G27)\n\
-n, --nativemode            Set wheel to native mode (separate axes, full wheel range, clutch pedal, H-shifter)\n\
-r, --range=degrees         Set wheel rotation range (up to 900 degrees).\n\
                            Note:\n\
                                -> Requires wheel to be in native (-n) mode!\n\
-a, --autocenter=value      Set autocenter bypassing hid driver. Value should be between 0 and 255 (0 -> no autocenter, 255 -> max autocenter force). \n\
                            Together with the rampspeed setting this allows much finer control of the autocenter behaviour as using the generic input interface.\n\
                            Note: \n\
                                -> Requires parameter '--rampspeed'\n\
-s, --rampspeed             Use in conjuntion with the --autocenter parameter. Specify how fast the autocenter force should increase when rotating wheel.\n\
                            Value should be between 0 and 7\n\
                            Low value means the centering force is increasing only slightly when turning the wheel.\n\
                            High value means the centering force is increasing very fast when turning the wheel.\n\
-b, --altautocenter=value   Set autocenter force using generic input interface. Value should be between 0 and 100 (0 -> no autocenter, 100 -> max autocenter force). \n\
                            Use this if --autocenter does not work for your device.\n\
                            Note: \n\
                                -> Requires parameter '--device' to specify the input device\n\
                                -> Only works with kernel >= 2.6.39\n\
                                -> The rampspeed can not be influenced using this method\n\
-g, --gain=value            Set forcefeedback gain. Value should be between 0 and 100 (0 -> no gain, 100 -> max gain). \n\
                            Note: \n\
                                -> Requires parameter '--device' to specify the input device\n\
-d, --device=inputdevice    Specify inputdevice for force-feedback related configuration (--gain and --altautocenter)\n\
\n\
Note: You can freely combine all configuration options.\n\
\n\
Examples:\n\
Put wheel into native mode:\n\
$ sudo ltwheelconf --wheel G25 --nativemode\n\
Set wheel rotation range to 900 degree:\n\
$ sudo ltwheelconf --wheel G27 --range 900 \n\
Set moderate autocenter:\n\
$ sudo ltwheelconf --wheel DFP --autocenter 100 --rampspeed 1\n\
Disable autocenter completely:\n\
$ sudo ltwheelconf --wheel G25 --autocenter 0 --rampspeed 0\n\
Set native mode, disable autocenter and set wheel rotation range of 540 degrees in one call:\n\
$ sudo ltwheelconf --wheel G25 --nativemode --range 540 --autocenter 0 --rampspeed 0\n\
\n\
Contact: michael@m-bauer.org\n\
\n");
}

int main (int argc, char **argv)
{
    unsigned short int range = 0;
    unsigned short int centerforce = 0;
    unsigned short int gain = 0;
    int do_validate_wheel = 0;
    int do_native = 0;
    int do_range = 0;
    int do_autocenter = 0;
    int do_alt_autocenter = 0;
    int do_gain = 0;
    int do_list = 0;
    int rampspeed = -1;
    char device_file_name[128];
    char shortname[255];
    memset(device_file_name, 0, sizeof(device_file_name));
    verbose_flag = 0;

    static struct option long_options[] =
    {
        {"verbose",    no_argument,       0,             'v'},
        {"help",       no_argument,       0,             'h'},
        {"list",       no_argument,       0,             'l'},
        {"wheel",      required_argument, 0,             'w'},
        {"nativemode", no_argument,       0,             'n'},
        {"range",      required_argument, 0,             'r'},
        {"altautocenter", required_argument, 0,          'b'},
        {"autocenter", required_argument, 0,             'a'},
        {"rampspeed",  required_argument, 0,             's'},
        {"gain",       required_argument, 0,             'g'},
        {"device",     required_argument, 0,             'd'},
        {0,            0,                 0,              0 }
    };

    while (optind < argc) {
        int index = -1;
        int result = getopt_long (argc, argv, "vhlw:nr:a:g:d:s:b:",
                                  long_options, &index);
        
        if (result == -1)
            break; /* end of list */

        switch (result) {
                case 'v':
                    verbose_flag = 1;
                    break;
                case 'n':
                    do_native = 1;
                    break;
                case 'r':
                    range = atoi(optarg);
                    do_range = 1;
                    break;
                case 'a':
                    centerforce = atoi(optarg);
                    do_autocenter = 1;
                    do_alt_autocenter = 0;
                    break;
                case 'b':
                    centerforce = atoi(optarg);
                    do_autocenter = 0;
                    do_alt_autocenter = 1;
                    break;
                case 's':
                    rampspeed = atoi(optarg);
                    break;
                case 'g':
                    gain = atoi(optarg);
                    do_gain = 1;
                    break;
                case 'd':
                    strncpy(device_file_name, optarg, 128);
                    break;
                case 'l':
                    do_list = 1;
                    break;
                case 'w':
                    strncpy(shortname, optarg, 255);
                    do_validate_wheel = 1;
                    break;
                case '?':
                default:
                    help();
                    break;
        }

    }
    
    if (argc > 1)
    {
        libusb_init(NULL);
        int wait_for_udev = 0;
        int wheelIndex = -1;

        if (do_list) {
            // list all devices, ignore other options...
            list_devices();
        } else {
            if (do_validate_wheel) {
                int i = 0;
                for (i=0; i < PIDs_END; i++) {
                    if (strncmp(wheels[i].shortname, shortname, 255) == 0) {
                        // found matching wheel
                        wheelIndex = i;
                        break;
                    }
                }
                if (wheelIndex == -1) {
                    printf("Wheel \"%s\" not supported. Did you spell the shortname correctly?\n", shortname);
                }
            }
            
            if (do_native) {
                if (wheelIndex == -1) {
                    printf("Please provide --wheel parameter!\n");
                } else {
                    set_native_mode(wheelIndex);
                    wait_for_udev = 1;
                }
            }
            
            if (do_range) {
                if (wheelIndex == -1) {
                    printf("Please provide --wheel parameter!\n");
                } else {
                    set_range(wheelIndex, range);
                    wait_for_udev = 1;
                }
            }
            
            if (do_autocenter) {
                if (wheelIndex == -1) {
                    printf("Please provide --wheel parameter!\n");
                } else {
                    if (rampspeed == -1) {
                        printf("Please provide '--rampspeed' parameter\n");
                    } else {
                        set_autocenter(wheelIndex, centerforce, rampspeed);
                        wait_for_udev = 1;
                    }
                } 
            }
            
            if (do_alt_autocenter) {
                if (strlen(device_file_name)) {
                    alt_set_autocenter(centerforce, device_file_name, wait_for_udev);
                    wait_for_udev = 0;
                } else {
                    printf("Please provide the according event interface for your wheel using '--device' parameter (E.g. '--device /dev/input/event0')\n");
                } 
            }
            
            if (do_gain) {
                if (strlen(device_file_name)) {
                    set_gain(gain, device_file_name, wait_for_udev);
                    wait_for_udev = 0;
                } else {
                    printf("Please provide the according event interface for your wheel using '--device' parameter (E.g. '--device /dev/input/event0')\n");
                }    
            }
        }        
        libusb_exit(NULL);
    } else {
        // display usage information if no arguments given
        help();
    }
    exit(0);
}
