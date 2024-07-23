/**********************************************************************/
//** ENGR-2350 Lab 2 Timed Game
//** NAME: Nicholas Danas and Curran Flanders
//** RIN: 662055547  and  662017081
//** Section: 1
//** Side B
//** Seat 25
/**********************************************************************/

// include statements
#include "engr2350_msp432.h"
#include <time.h>
#include <stdlib.h>

// function prototypes
void Timer_Init(void);
void GPIO_Init(void);
void Timer_ISR(void);
void life_logic(void);
void new_round(void);
void setRGB(int8_t color);
// global variables
Timer_A_UpModeConfig config;
uint8_t pb1;
uint8_t BMP2;
uint8_t print_flag = 1;
uint32_t timer_resets = 0;
uint8_t i;
uint8_t game_over = 0;
uint8_t rounds = 0;
uint16_t game_timer = 750;
uint16_t led_counter = 0;
uint16_t game_counter = 0;
uint16_t timeout = 0;
uint16_t color_cycle = 0;
uint8_t lives = 3;
uint8_t target_color;
uint16_t RGB = 0;
uint8_t available[7] = {1,1,1,1,1,1,1};
uint8_t all_zero = 0;

int main(void)    /*** Main Function ***/
{  
    // local variables

    // call the "SysInit()" first to set up the microcontroller
    SysInit();
    GPIO_Init();
    Timer_Init();
    Timer_ISR();



    // initialization code
    printf("Welcome to our Speed Match Game! You will attempt to match the color randomly selected on the terminal with the flashing lights on the car, by pressing BMP1. Press the Push Button to start!\r\n");
    setRGB(7);
    srand(time(0));


    while(!GPIO_getInputPinValue(GPIO_PORT_P3, GPIO_PIN3)){
    }

    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
    new_round();
    while(1)
    {  
        cycling();
        // code that runs continuously here

        if(game_over == 1){

            if(GPIO_getInputPinValue(GPIO_PORT_P3, GPIO_PIN3)){
                printf("NEW GAME\r\n");
                GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5);
                GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
                GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN7);
                lives = 3;
                rounds = 0;
                game_timer = 750;
                setRGB(7);
                timer_resets = 0;
                game_over = 0;
                GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0 | GPIO_PIN1);
                while(timer_resets < 1000);
                setRGB(0);
                timer_resets = 0;
                new_round();
                timeout = 0;
            }else{
                if(led_counter >= 500){
                    GPIO_toggleOutputOnPin(GPIO_PORT_P6, GPIO_PIN0 | GPIO_PIN1);
                    led_counter = 0;
                }
            }
        }else{

            if(timeout > 10000){
                timeout = 0;
                setRGB(0);
                GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0);
                GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN1);
                led_counter = 0;
                life_logic();
                while(led_counter <= 500);
                led_counter = 0;
                GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0 | GPIO_PIN1);
                if (lives == 0){
                    GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0);
                    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN1);
                }

            }else{
                if(!GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2)){
                    __delay_cycles(240e3);
                    while(!GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2));
                    __delay_cycles(240e3);
                    if(target_color == color_cycle){
                        printf("Correct color\r\n");
                        setRGB(0);
                        GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN1);
                        GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN0);
                        led_counter = 0;
                        while(led_counter <= 500);
                        led_counter = 0;
                        GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0 | GPIO_PIN1);
                        new_round();
                        game_timer *= 0.9;
                        printf("%u\r\n", game_timer);
                    }else{
                        printf("Incorrect color\r\n");
                        life_logic();

                        setRGB(0);
                        GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0);
                        GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN1);
                        led_counter = 0;
                        while(led_counter <= 500);
                        led_counter = 0;
                        GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0 | GPIO_PIN1);
                        if (lives == 0){
                            GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0);
                            GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN1);
                        }
                    }


                }


            }

        }

    }   
}   

// function declarations

void GPIO_Init(void){

    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN2);
    GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN3);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN0 | GPIO_PIN1);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN5 | GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN7);
    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5);
    GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN7);
    GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0 | GPIO_PIN1);

}


void Timer_Init(void){
    config.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    config.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_20; //Dividing by 20
    config.timerPeriod = 1200; //1 ms
    config.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;

    //Applying configuration to timer
    Timer_A_configureUpMode(TIMER_A1_BASE, &config);

    //Registering the ISR
    Timer_A_registerInterrupt(TIMER_A1_BASE, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT, Timer_ISR);

}

void setRGB(int8_t color){
    if (color == 1){
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0); //Red
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1 | GPIO_PIN2);
    }

    if (color == 2){
            GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1); //Green
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN2);
        }

    if (color == 3){
            GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2); //Blue
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1);
        }

    if (color == 4){
            GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1); //Yellow
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
        }

    if (color == 5){
            GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN2); //Magenta
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
        }

    if (color == 6){
            GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1 | GPIO_PIN2); //Cyan
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
        }

    if (color == 7){
            GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2); //White
        }

    if (color == 0){
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2); //none on

            }


}

void life_logic(void){

    lives--;
    if(lives == 0){
        printf(" LOST LIFE: 0 lives left\r\n");
        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN7);
        GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
        //GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN0);
        //GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN1);
        game_over = 1;

    }else{
        if(lives ==1){
            printf(" LOST LIFE: 1 life left\r\n");
            GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0);
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
            GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN7);
        }else{
            printf(" LOST LIFE: 2 lives left\r\n");
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
            GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN0);
            GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN7);
        }

        new_round();
    }

}


void new_round(void){


    target_color = rand()%7+1;
    while(available[target_color-1]==0){
        target_color = rand()%7+1;
    }
    available[target_color-1] = 0;
    all_zero = 1;
    for (i=0;i<7;i++){
        if (available[i]==1){
            all_zero = 0;
        }
    }
    if (all_zero){
        for (i=0;i<7;i++){
            available[i]=1;
        }
    }
    rounds++;
    timeout = 0;



    if (target_color == 1){
        printf("%u: RED\r\n", rounds);
    }
    if (target_color == 2){
        printf("%u: GREEN\r\n", rounds);
        }
    if (target_color == 3){
        printf("%u: BLUE\r\n", rounds);
        }
    if (target_color == 4){
        printf("%u: YELLOW\r\n", rounds);
        }
    if (target_color == 5){
        printf("%u: MAGENTA\r\n", rounds);
        }
    if (target_color == 6){
        printf("%u: CYAN\r\n", rounds);
        }
    if (target_color == 7){
        printf("%u: WHITE\r\n", rounds);
        }
}

void cycling(void){
    if(RGB>=game_timer){
        color_cycle++;
        RGB = 0;
        color_cycle=color_cycle%7+1;
        setRGB(color_cycle);
    }
}

// Add interrupt functions last so they are easy to find
void Timer_ISR(void){
    Timer_A_clearInterruptFlag(TIMER_A1_BASE);
    timer_resets++;
    led_counter++;
    game_counter++;
    timeout++;
    RGB++;

}
