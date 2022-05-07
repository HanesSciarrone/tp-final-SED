#include <stdint.h>
#include <string.h>
#include "common.h"
#include "include/sd.h"
#include "include/crono.h"


int isNumber(const char *str)
{
    int i = 0;
    int retVal = TRUE;

    for (i = 0; i < strlen(str); i++) {
        if ((str[i] >= '0' && str[i] <= '9') || str[i] == '-' || str[i] == '+') {
            retVal = TRUE;
        }
        else {
            retVal = FALSE;
            break;
        }
    }

    return retVal;
}

void saveLog(operation_t type, const char *topic, const char *message)
{
    FILE *fd = NULL;
    char timestamp[64] = {0};

    fd = SD_open("log.txt", "a");

    if (fd == NULL) {
        return;
    }

    CRONO_getTime(timestamp, sizeof(timestamp));

    switch (type) {
        case INPUT: {
            SD_printf(fd, "%s MQTT-IN: %s %s\n", timestamp, topic, message);
        }
        break;

        case OUTPUT: {
            SD_printf(fd, "%s MQTT-OUT: %s %s\n", timestamp, topic, message);
        }
        break;

        case SIMPLE_LOG: {
            SD_printf(fd, "%s %s %s\n", timestamp, topic, message);
        }
        break;
    }

    SD_close(fd);
    fd = NULL;
}
