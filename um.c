
/**************************************************************
 *                     um.c
 * 
 *     Assignment: Homework 6 - Universal Machine Program
 *     Authors: Katie Yang (zyang11) and Pamela Melgar (pmelga01)
 *     Date: November 24, 2021
 *
 *     Purpose: This C file will hold the main driver for our Universal
 *              MachineProgram (HW6). 
 *     
 *     Success Output:
 *              
 *     Failure output:
 *              1. 
 *              2. 
 *                  
 **************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "assert.h"
#include "uarray.h"
#include "seq.h"
#include "bitpack.h"
#include <sys/stat.h>

#define BYTESIZE 8


/* Instruction retrieval */
typedef enum Um_opcode {
        CMOV = 0, SLOAD, SSTORE, ADD, MUL, DIV,
        NAND, HALT, ACTIVATE, INACTIVATE, OUT, IN, LOADP, LV
} Um_opcode;


struct Info
{
    uint32_t op;
    uint32_t rA;
    uint32_t rB;
    uint32_t rC;
    uint32_t value;
};


typedef struct Info *Info;

#define num_registers 8
#define two_pow_32 4294967296
uint32_t MIN = 0;   
uint32_t MAX = 255;
uint32_t registers_mask = 511;
uint32_t op13_mask = 268435455;

///////////////////////////////////////////////////////////////////////////////////////////
/*Memory Manager*/
struct Memory
{
    Seq_T segments;
    Seq_T map_queue;
};

typedef struct Memory *Memory;


struct Array_T
{
    uint32_t *array;
    uint32_t size;
    uint32_t capacity;

};

typedef struct Array_T Array;


///////////////////////////////////////////////////////////////////////////////////////////
/*Register Manager*/
#define num_registers 8

static inline uint32_t get_register_value(uint32_t *all_registers, uint32_t num_register);

/* Memory Manager */
Memory initialize_memory();
//uint32_t memorylength(Memory memory);         not used
uint32_t segmentlength(Memory memory, uint32_t segment_index);
void add_to_seg0(struct Memory *memory, uint32_t word);
uint32_t get_word(Memory memory, uint32_t segment_index, 
                  uint32_t word_in_segment);
void set_word(Memory memory, uint32_t segment_index, uint32_t word_index, 
              uint32_t word);
uint32_t map_segment(Memory memory, uint32_t num_words);
void unmap_segment(Memory memory, uint32_t segment_index);
void duplicate_segment(Memory memory, uint32_t segment_to_copy);
void free_segments(Memory memory);
//////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////


/* Instruction retrieval */
Info get_Info(uint32_t instruction);

static inline void instruction_executer(Info info, Memory all_segments,
                          uint32_t *all_registers, uint32_t *counter);
//////////////////////////////////////////////////////////////////////////////////////////////////////

/*Making Helper functions for our ARRAYLIST*/

static inline uint32_t array_size(Array array);
static inline uint32_t element_at(Array array, uint32_t index);
static inline void replace_at(Array *array, uint32_t insert_value, uint32_t old_value_index);
static inline void push_at_back(Array *array, uint32_t insert_value);
//static inline Array expand_array(Array *array, uint32_t new_size);


//////////////////////////////////////////////////////////////////////////////


int main(int argc, char *argv[])
{
    /*Progam can only run with two arguments [execute] and [machinecode_file]*/
    
    if (argc != 2) {
        fprintf(stderr, "Usage: ./um [filename] < [input] > [output]\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(argv[1], "r");
    assert(fp != NULL);
    struct stat sb;

    const char *filename = argv[1];

    if (stat(filename, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE); // TODO what should the behavior be
    }

    int seg_0_size = sb.st_size / 4;

////////////////////////////////////////////////////////////////////////////
    /* EXECUTION.C MODULE  */
    //init memory

    Memory all_segments = malloc(sizeof(struct Memory));
    all_segments->segments = Seq_new(30);
    all_segments->map_queue = Seq_new(30);

    Array *segment0 = malloc(sizeof(Array));
    segment0->array = malloc(sizeof(uint32_t) * seg_0_size);
    segment0->size = 0;
    segment0->capacity = seg_0_size;

    //Seq_T segment0 = Seq_new(seg_0_size);
    //Seq_new(10);

    Seq_addhi(all_segments->segments, segment0);
    
    //init registers
    uint32_t all_registers[8] = { 0 };

/////////////////////////////////////// READ FILE ///////////////////////////
    uint32_t byte = 0; 
    uint32_t word = 0;
    int c;
    int counter = 0;

    /*Keep reading the file and parsing through "words" until EOF*/
    /*Populate the sequence every time we create a new word*/
    while ((c = fgetc(fp)) != EOF) {
        word = Bitpack_newu(word, 8, 24, c);
        counter++;
        for (int lsb = 2 * BYTESIZE; lsb >= 0; lsb = lsb - BYTESIZE) {
            byte = fgetc(fp);
            word = Bitpack_newu(word, BYTESIZE, lsb, byte);
        }
        
        /* Populate sequence */
        add_to_seg0(all_segments, word);
    }

   fclose(fp);

   /////////////////////////////////////// execution /////////////////////

    uint32_t program_counter = 0; /*start program counter at beginning*/
    
    /*Run through all instructions in segment0, note that counter may loop*/
    while (program_counter < (segmentlength(all_segments, 0)) ) {
        uint32_t instruction = get_word(all_segments, 0, program_counter);
        Info info = get_Info(instruction);
        program_counter++;

        /*program executer is passed in to update (in loop) if needed*/
        instruction_executer(info, all_segments, all_registers, 
                             &program_counter);
    }

    free_segments(all_segments);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    return EXIT_FAILURE; /*failure mode, out of bounds of $m[0]*/
}


static inline uint32_t array_size(Array array1)
{
    return array1.size;
}

static inline uint32_t array_capacity(Array array1)
{
    return array1.capacity;
}

static inline uint32_t element_at(Array array1, uint32_t index)
{
    return array1.array[index];
}

static inline void replace_at(Array *array1, uint32_t insert_value, 
                                               uint32_t old_value_index)
{
    
    // printf("old_vale_index is %u insert_value is %u\n", old_value_index, insert_value);

    if (old_value_index == array1->size){
        push_at_back(array1, insert_value);
    }
    else{
        // int size = array1->size;
        // printf("size is %d\n", size);
        array1->array[old_value_index] = insert_value;
    }

    // printf("exit\n");
}

static inline void push_at_back(Array *array1, uint32_t insert_value)
{
    uint32_t size = array_size(*array1);
    // printf("Size is : %u\n", size);

    // uint32_t capacity = array_capacity(*array1);
    // printf("Capacity is : %u\n", capacity);

        array1->array[size] = insert_value;
        array1->size = size + 1;

    // } else {
    //     *array1 = expand_array(array1, capacity * 2);

    //     array1->array[size] = insert_value;
    //     array1->size = size + 1;
    // }

    // printf("size after push_at_back is %d\n", size +1);

    // int hmmm = array1->size;

    // printf("hmmm is %d\n", hmmm);
}

// static inline Array expand_array(Array *array1, uint32_t capacity)
// {
//     Array *new_array = malloc(sizeof(Array));
//     new_array->array = malloc( (sizeof(uint32_t)) * capacity);
//     new_array->size = array1->size;
//     new_array->capacity = capacity;

//     int orig_size = array1->size;

//     uint32_t *array_new = new_array->array;
//     uint32_t *array_orig = array1->array;

//     for (int i = 0; i < orig_size; i++) {
//        array_new[i] = array_orig[i];
//     }
    
//     free(array1->array);
//     free(array1);

//     return *new_array;
// }

uint32_t segmentlength(Memory memory, uint32_t segment_index)
{
    uint32_t return_size = array_size(*(Array*)Seq_get(memory->segments, segment_index));
    //printf("What is this return size? : %u\n", return_size);

    return return_size;
}


void add_to_seg0(struct Memory *memory, uint32_t word)
{
    Array *segment_0 = ((Array *)(Seq_get(memory->segments, 0)));
    
    // printf("entering push at back\n");

    push_at_back(segment_0, word);
}

uint32_t get_word(struct Memory *memory, uint32_t segment_index, 
                  uint32_t word_in_segment)
{
    // Seq_T find_segment = (Seq_T)(Seq_get(memory->segments, segment_index));

    // uint32_t *find_word = (uint32_t *)(Seq_get(find_segment, word_in_segment));

    Array target = *((Array *)(Seq_get(memory->segments, segment_index)));

    uint32_t find_word = element_at(target, word_in_segment);

    return find_word;
}

void set_word(struct Memory *memory, uint32_t segment_index, 
              uint32_t word_index, uint32_t word)
{

    /*failure mode if out of bounds*/

    
    // Seq_T find_segment = Seq_get(memory->segments, segment_index);

    // uint32_t *word_ptr = malloc(sizeof(uint32_t));
    // assert(word_ptr != NULL);
    // *word_ptr = word;

    // /*failure mode if out of bounds*/
    // uint32_t *old_word_ptr = Seq_put(find_segment, word_index, word_ptr);
    // free(old_word_ptr);

    Array *target = (Array *)Seq_get(memory->segments, segment_index);

    // printf("word is %u word_index is %u\n", word, word_index);
    replace_at(target, word, word_index);
}

uint32_t map_segment(struct Memory *memory, uint32_t num_words)
{
    Array *new_segment = malloc(sizeof(Array));
    new_segment->array = malloc((sizeof(uint32_t)) * num_words);
    new_segment->capacity = num_words;
    new_segment->size = num_words;
    
    /*Initialize to ALL 0s*/
    for (uint32_t i = 0; i < num_words; i++) {
        replace_at(new_segment, 0, i);
    }

    if (Seq_length(memory->map_queue) != 0) {
        uint32_t *seq_index = (uint32_t *)Seq_remlo(memory->map_queue);
        Seq_put(memory->segments, *seq_index, new_segment);
        
        uint32_t segment_index = *seq_index;
        free(seq_index);
        return segment_index;
    }
    else {
        uint32_t length = (uint32_t)Seq_length(memory->segments);
        Seq_addhi(memory->segments, new_segment);
        return length;
    }
}

void unmap_segment(struct Memory *memory, uint32_t segment_index)
{
    /* can't un-map segment 0 */
    assert(segment_index > 0);

    Array *seg_to_unmap = (Array *)(Seq_get(memory->segments, segment_index));
    /* can't un-map a segment that isn't mapped */

    free(seg_to_unmap->array);
    free(seg_to_unmap);

    Seq_put(memory->segments, segment_index, NULL);

    uint32_t *num = malloc(sizeof(uint32_t));
    *num = segment_index;
    Seq_addhi(memory->map_queue, num);
}


void duplicate_segment(struct Memory *memory, uint32_t segment_to_copy)
{
    if (segment_to_copy != 0) {

        Array *seg_0 = (Array *)(Seq_get(memory->segments, 0));

        /*free all c array, that is  in segment 0*/
        free(seg_0->array);
        free(seg_0);

        /*hard copy - duplicate array to create new segment 0*/
        Array *target = (Array*)Seq_get(memory->segments, segment_to_copy);
        
        uint32_t seg_length = array_size(*target);

        Array *duplicate = malloc(sizeof(Array));
        duplicate->array = malloc(sizeof(uint32_t) * seg_length);
        duplicate->capacity = seg_length;
        duplicate->size = 0;


        /*Willl copy every single word onto the duplicate segment*/
        for (uint32_t i = 0; i < seg_length; i++) {
            uint32_t word = element_at(*target, i);
            push_at_back(duplicate, word);
        }

        /*replace segment0 with the duplicate*/
        Seq_put(memory->segments, 0, duplicate);

    } else {
        /*don't replace segment0 with itself --- do nothing*/
        return;
    }
}


void free_segments(struct Memory *memory)
{
    uint32_t num_sequences = Seq_length(memory->segments);
    /*Free all words in each segment of memory*/

    for (uint32_t i = 0; i < num_sequences; i++)
    {
        Array *target = ((Array *)(Seq_get(memory->segments, i)));

        if (target != NULL) {
            free(target->array);
            free(target);
        }
    }

    /* free map_queue Sequence that kept track of unmapped stuff*/
    uint32_t queue_length = Seq_length(memory->map_queue);
    
    /*free all the indexes words */
    for (uint32_t i = 0; i < queue_length; i++) {
        uint32_t *index = (uint32_t*)(Seq_get(memory->map_queue, i));
        free(index);
    }

    Seq_free(&(memory->segments));
    Seq_free(&(memory->map_queue));
    
    free(memory);
}

struct Info *get_Info(uint32_t instruction)
{
    struct Info *info = malloc(sizeof(struct Info));
    assert(info != NULL); /*Check if heap allocation successful*/

    uint32_t op = instruction;
    op = op >> 28;

    info->op = op;

    if (op != 13){
        uint32_t registers_ins = instruction &= registers_mask;

        info->rA = registers_ins >> 6;
        info->rB = registers_ins << 26 >> 29;
        info->rC = registers_ins << 29 >> 29;
    }
    else {

        uint32_t registers_ins = instruction &= op13_mask;

        info->rA = registers_ins >> 25;
        info->value = registers_ins << 7 >> 7;
    }

    return info;
}


//change back for kcachegrind
static inline void instruction_executer(Info info, Memory all_segments,
                           uint32_t *all_registers, uint32_t *counter)
{
    // printf("execute!\n");
    uint32_t code = info->op;

    /*We want a halt instruction to execute quicker*/
    if (code == HALT)
    {
        /////// halt /////
        free(info);
        
        free_segments(all_segments);

        exit(EXIT_SUCCESS);
    }

    /*Rest of instructions*/
    if (code == CMOV)
    {
        ///////// conditional_move /////
        uint32_t rA = info->rA;
        uint32_t rC = info->rC;

        uint32_t rC_val = get_register_value(all_registers, rC);

        if (rC_val != 0)
        {
            uint32_t rB = info->rB;

            uint32_t rB_val = get_register_value(all_registers, rB);

            //set_register_value(all_registers, rA, rB_val);
            uint32_t *word;
            word = &(all_registers[rA]);
            *word = rB_val;
        }
    }
    else if (code == SLOAD)
    {
        ////// segemented load /////
        /* Establishes which register indexes are being used */
        uint32_t rA = info->rA;
        uint32_t rB = info->rB;
        uint32_t rC = info->rC;

        /* Accesses the values at the register indexes*/
        uint32_t rB_val = get_register_value(all_registers, rB);
        uint32_t rC_val = get_register_value(all_registers, rC);

        uint32_t val = get_word(all_segments, rB_val, rC_val);

        //set_register_value(all_registers, rA, val);
        uint32_t *word;
        word = &(all_registers[rA]);
        *word = val;  
    }
    else if (code == SSTORE)
    {
        /////// segmented store /////
        /* Establishes which register indexes are being used */
        uint32_t rA = info->rA;
        uint32_t rB = info->rB;
        uint32_t rC = info->rC;

        /* Accesses the values at the register indexes*/
        uint32_t rA_val = get_register_value(all_registers, rA);
        uint32_t rB_val = get_register_value(all_registers, rB);
        uint32_t rC_val = get_register_value(all_registers, rC);

        set_word(all_segments, rA_val, rB_val, rC_val);
        
    }
    else if (code == ADD || code == MUL || code == DIV || code == NAND)
    {
        ////// arithetics /////
        uint32_t rA = info->rA;
        uint32_t rB = info->rB;
        uint32_t rC = info->rC;

        uint32_t rB_val = get_register_value(all_registers, rB);
        uint32_t rC_val = get_register_value(all_registers, rC);

        uint32_t value = 0;

        /*Determine which math operation to perform based on 4bit opcode*/
        if (code == ADD)
        {
            value = (rB_val + rC_val) % two_pow_32;
        }
        else if (code == MUL)
        {
            value = (rB_val * rC_val) % two_pow_32;
        }
        else if (code == DIV)
        {
            value = rB_val / rC_val;
        }
        else if (code == NAND)
        {
            value = ~(rB_val & rC_val);
        }
        else
        {
            exit(EXIT_FAILURE);
        }

        uint32_t *word;
        word = &(all_registers[rA]);
        *word = value;
    }
    else if (code == ACTIVATE)
    {
        ////////////// map_a_segment ////////
        uint32_t rB = info->rB;
        uint32_t rC_val = get_register_value(all_registers, info->rC);
        uint32_t mapped_index = map_segment(all_segments, rC_val);

        uint32_t *word;
        word = &(all_registers[rB]);
        *word = mapped_index;
    }
    else if (code == INACTIVATE)
    {
        ////////////// unmap_a_segment ////////
        uint32_t rC_val = get_register_value(all_registers, info->rC);
        unmap_segment(all_segments, rC_val);
    }
    else if (code == OUT)
    {
        ////////////// output ////////
        uint32_t rC = info->rC;
        uint32_t val = get_register_value(all_registers, rC);

        assert(val >= MIN);
        assert(val <= MAX);

        printf("%c", val);
    }
    else if (code == IN)
    {
        ////////////// input ////////
        uint32_t rC = info->rC;

        uint32_t input_value = (uint32_t)fgetc(stdin);
        uint32_t all_ones = ~0;

        /*Check if input value is EOF...aka: -1*/
        if (input_value == all_ones)
        {
            //set_register_value(all_registers, rC, all_ones);
            uint32_t *word;
            word = &(all_registers[rC]);
            *word = all_ones;
            
            return;
        }

        /* Check if the input value is in bounds */
        assert(input_value >= MIN);
        assert(input_value <= MAX);

        /* $r[C] gets loaded with input value */
        uint32_t *word;
        word = &(all_registers[rC]);
        *word = input_value;
    }
    else if (code == LOADP)
    {
        ////////////// load_program ////////
        uint32_t rB_val = all_registers[info->rB];

        uint32_t rC_val = get_register_value(all_registers, info->rC);
        duplicate_segment(all_segments, rB_val);
        *counter = rC_val;
    }
    else if (code == LV)
    {
        ////////////// load_value ////////
        uint32_t rA = info->rA;
        uint32_t val = info->value;

        uint32_t *word= &(all_registers[rA]);
        *word = val;
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    free(info);
    return;
}

static inline uint32_t get_register_value(uint32_t *all_registers, uint32_t num_register)
{
    return all_registers[num_register]; // TODO ????
}
