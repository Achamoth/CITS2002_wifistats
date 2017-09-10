//10 September 2017, Ammar Abu Shamleh

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

//Struct for storing information relating to one device
typedef struct Device {
    unsigned long long address;
    unsigned long long bytesTransmitted;
    unsigned long long bytesReceived;
} Device;

//Struct for storing information relating to one vendor
typedef struct Vendor {
    int numAddresses;
    unsigned long long *addresses;
    char *name;
    unsigned long long bytesTransmitted;
    unsigned long long bytesReceived;
} Vendor;

//Given a hexadecimal character, return the number it represents as an int
int hexCharToNum(char c) {
    switch (c) {
        case 'a':
        return 10;
        break;

        case 'b':
        return 11;
        break;

        case 'c':
        return 12;
        break;

        case 'd':
        return 13;
        break;

        case 'e':
        return 14;
        break;

        case 'f':
        return 15;
        break;

        case 'A':
        return 10;
        break;

        case 'B':
        return 11;
        break;

        case 'C':
        return 12;
        break;

        case 'D':
        return 13;
        break;

        case 'E':
        return 14;
        break;

        case 'F':
        return 15;
        break;

        default:
        return c - '0';
        break;
    }
}

//Given a string containing a hexadecimal MAC address, convert it to a decimal number, and return it
unsigned long long hexToDec(char *hex) {
    //Ordinal position starts at 12, so power starts at 11
    int power = 11;
    unsigned long long result = 0;

    //String length should be 17
    for(int i=0; i<17; i++) {
        //Skip colons
        if(hex[i] == ':' || hex[i] == '-') {
            continue;
        }
        //Convert numbers
        else {
            unsigned long long curNum = hexCharToNum(hex[i]) * pow(16,(power--));
            result += curNum;
        }
    }

    //Return number
    return result;
}

//Given a string containing a hexadecimal vendor address, convert it to a decimal number, and return it
unsigned long long venToDec(char *hex) {
    //Ordinal position starts at 6, so power starts at 5
    int power = 5;
    unsigned long long result = 0;

    //String length should be 8
    for(int i=0; i<8; i++) {
        //Skip colons
        if(hex[i] == ':' || hex[i] == '-') {
            continue;
        }
        //Convert numbers
        else {
            unsigned long long curNum = hexCharToNum(hex[i]) * pow(16,(power--));
            result += curNum;
        }
    }

    //Return number
    return result;
}

//Given a 6 byte (i.e. 48 bit) address as a decimal number, convert it to a hex addres and return the hex representation (as a string)
//i.e. ff:ff:ff:ff:ff:ff is the return format
char *decToHex(unsigned long long decAddress) {
    //Set up char array for storing result
    char *result = (char *) malloc(sizeof(char) * 18);
    for(int i=0;i<18; i++) result[i] = '0';
    result[2] = ':';
    result[5] = ':';
    result[8] = ':';
    result[11] = ':';
    result[14] = ':';
    result[17] = '\0';

    //Convert number
    int index = 16;
    while(decAddress != 0) {
        //Work out current hex digit
        int curDigit = decAddress % 16;
        switch (curDigit) {
            case 10:
                result[index--] = 'a';
                break;
            case 11:
                result[index--] = 'b';
                break;
            case 12:
                result[index--] = 'c';
                break;
            case 13:
                result[index--] = 'd';
                break;
            case 14:
                result[index--] = 'e';
                break;
            case 15:
                result[index--] = 'f';
                break;
            default:
                result[index--] = curDigit + '0';
                break;
        }

        //Divide number by 16
        decAddress /= 16;

        //Ensure index moves correctly
        if(index==14 || index==11 || index==8 || index==5 || index==2) {
            index--;
        }
    }
    return result;
}

//Given a 3 byte (i.e. 24 bit) address as a decimal number, convert it to a hex addres and return the hex representation (as a string)
//i.e. ff:ff:ff is the return format
char *decToVen(unsigned long long decAddress) {
    //Set up char array for storing result
    char *result = (char *) malloc(sizeof(char) * 9);
    for(int i=0;i<9; i++) result[i] = '0';
    result[2] = ':';
    result[5] = ':';
    result[8] = '\0';

    //If number is 16777215 (i.e. ff:ff:ff), then the vendor is unkown
    if(decAddress == 16777215) {
        for(int i=0; i<8; i++) {
            result[i] = '?';
        }
        result[2] = ':';
        result[5] = ':';
        return result;
    }

    //Convert number
    int index = 7;
    while(decAddress != 0) {
        //Work out current hex digit
        int curDigit = decAddress % 16;
        switch (curDigit) {
            case 10:
                result[index--] = 'a';
                break;
            case 11:
                result[index--] = 'b';
                break;
            case 12:
                result[index--] = 'c';
                break;
            case 13:
                result[index--] = 'd';
                break;
            case 14:
                result[index--] = 'e';
                break;
            case 15:
                result[index--] = 'f';
                break;
            default:
                result[index--] = curDigit + '0';
                break;
        }

        //Divide number by 16
        decAddress /= 16;

        //Ensure index moves correctly
        if(index==5 || index==2) {
            index--;
        }
    }
    return result;
}

/**
    Open and read in the OUIfile specified by filename
    Read all vendor data into struct pointer, and return it
    Update the pointer 'numVendorsFound' to inform the calling context of the number of unique vendors stored in the struct pointer
    Return the struct pointer
**/
Vendor *readOUIFile(char *filename, int *numVendorsFound) {
    //Continually update number of vendors found
    int numVendors = 0;

    //Allocate memory for vendors
    Vendor *vendors = (Vendor *) malloc(sizeof(Vendor));

    //Open file
    FILE *fin = fopen(filename, "r");
    if(fin == NULL) {
        //Error opening file
        fprintf(stderr, "Couldn't open file \'%s\'\n", filename);
        exit(EXIT_FAILURE);
    }

    //Read file line by line
    char line[BUFSIZ];
    while(fgets(line, BUFSIZ, fin) != NULL) {
        //Parse line
        char address[9];
        sscanf(line, "%s", address);

        //Read vendor name
        int len = strlen(line);
        //Allocate space for vendor name
        char vendorName[len-9];
        //Copy line (minus the address) into vendor name
        strcpy(vendorName, line+9);
        //Remove newline character from end of vendor name
        vendorName[len-10] = '\0';

        //Convert vendor address to a decimal number
        unsigned long long decAddress = venToDec(address);

        //Check if there is already a vendor with this name in the struct array
        if(numVendors > 0 && !strcmp(vendors[numVendors-1].name, vendorName)) {
            //There is a match; vendor already exists in struct. Just add new address to it
            vendors[numVendors-1].addresses = (unsigned long long *) realloc(vendors[numVendors-1].addresses, sizeof(unsigned long long)*(vendors[numVendors-1].numAddresses+1));
            vendors[numVendors-1].addresses[vendors[numVendors-1].numAddresses++] = decAddress;
        }

        //If not, create a new entry
        else {
            vendors[numVendors].numAddresses = 0;
            vendors[numVendors].addresses = (unsigned long long *) malloc(sizeof(unsigned long long) * 1);
            vendors[numVendors].addresses[vendors[numVendors].numAddresses++] = decAddress;
            vendors[numVendors].name = (char *) malloc(sizeof(char) * len-9);
            strcpy(vendors[numVendors].name,vendorName);
            vendors[numVendors].bytesTransmitted = 0;
            vendors[numVendors++].bytesReceived = 0;
            vendors = (Vendor *) realloc(vendors, sizeof(Vendor) * (numVendors+1));
        }
    }

    //Add a vendor at the end with the value 0, to represent unknown vendors
    Vendor v;
    v.numAddresses = 1;
    v.addresses = (unsigned long long *) malloc(sizeof (unsigned long long));
    v.addresses[0] = 16777215;
    v.name = "UNKNOWN VENDOR";
    v.bytesReceived = 0;
    v.bytesTransmitted = 0;
    vendors[numVendors++] = v;

    //Return number of vendors found
    *numVendorsFound = numVendors;
    return vendors;
}

/***
    Open and read in the packets file specified by filename
    Read all device data into an array of structs, and record the number of bytes transmitted and received for each device
    Return a pointer to the array of structs (pointer to Device struct)
    Also record the number of unique devices found, and return this value (by dereferencing and updating the int pointer given as the second argument)
***/
Device *readPacketsFile(char *filename, int *numDevices) {
    //Initialize data structure for storing device data
    Device *devices = (Device *) malloc(sizeof(Device));

    //Count number of devices found
    int numDevicesFound = 0;

    //Open file
    FILE *fin = fopen(filename, "r");
    if(fin == NULL) {
        //Error opening file
        fprintf(stderr, "Couldn't open file \'%s\'\n", filename);
        exit(EXIT_FAILURE);
    }

    //Read file line by line
    char line[BUFSIZ];
    while(fgets(line, BUFSIZ, fin) != NULL) {
        //Parse line
        char timestamp[100];
        char transmitter[18];
        char recipient[18];
        int numBytes;
        sscanf(line, "%s %s %s %d", timestamp, transmitter, recipient, &numBytes);

        //Convert transmitter and receiver addresses to decimal values
        unsigned long long transmitterDec = hexToDec(transmitter);
        unsigned long long recipientDec = hexToDec(recipient);

        //If the recipient is the broadcast address (ff:ff:ff:ff), ignore it
        if(recipientDec == 281474976710655) continue;

        //Check whether or not these devices have been encountered before
        bool transmitterFound = false;
        bool recipientFound = false;
        for(int i=0; i<numDevicesFound; i++) {
            //Check against current device
            if(devices[i].address == transmitterDec) {
                //Update number of transmitted bytes
                devices[i].bytesTransmitted += numBytes;
                //Mark transmitter as found
                transmitterFound = true;
            }
            else if(devices[i].address == recipientDec) {
                //Update number of received bytes
                devices[i].bytesReceived += numBytes;
                //Mark recipient as found
                recipientFound = true;
            }
            //If both transmitter and recipient have been found, exit the loop
            if(transmitterFound && recipientFound) {
                break;
            }
        }

        //If either of the devices are new, add a new device to the array of structs
        if(!transmitterFound) {
            devices[numDevicesFound].address = transmitterDec;
            devices[numDevicesFound].bytesTransmitted = numBytes;
            devices[numDevicesFound++].bytesReceived = 0;
            //Reallocate more memory
            devices = (Device *) realloc(devices, sizeof(Device) * (numDevicesFound+1));
        }
        if(!recipientFound) {
            devices[numDevicesFound].address = recipientDec;
            devices[numDevicesFound].bytesTransmitted = 0;
            devices[numDevicesFound++].bytesReceived = numBytes;
            //Reallocate more memory
            devices = (Device *) realloc(devices, sizeof(Device) * (numDevicesFound+1));
        }
    }

    //Return pointer to device struct array
    *numDevices = numDevicesFound;
    return devices;
}

/***
    Print all device data to stdout
    @param devices      Array of Device struct variables, containing all transmission and receipt data
    @param numDevices   The number of devices in the array
    @param what         A character (either 't' or 'r') indicating whether the user wants to print transmission or receipt data
    Doesn't print data for devices with 0 bytes transmitted ('t') or received ('r')
    Prints data in descending numerical order according to bytes transmitted ('t') or received ('r')
    For devices with the same numerical values, prints in ascending alphabetic order according to hex address
***/
void printDeviceData(Device *devices, int numDevices, char what) {
    //Print temporary data to file
    FILE *fout = fopen("temp", "w+");
    if(fout == NULL) {
        //Couldn't open file
        fprintf(stderr, "Couldn't open file for printing\n");
        exit(EXIT_FAILURE);
    }

    //Loop over all devices
    for(int i=0; i<numDevices; i++) {
        //Don't print anything for devices that didn't transmit/receive data
        if(what == 't' && devices[i].bytesTransmitted == 0)  continue;
        if(what == 'r' && devices[i].bytesReceived == 0) continue;

        //Convert decimal address to hexadecimal address (as a string)
        char *deviceAddress = decToHex(devices[i].address);
        //Print transmission/receipt data
        if(what == 't') {
            fprintf(fout, "%s\t%llu\n", deviceAddress, devices[i].bytesTransmitted);
        }
        else {
            fprintf(fout, "%s\t%llu\n", deviceAddress, devices[i].bytesReceived);
        }
        free(deviceAddress);
    }

    //Close file
    fclose(fout);

    //Now, sort contents of file using /usr/bin/sort
    //First, fork process
    pid_t npid, pid, wpid;
    pid = getpid();
    int success = fork();
    if(success < 0) {
        //Creation of child process was unsuccessful
        fprintf(stderr, "Couldn't create child process\n");
        exit(EXIT_FAILURE);
    }
    //Get process id
    npid = getpid();
    if(pid != npid) {
        //Child process. Execute sort on temp file (to sort alphabetically)
        char progName[] = "./sort";
        char arg1[] = "temp";
        char arg2[] = "-o";
        char arg3[] = "temp2";
        char *args[5];
        args[0] = progName;
        args[1] = arg1;
        args[2] = arg2;
        args[3] = arg3;
        args[4] = NULL;
        execv("/usr/bin/sort", args);
    }
    else {
        //Parent process. Wait for child to complete
        int status;
        wpid = wait(&status);
        if(!WIFEXITED(status)) {
            //Child did not exit normally
            fprintf(stderr, "Child process did not exit successfully\n");
            exit(EXIT_FAILURE);
        }
    }

    //Remove temp file
    remove("temp");

    //Now, fork process again
    pid = getpid();
    success = fork();
    if(success < 0) {
        //Creation of child process was unsuccessful
        fprintf(stderr, "Couldn't create child process\n");
        exit(EXIT_FAILURE);
    }
    //Get process id
    npid = getpid();
    if(pid != npid) {
        //Child process. Execute sort on temp2 file (to perform stable numerical sort on bytes, in descending order)
        char progName[] = "./sort";
        char arg1[] = "-r";
        char arg2[] = "-n";
        char arg3[] = "-k2";
        char arg4[] = "-s";
        char arg5[] = "-o";
        char arg6[] = "temp3";
        char arg7[] = "temp2";
        char *args[9];
        args[0] = progName;
        args[1] = arg1;
        args[2] = arg2;
        args[3] = arg3;
        args[4] = arg4;
        args[5] = arg5;
        args[6] = arg6;
        args[7] = arg7;
        args[8] = NULL;
        execv("/usr/bin/sort", args);
    }
    else {
        //Parent process. Wait for child to complete
        int status;
        wpid = wait(&status);
        if(!WIFEXITED(status)) {
            //Child did not exit normally
            fprintf(stderr, "Child process did not exit successfully\n");
            exit(EXIT_FAILURE);
        }
    }

    //Remove temp2 file
    remove("temp2");

    //Print contents of temp3 file to stdout
    FILE *fin = fopen("temp3", "r");
    if(fin == NULL) {
        fprintf(stderr, "Couldn't open temp file for printing\n");
        exit(EXIT_FAILURE);
    }
    char line[BUFSIZ];
    while(fgets(line, BUFSIZ, fin) != NULL) {
        //Print each line to stdout
        printf("%s", line);
    }

    //Remove temp3 file
    remove("temp3");
}

//Analyses the device data to calculate received and transmitted bytes for each vendor
void analyseDeviceData(Device *devices, int numDevices, Vendor *vendors, int numVendors) {
    //Loop through all devices
    for(int i=0; i<numDevices; i++) {
        bool match = false;
        //Loop through all vendors
        for(int j=0; j<numVendors-1; j++) {
            //Loop through all vendor addresses
            for(int k=0; k<vendors[j].numAddresses; k++) {
                //Check device address against vendor address
                unsigned long long curOUI = devices[i].address >> 24;
                if(curOUI == vendors[j].addresses[k]) {
                    //Vendor is a match. Update vendor's byte counts
                    vendors[j].bytesTransmitted += devices[i].bytesTransmitted;
                    vendors[j].bytesReceived += devices[i].bytesReceived;
                    //Break out of loop
                    match = true;
                    j = numVendors-1;
                    break;
                }
            }
        }
        if(!match) {
            //No vendor found. Record bytes under unknown vendor
            vendors[numVendors-1].bytesTransmitted += devices[i].bytesTransmitted;
            vendors[numVendors-1].bytesReceived += devices[i].bytesReceived;
        }
    }
}

/***
    Print all vendor data to stdout
    @param vendors      Array of Vendor struct variables, containing all transmission and receipt data
    @param numDevices   The number of vendors in the array
    @param what         A character (either 't' or 'r') indicating whether the user wants to print transmission or receipt data
    Doesn't print data for vendors with 0 bytes transmitted ('t') or received ('r')
    Prints data in descending numerical order according to bytes transmitted ('t') or received ('r')
    For vendors with the same numerical values, prints in ascending alphabetic order according to hex address
***/
void printVendorData(Vendor *vendors, int numVendors, char what) {
    //Print temporary data to file
    FILE *fout = fopen("temp", "w+");
    if(fout == NULL) {
        //Couldn't open file
        fprintf(stderr, "Couldn't open file for printing\n");
        exit(EXIT_FAILURE);
    }

    //Loop over all vendors
    for(int i=0; i<numVendors; i++) {
        //Convert vendor address back to hex
        char *vendorAddress = decToVen(vendors[i].addresses[0]);
        //Print data (excluding any 0 values)
        if(what == 't' && vendors[i].bytesTransmitted != 0)
            fprintf(fout, "%s\t%s\t%llu\n", vendorAddress, vendors[i].name, vendors[i].bytesTransmitted);
        else if(what == 'r' && vendors[i].bytesReceived != 0)
            fprintf(fout, "%s\t%s\t%llu\n", vendorAddress, vendors[i].name, vendors[i].bytesReceived);
        //Free vendorAddress
        free(vendorAddress);
    }

    //Close file
    fclose(fout);

    //Now, sort contents of file using /usr/bin/sort
    //First, fork process
    pid_t npid, pid, wpid;
    pid = getpid();
    int success = fork();
    if(success < 0) {
        //Creation of child process was unsuccessful
        fprintf(stderr, "Couldn't create child process\n");
        exit(EXIT_FAILURE);
    }
    //Get process id
    npid = getpid();
    if(pid != npid) {
        //Child process. Execute sort on temp file (to sort alphabetically)
        char progName[] = "./sort";
        char arg1[] = "temp";
        char arg2[] = "-o";
        char arg3[] = "temp2";
        char *args[5];
        args[0] = progName;
        args[1] = arg1;
        args[2] = arg2;
        args[3] = arg3;
        args[4] = NULL;
        execv("/usr/bin/sort", args);
    }
    else {
        //Parent process. Wait for child to complete
        int status;
        wpid = wait(&status);
        if(!WIFEXITED(status)) {
            //Child did not exit normally
            fprintf(stderr, "Child process did not exit successfully\n");
            exit(EXIT_FAILURE);
        }
    }

    //Remove temp file
    remove("temp");

    //Now, fork process again
    pid = getpid();
    success = fork();
    if(success < 0) {
        //Creation of child process was unsuccessful
        fprintf(stderr, "Couldn't create child process\n");
        exit(EXIT_FAILURE);
    }
    //Get process id
    npid = getpid();
    if(pid != npid) {
        //Child process. Execute sort on temp2 file (to perform stable numerical sort on bytes, in descending order)
        char progName[] = "./sort";
        char arg1[] = "-r";
        char arg2[] = "-n";
        char arg3[] = "-k3";
        char arg4[] = "-t";
        char arg5[] = "\t";
        char arg6[] = "-o";
        char arg7[] = "temp3";
        char arg8[] = "temp2";
        char arg9[] = "-s";
        char *args[11];
        args[0] = progName;
        args[1] = arg1;
        args[2] = arg2;
        args[3] = arg3;
        args[4] = arg4;
        args[5] = arg5;
        args[6] = arg6;
        args[7] = arg7;
        args[8] = arg8;
        args[9] = arg9;
        args[10] = NULL;
        execv("/usr/bin/sort", args);
    }
    else {
        //Parent process. Wait for child to complete
        int status;
        wpid = wait(&status);
        if(!WIFEXITED(status)) {
            //Child did not exit normally
            fprintf(stderr, "Child process did not exit successfully\n");
            exit(EXIT_FAILURE);
        }
    }

    //Remove temp2 file
    remove("temp2");

    //Print contents of temp3 file to stdout
    FILE *fin = fopen("temp3", "r");
    if(fin == NULL) {
        fprintf(stderr, "Couldn't open temp file for printing\n");
        exit(EXIT_FAILURE);
    }
    char line[BUFSIZ];
    while(fgets(line, BUFSIZ, fin) != NULL) {
        //Print each line to stdout
        printf("%s", line);
    }

    //Remove temp3 file
    remove("temp3");
}

int main(int argc, char **argv) {
    //Check command line arguments first
    if(argc != 3 && argc != 4) {
        //Incorrect number of command line arguments
        fprintf(stderr, "%s requires either 2 or 3 command line arguments. %d provided\n", argv[0], argc-1);
        exit(EXIT_FAILURE);
    }

    //User is providing an OUIfile, and wants vendor data (rather than individual device data)
    if(argc == 4) {
        //Check whether user is after transmission or receipt data
        bool transmit = (argv[1][0] == 't');
        if(!transmit && argv[1][0] != 'r') {
            //User's first command line argument is invalid
            fprintf(stderr, "%s requires \'t\' or \'r\' as the first command line argument\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        //Read in OUIfile, if the user has specified one
        int numVendors = 0;
        Vendor *vendors = readOUIFile(argv[3], &numVendors);

        //Read data from packets file into device struct array
        int numDevices = 0;
        Device *devices = readPacketsFile(argv[2], &numDevices);

        //Aggregate devices data into vendor struct array
        analyseDeviceData(devices, numDevices, vendors, numVendors);

        //Sort vendor data and print
        printVendorData(vendors, numVendors, argv[1][0]);
    }

    //User just wants device data; ignore vendor information
    else if(argc == 3) {
        //Check whether user is after transmission or receipt data
        bool transmit = (argv[1][0] == 't');
        if(!transmit && argv[1][0] != 'r') {
            //User's first command line argument is invalid
            fprintf(stderr, "%s requires \'t\' or \'r\' as the first command line argument\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        //Read data from packets file into device struct array
        int numDevices = 0;
        Device *devices = readPacketsFile(argv[2], &numDevices);

        //Sort device data and print
        printDeviceData(devices, numDevices, argv[1][0]);
    }
}
