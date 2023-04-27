#include "ooo_cpu.h"
#include <cmath>

#define COUNTER_BITS 3
#define USEFUL_BITS 2
#define PATH_HISTORY_BUFFER_LENGTH 32
#define L_1 4
#define ALPHA 1.6
#define RESET_USEFUL_INTERVAL 512000
#define DEFAULT_PREDICTOR 16384
#define MAX_INDEX_BITS 12
#define NUM_COMPONENTS 12
#define DEFAULT_COUNTER_BITS 2

struct entry
{
    public:
    uint8_t ctr;
    uint16_t tag;
    uint8_t useful;
    entry()
    {
        ctr = (1 << (COUNTER_BITS-1));
        tag = 0;
        useful = 0;
    }
};

uint8_t INDEX_BITS[NUM_COMPONENTS] = {10, 10, 11, 11, 11, 11, 10, 10, 10, 10, 9, 9};
uint8_t TAG_BITS[NUM_COMPONENTS] = {7, 7, 8, 8, 9, 10, 11, 12, 12, 13, 14, 15};

struct tage
{
    int num_branches;
    uint8_t Default[DEFAULT_PREDICTOR];
    struct entry table[NUM_COMPONENTS][1 << MAX_INDEX_BITS];
    uint8_t global_history[1024],path_history[PATH_HISTORY_BUFFER_LENGTH];
    uint8_t use_alt_on_na;
    int comp_history_len[NUM_COMPONENTS];
    uint8_t pred,alt_pred,tage_pred;
    int pred_comp,alt_comp;
    bool weak;
    tage()
    {
        float ratio = 1;
        use_alt_on_na = 8;
        for (int i=DEFAULT_PREDICTOR-1;i>=0;i--)
            Default[i] = ((1 << (DEFAULT_COUNTER_BITS - 1))-1);
        num_branches = 0;
        for (int i=0;i<NUM_COMPONENTS;i++)
        {
            comp_history_len[i] = (int)((float)(ratio * L_1) + 0.5);
            ratio*= ALPHA;
        }
        tage_pred = 1;
    }
    uint64_t get_compressed_global_history(int inSize, int outSize)
    {
        uint64_t compressed_history = 0; 
        uint64_t temp = 0;
        int a = 0;
        for (int i = 0; i < inSize; i++)
        {
            if(a == outSize){
                compressed_history ^= temp;
                temp = 0;
                a = 0;
            }
            
            temp = (temp << 1) | global_history[i];
            a++;
        }
        compressed_history ^= temp;
        return compressed_history;
    }
    void make_hash(uint64_t &hash,int component,int size)
    {
        hash = hash & ((1<<size)-1);
        uint64_t hash1 = hash >> INDEX_BITS[component - 1];
        hash1 = ((hash1 << component) & ((1 << INDEX_BITS[component - 1]) - 1)) + (hash1 >> abs(INDEX_BITS[component - 1] - component));
        hash = (hash & ((1 << INDEX_BITS[component - 1]) - 1) ) ^ hash1;
        hash = ((hash << component) & ((1 << INDEX_BITS[component - 1]) - 1)) + (hash >> abs(INDEX_BITS[component - 1] - component));   
    }
    uint64_t get_path_hash(int component)
    {
        uint64_t hash = 0;
        int size;
        if (comp_history_len[component-1] > 16)
            size = 16;
        else
            size = comp_history_len[component-1];
        for (int i=PATH_HISTORY_BUFFER_LENGTH-1;i>=0;i--)
        {
            hash = (hash << 1) | path_history[i];
        }
        make_hash(hash,component,size);
        // hash = hash & ((1<<size)-1);
        // uint64_t hash1 = hash >> INDEX_BITS[component - 1];
        // hash1 = ((hash1 << component) & ((1 << INDEX_BITS[component - 1]) - 1)) + (hash1 >> abs(INDEX_BITS[component - 1] - component));
        // hash = (hash & ((1 << INDEX_BITS[component - 1]) - 1) ) ^ hash1;
        // hash = ((hash << component) & ((1 << INDEX_BITS[component - 1]) - 1)) + (hash >> abs(INDEX_BITS[component - 1] - component));
        return hash;
    }
    uint16_t get_val(uint64_t ip,int component,bool tag)
    {
        // uint64_t hash =get_compressed_global_history(comp_history_len[component - 1], TAG_BITS[component - 1]);
        // hash ^= get_compressed_global_history(comp_history_len[component - 1], TAG_BITS[component - 1] - 1);
        if (tag)
        {
            uint64_t hash =get_compressed_global_history(comp_history_len[component - 1], TAG_BITS[component - 1]);
            hash ^= get_compressed_global_history(comp_history_len[component - 1], TAG_BITS[component - 1] - 1);
            hash ^= ip;
            return (hash) & ((1 << TAG_BITS[component - 1]) - 1);
        }
        else
        {
            uint64_t path_hash = get_path_hash(component);
            uint64_t hash = get_compressed_global_history(comp_history_len[component - 1], INDEX_BITS[component - 1]);
            hash = hash ^ ip;
            hash = hash ^ ((ip >> (abs(INDEX_BITS[component - 1] - component) + 1)) ^ path_hash);
            return (hash) & ((1 << INDEX_BITS[component-1]) - 1);
        }
    }
    // uint16_t get_index(uint64_t ip, int component)
    // {
    //     uint64_t path_hash = get_path_hash(component);
    //     uint64_t hash = get_compressed_global_history(comp_history_len[component - 1], INDEX_BITS[component - 1]);
    //     hash = hash ^ ip;
    //     hash = hash ^ ((ip >> (abs(INDEX_BITS[component - 1] - component) + 1)) ^ path_hash);
    //     return (hash) & ((1 << INDEX_BITS[component-1]) - 1);
    // }

    // uint16_t get_tag(uint64_t ip, int component)
    // {
   
    //     uint64_t hash =get_compressed_global_history(comp_history_len[component - 1], TAG_BITS[component - 1]);
    //     hash ^= get_compressed_global_history(comp_history_len[component - 1], TAG_BITS[component - 1] - 1);
    //     hash ^= ip;
    //     return (hash) & ((1 << TAG_BITS[component - 1]) - 1);

    // }

    int get_match_below_n(uint64_t ip, int component)
    {
        for (int i = component ; i > 1; i--)
        {
            uint16_t index = get_val(ip, i-1,0);
            uint16_t tag = get_val(ip, i-1,1);

            if (table[i - 2][index].tag == tag) 
            {
                return i-1;
            }
        }

        return 0; 
    }
    bool is_weak(uint8_t comp,uint16_t index)
    {
        return abs(2 * table[comp - 1][index].ctr + 1 - (1 << COUNTER_BITS)) <= 1;
    }
    // int randm(int free){
        
    //     long int rand = random()%((1 << free)-1)+1;
    //     int x = int(log(rand)/log(2));
    //     return (free-x-1);
    // }
    void tage_get_pred(uint64_t ip, int comp,uint8_t &pred)
    {
        if (comp == 0)
        {
            uint16_t default_index = ip & (DEFAULT_PREDICTOR - 1);
            pred = !(Default[default_index] < (1 << (DEFAULT_COUNTER_BITS - 1)));
            return;
        }
        else
        {
            uint16_t pred_index = get_val(ip,comp,0);
            pred = !(table[comp-1][pred_index].ctr < (1 << (COUNTER_BITS - 1)));
            return;
        }
    }
    uint8_t tage_prediction(uint64_t ip)
    {
        pred_comp = get_match_below_n(ip, NUM_COMPONENTS + 1);
        alt_comp = get_match_below_n(ip,pred_comp);
        tage_get_pred(ip,pred_comp,pred);
        tage_get_pred(ip , alt_comp,alt_pred);
        if (pred_comp != 0)
        {
            uint16_t index = get_val(ip,pred_comp,0);
            weak = is_weak(pred_comp,index);
            if (use_alt_on_na > 8 && weak)
            {
                tage_pred = alt_pred;
            }
            else
            {
                tage_pred = pred;
            }
            return tage_pred;
        }
        else
        {
            tage_pred = pred;
            return tage_pred;
        }
    }
    void updatectr(uint8_t &ctr,int condition, int lower_bound, int upper_bound )
    {
        if (condition && ctr < upper_bound)
        {
            ctr++;
            return;
        }
        if (!condition && ctr > lower_bound)
        {
            ctr--;
            return;
        }
    }
    void update(uint64_t ip,uint8_t taken)
    {
        if (pred_comp == 0)
        {
            uint16_t index = ip & (DEFAULT_PREDICTOR - 1);
            updatectr(Default[index],taken,0,((1 << DEFAULT_COUNTER_BITS) - 1));
        }
        else
        {
            struct entry *etr = &table[pred_comp-1][get_val(ip,pred_comp,0)];
            uint8_t use = etr->useful;
            if (weak)
            {
                if (pred != alt_pred)
                    updatectr(use_alt_on_na,pred != taken, 0, 15);
            }
            updatectr(etr->ctr,taken,0,((1 << COUNTER_BITS) - 1));
            if (use == 0)
            {
                if (alt_comp > 0)
                    updatectr(table[alt_comp-1][get_val(ip,alt_comp,0)].ctr,taken,0,((1 << COUNTER_BITS) - 1));
                else
                    updatectr(Default[ip & (DEFAULT_PREDICTOR - 1)],taken,0,((1 << DEFAULT_COUNTER_BITS) - 1));
            }
            if (pred == taken && pred != alt_pred)
            {
                if (etr->useful < ((1 << USEFUL_BITS) - 1))
                    etr->useful++;
            }
            else if (use_alt_on_na < 8 && etr->useful > 0 && pred != alt_pred)
            {
                etr->useful--;
            }
        }
        if (tage_pred != taken)
        {
            long rand = random()& ((1 << (NUM_COMPONENTS - pred_comp - 1)) - 1);
            //rand = rand % 4;
            int start_comp = pred_comp + 1;
            if (rand & 1)
            {
                start_comp +=1;
                if (rand & 2)
                    start_comp++;
            }
            bool free = 0;
            //uint8_t list[NUM_COMPONENTS];
            for (int i = pred_comp+1 ;i <= NUM_COMPONENTS;i++ )
            {
                if (table[i - 1][get_val(ip, i,0)].useful == 0)
                {
                    //list[free]=i-1;
                    free = true;
                    if (i >= start_comp && start_comp <= NUM_COMPONENTS)
                    {
                        int k = get_val(ip, i,0);
                        table[i - 1][k].tag = get_val(ip, i,1);
                        table[i - 1][k].ctr = (1 << (COUNTER_BITS - 1));
                    }
                    break;
                }    
            }
            if (free == false && start_comp <= NUM_COMPONENTS)
            {
                // for (int i = start_comp ;i <= NUM_COMPONENTS;i++ )
                //     table[i - 1][get_index(ip, i)].useful--;
                int k = get_val(ip,start_comp,0);
                table[start_comp-1][k].useful = 0;
                table[start_comp - 1][k].tag = get_val(ip, start_comp,1);
                table[start_comp - 1][k].ctr = (1 << (COUNTER_BITS - 1));
            }
            // else
            // {
            //     int replace_comp = list[randm(free)];
            //     int index = get_index(ip,replace_comp+1);
            //     table[replace_comp][index].ctr = (1 << (COUNTER_BITS - 1));
            //     table[replace_comp][index].tag = get_tag(ip,replace_comp+1);
            // }
            // for (int i = start_comp; i <= NUM_COMPONENTS; i++)
            // {
            //     int k = get_index(ip, i);
            //     if (table[i - 1][k].useful == 0)
            //     {
            //         table[i - 1][k].tag = get_tag(ip, i);
            //         table[i - 1][k].ctr = (1 << (COUNTER_BITS - 1));
            //         break;
            //     }
            // }
        }
        for (int i = 1023; i > 0; i--)
            global_history[i] = global_history[i - 1];
        global_history[0] = taken;
        num_branches++;
        for (int i = 0;i < PATH_HISTORY_BUFFER_LENGTH - 1; i++)
            path_history[i+1] = path_history[i];
        path_history[0] = ip & 1;
        // if (num_branches % RESET_USEFUL_INTERVAL == 0)
        // {
            
        //     for (int i = 0; i < NUM_COMPONENTS; i++)
        //     {
        //         for (int j = 0; j < (1 << INDEX_BITS[i]); j++)
        //             table[i][j].useful &= 2;
        //     }
                
             
        // }
        // if (num_branches % 2*RESET_USEFUL_INTERVAL == 0)
        // {
        //     for (int i = 0; i < NUM_COMPONENTS; i++)
        //     {
        //         for (int j = 0; j < (1 << INDEX_BITS[i]); j++)
        //         {
        //            if(table[i][j].useful == 1 || table[i][j].useful == 2)
        //            { 
        //                 table[i][j].useful = 3 - table[i][j].useful;
        //            }
        //         }
        //     }
        //     num_branches = 0;
        // }
        if (num_branches == RESET_USEFUL_INTERVAL)
        {
            num_branches = 0;
            for (int i = 0; i < NUM_COMPONENTS; i++)
            {
                for (int j = 0; j < (1 << INDEX_BITS[i]); j++)
                    table[i][j].useful = table[i][j].useful>>1 ;
            }
        }
    }
};

