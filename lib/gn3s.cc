/*----------------------------------------------------------------------------------------------*/
/*! \file gn3s.cpp
//
// FILENAME: gn3s.cpp
//
// DESCRIPTION: Impelements the GN3S class.
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

#include "gn3s.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <libusb.h>

static char debug = 1; //!< 1 = Verbose

static libusb_context *ctx = nullptr;
static unsigned char buffer[USB_NTRANSFERS][USB_BUFFER_SIZE];
static int bcount;
static int bufptr;

/*----------------------------------------------------------------------------------------------*/
/*!
 * All libusb callback functions should be marked with the LIBUSB_CALL macro
 * to ensure that they are compiled with the same calling convention as libusb.
 */

//libusb_transfer_cb_fn
static void LIBUSB_CALL callback(libusb_transfer *transfer)
{
    bcount += transfer->actual_length;
    if (bcount >= sizeof(buffer))
        bcount -= sizeof(buffer);
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        libusb_submit_transfer(transfer);
    }
    else
    {
        libusb_free_transfer(transfer);
    }
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
gn3s::gn3s(int _which)
{

        //int fsize;
		bool ret;
        int r;
		which = _which;

        fx2_device 	= nullptr;
        fx2_handle 	= nullptr;
		gn3s_vid 	= GN3S_VID;
		gn3s_pid 	= GN3S_PID;

        r = libusb_init(&ctx);
        if (r < 0)
        {
            printf("Libusb init error: %s\n", libusb_error_name(ret));
            throw (1);
        }

#if LIBUSB_API_VERSION >= 0x01000106
        libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
#else
        libusb_set_debug(ctx, 3);
#endif

        /* Get the firmware embedded in the executable */
		//fstart = (int) &_binary_usrp_gn3s_firmware_ihx_start;
		//fsize = strlen(_binary_usrp_gn3s_firmware_ihx_start);
		//gn3s_firmware = new char[fsize + 10];
		//memcpy(&gn3s_firmware[0], (void *)fstart, fsize);

		// Load the firmware from external file (Javier)


		//gn3s_firmware[fsize] = NULL;

		/* Search all USB busses for the device specified by VID/PID */
        fx2_device = usb_fx2_find(gn3s_vid, gn3s_pid, debug, 0);
        if (!fx2_device)
		{
			/* Program the board */
            ret = prog_gn3s_board();
            if(ret)
			{
				fprintf(stdout, "Could not flash GN3S device\n");
				throw(1);
			}

			/* Need to wait to catch change */
			sleep(2);

			/* Search all USB busses for the device specified by VID/PID */
			fx2_device = usb_fx2_find(gn3s_vid, gn3s_pid, debug, 0);
		}
		else
		{
            fprintf(stdout, "Found GN3S Device\n");
		}

		/* Open and configure FX2 device if found... */
		ret = usb_fx2_configure(fx2_device, &fx2_config);
		if(ret)
		{
			fprintf(stdout, "Could not obtain a handle to the GN3S device\n");
			throw(1);
        }

        ret = usb_fx2_start_transfers();
        if(!ret)
        {
            printf("Could not start USB transfers\n");
            throw(1);
        }
//TEST
        printf("Transfers started\n");
//TEST
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
gn3s::~gn3s()
{

	usrp_xfer(VRQ_XFER, 0);

    usb_fx2_cancel_transfers();
    usleep(1000);

    libusb_release_interface(fx2_handle, RX_INTERFACE);
    libusb_close(fx2_handle);
    libusb_exit(ctx);

}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
int gn3s::prog_gn3s_board()
{

    unsigned char a;
    unsigned int vid, pid;

	vid = (VID_OLD);
	pid = (PID_OLD);

    fx2_device = usb_fx2_find(vid, pid, debug, 0);

    if(fx2_device == nullptr)
	{
		fprintf(stderr,"Cannot find vid 0x%x pid 0x%x \n", vid, pid);
		return -1;
	}

//	printf("Using device vendor id 0x%04x product id 0x%04x\n",
//			fx2_device->descriptor.idVendor, fx2_device->descriptor.idProduct);

    int ret = libusb_open(fx2_device, &fx2_handle);

	/* Do the first set 0xE600 1 */
	char c[] = "1";
	char d[] = "0";

	a = atoz(c);

	fprintf(stdout,"GN3S flashing ... \n");

	upload_ram(&a, (PROG_SET_CMD),1);

    program_fx2(nullptr, 1);

	a = atoz(d);

	upload_ram(&a, (PROG_SET_CMD),1);

	fprintf(stdout,"GN3S flash complete! \n");

    libusb_close(fx2_handle);

	return(0);
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
int gn3s::atoz(char *s)
{
    int a;
    if(!strncasecmp("0x", s, 2)){
        sscanf(s, "%x",&a);
        return a;
    }
    return atoi(s);
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
void gn3s::upload_ram(unsigned char *buf, int start, int len)
{
	int i;
    int tlen;
	int quanta = 16;
	int a;

	for (i = start; i < start + len; i += quanta) {
		tlen = len + start - i;

		if (tlen > quanta)
			tlen = quanta;

		if (debug >= 3)
			printf("i = %d, tlen = %d \n", i, tlen);
        a = libusb_control_transfer(fx2_handle, 0x40, 0xa0, i, 0,
				buf + (i - start), tlen, 1000);

		if (a < 0) {
			fprintf(stderr, "Request to upload ram contents failed: %s\n",
                    libusb_error_name(a));
			return;
		}
	}
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
void gn3s::program_fx2(char *filename, char mem)
{
	FILE *f;
	char s[1024];
    unsigned char data[256];
	char checksum, a;
	int length, addr, type, i;
	unsigned int b;

	// *** mod javier: load firmware from external file ***

	//f = tmpfile();

	/* Dump firmware into temp file */
	//fputs(gn3s_firmware, f);
	//rewind(f);

	  f = fopen ("gn3s_firmware.ihx","r");
      if (f!=nullptr)
	  {
		printf("GN3S firmware file found!\n");
	  }else{
		  printf("Could not open GN3S firmware file!\n");
		  return;
	  }

	while (!feof(f)) {
		fgets(s, 1024, f); /* we should not use more than 263 bytes normally */

		if (s[0] != ':') {
			fprintf(stderr, "%s: invalid string: \"%s\"\n", filename, s);
			continue;
		}

		sscanf(s + 1, "%02x", &length);
		sscanf(s + 3, "%04x", &addr);
		sscanf(s + 7, "%02x", &type);

		if (type == 0) {
			// printf("Programming %3d byte%s starting at 0x%04x",
			//     length, length==1?" ":"s", addr);
			a = length + (addr & 0xff) + (addr >> 8) + type;

			for (i = 0; i < length; i++) {
				sscanf(s + 9 + i * 2, "%02x", &b);
				data[i] = b;
				a = a + data[i];
            }

			sscanf(s + 9 + length * 2, "%02x", &b);
            checksum = b;

			if (((a + checksum) & 0xff) != 0x00) {
				printf("  ** Checksum failed: got 0x%02x versus 0x%02x\n", (-a)
						& 0xff, checksum);
				continue;
			} else {
				//printf(", checksum ok\n");
			}

			upload_ram(data, addr, length);

		} else {
			if (type == 0x01) {
				printf("End of file\n");
				fclose(f);

				return;
			} else {
				if (type == 0x02) {
					printf("Extended address: whatever I do with it ?\n");
                    continue;
				}
			}
		}
	}

	fclose(f);
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
struct libusb_device* gn3s::usb_fx2_find(unsigned int vid, unsigned int pid, char info, int ignore)
{
    libusb_device **devs;
    struct libusb_device *fx2 = nullptr;
    long count = 0;
    int ret;

    count = libusb_get_device_list(ctx, &devs);
    if (count < 0)
    {
        printf("Unable to list devices\n");;
    }
    else
    {
        for (int idx=0; idx < count; ++idx)
        {
            libusb_device *dev = devs[idx];
            libusb_device_descriptor desc = {0};

            ret = libusb_get_device_descriptor (dev, &desc);
            if ((desc.idVendor == vid) && (desc.idProduct == pid))
                 fx2 = dev;
        }
    }

    libusb_free_device_list(devs, 1);

	return fx2;
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
bool gn3s::usb_fx2_configure(struct libusb_device *fx2, fx2Config *fx2c)
{

  char status = 0;
  int ret;

  ret = libusb_open(fx2_device, &fx2_handle);

  if(ret != 0)
  {
      printf("Could not obtain a handle to GNSS Front-End device \n");
      return -1;
  }
  else
  {
	  if(debug)
		  printf("Received handle for GNSS Front-End device \n");

      ret = libusb_set_configuration (fx2_handle, 1);
      if(ret != 0)
      {
          printf("FX2 configure error: %s\n", libusb_error_name(ret));
          libusb_close (fx2_handle);
          status = -1;
      }

      ret = libusb_claim_interface (fx2_handle, RX_INTERFACE);
      if (ret < 0)
      {
          printf("Interface claim error: %s\n", libusb_error_name(ret));
          printf ("\nDevice not programmed? \n");
          libusb_close (fx2_handle);
          status = -1;
      }
      else
          printf("Claimed interface\n");

      ret = libusb_set_interface_alt_setting(fx2_handle, RX_INTERFACE, RX_ALTINTERFACE);
      if (ret !=0)
      {
          printf ("Failed to start alternate setting: %s\n", libusb_error_name(ret));
          libusb_release_interface (fx2_handle, RX_INTERFACE);
          libusb_close (fx2_handle);
          status = -1;
      }

      return status;
  }
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
bool gn3s::usb_fx2_start_transfers()
{
    int ret;
    bool success = true;
    bufptr = 0;
    bcount = 0;

    for (int i = 0; i < USB_NTRANSFERS; i++)
    {
        transfer[i] = libusb_alloc_transfer(0);
        libusb_fill_bulk_transfer(transfer[i], fx2_handle, RX_ENDPOINT, buffer[i],
                USB_BUFFER_SIZE, libusb_transfer_cb_fn(&callback), nullptr, 1000);
        ret = libusb_submit_transfer(transfer[i]);
        if (ret != 0)
        {
            printf ("Failed to start endpoint streaming: %s\n", libusb_error_name(ret));
            success = false;
        }
    }
    return (success);
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
bool gn3s::usb_fx2_cancel_transfers()
{
    int ret;
    bool success = true;
    for (int i = 0; i < USB_NTRANSFERS; i++)
    {
        ret = libusb_cancel_transfer(transfer[i]);
        if (ret != 0)
        {
            printf ("Failed to cancel transfer: %s\n", libusb_error_name(ret));
            success = false;
        }
    }
    return (success);
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
int gn3s::read(unsigned char *buff, int bytes)
{
    int n;
    if (bcount < bufptr)
        n = bcount +sizeof(buffer) - bufptr;
    else
        n = bcount - bufptr;
    if (n > bytes)
        n = bytes;
    for (int i=0; i<n; i++)
    {
        buff[i] = buffer[0][bufptr++];
        if (bufptr == sizeof(buffer))
            bufptr = 0;
    }
    return(n);
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
bool gn3s::check_rx_overrun()
{
	bool overrun;

	_get_status(GS_RX_OVERRUN, &overrun);

	return(overrun);
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
bool gn3s::_get_status(int command, bool *trouble)
{
	unsigned char status;

	if(write_cmd(VRQ_GET_STATUS, 0, command, &status, sizeof(status)) != sizeof (status))
		return false;

	*trouble = status;
	return true;
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
bool gn3s::usrp_xfer(char VRQ_TYPE, bool start)
{
  int r;

  r = write_cmd(VRQ_TYPE, start, 0, nullptr, 0);

  return(r == 0);
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
int gn3s::write_cmd(int request, int value, int index, unsigned char *bytes, int len)
{
	int requesttype;
	int r;

	requesttype = (request & 0x80) ? VRT_VENDOR_IN : VRT_VENDOR_OUT;
    r = libusb_control_transfer (fx2_handle, requesttype, request, value, index, (unsigned char *) bytes, len, 1000);
    if(r < 0)
	{
		/* We get EPIPE if the firmware stalls the endpoint. */
		if(errno != EPIPE)
            fprintf (stdout, "usb_control_msg failed: %s\n", libusb_error_name(r));
	}
    return r;
}
/*----------------------------------------------------------------------------------------------*/
