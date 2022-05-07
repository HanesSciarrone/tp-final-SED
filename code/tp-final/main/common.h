#ifndef COMMON_H_
#define COMMON_H_

#define TRUE    1
#define FALSE   0

typedef enum {
    OUTPUT,
    INPUT,
    SIMPLE_LOG
}operation_t;

int isNumber(const char *str);
void saveLog(operation_t type, const char *topic, const char *message);

#endif /* COMMON_H_ */
