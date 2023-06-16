sensor.c
#include "contiki.h"
#include "dev/light-sensor.h"
#include "dev/sht11-sensor.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
// constants for buffer and aggregation
#define BUFFER_MAX 12
#define LL_ACTIVITY 10
#define ML_ACTIVITY 1000
#define HL_ACTIVITY 10000
// global variables for variables
int bufferLength=0;
int aggbufferLength=0;
int front=-1;
int rear=-1;
float lightBuffer[BUFFER_MAX];
float light_std_dev;
float agg_buffer[4];
bool push_flag=true;
// function to calculate square root (in-built function of math.h : "sqrt()" is
not working)
float calculate_sqrt(float val) {
float temp = 0.0, sqrt;
sqrt = val / 2;
while (sqrt != temp) {
temp = sqrt;
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
return(1000*(f-d1(f)));
Page 1

sensor.c
else
return(1000*(d1(f)-f));
}
// function to calculate mean
float calculate_mean(float arr[]) {
float sum_buffer = 0.0;
int i;
for (i = 0; i < 12 ; i++) {
sum_buffer = sum_buffer + arr[i];
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
float I = V_sensor/100000;
// xm1000 uses 100kohm resistor
float light_lx = 0.625*1e6*I*1000; // convert from current to light intensity
return light_lx;
}
// function for inserting values in queue
void push_buffer(float queue[] , int value){
if(bufferLength==12){
printf("\n Buffer is full! \n");
}else{
rear++;
queue[rear]=value;
printf("Reading = %u.%03u lux\n", d1(queue[rear]),d2(queue[rear]));
bufferLength++;
if(front==-1){
front=0;
}
}
}
// function to display queue
float disp(float queue[])
Page 2

sensor.c
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
if(front==-1||front==rear+1)
printf("\nQueue is empty");
else
{
queue[front]=0;
front=front+1;
bufferLength--;
}
}
// function to calculate standard deviation
float buffer_std_dev(float arr_buffer[]) {
float mean_buffer = calculate_mean(arr_buffer), var_buffer_sum = 0.0, temp,
diff;
int j;
for (j = 0; j < 12; j++) {
diff = arr_buffer[j] - mean_buffer;
temp = square_of_number(diff);
var_buffer_sum += temp;
}
float sqrt_sum = calculate_sqrt(var_buffer_sum / 12);
//sqrt_sum = 11000.890; // for showcasing the high level activity
printf("\n Standard Deviation = %u.%03u", d1(sqrt_sum),d2(sqrt_sum));
return sqrt_sum;
}
/*---------------------------------------------------------------------------*/
PROCESS(sensor_reading_process, "Sensor reading process");
AUTOSTART_PROCESSES(&sensor_reading_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_reading_process, ev, data)
Page 3

sensor.c
{
static struct etimer timer;
PROCESS_BEGIN();
etimer_set(&timer, CLOCK_CONF_SECOND*2);
SENSORS_ACTIVATE(light_sensor);
static int counter = 0;
do
{
PROCESS_WAIT_EVENT_UNTIL(ev=PROCESS_EVENT_TIMER);
int light_val = getLight();
push_flag=true;
push_buffer(lightBuffer , light_val); // creating buffer for light sensor
readings
counter++;
etimer_reset(&timer);
}while(bufferLength<=11);
disp(lightBuffer);
light_std_dev = buffer_std_dev(lightBuffer);
// aggregation of values
if (light_std_dev < LL_ACTIVITY) {
memset(agg_buffer, 0, sizeof(agg_buffer));
printf("\n Aggregation = 12-to-1\n");
int i;
float sum, avg;
for (i = 0; i < 12; i++) {
sum = sum + lightBuffer[i];
}
avg = sum / 12;
agg_buffer[0]=avg;
disp(agg_buffer);
}else if (light_std_dev <= HL_ACTIVITY && light_std_dev >= LL_ACTIVITY) {
memset(agg_buffer, 0, sizeof(agg_buffer));
printf("\n Aggregation = 4-to-1\n");
float avg1 = 0, avg2 = 0, avg3 = 0, sum1 = 0, sum2 = 0, sum3 = 0;
int i, j, k, l;
for (i = 0; i < 4; i++) {
sum1 = sum1 + lightBuffer[i];
}
for (j = 4; j < 8; j++) {
sum2 = sum2 + lightBuffer[j];
}
for (k = 8; k < 12; k++) {
sum3 = sum3 + lightBuffer[k];
}
avg1 = sum1 / 4;
Page 4

sensor.c
avg2 = sum2 / 4;
avg3 = sum3 / 4;
agg_buffer[0]=avg1;
agg_buffer[1]=avg2;
agg_buffer[2]=avg3;
printf("\n agg_buffer = [");
for(l=0;l<3;l++){
printf("%u.%01u ,",agg_buffer[l]);
}
printf("]\n");
}else{
printf("\n No aggregation to be used.\n");
}
PROCESS_END();
}
/*---------------------------------------------------------------------------*/

Page 5

