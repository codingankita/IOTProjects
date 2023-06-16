#include "contiki.h" #include "dev/light-sensor.h" #include "dev/sht11-sensor.h" #include <stdio.h>
#include <stdbool.h> #include <stdlib.h> #include <string.h> #include <math.h>

// constants for buffer
#define BUFFER_MAX 12

// global variables for variables
int bufferLength=0; int tempbufferLength=0; int front=-1;
int rear=-1;
float lightBuffer[BUFFER_MAX]; float tempBuffer[BUFFER_MAX]; float light_std_dev;
float agg_buffer[4]; bool push_flag=true;

// function to calculate square root (in-built function of math.h : "sqrt()" is not working)
// “”
float calculate_sqrt(float val) {
float temp = 0.0, sqrt; sqrt = val / 2;

while (sqrt != temp) { temp = sqrt;
sqrt = (val / temp + temp) / 2;
}
return sqrt;
}

// functions for calculating float values
int d1(float f)
{
return((int)f);
}

unsigned int d2(float f)
{
if (f>0)
return(1000*(f-d1(f))); else
 
return(1000*(d1(f)-f));
}

// function to calculate mean
float calculate_mean(float arr[]) {
float sum_buffer = 0.0;
int i;
for (i = 0; i < 12 ; i++) { sum_buffer = sum_buffer + arr[i];
}
float flf = sum_buffer/12.0;
return flf;
}

// function to calculate squares
float square_of_number(float num)
{
return (num*num);
}

// capturing light sensor values
int getLight(void)
{
float V_sensor = 1.5 * light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC)/4096;
// ^ ADC-12 uses 1.5V_REF
float I = V_sensor/100000;	// xm1000 uses 100kohm resistor
float light_lx = 0.625*1e6*I*1000; // convert from current to light intensity
return light_lx;
}

// capturing temperature sensor values
float getTemperature(void) {
int tempADC = sht11_sensor.value(SHT11_SENSOR_TEMP_SKYSIM);
float temp = 0.04 * tempADC - 39.6; // skymote uses 12-bit ADC, or 0.04 resolution
return temp;
}

// function for inserting values in queue
void push_buffer(float queue[] , int value, int bufferLen, char* type){
if(bufferLen==12){
printf("\n Buffer is full! \n");
}else{
rear++; queue[rear]=value;
if(strcmp(type , "temp")==0){
printf("Temperature Reading = %u.%03u C\n", d1(queue[rear]),d2(queue [rear]));
 
}else{
printf("Light Reading = %u.%03u lux\n", d1(queue[rear]),d2(queue[rear]));
}
bufferLen++;
if(front==-1){ front=0;
}
}
}

// function to display queue
float disp(float queue[])
{
int i;
if(front==-1){
printf("\nBuffer is Empty\n");
}else{
printf("\nBuffer = [");
for(i=front;i<=rear;i++)
{
printf("%u.%03u ",d1(queue[i]),d2(queue[i]));
}
printf("]\n");
}
}

// function to dequeue a member from queue
void pop_buffer(float queue[])
{
if(front==-1||front==rear+1) printf("\nQueue is empty");
else
{
queue[front]=0; front=front+1; bufferLength--;
}
}

// function to calculate standard deviation
float buffer_std_dev(float arr_buffer[], char* type) {
float mean_of_buffer = calculate_mean(arr_buffer), sum_of_buffer = 0.0 , temp, diff;
int j;
for (j = 0; j < 12; j++) {
diff = arr_buffer[j] - mean_of_buffer; temp = square_of_number(diff); sum_of_buffer += temp;
 
}
float sqrt_sum = calculate_sqrt(sum_of_buffer / 12);
if(strcmp(type , "temp")==0){
printf("\n Temperature Standard Deviation = %u.%03u", d1(sqrt_sum),d2 (sqrt_sum));
}else{
printf("\n Light Standard Deviation = %u.%03u", d1(sqrt_sum),d2(sqrt_sum));
}
return sqrt_sum;
}

/*	*/
PROCESS(sensor_reading_process, "Sensor reading process"); AUTOSTART_PROCESSES(&sensor_reading_process);
/*	*/
PROCESS_THREAD(sensor_reading_process, ev, data)
{
static struct etimer timer; PROCESS_BEGIN();
etimer_set(&timer, CLOCK_CONF_SECOND*2); SENSORS_ACTIVATE(light_sensor);

static int counter = 0;
do
{
PROCESS_WAIT_EVENT_UNTIL(ev=PROCESS_EVENT_TIMER);
int light_val = getLight();
float temp_val = getTemperature(); push_flag=true;
push_buffer(lightBuffer , light_val, bufferLength, "light"); // creating buffer for light sensor readings
push_buffer(tempBuffer , temp_val , tempbufferLength, "temp"); // creating buffer for temprature sensor readings
counter++; etimer_reset(&timer);
}while(counter<=11);

// Calculating Euclidean distance
int i = 0;
float diff = 0, sum = 0, eu_dist = 0;

for (i = 0; i < 12; i++) {
diff = lightBuffer[i] - tempBuffer[i]; sum += square_of_number(diff);
}

eu_dist = calculate_sqrt(sum); printf("\n\n");
 
printf("Euclidean distance = %d.%03u \n", d1(eu_dist), d2(eu_dist));

// corr Coefficient

float covar, mean_light_read, mean_temp_read, sum_var, stdev_light_read, stdev_temp_read, corr, lightSQ_sum, tempSQ_sum, stdev_prod;

mean_light_read = calculate_mean(lightBuffer); mean_temp_read = calculate_mean(tempBuffer);

for (i = 0; i < 12; i++) {
sum_var += (lightBuffer[i] - mean_light_read) * (tempBuffer[i] - mean_temp_read);
lightSQ_sum += square_of_number(lightBuffer[i] - mean_light_read); tempSQ_sum += square_of_number(tempBuffer[i] - mean_temp_read);
}

covar = sum_var / 11;

stdev_light_read = buffer_std_dev(lightBuffer,"light"); stdev_temp_read = buffer_std_dev(tempBuffer,"temp"); stdev_prod = (stdev_light_read * stdev_temp_read);

corr = covar / stdev_prod;
printf("\nStd_mult = %u.%03u" , d1(stdev_prod),d2(stdev_prod)); printf("\n covar = %u.%03u" , d1( cova r),d2( covar ));
if( stdev_prod > 0 ){
if (corr>-1 && corr<0) {
printf("\n corr Coefficient = - %d.%03u \n", d1(corr), d2(corr));
} else {
printf("\n corr Coefficient = %d.%03u \n", d1(corr), d2(corr));
}
}
else {
printf("\n corr Coefficient = 0.000 \n");
}
// Linear Regression

float grad, inters;
grad = sum_var / lightSQ_sum; // Linear regression graph points
inters = mean_temp_read - (grad * mean_light_read); // y = mx + c => hence c = y - mx

printf(" y(predicted) = %d.%03ux + %d.%03u \n", grad, inters);

float yPred, yPred_sum, Rsq;
for (i = 0; i < 12; i++) {
yPred = (grad * lightBuffer[i]) + inters; // predict y (temperature) using
 
above regression line
yPred_sum += square_of_number(yPred - mean_temp_read); // finding the difference between predicted and y average
}

Rsq = yPred_sum / tempSQ_sum; // RSquare


if( tempSQ_sum > 0 ){
// Quick fix for negative and positive r squares
if ((yPred_sum > 0.0 && tempSQ_sum < 0.0) || (yPred_sum < 0.0 && tempSQ_sum > 0.0)) {
printf("R Squared = - %d.%03u \n", d1(Rsq), d2(Rsq));
} else {
printf("R Squared = %d.%03u \n", d1(Rsq), d2(Rsq));
}
}
else {
printf("R Squared = 0.000 \n");
} PROCESS_END();
}
/*	*/
