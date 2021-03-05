#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

const int NUM_OF_BYTES = 4;
const int BITS_IN_BYTE = 8;
const int TIME_TO_WAIT = 1;

typedef struct _interface {
    char mask[16];
    char ip[16];
} interface;

typedef struct _thread_data {
    char *start_addr;
    char *interface_addr;
    int am_to_process;
} thread_data;

thread_data *fill_structure(char *start_addr, char *interface_addr, int am_to_proc);

void print_this(char *ip) ;

int get_mask_number(const char *mask);

char *to_string(int *byte_arr);

int *to_byte_arr(const char *);

char *get_subnet_addr(char *mask, char *ip);

void increment_addr(int *byte_arr);

void increment_addr_by(int *byte_arr, int num);

void ping(interface an_interface);

void parallel_ping(int am_of_addr, char *subnet_addr, char *interface_addr);

void print_result();

void *thread_func(void *data) {
    thread_data *achieved_data = (thread_data *) data;
    char *interface_addr = achieved_data->interface_addr;
    int am_to_process = achieved_data->am_to_process;
    int *addr_bytes = to_byte_arr(achieved_data->start_addr);
    char command[50] = {};
    for (int i = 0; i < am_to_process; ++i) {
        sprintf(command, "ping -I %s -c 1 -q -W %d %s >> ./ping.txt",
                interface_addr, TIME_TO_WAIT, to_string(addr_bytes));
        system(command);
        increment_addr(addr_bytes);
    }
    free(achieved_data);
    pthread_exit(0);
}


int main() {
    puts("Working...");
    system("ifconfig | grep 'inet ' | grep -v '127.0.0.1'| awk '{print $4\" \"$2}' > tmp.txt");
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
        print_this(ifs[i].ip);
    }
    system("rm ping.txt");
    system("arp -n | grep -v 'incomplete' | grep -v 'Address'| awk '{print $1\"  \"$3\"  \"$5}' > arp.txt");
    print_result();
    puts("\nPress ENTER to finish...");
    getchar();
    return 0;
}


void ping(interface an_interface) {
    int mask_number = get_mask_number(an_interface.mask);
    int am_of_addr = pow(2, (BITS_IN_BYTE * NUM_OF_BYTES - mask_number)) - 2;
    char *subnet_addr = get_subnet_addr(an_interface.mask, an_interface.ip);
    parallel_ping(am_of_addr, subnet_addr, an_interface.ip);
}


void parallel_ping(int am_of_addr, char *subnet_addr, char *interface_addr) {
    int time_of_work = ceil(log2(am_of_addr));
    //printf("time: %d\n", time_of_work);
    int am_of_threads = (am_of_addr * TIME_TO_WAIT) / time_of_work;
    int am_for_thread = am_of_addr / am_of_threads;
    int delta = am_of_addr - am_for_thread * am_of_threads;
    int *curr_addr_bytes = to_byte_arr(subnet_addr);
    increment_addr(curr_addr_bytes);
    am_of_threads = delta ? (am_of_threads + 1) : am_of_threads;
    //printf("am: %d\n", am_of_threads);
    pthread_t *threads = (pthread_t *) calloc(am_of_threads, sizeof(pthread_t));
    for (int i = 0; i < (delta ? (am_of_threads - 1) : am_of_threads); ++i) {
        thread_data *data = fill_structure(to_string(curr_addr_bytes), interface_addr, am_for_thread);
        pthread_create(&threads[i], NULL, thread_func, (void *) data);
        increment_addr_by(curr_addr_bytes, am_for_thread);
    }
    if (delta) {
        thread_data *data = fill_structure(to_string(curr_addr_bytes), interface_addr, delta);
        pthread_create(&threads[am_of_threads - 1], NULL, thread_func, (void *) data);
    }
    for (int i = 0; i < am_of_threads; ++i) {
        pthread_join(threads[i], 0);
    }
}


thread_data *fill_structure(char *start_addr, char *interface_addr, int am_to_proc) {
    thread_data *data = (thread_data *) calloc(1, sizeof(thread_data));
    data->start_addr = start_addr;
    data->interface_addr = interface_addr;
    data->am_to_process = am_to_proc;
    return data;
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

void increment_addr_by(int *byte_arr, int num) {
    for (int i = 0; i < num; ++i) {
        increment_addr(byte_arr);
    }
}

void print_result() {
    FILE *res = fopen("arp.txt", "r");
    char ip[16] = {};
    char mac[18] = {};
    char an_interface[20] = {};
    printf("\n\n%-16s        %-17s        %-20s        %-20s\n\n", "IP-address:", "MAC-address:", "Name:", "Interface:");
    while (fscanf(res, "%s %s %s", ip, mac, an_interface) != EOF) {
        char command[100];
        sprintf(command, "nslookup %s | grep 'name' | awk ' NR==1 {print $4}' > ./names.txt", ip);
        system(command);
        char name[100];
        FILE *file = fopen("./names.txt", "r");
        fscanf(file, "%s", name);
        printf("%-16s        %-17s        %-20s        %-20s\n", ip, mac, name, an_interface);
        fclose(file);
    }
    system("rm ./names.txt");
    fclose(res);
}

void print_this(char *ip) {
    printf("\nYour IP-address: %s", ip);
    system("ifconfig | grep 'ether '| awk '{print \"Your MAC-address: \"$2}'");
    char command[100];
    sprintf(command, "nslookup %s | grep 'name' | awk 'NR==1 {print \"Your computer name: \"$4}'", ip);
    system(command);
}

