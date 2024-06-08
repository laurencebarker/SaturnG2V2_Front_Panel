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


char* pi_i2c_device = "/dev/i2c-1";
unsigned int G2V2Arduino = 0x15;                    // i2c slave address of Arduino on G2V2
int i2c_fd;                                  // file reference
bool FoundG2V2Panel = false;


//
// function to test data connection to front panel
//
void TestG2V2Panel(void)
{
    uint16_t Retval;

    Retval = i2c_read_word_data(0x0C);
    printf("ID & version=%04x\n", Retval);

    Retval = i2c_write_word_data(0x0A, 0x0102);

    Retval = i2c_read_word_data(0x0A);
    printf("LED read=%04x\n", Retval);
}



//
// function to initialise a connection to the front panel; call if selected as a command line option
// establish which if any front panel is attached, and get it set up.
//
void main(void)
{
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


