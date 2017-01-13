//
// Created by Anders Stenberg on 2016-12-14.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/io.h>

// #include "io.h"

#define VERSION     "1.03.003"
#define VDATE       "2017.01.11"

#define base 0x378   //LPT1  ( 0x278 LPT2 )

/* Synntax
 * parport value [-p port ] [ -m mask ] [ -r mask ]
 *
 * value - decimal port value
 * -p port  printerport (default 0x378)
 * -p mask  only change bits in mask (decimal)
 * -r mask  return 0-255 mask
 *
 */



/*  to compile:  gcc -O parport.c -o parport
    after compiling, set suid:  chmod +s parport   then, copy to /usr/sbin/


    1.  vi /etc/modprobe.d/blacklist.conf
        add:    blacklist ppdev
                blacklist lp
                blacklist parport_pc
                blacklist parport

    2.  And if cups is installed, you should modify /etc/modules-load.d/cups-filters.conf:
                #lp
                #ppdev
                #parport_pc


Port adresses base+0:  Output D0-7 on pin.
  D0    Data bit 0      Pin 2   (OUTPUT latched)
  D1    Data bit 1      Pin 3   (OUTPUT latched)
  D2    Data bit 2      Pin 4   (OUTPUT latched)
  D3    Data bit 3      Pin 5   (OUTPUT latched)
  D4    Data bit 4      Pin 6   (OUTPUT latched)
  D5    Data bit 5      Pin 7   (OUTPUT latched)
  D6    Data bit 6      Pin 8   (OUTPUT latched)
  D7    Data bit 8      Pin 9   (OUTPUT latched)

Status port (read only) Input adress base+1:
  D0 - reserved
  D1 - reserved
  D2 - _IRQ_
  D3 - _ERROR_          Pin 15  (INPUT)
  D4 - SLECT            Pin 13  (INPUT)
  D5 - PE (PaperEnd)    Pin 12  (INPUT)
  D6 - _ACK_            Pin 10  (INPUT)
  D7 - BUSY             Pin 11  (invert INPUT)

Control port base+2 (Read/Write)
  D0    _STROBE_        Pin 1   (inverted OUTPUT)
  D1    _AUTOFEED_      Pin 14  (inverted OUTPUT)
  D2    INIT PRN        Pin 16  (OUTPUT)
  D3    _SELECT_        Pin 17  (inverted OUTPUT)
  D4    Enable IRQ via Ack
  D5    Enable BI-Directional Port
  D6    Unused
  D7    Unused

*/

#define OFLAG       0x01        // Set output bits
#define IFLAG       0x02        // Return data in stdio
#define ON_FLAG     0x04        // Set bits on
#define OFF_FLAG    0x08        // Set bits off
#define TFLAG       0x10        // Testflag
#define VFLAG       0x20        // Version
#define HFLAG       0x40        // Helptext


void usage(), help(), version();

int main(int argc, char *argv[]) {
    int i, port;

    unsigned int flags = 0;

    int c;
    int digit_optind = 0;

    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
                {"all",     no_argument,        0,  'a'},
                {"help",    no_argument,        0,  'h'},
                {"input",   no_argument,        0,  'i'},
                {"output",  required_argument,  0,  'o'},
                {"swon",    required_argument,  0,  '1'},
                {"swoff",   required_argument,  0,  '0'},
                {"test",    no_argument,        0,  't'},
                {"version", no_argument,        0,  'v'},
                {0,         0,                  0,  0}
        };

        c = getopt_long(argc, argv, "ahitvo:1:0:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'a':
                flags |= (OFLAG | IFLAG | ON_FLAG | OFF_FLAG);
                break;
            case 'h':
                flags |= HFLAG;
                break;
            case 'i':
                flags |= IFLAG;
                break;
            case 'o':
                flags |= OFLAG;
                port = atoi(optarg);
                break;
            case '1':
                flags |= ON_FLAG;
                port = atoi(optarg);
                break;
            case '0':
                flags |= OFF_FLAG;
                port = atoi(optarg);
                break;
            case 't':
                flags |= TFLAG;
                break;
            case 'v':
                flags |= VFLAG;
                break;

            case '?':
            default:
                usage();
                return 0;
        }
    }


    if ( flags & HFLAG) {
        help();
        return 0;
    }
    if ( flags & VFLAG ) {
        version();
        return 0;
    }

    if(ioperm(base,3,1))
        fprintf(stderr, "Couldn't open parallel port\n"), exit(1);

    if ( flags & OFLAG )
        outb(port, base);

    if ( flags & ON_FLAG )
        outb( port | inb(base), base);

    if (flags & OFF_FLAG )
        outb( ~port & inb(base), base);

    if ( flags & TFLAG ) {
        port=1;
        for (i = 0; i < 8; i++) {
            outb(port, base);
            port = port << 1;
            usleep(100000);

        }
        usleep(200000);

        for (i = 0; i < 8; i++) {
            port = port >> 1;
            outb(port, base);
            usleep(100000);

        }

        printf("status 0x%04X: 0x%02X\n", base, inb(base));
        printf("status 0x%04X: 0x%02X\n", base+1, inb(base + 1));
        printf("status 0x%04X: 0x%02X\n", base+2, inb(base + 2));
        usleep(200000);
        outb(255, base);  //set all pins hi
        usleep(150000);
        outb(0, base);    //set all pins lo
    }

    if ( flags & IFLAG )
        printf("%d\n", inb(base));

    return 0;
}

void usage(){
    printf("Usage: parport [option] <value>  \n");
}

void help() {
    puts("USAGE: parport [options] <input>\n");
    puts("OPTIONS:");
    puts(" -h           --help             Display this help");
    puts(" -i           --input            Return port data in decimal value 0-255");
    puts(" -o <value>   --output=<value>   Set port data output 0-255");
    puts(" -1 <value>   --swon=<value>     Set only port data bit mask on (1)");
    puts(" -0 <value>   --swoff=<value>    Set only port data bit mask off (0)");
    puts(" -t           --test             Show testdata and return input values");
    puts(" -v           --version          Show version\n");
}

void version() {
    printf("Compiled %s Version %s by ANS", VDATE, VERSION);
    puts("(c) Anders Stenberg SM1IRS\n");
}