#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>
#include <sched.h>
#include "lcd.h"
#define NUM_VALID_DEVICES 2

#define MAX_BUFFER_SIZE 100

// index 0 corresponds to the device's first gpio pin and 1 corresponds

struct device_t {
    int32_t gpio_numbers[NUM_VALID_DEVICES];
};


static const char *GPIO_Path = (char *) "/sys/class/gpio/";
static const char *TEMP_PATH = (char *) "/sys/bus/w1/devices/28-2b46d446b48a/hwmon/hwmon0/";

static void *modifyLED(void* arg);
static void *monitorTemperature(void* arg);
static void *monitorWeight(void* arg);

static int32_t promptUserForGPIOS(struct device_t *devices, int32_t * isTemp);
static int32_t writeGPIO(int32_t gpio_number, char *output);
static int32_t initializeDevices(struct device_t *devices);
static int32_t initializeSensors(struct device_t *devices);
static int32_t start_system();
static bool handleUnsafeOperations();
static double readGPIO(int32_t gpio_number, int32_t);
static int32_t promptUserForkegWeight(double * kegWeight);
static double convertToPercentage();

// shared variables
static struct device_t displaySensor= {0};
static int32_t temperatureSensor_gpio=1 ;
static struct device_t weightSensor= {0};
double current_weight = -1;
double current_temperature = -1;

// locks
static pthread_mutex_t weight_mutex;
static pthread_mutex_t temperature_mutex; 

// user provided inputs for calibration to compute % Remaining Beer
double EmptykegWeight = -1;
double FullkegWeight = -1;

// TODO - update
// custom single handler for SIGINT, turns off the LEDs, turns off buzzer, and raises train crossing 
static void handler(int32_t sig) {
    int32_t i;
    int32_t result;
    printf("Caught Signal %d: Working on clean shutdown...\n", sig);

    // note: nothing down with error return values since shutting down anyways
    for (i = 0; i < NUM_VALID_DEVICES; i++) {
        result = writeGPIO(temperatureSensor.gpio_numbers[i], "0");//EDIT HERE
        if (result != 0) {
            printf("Error occurred while attempting to turn off GPIO pin %d. Continuing with shutdown...\n", result);
        }
    }
    exit(0);
}

int main(){
    int32_t display_flag = -1;
    int32_t weight_flag = -1;
    int32_t temp_flag = -1;
    int32_t signal_num = SIGINT;
    int32_t device_flag = -1;
    int32_t keg_weight_flag=-1;
    int32_t result = 0;
    struct sigaction sa = {0};
     // Install Signal Handler for SIGINT
    sa.sa_handler = handler;
    result = sigaction(signal_num, &sa, (void *) ((int32_t) 0));
   
    if (result == 0) {
            printf("Enter GPIO Input for Weight Sensor: \n");

        //function to ask for dislayScreen motor and piezo motor input 
        weight_flag= promptUserForGPIOS(&weightSensor,0);

        if (weight_flag==0) {
            printf("Enter GPIO Input for Temperature Sensor: \n");
            temp_flag = promptUserForGPIOS(NULL, &temperatureSensor_gpio);


            if(temp_flag==0){
                printf("Enter weight of Empty KEG: \n");
                keg_weight_flag=  promptUserForkegWeight(&EmptykegWeight);

                if(keg_weight_flag==0){
                printf("Enter weight of full KEG: \n");
                keg_weight_flag=  promptUserForkegWeight(&FullkegWeight);
            }
            }
        }
        

        if (weight_flag == 0 && temp_flag==0) {
            printf("Weight Sensor GPIOs- %d, %d\n",weightSensor.gpio_numbers[0],weightSensor.gpio_numbers[1]);
            printf("Temperature GPIO- %d\n", temperatureSensor_gpio);
            printf("Empty Keg is %.0lf\n\n", EmptykegWeight);
            printf("Full Keg is %.0lf\n\n", FullkegWeight);
            printf("Input module SUCCESSFULL\n");
            printf("\n");
            printf("\n");

        } else {
            printf("INPUT module Failed\n");
            printf("\n");
            printf("\n");
            exit(0);
        }

   
    device_flag = -1;

    device_flag = initializeSensors(&weightSensor);
    printf("weight init with %d\n",device_flag);
    if (device_flag == 0) {
           // i2c_init();
    }else{
        printf("Device flag is not 0 \n");
    }


    // checks if the initialization of all devices was successful
    if (device_flag == 0) {
        if (start_system() == 1){
            printf("EXITING CODE");
        }
    }
  }  
  return 0;
}

static int32_t promptUserForGPIOS(struct device_t *devices, int32_t *isTemp){
   char buffer[MAX_BUFFER_SIZE] = {0};
    // used to check if fgets was successful
    char *fgets_flag = NULL;
    // used to check if sscanf was successful
    int32_t sscanf_flag = EOF;
    int32_t result = 1;
   
    for( int32_t i=0; i<NUM_VALID_DEVICES; i++){
        

        // note: fgets + sscanf used instead of scanf for memory safety
        fgets_flag = fgets((char *) buffer, MAX_BUFFER_SIZE, stdin);

        if (fgets_flag != NULL) {
            printf("Enter GPIO: \n");
             if(devices==NULL && isTemp){
                sscanf_flag = sscanf(buffer, "%d", isTemp);
                printf("temp GPIO Input recorded: %d \n",*isTemp);
                 if (sscanf_flag == 1) {
                    result = 0;
                }
                break;
            }else{
                sscanf_flag = sscanf(buffer, "%d", &devices->gpio_numbers[i]);
                printf("GPIO Input recorded: %d \n",devices->gpio_numbers[i]);
            }
          
            if (sscanf_flag == 1) {
                result = 0;
            }
        }
    }
    printf("\n");
    printf("\n");

    return result;
}



static int32_t promptUserForkegWeight(double *kegWeight){
   char buffer[MAX_BUFFER_SIZE] = {0};
    // used to check if fgets was successful
    char *fgets_flag = NULL;
    // used to check if sscanf was successful
    int32_t sscanf_flag = EOF;
    int32_t result = 1;
    
    fgets_flag = fgets((char *) buffer, MAX_BUFFER_SIZE, stdin);

    if (fgets_flag != NULL) {
        sscanf_flag = sscanf(buffer, "%lf", kegWeight);
        printf("KEG Weight recorded: %.2lf \n",*kegWeight);

        if (sscanf_flag == 1) {
            result = 0;
        }
    }

    printf("\n");
    printf("\n");

    return result;
}

// used to initialize tempRatch (aka push buttons)
static int32_t initializeSensors(struct device_t *devices) {
    int32_t j;
    int32_t cur_gpio_num;
    int32_t result = 0;
    int32_t flag;
    char direction_path[MAX_BUFFER_SIZE] = {0};
    FILE *fp = NULL;

    for (j = 0; j < NUM_VALID_DEVICES; j++) {
        cur_gpio_num = devices->gpio_numbers[j];
        result = cur_gpio_num;

        // open direction file
        flag = snprintf(direction_path, MAX_BUFFER_SIZE, "%sgpio%d/direction", GPIO_Path, cur_gpio_num);
        printf("weight path - %s\n",direction_path);
        // check that no error occurred during the writing of direction_path
        if (flag >= 0) {
            fp = fopen(direction_path, "w");

            // check that file was successfully opened
            if (fp != NULL) {
                flag = fprintf(fp, "in");

                // check that file was successfully written to
                if (flag >= 0) {
                    flag = fclose(fp);
                    if (flag != EOF) {
                        result = 0;
                    }
                } else {
                    flag = fclose(fp);
                }
            }
        }
        if (result != 0) {
            break;
        }                
    }
    return result;
}


static int32_t writeGPIO(int32_t gpio_number, char *output) {
    FILE *fp = NULL;
    char path[MAX_BUFFER_SIZE] = {0};
    int32_t result = gpio_number;
    int32_t flag = 0;

    // open value file
    flag = snprintf(path, MAX_BUFFER_SIZE, "%sgpio%d/value", GPIO_Path, gpio_number);

    // check that no errors occurred while writing to path
    if (flag >= 0) {
        fp = fopen(path, "w");

        // check that the specified file was opened correctly
        if (fp != NULL) {
            flag = fprintf(fp, "%s", output);

            // check that the output was written out to the file without error
            if (flag >= 0) {
                flag = fclose(fp);

                // check if file was closed without error 
                if (flag == 0) {
                    result = 0;
                }
            } else {
                flag = fclose(fp);
            }  
        }     
    }
    return result;
}

static double readGPIO(int32_t gpio_number, int32_t isTemp) {
    FILE *fp = NULL;
    char path[MAX_BUFFER_SIZE] = {0};
    int32_t flag = -1;
    double result = -1;
    char value[60];
    char buffer[MAX_BUFFER_SIZE] = {0};
    if(!isTemp){
    // open value file
        flag = snprintf(path, MAX_BUFFER_SIZE, "%sgpio%d/value", GPIO_Path, gpio_number);
    }else{
           // open value file
        flag = snprintf(path, MAX_BUFFER_SIZE, "%s/temp1_input", TEMP_PATH);
        //printf("Temp input is %s\n",path);
    }
 
    // check that path was correctly written to
    if (flag >= 0) {
        fp = fopen(path, "r");
        // check that file opened successfully
        if (fp != NULL) {
            if(fgets(value, 60, fp)!=NULL){
                 char *endptr;
                result = strtod(value, &endptr);
                // Check if conversion was successful
                if (*endptr != '\0' && *endptr != '\n') {
                    printf("Error: Invalid Value was written to GPIO pin %d's value file.\n", gpio_number);
                    result = -1;
                }
                //printf("RESULT- %f\n",result);

            }
            else{ 
                printf("Error: Invalid Value was written to GPIO pin %d's value file.\n", gpio_number);
            }

            flag = fclose(fp);

            if (flag != 0) {
                printf("Error with closing GPIO's value file.\n;");
                result = -1;
            }
        }
    }
    return result;
}


// Train sensors, crossing gate, and wght lights should be separate threads
// Mutex or semaphore protection of shared data (e.g. wait timers, device/sensor states)
// Correct logic with trains approaching and leaving from either directoin
// Code should provide for safe operation even if sensors are not detected in the correct order

// static int32_t start_system(int32_t dislayScreen_port, int32_t alarm_port)
static int32_t start_system()
{

    pthread_attr_t temperature_attr, display_attr, weight_attr;
    int32_t tempR=0,display=0,wght=0;

    // Initialize attributes for each thread
    tempR= pthread_attr_init(&temperature_attr);
    display= pthread_attr_init(&display_attr);
    wght= pthread_attr_init(&weight_attr);

    if(tempR || display || wght){
        printf("Error initializing attributes\n\n");
    }

   struct sched_param param_temperature, param_display, param_weight;

    tempR= pthread_attr_getschedparam(&temperature_attr, &param_temperature);
    display=pthread_attr_getschedparam(&display_attr, &param_display);
    wght=pthread_attr_getschedparam(&weight_attr, &param_weight);
    
    
    // printf("Getting schedular params  tempR -%d\n",tempR);
    // printf("Getting schedular params  display -%d\n",display);
    // printf("Getting schedular params  wght -%d\n",wght);


    tempR = pthread_attr_setschedpolicy(&temperature_attr, SCHED_FIFO);
    display = pthread_attr_setschedpolicy(&display_attr, SCHED_FIFO);
    wght = pthread_attr_setschedpolicy(&weight_attr, SCHED_FIFO);
    
    // printf("Setting policy  tempR -%d\n",tempR);
    // printf("Setting policy display -%d\n",display);
    // printf("Setting policy  wght -%d\n",wght);
    
    param_temperature.sched_priority = 3; // Lower priority for tempR
    param_display.sched_priority = 1;   // Higher priority for display device
    param_weight.sched_priority = 2;   // Medium priority for wght lights

    tempR= pthread_attr_setschedparam(&temperature_attr, &param_temperature);
    display=pthread_attr_setschedparam(&display_attr, &param_display);
    wght=pthread_attr_setschedparam(&weight_attr, &param_weight);
    
    // printf("setting schedular params  tempR -%d\n",tempR);
    // printf("setting schedular params  display -%d\n",display);
    // printf("setting schedular params  wght -%d\n",wght);    
 
    
    pthread_t temperature_device,display_device, weight_device;

      
    if(pthread_create(&temperature_device,&temperature_attr, monitorTemperature, NULL)!=0)
    {  
        perror("Error creating train thread \n");
        printf("with error %d ",tempR);
        exit(1);
    }

    if(pthread_create(&display_device, &display_attr, modifyLED , NULL)!=0)
    {
        perror("Error creating display sensor thread \n");
        exit(1);
    }
    
    if(pthread_create(&weight_device, &weight_attr, monitorWeight , NULL)!=0)
    {
        perror("Error creating wght lights thread \n");
        exit(1);
    }   

    pthread_join(temperature_device,NULL);
    pthread_join(display_device,NULL);
    pthread_join(weight_device,NULL);
    
    // Cleanup attributes (optional)

    pthread_cancel(temperature_device);
    pthread_cancel(display_device);
    pthread_cancel(weight_device);
    pthread_attr_destroy(&temperature_attr);
    pthread_attr_destroy(&display_attr);
    pthread_attr_destroy(&weight_attr);
          
    printf("cleaning up\n\n");
    return 1;
}
static double convertToPercentage(){
    pthread_mutex_lock(&weight_mutex);
            double localWeight= current_weight;
    pthread_mutex_unlock(&weight_mutex);
    
    if(localWeight!=-1){
        localWeight= (localWeight-EmptykegWeight)/FullkegWeight;
        localWeight*=100;
    }

    return localWeight;
}


//10ms
//The "reset" button zeros the counter.  This can be done regardless of whether the tempRatch is started or stopped.
//time
static void *monitorTemperature(void * arg){
    struct timeval s,e;

    pthread_t tid = pthread_self(); 
    printf("Process ID of monitorTemperature Thread is : %lu\n", tid);
    struct timeval start_time, current_time;
    bool resetEvent=false;

    //clearDisplay();

    while(true){
        usleep(5000000);
        
        pthread_mutex_lock(&temperature_mutex);
            current_temperature=readGPIO(temperatureSensor_gpio,1)/1000;
        pthread_mutex_unlock(&temperature_mutex);
    }

    printf("exiting THREAD monitorTemperature\n\n");
    return NULL;   
}



void *monitorWeight(void *arg) {
        struct timeval s,e;
    pthread_t tid = pthread_self();
    printf("Process ID of modifyLED Thread is : %lu\n", tid);
    
    while (true) {
             
        usleep(1000000);
          pthread_mutex_lock(&weight_mutex);
            current_weight=readGPIO(weightSensor.gpio_numbers[0],0);
        pthread_mutex_unlock(&weight_mutex);
    }
    printf("Exiting THREAD modifyLED\n\n");
    return NULL;
}


//While the tempRatch is started, you shall display the running time in seconds on your terminal display to a resolution of 100ms. 
// When the tempRatch is stopped, the display shall show the time with a resolution of 10ms.
//Terminal display must be updated every 100ms. 
void *modifyLED(void *arg) {
    pthread_t tid = pthread_self();
    printf("Process ID of modifyLED Thread is : %lu\n", tid);
    double localTemp=-1,localWeight=-1;
    char *lines[100];
    i2c_init();
    while (true) {
        
        usleep(3000000);    

        localWeight=convertToPercentage();
        
        pthread_mutex_lock(&temperature_mutex);
           localTemp= current_temperature;
        pthread_mutex_unlock(&temperature_mutex);

        char *lines = (char *)malloc(100 * sizeof(char));
        snprintf(lines, 100, "Temp:%.00fC Wght:%.00f%%", localTemp, localWeight);

         // printf("lines- %s",lines);
         int32_t flag= i2c_msg(lines);
        //printf("Temperature- %.0lf , Quantity- %.0lf\% \n",localTemp,localWeight);
    }
    i2c_stop();
    printf("Exiting THREAD modifyLED\n\n");
    return NULL;
}


