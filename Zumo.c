#include <project.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "Motor.h"
#include "Ultra.h"
#include "Nunchuk.h"
#include "Reflectance.h"
#include "Gyro.h"
#include "Accel_magnet.h"
#include "LSM303D.h"
#include "IR.h"
#include "Beep.h"
#include "mqtt_sender.h"
#include <time.h>
#include <sys/time.h>
#include "serial1.h"
#include <unistd.h>
#include <stdlib.h>

#define ON 1
#define OFF 0
#define BLACK 1
#define WHITE 0

void progStart(uint32_t delay);
void progEnd(void);
void tankTurnRight(uint32_t angle);
void tankTurnLeft(uint32_t angle);


//Sumo wrestling

#if 0
    
void zmain(void)
{
    //Initializing variables
    unsigned lineCount = 0;
    struct sensors_ dig;
    TickType_t startTime = 0;
    TickType_t stopTime = 0;
    TickType_t atObstacle = 0;
    
    //Robot startup
    IR_Start();
    reflectance_start();
    reflectance_set_threshold(15000, 15000, 11000, 11000, 15000, 15000);
    Ultra_Start();
    motor_start();
    motor_forward(0, 0);
    progStart(1000);
    
    //Drive to the start line and wait for IR signal
    while(lineCount == 0)
    {
        motor_forward(100, 0);
        reflectance_digital(&dig);
        
        if(dig.L3 == BLACK && dig.R3 == BLACK)
        {
            motor_forward(0, 0);
            print_mqtt("Zumo02/ready", "zumo");
            IR_wait();
            startTime = xTaskGetTickCount();
            print_mqtt("Zumo02/start", "%d", startTime);
            motor_forward(100, 100);
            lineCount++;           
        }
    }
    
    //Move forward unles obstacle is detected
    while(SW1_Read() == true)
    {
        int distance = Ultra_GetDistance();
        reflectance_digital(&dig);
        motor_forward(100, 5);
        
        //Move backwards and turn left if obstacle is detected with ultra sensor
        if(distance < 10)
        {
            atObstacle = xTaskGetTickCount();
            print_mqtt("Zumo02/obstacle", "%d", atObstacle);
            motor_forward(0, 0);
            motor_backward(100, 200);
            tankTurnLeft(90);
        }
        //Stop and make 180Deg turn if ring boundaries are detected
        if(dig.L3 == BLACK || dig.R3 == BLACK)
        {
            atObstacle = xTaskGetTickCount();
            print_mqtt("Zumo02/obstacle", "%d", atObstacle);
            motor_forward(0 ,0);
            tankTurnLeft(180);    
        }
        vTaskDelay(100);
    }
    
    motor_forward(0,0);
    stopTime = xTaskGetTickCount();
    print_mqtt("Zumo02/stop", "%d", stopTime);
    print_mqtt("Zumo02/time", "%d", stopTime-startTime); //Print run time
    progEnd();
}

#endif

//Line follower

#if 0
    
void zmain(void)
{
    //Initializing variables
    unsigned lineCount = 0;
    struct sensors_ dig;
    TickType_t startTime = 0;
    TickType_t lineMissTime = 0;
    TickType_t stopTime = 0;
    bool lineMiss = false;
    
    //Robot startup
    IR_Start();
    reflectance_start();
    reflectance_set_threshold(15000, 15000, 11000, 11000, 15000, 15000);
    motor_start();
    motor_forward(0, 0);
    progStart(1000);

    //Drive to the start line and wait for IR signal
    while(lineCount == 0)
    {
        motor_forward(100, 0);
        reflectance_digital(&dig);
        
       if(dig.L3 == BLACK && dig.R3 == BLACK)
        {
            motor_forward(0, 0);
            print_mqtt("Zumo02/ready", "line");
            IR_wait();
            startTime = xTaskGetTickCount();
            print_mqtt("Zumo02/start", "%d", startTime);
            motor_forward(100, 100);
            lineCount++;           
        }
    }
    
    //Drive forward when line is straight
    while(lineCount < 3)
    {
        reflectance_digital(&dig);
        motor_forward(255,2);
        
        //Correct position to left
        if(dig.L2 == BLACK || dig.L3 == BLACK)
        {
            motor_turn(0, 255, 10);
        }
        //Correct position to right
        if(dig.R2 == BLACK || dig.R3 == BLACK)
        {
            motor_turn(255, 0, 10);
        }
        //Both center sensors are off track
        if(dig.L1 == WHITE && dig.R1 == WHITE && lineMiss == false)
        {
            lineMissTime = xTaskGetTickCount();
            print_mqtt("Zumo02/miss", "%d", lineMissTime);
            lineMiss = true;
        }
        //Both center sensors are back on track
        if(dig.L1 == BLACK && dig.R1 == BLACK && lineMiss == true)
        {
            lineMissTime = xTaskGetTickCount();
            print_mqtt("Zumo02/line", "%d", lineMissTime);
            lineMiss = false;
        }
        //Count intersections
        if(dig.L3 == BLACK && dig.R3 == BLACK)
        {
            motor_forward(255,20);
            lineCount++;
        }
    }
    
    motor_forward(0, 0);
    stopTime = xTaskGetTickCount();
    print_mqtt("Zumo02/stop", "%d", stopTime);
    print_mqtt("Zumo02/time", "%d", (stopTime-startTime)); //Print run time
    progEnd();
 }   

#endif

// Grid 

#if 1
    
void zmain (void) {
    
    // Initializing variables
    struct sensors_ dig;
    int distance = 0;
    int rNum = 0;
    bool firstLinePassed = false;
    TickType_t leftCrossing, sinceCrossing, startTime, stopTime;
    leftCrossing = xTaskGetTickCount();
    int direction = 0; // 0 == up, 1 == right, 2 == left, (3 == down)
    srand(100);
    
    // Robot startup
    IR_Start();
    Ultra_Start();
    reflectance_start();
    reflectance_set_threshold(15000, 15000, 11000, 11000, 15000, 15000);
    motor_start();
    motor_forward(0, 0);
    progStart(1000);
    
    // Driving to the first line
    while (firstLinePassed == false)
    {
        direction = 0;
        motor_forward(100,5);
        reflectance_digital(&dig);
        
        if (dig.L3 == BLACK && dig.R3 == BLACK)
        {
            motor_forward(0,0);
            send_mqtt("Zumo02/ready", "maze");
            IR_wait();
            startTime = xTaskGetTickCount();
            print_mqtt("Zumo02/start", "%d", startTime);
            motor_forward(100,100);
            firstLinePassed = true;
        }
        leftCrossing = xTaskGetTickCount();
    }
    
    // After passing the first line
    while (firstLinePassed == true)
    {
        
        motor_forward(100,5);
        reflectance_digital(&dig);
        sinceCrossing = xTaskGetTickCount() - leftCrossing;
        
        // Arriving at an intersection
        if((dig.L3 == BLACK && dig.R3 == BLACK) || (dig.L3 == BLACK && dig.L1 == BLACK) || (dig.R3 == BLACK && dig.R1 == BLACK))
        {
            motor_forward(0,0);
            distance = Ultra_GetDistance();
            
            // Obstacle detected at the next intersection
            if(distance<15)
            {
                motor_forward(0,0);
                motor_forward(100,220);
                
                // Decide which way to turn based on the robots orientation when detecting the obstacle
                switch(direction){
                    case 0:
                        rNum = (rand() % 2);
                        switch(rNum) {
                            case 0:
                                direction = 2;
                                tankTurnLeft(90);
                                break;
                            case 1:
                                direction = 1;
                                tankTurnRight(90);
                                break;
                        }
                        break;
                    case 1:
                        tankTurnLeft(90);
                        direction = 0;
                        break;
                    case 2:
                        tankTurnRight(90);
                        direction = 0;
                        break;
                }
                leftCrossing = xTaskGetTickCount();
            } 
            
            // No obstacle at next intersection. Also determine next direction.
            else {
                vTaskDelay(5);
                switch(direction) {
                    case 0:
                        motor_forward(100,220);
                        break;
                        direction = 0;
                    case 1:
                        motor_forward(0,0);
                        motor_forward(100,220);
                        tankTurnLeft(90);
                        distance = Ultra_GetDistance();
                        if(distance < 14)
                        {
                        tankTurnRight(90);
                        direction = 1;
                        } else {
                        direction = 0;
                        }
                        break;
                    case 2:
                        motor_forward(0,0);
                        motor_forward(100,220);
                        tankTurnRight(90);
                        distance = Ultra_GetDistance();
                        if(distance < 14)
                        {
                            tankTurnLeft(90);
                            direction = 2;
                        } else {
                        direction = 0;
                        }
                        break;
                }
                leftCrossing = xTaskGetTickCount();
            }
        }
        // What to do when the robot is at the edge of the track
        if (dig.L1 == WHITE || dig.R1 == WHITE) 
        {
            switch (direction) 
            {
                case 1:
                    motor_forward(0,0);
                    tankTurnLeft(180);
                    direction = 2;
                    break;
                case 2:
                    motor_forward(0,0);
                    tankTurnRight(180);
                    direction = 1;
                    break;
                case 0:
                    motor_forward(0,0);
                    tankTurnLeft(90);
                    direction = 2;
                    break;
            }
            leftCrossing = xTaskGetTickCount();
        }
        // If the time between crossings has been too long, break out of program (i.e the last long line)
        if (sinceCrossing > 800) {
            break;
        }
    }
    motor_forward(0,0);
    stopTime = xTaskGetTickCount();
    print_mqtt("Zumo02/stop", "%d", stopTime);
    print_mqtt("Zumo02/time", "%d", stopTime-startTime);
    progEnd();
}
#endif

// Custom functions below

void progStart(uint32_t delay){
    while(SW1_Read() == 1); // When the button is pressed, value = 0, break out of loop.
    BatteryLed_Write(ON);
    vTaskDelay(delay);
    BatteryLed_Write(OFF);
}

void progEnd(void){
    bool led = 0;
    while(true){
        vTaskDelay(100);
        BatteryLed_Write(led ^= 1);
    }
}

void tankTurnRight(uint32_t angle){
    float angleMultiplier = 2.911111111111;
    SetMotors(0, 1, 100, 100, angleMultiplier * angle);
}

void tankTurnLeft(uint32_t angle){
    float angleMultiplier = 2.911111111111;
    SetMotors(1, 0, 100, 100, angleMultiplier * angle);
}



/* [] END OF FILE */
