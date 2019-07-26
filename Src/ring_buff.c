#include <ring_buff.h>

void initRingBuff(Ring_buff_t *buff ,uint8_t buff_size, uint8_t *ptr)
{
    /*
        This function init a ring buffer.It accepts a Ring_buff_t structure and a mulloc pointer
        associated with that Ring_buff_t
     */
    
    // assign initial value to structure variables
    buff->buffer = ptr;
    buff->max = buff_size;

    // assign zeros to all positions in a buffer
    resetBuff(buff);
}

bool buffIsFull(Ring_buff_t *buff)
{
    /*
        This function checks wether a buffer is full.
        If a buffer is full it returns true, otherwise false.
     */

    if (buff->count == buff->max) {
        return true;
    } else {
        return false;
    }
}

int8_t addToBuff(Ring_buff_t *buff, uint8_t value)
{
    /*
        This function adds 1 element into a buffer at its current head position.
        Returns -1 if the buffer is full, otherwise 0(successfully added an element).
     */

    // check if buffer is full
    if (buffIsFull(buff)) {
        return -1;
    }

    uint8_t *ptr = buff->buffer + buff->head;
    *ptr = value;

    buff->head++;
    buff->count++;
    // check if buffer reaches its end
    if (buff->head == buff->max) {
        buff->head = 0;
    }

    return 0;
}


bool buffIsEmpty(Ring_buff_t *buff)
{
    /*
        This function returns true if a buffer is empty, otherwise false.
     */

    if (buff->count == 0) {
        return true;
    }
    else {
        return false;
    }
}

uint8_t* readBuff(Ring_buff_t *buff)
{
    /*
        This function returns an element from a buffer if a buffer is not empty.
        If a buffer is empty it'll return -1.
     */

    // check if buffer is empty
    if (buff->count == 0) {
        return (uint8_t) 0; //cannot read buffer
    }

    // check if tail reaches the end of a buffer
    if (buff->tail == buff->max) {
        buff->tail = 0;
    }

    buff->tail++;
    buff->count--;

    return buff->buffer + buff->tail - 1;
}

void resetBuff(Ring_buff_t *buff)
{   
    /*
        This function set all positions in a buffer to zero and set all its properties to
        their initial values.
     */
    
    uint8_t *ptr = buff->buffer;
    for (int i = 0; i < buff->max; ++i) {
        ptr = buff->buffer + i;
        *ptr = 0;
    }

    buff->head = 0;
    buff->tail = 0;
    buff->count = 0;
}

uint8_t getBuffLength(Ring_buff_t *buff)
{
    /*
        This function returns a number of unread elements in a buffer.
     */
    return buff->count;
}

/*
void printBuff(Ring_buff_t *buff)
{
    for (int i = 0; i < buff->max; ++i) {
        printf("%d ", *(buff->buffer + i));
    }
    printf("\n\n");
}
*/
