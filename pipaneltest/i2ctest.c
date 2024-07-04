/////////////////////////////////////////////////////////////
//
// Saturn project: i2ctest
//
// test i2c connecton to front panel controls
//
//
// before building and running you may need to execute:
//
// sudo apt-get install gpiod -y
// sudo apt-get install libgpiod-dev -y
// sudo apt-get install libi2c-dev
//
//
//////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <linux/i2c-dev.h>
#include "i2cdriver.h"
#include "gpiod.h"


char* pi_i2c_device = "/dev/i2c-1";
char* gpiod_device = "/dev/gpiochip0";
unsigned int G2V2Arduino = 0x15;                    // i2c slave address of Arduino on G2V2
int i2c_fd;                                         // file reference
bool FoundG2V2Panel = false;
int Version;
struct gpiod_chip *chip;                            // gpiod for gpio access
struct gpiod_line *intline;


#define VNOEVENT 0
#define VVFOSTEP 1
#define VENCODERSTEP 2
#define VPBPRESS 3
#define VPBLONGRESS 4
#define VPBRELEASE 5

//
// function to test data connection to front panel
//
void TestG2V2Panel(void)
{
    int IntState;
    uint16_t Retval;
    uint16_t Version;
    uint8_t EventCount;
    uint8_t EventID;
    uint8_t EventData;
    int8_t Steps;
    bool Error;
    struct timespec ts = {1, 0};                                    // timeot time = 1s
    struct gpiod_line_event intevent;

//
// read product ID and version register
//
    Retval = i2c_read_word_data(0x0C, &Error);                      // read ID register
    if(!Error)
    {
        Version = (Retval >> 8) &0xFF;
        printf("product ID=%d", Version);
        Version = Retval & 0xFF;
        printf("; S/W verson = %d\n", Version);
    }

//
// now loop waiting for interrupt, then reading the i2c Event register
// need to read all i2c data, until it reports no more events
//
    while(1)
    {
        Retval = gpiod_line_event_wait(intline, &ts);                   // wait for interrupt from Arduino
        if(Retval > 0)                                                  // if event occurred ie not timeout
        {
            Retval = gpiod_line_event_read(intline, &intevent);         // UNDOCUMENTED: read event to cancel it
            //
            // the interrupt line has reached zero. Read and process one i2c event
            // if there is more than one event present, the interrupt line will stay low
            //
            while(1)
            {
                Retval = i2c_read_word_data(0x0B, &Error);                  // read Arduino i2c event register
                if(!Error)
                {
                    printf("data=%04x; ", Retval);
                    EventID = (Retval >> 8) & 0x0F;
                    EventCount = (Retval >> 12) & 0x0F;
                    EventData = Retval & 0xFF;

                    switch(EventID)
                    {
                        case VNOEVENT:
                            break;
                                
                        case VVFOSTEP:
                            Steps = (int8_t)EventData;
                            printf("VFO encoder step, steps = %d; ", Steps);
                            break;

                        case VENCODERSTEP:
                            Steps = (int8_t)(EventData & 0xF);
                            if (Steps >= 8)
                                Steps = -(16-Steps);
                            printf("normal encoder step, encoder = %d, steps = %d; ", ((EventData>>4) + 1), Steps);
                            break;

                        case VPBPRESS:
                            printf("Pushbutton press, scan code = %d; ", EventData);
                            break;

                        case VPBLONGRESS:
                            printf("Pushbutton longpress, scan code = %d; ", EventData);
                            break;

                        case VPBRELEASE:
                            printf("Pushbutton release, scan code = %d; ", EventData);
                            break;

                        default:
                            printf("spurious event code = %d; ", EventID);
                            break;

                    }
                    printf(" Remaining Events Count = %d\n", EventCount);
                    if(EventCount <= 1)
                        break;
                }
                usleep(1000);                                                   // small 1ms delay between i2c reads
            }
        }
    }
}





//
// function to initialise a connection to the front panel; call if selected as a command line option
// establish which if any front panel is attached, and get it set up.
//
void main(void)
{
    chip = gpiod_chip_open(gpiod_device);
    if(!chip)
        perror("gpiod_chip_open");

    intline = gpiod_chip_get_line(chip, 4);
    if(!intline)
        perror("gpiod_chip_get_line");

//
// setup interrupt line as an input, with falling edge events
//    
    gpiod_line_request_falling_edge_events(intline, "interrupt");

    i2c_fd=open(pi_i2c_device, O_RDWR);
    if(i2c_fd < 0)
        printf("failed to open i2c device\n");
    else
    {
        // check for G2 front panel
        if(ioctl(i2c_fd, I2C_SLAVE, G2V2Arduino) >= 0)
        {
            printf("found G2 V2 front panel\n");
            FoundG2V2Panel = true;
            TestG2V2Panel();
        }
    }
}


