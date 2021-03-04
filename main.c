#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>


typedef struct _interface {
    char mask[16];
    char ip[16];
} interface;

int get_mask_number(const char *mask);

char *to_string(int *byte_arr);

int *to_byte_arr(const char *);

char *get_subnet_addr(char *mask, char *ip);

void increment_addr(int *byte_arr);

void ping(interface an_interface);

const int NUM_OF_BYTES = 4;
const int BITS_IN_BYTE = 8;


void *thread_func(void *thread_data) {
    int x = system((char *) thread_data);
    if (!x) {
        puts(thread_data);
    }
    free(thread_data);
    pthread_exit(0);
}


int main() {
    system("> ./ping.txt");
    system("ifconfig | grep 'inet ' | grep -v '127'| awk '{print $4\" \"$2}' > tmp.txt");
    system("wc -l tmp.txt | awk '{print $1}' > log.txt; cat tmp.txt >> log.txt; rm tmp.txt");
    FILE *tmp = fopen("log.txt", "r");
    int num_of_ifs = 0;
    fscanf(tmp, "%d", &num_of_ifs);
    interface *ifs = (interface *) calloc(num_of_ifs, sizeof(interface));
    for (int i = 0; i < num_of_ifs; ++i) {
        fscanf(tmp, "%s%s", ifs[i].mask, ifs[i].ip);
    }
    fclose(tmp);
    system("rm log.txt");
    for (int i = 0; i < num_of_ifs; ++i) {
        ping(ifs[i]);
    }
    system("arp -n | grep -v 'incomplete' > arp.txt");
    return 0;
}

char *get_subnet_addr(char *mask, char *ip) {
    int *mask_to_byte = to_byte_arr(mask);
    int *ip_to_byte = to_byte_arr(ip);
    int *subnet_byte = (int *) calloc(NUM_OF_BYTES, sizeof(int));
    for (int i = 0; i < NUM_OF_BYTES; i++) {
        subnet_byte[i] = ip_to_byte[i] & mask_to_byte[i];
    }
    return to_string(subnet_byte);
}

int get_mask_number(const char *mask) {
    int *arr = to_byte_arr(mask);
    int mask_number = 0;
    for (int i = 0; i < NUM_OF_BYTES; i++) {
        while (arr[i] > 0) {
            if (arr[i] % 2) {
                mask_number++;
            }
            arr[i] /= 2;
        }
    }
    return mask_number;
}

int *to_byte_arr(const char *input) {
    char buffer[4];
    int *arr = (int *) calloc(NUM_OF_BYTES, sizeof(int));
    int byte_num = 0;
    int index = 0;
    for (int i = 0; input[i]; i++) {
        if (input[i] == '.') {
            arr[byte_num++] = atoi(buffer);
            index = 0;
            continue;
        }
        buffer[index++] = input[i];
        buffer[index] = '\0';
    }
    arr[byte_num] = atoi(buffer);
    //printf("%d.%d.%d.%d\n", arr[0], arr[1], arr[2], arr[3]);
    return arr;
}

char *to_string(int *byte_arr) {
    // first 3 is max len of number + 3 dots + 1 \0
    char *result = (char *) calloc((NUM_OF_BYTES * 3 + 3 + 1), sizeof(char));
    sprintf(result, "%d.%d.%d.%d", byte_arr[0], byte_arr[1], byte_arr[2], byte_arr[3]);
    return result;
}

void ping(interface an_interface) {
    int *subnet_to_bytes = to_byte_arr(get_subnet_addr(an_interface.mask, an_interface.ip));
    int mask_number = get_mask_number(an_interface.mask);
    increment_addr(subnet_to_bytes);
    pthread_t threads[255] = {};
    int counter = 0;
    for (int i = 0; i < (pow(2, (BITS_IN_BYTE * NUM_OF_BYTES - mask_number)) - 2); i++) {
        char *command = (char *) calloc(100, sizeof(char));
        sprintf(command, "ping -I %s -c 1 -q -W 5 %s >> ./ping.txt", an_interface.ip, to_string(subnet_to_bytes));
        pthread_create(&threads[counter++], NULL, thread_func, command);
        increment_addr(subnet_to_bytes);
    }
    for (int i = 0; i < 255; i++) {
        pthread_join(threads[i], NULL);
    }
}

void increment_addr(int *byte_arr) {
    int is_incremented = 0;
    int offset = 1;
    while (!is_incremented) {
        if (byte_arr[NUM_OF_BYTES - offset] < 255) {
            byte_arr[NUM_OF_BYTES - offset]++;
            is_incremented = 1;
        } else {
            byte_arr[NUM_OF_BYTES - offset] = 0;
            if (offset < NUM_OF_BYTES - 1)
                offset++;
            else return;
        }
    }
}
