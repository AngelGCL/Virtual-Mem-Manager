/*
This is a virtual memory management implementation for the Operating System course at University of Puerto Rico 
Mayagüez Campus. 
It is a simple implementation, does not include swap in and swap out operations.
It only takes into consideration LRU and FIFO scheduling

How To Use: Once compiled, to run the program, you need to give it a parameter -a [FIFO or LRU] and -o [output file]
NOTE: operations.in file has the operations that will be executed by the VMM and the pages that are used (op|page).
AUTHOR: AngelGCL
*/
#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>

/*
=====================================================================
Page Table Entries = 16
Frames = 4
Atributes: PRESENT, MODIFIED, REFERENCED
Log every op
operations.in line -> [read(0) or write(1), virtual page #]
*NOTE:PAGES CAN BE A,B,C,D, ETC
*FRAMES ARE FILLED WITH THE PAGE
FIFO EXAMPLE:

frame 1 |A|A|A|A|A|D|D|D|D|C|C|

frame 2 |=|B|B|B|B|B|A|A|A|A|A|

frame 3 |=|=|C|C|C|C|C|C|B|B|B|
=====================================================================
*/

static int lruFrame;
static int timer;
static int oldest;

static struct statistics {
    
    int unmaps; //victim replace
    int maps; //write to
    int writes; //writes
    int reads; //reads
    int ins;
    int outs;

} stats;

struct pages{

   int modified;//when it is written
   int referenced;//aumenta
   int present;//in frame?
   int frame;//frame number
};

struct frame{
    int page_num;
    int ref_time;
} frames[4];

struct pages PT[16];

void initializer(){

    int i;
    for (i=0; i<4; i++){
        frames[i].page_num = -1;
        frames[i].ref_time = 0;
    }
    for (i=0; i<16; i++){
        PT[i].modified = 0;
        PT[i].referenced = 0;
        PT[i].present = 0;
        PT[i].frame = -1;
    }
    stats.maps = 0;
    stats.reads = 0;
    stats.unmaps = 0;
    stats.writes = 0;
    stats.ins = 0;
    stats.outs = 0;
}

void exchangeFIFO(int page){
    if(PT[page].present != 1){
        PT[frames[oldest].page_num].frame = -1;
        PT[frames[oldest].page_num].present = 0;
        frames[oldest].page_num = page;
        stats.unmaps++;
        stats.maps++;
        stats.ins++;
        stats.outs++;
        PT[page].frame = oldest;
        PT[page].present = 1;
    }
    timer++;
    oldest++;
    if(oldest = 4){oldest = 0;}
}
int FIFO(int op, int page){

    if (op == 1){
        stats.writes++;
        PT[page].modified = 1;
    }
    if(op == 0){
        stats.reads++;
        PT[page].referenced = 1;
    }

    int i;
    /* Initially empty */
    if (timer < 4 && PT[page].present != 1){
        timer++;
        for(i=0; i<4; i++){
            if(frames[i].page_num == -1){
                frames[i].page_num = page;
                PT[page].present = 1;
                PT[page].frame = i;
                stats.maps++;
                return 0;
            }
        }
    }
    else{
        exchangeFIFO(page);
        return 0;
    }
    return -1;
}
/*
==============================================
Initial state: frames have zero uses
Second state: all frames have been used once
third state: frames will be used as they are referenced changing the one with the oldest reference
==============================================
*/

int findMIN(){
    int min = __INT_MAX__;
    int i;
    int least=0;
    for(i=0; i<4; i++){
        if(min>frames[i].ref_time){
            min = frames[i].ref_time;
            least = i;
        }
    }
    return least;
}
void exchangeLRU(int page){
    lruFrame = findMIN();
    if(PT[page].present == 0){
        PT[frames[lruFrame].page_num].frame = -1;
        PT[frames[lruFrame].page_num].present = 0;
        frames[lruFrame].page_num = page;
        frames[lruFrame].ref_time = timer;
        stats.unmaps++;
        stats.maps++;
        stats.ins++;
        stats.outs++;
        PT[page].frame = lruFrame;
        PT[page].present = 1;
    }
    else{
        //if inside a frame
        frames[PT[page].frame-1].ref_time = timer;
    }
}

int LRU(int op, int page){
    if (op == 1){
        stats.writes++;
        PT[page].modified = 1;
    }else{
        stats.reads++;
        PT[page].referenced = 1;
    }

    if (timer < 4 && PT[page].present != 1){
        int i;
        for(i=0; i<4; i++){
            if(frames[i].page_num == -1){
                frames[i].page_num = page;
                frames[i].ref_time = timer;
                PT[page].present = 1;
                PT[page].frame = i;
                stats.maps++;
                timer++;
                return 0;
            }
        }
    }
    else{
        exchangeLRU(page);
        timer++;
        return 0;
    }
    return -1;
}

int main(int argc, char *argv[]){
    
    oldest = 0;
    timer = 0;
    lruFrame = 0;
    initializer();

    FILE *archivo,*salida;
    char *output;
    archivo = fopen("operations.in","r");
    
    if(archivo == 0){
        printf("archivo no existe");
        getchar();
        getchar();
        return 0;
    }    
    
    int operation,pageNumber;
    
    int options = 0;
    char *func = NULL;
    if(argc == 5){
        while((options = getopt(argc, argv, "a:o:")) != -1){
            switch (options){
                case 'a':
                    func = optarg;
                    break;
                case 'o':
                    output = optarg;
                    salida = fopen(output,"w");
                    break;
            }
        }
    }
    else{
        printf("Invalid command\n");
        return EXIT_FAILURE;
    }

    while(fscanf(archivo,"%d%d",&operation,&pageNumber) == 2){
        //Imprimir en consola
        if(strcmp(func, "LRU") == 0)                                                    
            LRU(operation, pageNumber);
        else if((strcmp(func, "FIFO")) == 0)
            FIFO(operation, pageNumber);
        else{
            LRU(operation, pageNumber);
            FIFO(operation, pageNumber);
        }

        

        if(strcmp(output, "standard") == 0){
            printf("frames\n Frame 1 = %d :: \tFrame 2: %d :: \tFrame 3: %d :: \tFrame 4: %d\n", frames[0].page_num, frames[1].page_num, 
                    frames[2].page_num, frames[3].page_num);
        }
        else{
            fprintf(salida, "frames\n Frame 1 = %d :: \tFrame 2: %d :: \tFrame 3: %d :: \tFrame 4: %d\n", frames[0].page_num, frames[1].page_num, 
                    frames[2].page_num, frames[3].page_num);
        }
    }
    int inst_count = stats.reads + stats.writes;
    int totalcost = ((stats.maps + stats.outs)*400) + ((stats.ins + stats.outs)*3000) + (inst_count);
    if(strcmp(output, "standard") == 0){
        printf("Numero de operaciones %d U=%d M=%d I=%d O=%d ===> %d\n", inst_count, stats.unmaps, stats.maps, 
            stats.ins, stats.outs, totalcost);
    }
    else{
    fprintf(salida, "Numero de operaciones %d U=%d M=%d I=%d O=%d ===> %d\n", inst_count, stats.unmaps, stats.maps, 
        stats.ins, stats.outs, totalcost);                                                    
    } 
    fclose(salida);
    getchar();
    getchar();
    return 0;
}