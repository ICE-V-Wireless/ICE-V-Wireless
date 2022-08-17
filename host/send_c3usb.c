/*
 * send_c3usb.c - send fpga bitstream via USB CDC -> ICE-V
 * 08-16-22 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#define VERSION "0.1"

static void help(void) __attribute__ ((noreturn));

int cdc_file;		/* CDC device */
char buffer[64];

/*
 * print help
 */
static void help(void)
{
	fprintf(stderr,
	    "Usage: send_c3usb <options> [FILE]\n"
		"  FILE is a file containing the FPGA bitstream to ICE-V\n"
		"  -b gets the current battery voltage\n"
		"  -f write bitstream to SPIFFS instead of to FPGA\n"
		"  -p <port> specify the CDC port (default = /dev/ttyACM0)\n"
		"  -r <reg> read FPGA register\n"
		"  -w <reg> <data> write FPGA register\n"
		"  -v enables verbose progress messages\n"
		"  -V prints the tool version\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int flags = 0, verbose = 0, err = 0, sz, cmd = 15, reg = 0, rsz, wsz, buffs;
	unsigned int data = 0;
    char *port = "/dev/ttyACM0";
    FILE *infile;
    struct termios termios_p;
    
	/* handle (optional) flags first */
	while (1+flags < argc && argv[1+flags][0] == '-')
	{
		switch (argv[1+flags][1])
		{
		case 'b':
			cmd = 2;
			break;
		case 'f':
			cmd = 14;
			break;
		case 'p':
			if (2+flags < argc)
                port = argv[flags+2];
			flags++;
			break;
		case 'r':
			cmd = 0;
			if (2+flags < argc)
                reg = atoi(argv[flags+2]);
			flags++;
			break;
		case 'w':
			cmd = 1;
			if (2+flags < argc)
                reg = atoi(argv[flags+2]);
			flags++;
			if (2+flags < argc)
                data = atoi(argv[flags+2]);
			flags++;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'V':
			fprintf(stderr, "send_c3usb version %s\n", VERSION);
			exit(0);
		default:
			fprintf(stderr, "Error: Unsupported option "
				"\"%s\"!\n", argv[1+flags]);
			help();
		}
		flags++;
	}
	
    /* open output port */
    cdc_file = open(port, O_RDWR);
    if(cdc_file<0)
	{
		fprintf(stderr,"Couldn't open cdc device %s\n", port);
        exit(1);
	}
	else
	{
        /* port opened so set up termios for raw binary */
        if(verbose) fprintf(stderr,"opened cdc device %s\n", port);
        
        /* get TTY attribs */
        tcgetattr(cdc_file, &termios_p);
        
        /* input modes */
        termios_p.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IUCLC | INLCR| IXANY );

        /* output modes - clear giving: no post processing such as NL to CR+NL */
        termios_p.c_oflag &= ~(OPOST|OLCUC|ONLCR|OCRNL|ONLRET|OFDEL);

        /* control modes - set 8 bit chars */
        termios_p.c_cflag |= ( CS8 ) ;

        /* local modes - clear giving: echoing off, canonical off (no erase with 
        backspace, ^U,...), no extended functions, no signal chars (^Z,^C) */
        termios_p.c_lflag &= ~(ECHO | ECHOE | ICANON | IEXTEN | ISIG);

        termios_p.c_cflag |= CRTSCTS;	// using flow control via CTS/RTS
        
        /* set attribs */
        tcsetattr(cdc_file, 0, &termios_p);
		
		if(verbose) fprintf(stderr, "Port configured\n");
    }
	
	/* flush out any stuff in the rx pipe */
	sz = read(cdc_file, buffer, 64);
	if(verbose) fprintf(stderr, "Flushed %d bytes from rx\n", sz);
	
	/* get size */
	if(cmd==1)
	{
		/* write command */
		sz = 8;
	}
	else if(cmd >= 14)
	{
		/* get file */
		if (2+flags > argc)
		{
			/* missing argument */
			fprintf(stderr, "Error: Missing bitstream file\n");
			close(cdc_file);
			help();
			exit(1);
		}
		else
		{
			/* open file */
			if(!(infile = fopen(argv[flags + 1], "rb")))
			{
				fprintf(stderr, "Error: unable to open file %s for read\n", argv[flags + 1]);
				close(cdc_file);
				exit(1);
			}
			
			/* get length */
			fseek(infile, 0L, SEEK_END);
			sz = ftell(infile);
			fseek(infile, 0L, SEEK_SET);
			if(verbose) fprintf(stderr, "Opened file %s with size %d\n", argv[flags + 1], sz);
		}
    }
	else
	{
		/* all others */
		sz = 4;
	}
	
	/* Send command header */
	buffer[0] = 0xE0 | (cmd & 0xF);
	buffer[1] = 0xBE;
	buffer[2] = 0xFE;
	buffer[3] = 0xCA;
	buffer[4] = sz&0xff;
	buffer[5] = (sz>>8)&0xff;
	buffer[6] = (sz>>16)&0xff;
	buffer[7] = (sz>>24)&0xff;
	write(cdc_file, buffer, 8);
		
	if(verbose) fprintf(stderr, "Header Sent\n");
	
	/* Send payload */
    if(cmd >= 14)
	{
		/* send the payload from a file 64 bytes at a time */
		wsz = 0;
		buffs = 0;
		while((rsz = fread(buffer, 1, 64, infile)) > 0)
		{
			write(cdc_file, buffer, rsz);
			wsz += rsz;
			buffs++;
		}
		fclose(infile);
		if(verbose) fprintf(stderr, "Sent %d bytes, %d buffers\n", wsz, buffs);
	}
	else
	{
		/* send the reg */
		write(cdc_file, (unsigned int *)&reg, 4);
		
		/* if write, send data */
		if(cmd==1)
		{
			write(cdc_file, (unsigned int *)&data, 4);
		}
		
		if(verbose) fprintf(stderr, "Sent %d bytes\n", sz);		
	}
	
	/* wait a bit and get result SPIFFS writes can take a while! */
	usleep(cmd == 14 ? 2000000 : 100000);
	sz = read(cdc_file, buffer, 64);
	if(sz)
	{
		if(verbose) fprintf(stderr, "Result: %s (len = %d)\n", buffer, sz);
		char *tok = strtok(buffer, " \n");
		err = strtol(tok, NULL, 16);
		tok = strtok(NULL, " \n");
		data = strtol(tok, NULL, 16);
		
		/* report error or return value if applicable */
		if(err)
			fprintf(stderr, "Error %d\n", err);
		else
		{
			if(cmd == 0)
				fprintf(stdout, "Reg 0x%02X = 0x%08X\n", reg, data);
			else if(cmd == 2)
				fprintf(stdout, "Battery = %d mV\n", data);
		}
	}
	else
	{
		fprintf(stderr, "Error - no reply\n");
		err = 1;
	}
		
    close(cdc_file);
    return err;
}