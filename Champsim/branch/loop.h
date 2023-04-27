#include <iostream>
#include <inttypes.h>
using namespace std;

#define NO_ENTRIES 256
#define SET_BITS 6
#define WAY_BITS 2
#define WAY 4
#define TAG_BITS 14
#define INIT_AGE 31
#define ITER_BITS 14

struct Entry{
    uint8_t confidence;
    uint8_t age;
    uint16_t tag;
    uint16_t curr_iter;
    uint16_t prev_iter;
};

struct LPred{
    Entry entry[NO_ENTRIES];
    uint8_t set,way,prediction;
    bool validity;
    uint16_t tag_calc;

    void init();
    uint8_t predict(uint64_t ip);
    void update(uint8_t tage_prediction, uint8_t taken);
};

void LPred::init(){
    for(int i=0; i<NO_ENTRIES; i++){
        entry[i].confidence = 0;
        entry[i].age = 0;
        entry[i].tag = 0;
        entry[i].curr_iter = 0;
        entry[i].prev_iter = 0;
    }
}

uint8_t LPred::predict(uint64_t ip){
    tag_calc = (ip >> SET_BITS) & ((1 << TAG_BITS) - 1);
    set = ip & ((1 << SET_BITS) - 1);
    way = -1;
    validity = false;
    prediction = 0;

    uint16_t index = set << WAY_BITS;

    for(int i=index; i<index+WAY; i++){

        if(entry[i].tag == tag_calc){
            way = i-index;

            if(entry[i].confidence == 3){
                validity = true;
            }

            if(entry[i].curr_iter + 1 == entry[i].prev_iter){
                return 0;
            }

            prediction = 1;
            return 1;

        }
    }

    return 0;
}

void LPred::update(uint8_t tage_prediction, uint8_t taken){
    uint16_t index = 2*(set << WAY_BITS);
    index += way;

    if(way != -1){

        if(validity){

            if(prediction != taken){
                entry[index].confidence = 0;
                entry[index].age = 0;
                entry[index].curr_iter = 0;
                entry[index].prev_iter = 0;
                return;
            }

            if(tage_prediction != taken){
                if(entry[index].age < INIT_AGE) entry[index].age++;
            }
        }

        entry[index].curr_iter++;
        entry[index].curr_iter &= ((1 << ITER_BITS) - 1);

        if(entry[index].curr_iter > entry[index].prev_iter){

            entry[index].confidence = 0;

            if(entry[index].prev_iter != 0){
                entry[index].age = 0;
                entry[index].prev_iter = 0;
            }
        }

        if(taken == 0){

            if(entry[index].curr_iter == entry[index].prev_iter){

                if(entry[index].confidence < 3){
                    entry[index].confidence++;
                }

                if(entry[index].prev_iter > 0 && entry[index].prev_iter < 3){
                    entry[index].prev_iter = 0;
                    entry[index].age = 0;
                    entry[index].confidence = 0;
                }

            }
            else{

                if(entry[index].prev_iter == 0){
                    entry[index].prev_iter = entry[index].curr_iter;
                    entry[index].confidence = 0;
                }
                else{
                    entry[index].age = 0;
                    entry[index].prev_iter = 0;
                    entry[index].confidence = 0;
                }
            }

            entry[index].curr_iter = 0;
        }
    }

    else if(taken==1){
        
        int index = (set << WAY_BITS);
        for(int i=0; i<WAY; i++){

            if(entry[index].age == 0){
                entry[index].confidence = 0;
                entry[index].age = INIT_AGE;
                entry[index].tag = tag_calc;
                entry[index].prev_iter = 0;
                entry[index].curr_iter = 1;

                break;
            }
            else{
                entry[index].age--;
            }

            index++;
        }
    }

}