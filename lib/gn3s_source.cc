/*----------------------------------------------------------------------------------------------*/
/*! \file gps_source.cpp
//
// FILENAME: gps_source.cpp
//
// DESCRIPTION: Implements member functions of the GPS_Source class.
//
// DEVELOPERS: Gregory W. Heckler (2003-2009), Javier Arribas (2012)
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


#include "gn3s_source.h"
#include <time.h>


/*----------------------------------------------------------------------------------------------*/
gn3s_Source::gn3s_Source()
{

    Open_GN3S();

	overflw = soverflw = 0;
	agc_scale = 1;

	/* Assign to base */
	ms_count = 0;
	flag_first_read=true;
    fprintf(stdout,"Creating GPS Source\n");

}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
gn3s_Source::~gn3s_Source()
{

	Close_GN3S();
	fprintf(stdout,"Destructing GPS Source\n");
}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
void gn3s_Source::Read(gn3s_ms_packet *_p,int n_samples)
{

	Read_GN3S(_p,n_samples);
	ms_count++;

}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
void gn3s_Source::Open_GN3S()
{


	/* Create the object */
	gn3s_a = new gn3s(0);


	/* Everything is super! */
//    fprintf(stdout,"GN3S Start 1\n");

}
/*----------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------*/
void gn3s_Source::Close_GN3S()
{

    if(gn3s_a != nullptr) delete gn3s_a;
    fprintf(stdout,"Destructing GN3S\n");

}
/*----------------------------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------------------------*/
void gn3s_Source::Read_GN3S(gn3s_ms_packet *_p,int n_samples)
{

    int bread;
	int started=0;
	int check;
	bool overrun;

	short int LUT4120[2] = {1,-1};
	char shift = 0;
	char endshift = 0;

	short int *pbuff;

	int BUFSIZE=n_samples*2;

    if (flag_first_read==true)
	{
		/* Start transfer */
        while(!started)
		{
            usleep(100);
			started = gn3s_a->usrp_xfer(VRQ_XFER, 1);
        }
		fprintf(stdout,"started TX\n");
		flag_first_read=false;
	}

	/* Check the overrun */
	overrun = gn3s_a->check_rx_overrun();
	if(overrun)
	{
		time(&rawtime);
		timeinfo = localtime (&rawtime);
		fprintf(stdout, "GN3S overflow at time %s\n", asctime(timeinfo));
		fflush(stdout);
	}


		/* Read 5 ms */
        bread = gn3s_a->read(gbuff, BUFSIZE);
		// FUSB Read...
		//ret = fx2c.d_ephandle->read (buf, bufsize);

		if (bread != BUFSIZE) {
		  fprintf (stderr, "fusb_read: ret = %d (bufsize: %d) \n", bread, BUFSIZE);
//          fprintf (stderr, "%s\n", libusb_error_name());
		}

		// Store IF data as 8bit signed values
	   pbuff = (short int *)&buff[0];
	/* Make sure we are reading I0,Q0,I1,Q1,I2,Q2.... etc */
	   if ((gbuff[0] & 0x2) == 2)       //if true, we don't have to shift data
	   { shift = 0; }
	   else
	   { shift = 1; }

	   if ((gbuff[BUFSIZE-1] & 0x02) == 0) //if true, we don't drop last data byte
	   { endshift = 0; }
	   else
	   { endshift = 1; }

	       for (int j=0;j<BUFSIZE;j++)
	       {
	           if (shift == 1)
	           {
	               if ((j == (BUFSIZE-1)) && (endshift == 0))
	                 { pbuff[j] = 0; }
	               else if ((j == (BUFSIZE-1)) && (endshift == 1))
	                 { pbuff[j-1] = 0;  }
	               else
	               {
	            	   //printf("%i.",j);
	            	   pbuff[j] = LUT4120[gbuff[j+1] & 0x1];
	               }
	           } else if (shift == 0)
	           {
	               if ((j == (BUFSIZE-1)) && (endshift == 1))
	                 { pbuff[j] = 0; }
	               else
	               { pbuff[j] = LUT4120[gbuff[j] & 0x1]; }

	           }
	       }
	/* Copy to destination */
	memcpy(_p->data, pbuff, n_samples*sizeof(GN3S_CPX));
}
