/******************************************************************************
 *	fbtest - fbtest.c
 *	test program for the tuxbox-framebuffer device
 *	tests all GTX/eNX supported modes
 *                                                                            
 *	(c) 2003 Carsten Juttner (carjay@gmx.net)
 *
 * 	This program is free software; you can redistribute it and/or modify
 * 	it under the terms of the GNU General Public License as published by
 * 	The Free Software Foundation; either version 2 of the License, or
 * 	(at your option) any later version.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 * 	You should have received a copy of the GNU General Public License
 * 	along with this program; if not, write to the Free Software
 * 	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *  									      
 ******************************************************************************
 * $Id: fbtest.c,v 1.5 2005/01/14 23:14:41 carjay Exp $
 ******************************************************************************/

// TODO: - should restore the colour map and mode to what it was before
//	 - is colour map handled correctly?

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>


#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>



int fd;

int main(int argc, char **argv)
{

	fd = open("/dev/mem", O_RDWR | O_NONBLOCK);
	if (fd < 0)
	{
		printf("can't open : /dev/mem!\n");
		return -1;
	}

	
	printf("Open  /dev/mem = 0x%x!\n",fd);
	
	//ioctl(fd, WDIOC_GETSTATUS);

	return 0;
}



