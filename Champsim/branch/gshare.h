#include "ooo_cpu.h"

#define GLOBAL_HISTORY_LENGTH 14
#define GLOBAL_HISTORY_MASK (1 << GLOBAL_HISTORY_LENGTH) - 1
#define GS_HISTORY_TABLE_SIZE 16384

struct gshare
{
    int branch_history_vector;
    int gs_history_table[GS_HISTORY_TABLE_SIZE];
    int my_last_prediction;
    gshare()
    {
        branch_history_vector = 0;
        my_last_prediction = 0;
        for(int i=0; i<GS_HISTORY_TABLE_SIZE; i++)
            gs_history_table[i] = 2;
    }
    unsigned int table_hash(uint64_t ip,int bh_vector)
    {
        unsigned int hash = ip^(ip>>GLOBAL_HISTORY_LENGTH)^(ip>>(GLOBAL_HISTORY_LENGTH*2))^bh_vector;
        hash = hash%GS_HISTORY_TABLE_SIZE;
        return hash;
    }
    int get_pred(uint64_t ip)
    {
        int pred = 1;
        int hash = table_hash(ip,branch_history_vector);
        if (gs_history_table[hash] >= 2)
        {
            pred = 1;
            my_last_prediction = pred;
            return pred;
        }
        else
        {
            pred = 0;
            my_last_prediction = pred;
            return pred;
        }
    }
    void update(uint64_t ip,uint8_t taken)
    {
        int hash = table_hash(ip,branch_history_vector);
        if (taken == 0)
        {
            if(gs_history_table[hash] > 0)
                gs_history_table[hash]--;
        }
        else
        {
            if(gs_history_table[hash] < 3)
                gs_history_table[hash]++;
        }
        branch_history_vector <<= 1;
        branch_history_vector &= GLOBAL_HISTORY_MASK;
        branch_history_vector |= taken;
    }
};