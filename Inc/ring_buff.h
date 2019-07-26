#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// struct
typedef struct {
    uint8_t * buffer; // points to the first position inside the buffer
    uint8_t head;
    uint8_t tail;
    uint8_t max;   // maximum capacity of a buffer
    uint8_t count; // number of entries that hasn't been read
} Ring_buff_t;

// prototypes
void initRingBuff(Ring_buff_t* buff ,uint8_t buff_size, uint8_t *ptr);
bool buffIsFull(Ring_buff_t *buff);
int8_t addToBuff(Ring_buff_t *buff, uint8_t value);
bool buffIsEmpty(Ring_buff_t *buff);
uint8_t* readBuff(Ring_buff_t *buff);
void resetBuff(Ring_buff_t *buff);
uint8_t getBuffLength(Ring_buff_t *buff);

//void printBuff(Ring_buff_t *buff);
