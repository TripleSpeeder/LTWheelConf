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

#define VENDOR 0x046d
#define DFPNORMAL 0xc294
#define DFPEXTENDED 0xc298
#define G25EXTENDED 0xc299

#define WAIT_TIMEOUT 5000

static unsigned char native_mode_dfp[] = { 0xf8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
static unsigned char native_mode_g25[] = { 0xf8, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 };
int verbose_flag = 0;

/*
 * Send custom command to USB device using interrup transfer
 */
int send_command(libusb_device_handle *handle, unsigned char command[7] ) {

    int stat;
    stat = libusb_detach_kernel_driver(handle, 0);
    if ((stat < 0 ) || verbose_flag) perror("Detach kernel driver");

    stat = libusb_claim_interface( handle, 0 );
    if ( (stat < 0) || verbose_flag) perror("Claiming USB interface");

    int transferred = 0;
    stat = libusb_interrupt_transfer( handle, 1, command, sizeof( command ), &transferred, WAIT_TIMEOUT );
    if ( (stat < 0) || verbose_flag) perror("Sending USB command");

    /* In case the command just sent caused the device to switch from restricted mode to native mode
     * the following two commands will fail due to invalid device handle (because the device changed
     * its pid on the USB bus). 
     * So it is not possible anymore to release the interface or re-attach kernel driver.
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
 *  - the axes for throttle and brake are always combined
 *  - the rotation range is limited to 540 degrees
 *  - the clutch pedal of G25/G27 does not work
 * 
 * In restricted mode the wheels register on USB with pid 0xc294.
 * In native mode they register on USB with pid 0xc298 (DFP) or 0xc299 (G25/G27)
 * 
 * This function takes care to switch the wheel to "native" mode with no restrictions.
 * 
 */
int set_native_mode() {
    // First check if there already is G25/G27 or DFP in native mode
    libusb_device_handle *handle = libusb_open_device_with_vid_pid(NULL, VENDOR, G25EXTENDED);
    if ( handle != NULL ) {
        printf( "Found a Logitech G25/G27 already set to native mode.\n" );
        return 0;
    }
    handle = libusb_open_device_with_vid_pid(NULL, VENDOR, DFPEXTENDED);
    if ( handle != NULL ) {
        printf ( "Found a Logitech Driving Force Pro already set to native mode.\n" );
        return 0;
    }

    // Now search for a wheel in restricted "DFP" mode
    handle = libusb_open_device_with_vid_pid(NULL, VENDOR, DFPNORMAL );
    if ( handle != NULL ) {

        /* Assume the device is a G25/G27 in restricted mode and send command to switch to 
         * extended mode.
         * G25/G27 will change it's ID, DFP will ignore the command
         */
        send_command(handle, native_mode_g25);
        
        // wait a second until wheel is reconfigured...
        sleep(1);

        // If above command was successfully we should now find a G25/G27 in extended mode
        handle = libusb_open_device_with_vid_pid(NULL, VENDOR, G25EXTENDED );
        if ( handle != NULL ) {
            printf ( "Logitech G25/G27 wheel is now set to native mode.\n" );
        } else { 
            /* Assume the device is a DFP in restricted mode and send command to switch to 
             * extended mode. 
             */
            handle = libusb_open_device_with_vid_pid(NULL, VENDOR, DFPNORMAL );
            send_command(handle, native_mode_dfp);

            // wait a second until wheel is reconfigured...
            sleep(1);
            
            // If above command was successfully we should now find a DFP in extended mode
            handle = libusb_open_device_with_vid_pid(NULL, VENDOR, DFPEXTENDED );
            if ( handle != NULL ) {
                printf ( "Logitech Driving Force Pro wheel is now set to native mode.\n" );
            } else {
                // this should not happen, just in case
                printf ( "Unable to set the wheel to native mode.\n" );
                return -1;
            }
        }
    } else {
        printf ( "Unable to find an uninitialised Logitech G25/G27/Driving Force Pro wheel.\n" );
        return -1;
    }

    return 0;
}

/*
 * Set maximum rotation range of wheel in degrees
 * G25/G27/DFP support up to 900 degrees.
 */
int set_range(unsigned short int range) {

    // probe for a G25 (already set to nativemode)
    libusb_device_handle *handle = libusb_open_device_with_vid_pid(NULL, VENDOR, G25EXTENDED );
    if ( handle != NULL ) {
        if (verbose_flag) printf ( "Setting range to %d.\n", range );
        unsigned char setrange[] = { 0xf8, 0x81, range & 0x00ff , (range & 0xff00) >> 8, 0x00, 0x00, 0x00 };
        send_command(handle, setrange );
    } else {
        // probe for a DFP (already set to nativemode)
        handle = libusb_open_device_with_vid_pid(NULL, VENDOR, DFPEXTENDED );
        if ( handle != NULL ) {
            printf ( "Setting range to %d.\n", range );
            unsigned char setrange[] = { 0xf8, 0x03, range & 0x00ff , (range & 0xff00)>>8, 0x00, 0x00, 0x00 };
            send_command(handle, setrange );
        } else {
            printf ( "No suitable wheel (DFP/G25/G27) found. Make sure your wheel is set to native mode.\n" );
            return -1;
        }
    }
    printf ("Wheel rotation range is now set to %d degrees.\n", range);
    return 0;
}

int set_autocenter(int centerforce, char *device_file_name, int wait_for_udev)
{
    if (verbose_flag) printf ( "Device %s: Setting autocenter force to %d.\n", device_file_name, centerforce );
    
    /* sleep 2 seconds to allow udev to set up device nodes due to kernel 
     * driver re-attaching while setting native mode or wheel range before
     */
    if (wait_for_udev) sleep(2);

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
        ie.value = 0xFFFFUL * centerforce/ 100;
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
    
    /* sleep 2 seconds to allow udev to set up device nodes due to kernel 
     * driver re-attaching while setting native mode or wheel range before
     */
    if (wait_for_udev) sleep(2);
    
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
    printf ( "%s","\nltwheelconf - Configure Logitech Driving Force Pro, G25, G27 wheels\n\
\n\
ltwheelconf Copyright (C) 2011 Michael Bauer\n\
This program comes with ABSOLUTELY NO WARRANTY.\n\
\n\
General Options: \n\
-v, --verbose               Verbose output\n\
-h, --help                  This help text\n\
\n\
Wheel configuration: \n\
-n, --nativemode            Set wheel to native mode (separate axes, full wheel range)\n\
-r, --range=degrees         Set wheel rotation range (up to 900 degrees).\n\
                            Note:\n\
                                -> Requires wheel to be in native (-n) mode!\n\
-a, --autocenter=value      Set autocenter force. Value should be between 0 and 100 (0 -> no autocenter, 100 -> max autocenter force). \n\
                            Note: \n\
                                -> Requires parameter '--device' to specify the input device\n\
                                -> Only works with kernel >= 2.6.39\n\
-g, --gain=value            Set forcefeedback gain. Value should be between 0 and 100 (0 -> no gain, 100 -> max gain). \n\
                            Note: \n\
                                -> Requires wheel to be in native (-n) mode!\n\
                                -> Requires parameter '--device' to specify the input device\n\
-d, --device=inputdevice    Specify inputdevice for force-feedback related configuration (e.g. autocenter)\n\
\n\
Note: You can freely combine all configuration options.\n\
\n\
Example:\n\
Enable separate axes, disable autocenter and set wheel rotation range of 540 degrees:\n\
$ ltwheelconf --nativemode --range 540 --autocenter 0 --device /dev/input/G25\n\
\n\
Contact: michael@m-bauer.org\n\
\n");
}

int main (int argc, char **argv)
{
    unsigned short int range = 0;
    unsigned short int centerforce = 0;
    unsigned short int gain = 0;
    int do_native = 0;
    int do_range = 0;
    int do_autocenter = 0;
    int do_gain = 0;
    char device_file_name[128];
    memset(device_file_name, 0, sizeof(device_file_name));
    verbose_flag = 0;

    static struct option long_options[] =
    {
        {"verbose",    no_argument,       0,             'v'},
        {"help",       no_argument,       0,             'h'},
        {"nativemode", no_argument,       0,             'n'},
        {"range",      required_argument, 0,             'r'},
        {"autocenter", required_argument, 0,             'a'},
        {"gain",       required_argument, 0,             'g'},
        {"device",     required_argument, 0,             'd'},
        {0,            0,                 0,              0 }
    };

    while (optind < argc) {
        int index = -1;
        int result = getopt_long (argc, argv, "vhnr:a:g:d:",
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
                        break;
                case 'g':
                        gain = atoi(optarg);
                        do_gain = 1;
                        break;
                case 'd':
                        strncpy(device_file_name, optarg, 128);
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
        if (do_native) {
            set_native_mode();
            wait_for_udev = 1;
        }
        
        if (do_range) {
            set_range(range);
            wait_for_udev = 1;
        }
        
        if (do_autocenter) {
            if (strlen(device_file_name)) {
                set_autocenter(centerforce, device_file_name, wait_for_udev);
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
        
        libusb_exit(NULL);
    } else {
        // display usage information if no arguments given
        help();
    }
    exit(0);
}
