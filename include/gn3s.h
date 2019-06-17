/*----------------------------------------------------------------------------------------------*/
/*! \file gn3s.h
//
// FILENAME: gn3s.h
//
// DESCRIPTION: Defines the GN3S class.
//
// DEVELOPERS: Gregory W. Heckler (2003-2009)
//
// LICENSE TERMS: Copyright (c) Gregory W. Heckler 2009
//
// This file is part of the GPS Software Defined Radio (GPS-SDR)
//
// The GPS-SDR is free software; you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version. The GPS-SDR is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// Note:  Comments within this file follow a syntax that is compatible with
//        DOXYGEN and are utilized for automated document extraction
//
// Reference:
*/
/*----------------------------------------------------------------------------------------------*/


#ifndef GN3S_H_
#define GN3S_H_


/* Includes */
/*--------------------------------------------------------------*/
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <libusb.h>
/*--------------------------------------------------------------*/


/* FX2 Configuration Structure */
/*--------------------------------------------------------------*/
struct fx2Config
{
	int interface;
	int altinterface;
    libusb_device_handle *udev;
};
/*--------------------------------------------------------------*/


/* FX2 Stuff */
/*--------------------------------------------------------------*/
#define RX_ENDPOINT		(0x86)
#define VRT_VENDOR_IN	(0xC0)
#define VRT_VENDOR_OUT	(0x40)
#define RX_INTERFACE	(2)
#define RX_ALTINTERFACE (0)
#define VRQ_GET_STATUS	(0x80)
#define GS_RX_OVERRUN	(1)  //!< Returns 1 byte
#define VRQ_XFER		(0x01)
/*--------------------------------------------------------------*/


/* GN3S Stuff */
/*--------------------------------------------------------------*/
#define GN3S_VID 	 		(0x16C0)
#define GN3S_PID 	 		(0x072F)
#define VID_OLD  	 		(0x1781)
#define PID_OLD  	 		(0x0B39)
#define PROG_SET_CMD 		(0xE600)
#define USB_BUFFER_SIZE     (16384)           //!< 8 MB
#define USB_BLOCK_SIZE      (512)             //!< 16KB is hard limit
#define USB_NBLOCKS         (USB_BUFFER_SIZE / USB_BLOCK_SIZE)
#define USB_NTRANSFERS      (16)
#define USB_TIMEOUT         (1000)
/*--------------------------------------------------------------*/


/* The firmware is embedded into the executable */
/*--------------------------------------------------------------*/
extern char _binary_usrp_gn3s_firmware_ihx_start[];
/*--------------------------------------------------------------*/


/*--------------------------------------------------------------*/
/*! \ingroup CLASSES
 *
 */
class gn3s
{

	private:

		/* First or second board */
		int which;

		/* GN3S FX2 Stuff */
		struct fx2Config fx2_config;
        struct libusb_device *fx2_device;
        struct libusb_device_handle *fx2_handle;
        struct libusb_transfer *transfer[USB_NTRANSFERS];

		/* USB IDs */
		unsigned int gn3s_vid, gn3s_pid;

		/* Pull in the binary firmware */
		int fstart;
		int fsize;
		//char *gn3s_firmware;

	public:

		gn3s(int _which);		//!< Constructor
		~gn3s();				//!< Destructor

		/* FX2 functions */
        struct libusb_device* usb_fx2_find(unsigned int vid, unsigned int pid, char info, int ignore);
        bool usb_fx2_configure(struct libusb_device *fx2, fx2Config *fx2c);
        bool usb_fx2_start_transfers();
        bool usb_fx2_cancel_transfers();
        int read(unsigned char *buff, int bytes);
		int write_cmd(int request, int value, int index, unsigned char *bytes, int len);
		bool _get_status(int which, bool *trouble);
		bool check_rx_overrun();
		bool usrp_xfer(char VRQ_TYPE, bool start);

		/* Used to flash the GN3S */
		int atoz(char *s);
        void upload_ram(unsigned char *buf, int start, int len);
		void program_fx2(char *filename, char mem);
		int prog_gn3s_board();

};
/*--------------------------------------------------------------*/


#endif /*GN3S_H_ */
