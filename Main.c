#include <RTL.h>
#include "LPC17xx.H"                    
#include "GLCD.h"
#include "LED.h"
#include "KBD.h"
#include "ADC.h"
#include <stdio.h>
#include <math.h>

/**************************
 BITMAPS! BITMAPS!! BITMAPS!!!
 **************************/
#include "Paddle.c"
#include "BlackPaddle.c"
#include "EasyBlock.c"
#include "MediumBlock.c"
#include "HardBlock.c"
#include "Ball.c"
#include "BlackBall.c"
#include "BlackBlock.c"

#define __FI        1                   /* Font index 16x24                 */
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define M_PI 3.14159265358979323846
#define POINTER_LENGTH 30

OS_TID t_led;                           /* assigned task id of task: led */
OS_TID t_adc;                           /* assigned task id of task: adc */
OS_TID t_kbd;                           /* assigned task id of task: keyread */
OS_TID t_jst   ;                        /* assigned task id of task: joystick */
OS_TID t_clock;                         /* assigned task id of task: clock   */
OS_TID t_lcd;                           /* assigned task id of task: lcd     */
OS_TID t_build_a_wall;
OS_TID t_ball;
OS_TID t_paddle;
OS_TID t_brick;

OS_MUT mut_GLCD;                        /* Mutex to controll GLCD access     */

unsigned int ADCStat = 0;
unsigned int ADCValue = 0;
unsigned int paddleX = (SCREEN_WIDTH-PADDLE_WIDTH)/2;
unsigned int paddleY = 280;
float ballX = (SCREEN_WIDTH-BALL_WIDTH)/2;
float ballY = 280 - BALL_HEIGHT - 1;
int numBricks = 0; 
int angle;
int numLives = 3;

unsigned int ball_launched = 0;

const float speed = 1.5;
float ball_x_speed = 0;
float ball_y_speed = 0;
int paddleSpeed = 2;
int delay = 1;

int is_game_over = 0;

int numBricksLeft = 0;

typedef struct{
    unsigned int x_loc;
	unsigned int y_loc;
	unsigned int height;
	unsigned int width;
	int numHitsLeft;
}brick_t;

brick_t bricks [28];

double angleX [10] = {0.7071, 0.5736, 0.4226, 0.2588, 0.0872, -0.0872, -0.2588, -0.4226, -0.5736, -0.7071};
double angleY [10] = {-0.7071, -0.8192, -0.9063, -0.9659, -0.9962, -0.9962, -0.9659, -0.9063, -0.8192, -0.7071};

/*----------------------------------------------------------------------------
 Searches array for specified value
 Return -1 if not found
 *---------------------------------------------------------------------------*/
int getIndexForValue(float val) {
    int i;
    int n = sizeof(angleX)/sizeof(angleX[0]);
    for(i = 0; i < n; i++){
        //Checks for equality with tolerance
        if(fabs(val - angleX[i]) < 0.01){
            return i;
        }
    }
    return -1;
}

/*----------------------------------------------------------------------------
 Determines the index of a brick that was collided. Return -1 if no collision
 If there is a collision, the speed is changed appropriately based on the side hit
 *---------------------------------------------------------------------------*/
int brickCollision (void) {
    int i, j = 0;
    int retVal[2] = {-1,-1};
    int arb = 2;
    int directionChanged = 0;
    
    for (i = 0; i < numBricks; i++){
        brick_t brick = bricks[i];
        if (brick.numHitsLeft > 0){
            
            //Hit bottom
            if((ballX + BALL_WIDTH) >= brick.x_loc && ballX <= (brick.x_loc + EASY_BLOCK_WIDTH)
               && ballY <= (brick.y_loc + EASY_BLOCK_HEIGHT) && ballY > brick.y_loc
               && ball_y_speed < 0) {
                ball_y_speed = -1*ball_y_speed;
                return i;
            }
            //Hit right
            else if (ballX >= brick.x_loc && ballX <= (brick.x_loc + EASY_BLOCK_WIDTH) && (ballY + BALL_HEIGHT) >= brick.y_loc
                     && ballY <= (brick.y_loc + EASY_BLOCK_HEIGHT)
                     && ball_x_speed < 0) {
                ball_x_speed = -1*ball_x_speed;
                return i;
            }
            //Hit left
            else if(ballX <= (brick.x_loc + EASY_BLOCK_WIDTH) && ballX >= (brick.x_loc - BALL_WIDTH) && (ballY + BALL_HEIGHT) >= brick.y_loc
                    && ballY <= (brick.y_loc + EASY_BLOCK_HEIGHT)
                    && ball_x_speed > 0) {
                ball_x_speed = -1*ball_x_speed;
                return i;
            }
            //Hit top
            else if((ballX + BALL_WIDTH) >= brick.x_loc && ballX <= (brick.x_loc + EASY_BLOCK_WIDTH)
                    && (ballY + BALL_HEIGHT) >= (brick.y_loc) && ballY < (brick.y_loc + EASY_BLOCK_HEIGHT)
                    && ball_y_speed > 0) {
                ball_y_speed = -1*ball_y_speed;
                return i;
            }
        }
    }
    return -1;
}

/*----------------------------------------------------------------------------
 Moves the ball left with the paddle before launch
 *---------------------------------------------------------------------------*/
void moveBallLeft(void) {
	GLCD_Bitmap(ballX, ballY, BALL_WIDTH ,BALL_HEIGHT , (unsigned char*)&BLACK_BALL_pixel_data);	
	ballX = ballX - paddleSpeed;	
	GLCD_Bitmap(ballX, ballY, BALL_WIDTH ,BALL_HEIGHT , (unsigned char*)&BALL_pixel_data);
}

/*----------------------------------------------------------------------------
 Moves the paddle left before launch
 *---------------------------------------------------------------------------*/
void moveLeft(void) {
	os_mut_wait(mut_GLCD, 0xffff);
    {
        GLCD_Bitmap(paddleX, paddleY, BLACK_PADDLE_WIDTH ,BLACK_PADDLE_HEIGHT , (unsigned char*)&BLACK_PADDLE_pixel_data);
        
        if (!ball_launched){
            GLCD_Bitmap(paddleX + (PADDLE_WIDTH - BALL_HEIGHT)/2, paddleY - BALL_HEIGHT, BALL_WIDTH ,BALL_HEIGHT , (unsigned char*)&BLACK_BALL_pixel_data);
        }
        //Don't want the paddle to leave the screen
        if (paddleX > paddleSpeed){
            paddleX = paddleX - paddleSpeed;
            if (!ball_launched){	
                moveBallLeft();
            }
        }
        
        GLCD_Bitmap(paddleX, paddleY, PADDLE_WIDTH ,PADDLE_HEIGHT , (unsigned char*)&PADDLE_pixel_data);
    }
	os_mut_release(mut_GLCD);
	os_dly_wait(delay);
}

/*----------------------------------------------------------------------------
 Moves the ball right with the paddle before launch
 *---------------------------------------------------------------------------*/
void moveBallRight(void) {	
	GLCD_Bitmap(ballX, ballY, BALL_WIDTH ,BALL_HEIGHT , (unsigned char*)&BLACK_BALL_pixel_data);	
	ballX = ballX + paddleSpeed;	
	GLCD_Bitmap(ballX, ballY, BALL_WIDTH ,BALL_HEIGHT , (unsigned char*)&BALL_pixel_data);
}

/*----------------------------------------------------------------------------
 Moves the paddle to the right before launch
 *---------------------------------------------------------------------------*/
void moveRight(void) {
	os_mut_wait(mut_GLCD, 0xffff);
    {
        GLCD_Bitmap(paddleX, paddleY, BLACK_PADDLE_WIDTH ,BLACK_PADDLE_HEIGHT , (unsigned char*)&BLACK_PADDLE_pixel_data);

        if (paddleX < (SCREEN_WIDTH - BLACK_PADDLE_WIDTH - paddleSpeed)){
            paddleX = paddleX + paddleSpeed;
            if(!ball_launched){
                moveBallRight();
            }
        }
        
        GLCD_Bitmap(paddleX, paddleY, PADDLE_WIDTH ,PADDLE_HEIGHT , (unsigned char*)&PADDLE_pixel_data);
    }
	os_mut_release(mut_GLCD);
	os_dly_wait(delay);
}

/*----------------------------------------------------------------------------
 Task 1 'LED': Set, reset lights or flash them
 *---------------------------------------------------------------------------*/
__task void led (void) {
    while(1)
    {
        if (numLives == 3) {
            LED_On (2);
            LED_On (1);
            LED_On (0);
        }
        if (numLives == 2) {
            LED_Off(2);
            LED_On (1);
            LED_On (0);
        }
        if (numLives == 1) {
            LED_Off(2);
            LED_Off(1);
            LED_On (0);
        }
        if (numLives == 0) {
            while(1){
                os_dly_wait (50);
                LED_Off(2);
                LED_Off(1);
                LED_Off(0);	
                os_dly_wait (50);		
                LED_On (2);
                LED_On (1);
                LED_On (0);	
            }
        }
        os_dly_wait (50);
    }
}

/*----------------------------------------------------------------------------
  Task 2 'keyread': process key stroke from int0 push button
 *---------------------------------------------------------------------------*/
__task void keyread (void) {
	unsigned int bitMapLeft = 1<<3;
	unsigned int bitMapRight = 1<<5;
	unsigned int kbdVal;
    unsigned int temp;
	
	while (1) {
        kbdVal = KBD_Get();
        temp = kbdVal;
        temp &= bitMapLeft;
        temp = ~kbdVal;
        temp &= bitMapLeft;
        if(temp > 0 && !is_game_over){
            moveLeft();
        }
        
        temp = kbdVal;
        temp &= bitMapRight;
        temp = ~kbdVal;
        temp &= bitMapRight;
        if(temp > 0 && !is_game_over){
            moveRight();
        }
            
        if (INT0_Get() == 0) {
            os_mut_wait(mut_GLCD, 0xffff);

            GLCD_SetBackColor(Black);
            GLCD_SetTextColor(Black);
            
            GLCD_DisplayString(15, 8, 0, "Move Paddle With Joystick");
            GLCD_DisplayString(17, 9, 0, "Change Angle With Pot");
            GLCD_DisplayString(19, 8, 0, "Press Button To Start!");
            
            os_mut_release(mut_GLCD);
            ball_launched = 1;
        }
        os_dly_wait(delay);
    }
}

/*----------------------------------------------------------------------------
  Task 3 'ADC': Read potentiometer
 *---------------------------------------------------------------------------*/
__task void adc (void) {
	int startX = ballX;
	int startY = ballY;
	int endX = 0;
	int endY = 0;
	int lineLength = 30;
	int prevAngle = -1;
	int prevEndX = 0;
	int prevEndY = 0;
	int octant;
	int prevOctant;
	int prevBallX = -1;
	
	for (;;) {
		startX = ballX + BALL_WIDTH/2;
		startY = ballY;
		if(ball_launched) {
			os_mut_wait(mut_GLCD, 0xffff);
			GLCD_SetTextColor(Black);
			GLCD_DrawLine(octant,startX,startY,endX,endY);
			os_mut_release(mut_GLCD);
			ball_x_speed = speed * angleX[(angle-45)/10];
			ball_y_speed = speed * angleY[(angle-45)/10];
			os_tsk_delete_self();
		}
		ADC_ConverstionStart();			
		//Convert ADC value
		angle = (ADCValue*9/4090)*10 + 45;
		
		if(prevAngle < 0){
			prevAngle = angle;
		} 
		if(prevBallX < 0){
			prevBallX = startX;
		}

		endX = startX + POINTER_LENGTH*angleX[(angle-45)/10];
		endY = startY + POINTER_LENGTH*angleY[(angle-45)/10];
		prevEndX = prevBallX + POINTER_LENGTH*angleX[(prevAngle-45)/10];
		prevEndY = startY + POINTER_LENGTH*angleY[(prevAngle-45)/10];

		if(prevAngle != angle ||  prevBallX != startX){
			if(angle < 90) {
				octant = 6;
			} else {
				octant = 5;
			}
			if(prevAngle < 90) {
				prevOctant = 6;
			} else {
				prevOctant = 5;
			}
			os_mut_wait(mut_GLCD, 0xffff);
            {
                GLCD_SetTextColor(Black);
                GLCD_DrawLine(prevOctant,prevBallX,startY,prevEndX,prevEndY);
                GLCD_SetTextColor(White);
                GLCD_DrawLine(octant,startX,startY,endX,endY);
            }
			os_mut_release(mut_GLCD);
		}

		prevAngle = angle;
		prevBallX = startX;
		os_dly_wait (delay);
    }
}

/*----------------------------------------------------------------------------
 Task 4: Paddle Generation. Goes to sleep after creating paddle
 *---------------------------------------------------------------------------*/
__task void paddle (void) {
	os_mut_wait(mut_GLCD, 0xffff);
    {
        GLCD_Bitmap(paddleX, paddleY, PADDLE_WIDTH ,PADDLE_HEIGHT , (unsigned char*)&PADDLE_pixel_data);
    }
    os_mut_release(mut_GLCD);
	os_tsk_delete_self();
}

/*----------------------------------------------------------------------------
 Task 5: Brick control. Gets woken up by the ball task when a collision occurs
 *---------------------------------------------------------------------------*/
__task void brick (void *index) {
    int indx = (int)(*(int*)index);
	brick_t brick = bricks[indx];
	brick.numHitsLeft--;
	if(brick.numHitsLeft == 0){
		os_mut_wait(mut_GLCD, 0xffff);
        {
            GLCD_Bitmap(brick.x_loc, brick.y_loc, EASY_BLOCK_WIDTH ,EASY_BLOCK_HEIGHT , (unsigned char*)&BLACK_BLOCK_pixel_data);
        }
        os_mut_release(mut_GLCD);
		numBricksLeft--;
	} else if(brick.numHitsLeft == 1){
		os_mut_wait(mut_GLCD, 0xffff);
        {
            GLCD_Bitmap(brick.x_loc, brick.y_loc, EASY_BLOCK_WIDTH ,EASY_BLOCK_HEIGHT , (unsigned char*)&EASY_BLOCK_pixel_data);
        }
        os_mut_release(mut_GLCD);
	} else if(brick.numHitsLeft == 2) {
		os_mut_wait(mut_GLCD, 0xffff);
        {
            GLCD_Bitmap(brick.x_loc, brick.y_loc, EASY_BLOCK_WIDTH ,EASY_BLOCK_HEIGHT , (unsigned char*)&MEDIUM_BLOCK_pixel_data);
        }
        os_mut_release(mut_GLCD);
	}
	bricks[indx] = brick;
	os_tsk_delete_self();
}

/*----------------------------------------------------------------------------
 Task 6: Ball Control. Handles ball movement and interaction with other elements
 *---------------------------------------------------------------------------*/
__task void ball (void) {
	int i;
	int arrayIndex = 0;
	int arraySize = sizeof(angleX)/sizeof(angleX[0]);
	int brickCollisionIndex;
	int edgeRatio = 3;
	for (;;) {
        // If the ball is still on the paddle
		if (!ball_launched){
			os_mut_wait(mut_GLCD, 0xffff);
            {
                GLCD_Bitmap(ballX, ballY, BALL_WIDTH ,BALL_HEIGHT , (unsigned char*)&BALL_pixel_data);
            }
			os_mut_release(mut_GLCD);
		} else {
            //Hit left or right of screen
			if (ballX <= 0 || ballX >= SCREEN_WIDTH - BALL_WIDTH){
				ball_x_speed = -1*ball_x_speed;
			}
            //Hit top of screen
			if(ballY <= 0) {
				ball_y_speed = -1*ball_y_speed;
			}
            //Hit the paddle
			if ((ballY >= paddleY - BALL_HEIGHT && ballX + BALL_WIDTH >= paddleX && ballX < paddleX + PADDLE_WIDTH)){
				//Hit Left
				if(ballX + BALL_WIDTH < paddleX + PADDLE_WIDTH/edgeRatio){
					arrayIndex = getIndexForValue(ball_x_speed/speed);
					
					if(arrayIndex < 0){
						ball_x_speed = speed*angleX[6];
						ball_y_speed = speed*angleY[6];
					} else if(arrayIndex < arraySize - 1) {
						ball_x_speed = speed*angleX[arrayIndex+1];
						ball_y_speed = speed*angleY[arrayIndex+1];
					} else {
						ball_y_speed = -1*ball_y_speed;
					}
				}
				//Hit Right
				else if(ballX > paddleX + PADDLE_WIDTH - PADDLE_WIDTH/edgeRatio){
					arrayIndex = getIndexForValue(ball_x_speed/speed);
					if(arrayIndex < 0){
						ball_x_speed = speed*angleX[6];
						ball_y_speed = speed*angleY[6];
					} else if(arrayIndex > 0) {
						ball_x_speed = speed*angleX[arrayIndex-1];
						ball_y_speed = speed*angleY[arrayIndex-1];
					} else {
						ball_y_speed = -1*ball_y_speed;
					}
				}
				//Hit Center
				else {
					ball_y_speed = -1*ball_y_speed;
				}
							
			}
            
            //Lose life. Ball below paddle
			if (ballY >= paddleY){
				numLives--;
				if (numLives == 0){
					is_game_over = 1;
					
					os_mut_wait(mut_GLCD, 0xffff);
					GLCD_Bitmap(ballX, ballY, BALL_WIDTH ,BALL_HEIGHT , (unsigned char*)&BLACK_BALL_pixel_data);
					
					GLCD_SetBackColor(Black);
					GLCD_SetTextColor(Green);
					GLCD_DisplayString(4, 0, __FI, "   GAME OVER");
					os_mut_release(mut_GLCD);
					os_tsk_delete_self();
				} else {
					ball_launched = 0;
					GLCD_Bitmap(ballX, ballY, BALL_WIDTH ,BALL_HEIGHT , (unsigned char*)&BLACK_BALL_pixel_data);
					ballX = paddleX + PADDLE_WIDTH/2 - BALL_WIDTH/2;
					ballY = 280 - BALL_HEIGHT - 1;
					ball_x_speed = 0;
					ball_y_speed = 0;
					t_adc 	 = os_tsk_create (adc, 1);
				}
				
			}
            
            //Won game
			if (numBricksLeft == 0){
				is_game_over = 1;
					
				os_mut_wait(mut_GLCD, 0xffff);
                {
                    GLCD_Bitmap(ballX, ballY, BALL_WIDTH ,BALL_HEIGHT , (unsigned char*)&BLACK_BALL_pixel_data);
					
                    GLCD_SetBackColor(Black);
                    GLCD_SetTextColor(Green);
                    GLCD_DisplayString(4, 0, __FI, "    YOU WIN");
                    numLives = 0;
                    
                }
                os_mut_release(mut_GLCD);
                
				os_tsk_delete_self();
			}
			
            //Check collision with bricks and destroy if necessary
			brickCollisionIndex = brickCollision();
			if(brickCollisionIndex >= 0) {
				os_tsk_create_ex(brick,0,&brickCollisionIndex);
			}

			os_mut_wait(mut_GLCD, 0xffff);
            {
                GLCD_Bitmap(ballX, ballY, BALL_WIDTH ,BALL_HEIGHT, (unsigned char*)&BLACK_BALL_pixel_data);
                ballX = ballX + ball_x_speed;
                ballY = ballY + ball_y_speed;
                GLCD_Bitmap(ballX, ballY, BALL_WIDTH ,BALL_HEIGHT, (unsigned char*)&BALL_pixel_data);
            }
			os_mut_release(mut_GLCD);
		}
		os_dly_wait(delay);
    }
}

/*----------------------------------------------------------------------------
 Task 7: Initialization For Bricks. Creates all bricks then sleeps
 *---------------------------------------------------------------------------*/
__task void build_a_wall(){
	int i = 0;
	int x, y, ran_num;
	int num_hard_blocks = 0;
	int num_medium_blocks = 0;
		
	for (x=7; x < 226; x+=32){
		for ( y=7; y < 50; y+=12){
			bricks[i].x_loc = x;
			bricks[i].y_loc = y;
			ran_num = rand()%56;
			if ( ran_num < 4 ){
				bricks[i].numHitsLeft = 3;
			} else if(ran_num <= 10){
				bricks[i].numHitsLeft = 2;
			} else{
				bricks[i].numHitsLeft = 1;
			}
			
			bricks[i].width = EASY_BLOCK_WIDTH;
			bricks[i].height = EASY_BLOCK_HEIGHT;
			
			os_mut_wait(mut_GLCD, 0xffff);
            {
				if(bricks[i].numHitsLeft == 1){
					GLCD_Bitmap(bricks[i].x_loc, bricks[i].y_loc, bricks[i].width ,bricks[i].height , (unsigned char*)&EASY_BLOCK_pixel_data);  
				}			
				if(bricks[i].numHitsLeft == 2){
					GLCD_Bitmap(bricks[i].x_loc, bricks[i].y_loc, bricks[i].width ,bricks[i].height , (unsigned char*)&MEDIUM_BLOCK_pixel_data);  
				}		
				if(bricks[i].numHitsLeft == 3){
					GLCD_Bitmap(bricks[i].x_loc, bricks[i].y_loc, bricks[i].width ,bricks[i].height , (unsigned char*)&HARD_BLOCK_pixel_data);  
				}
			}
            os_mut_release(mut_GLCD);
			i++;
		}
	}
	numBricks = sizeof(bricks)/sizeof(bricks[0]);
	numBricksLeft = numBricks;
	
    //Starting Instructions
	os_mut_wait(mut_GLCD, 0xffff);
    {
        GLCD_SetBackColor(Black);
        GLCD_SetTextColor(Green);
        
        GLCD_DisplayString(15, 8, 0, "Move Paddle With Joystick");
        GLCD_DisplayString(17, 9, 0, "Change Angle With Pot");
        GLCD_DisplayString(19, 8, 0, "Press Button To Start!");
    }
	os_mut_release(mut_GLCD);
	
	os_tsk_delete_self ();
}

/*----------------------------------------------------------------------------
  Task 8 'init': Initialize
 *---------------------------------------------------------------------------*/
__task void init (void) {

    os_mut_init(mut_GLCD);
	t_build_a_wall = os_tsk_create(build_a_wall, 1); /* Build Blocks */
    t_led          = os_tsk_create (led, 0);		 /* start the led task */
	t_kbd          = os_tsk_create (keyread, 0);     /* start the kbd task */
	t_adc          = os_tsk_create (adc, 1);		 /* start the adc task */
	t_ball         = os_tsk_create (ball, 0);        /* start the ball task */
	t_paddle       = os_tsk_create (paddle, 0);      /* start the paddle task */
	
  os_tsk_delete_self ();
}

/*----------------------------------------------------------------------------
  Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/
int main (void) {
    NVIC_EnableIRQ( ADC_IRQn ); 							/* Enable ADC interrupt handler  */
	
    LED_Init ();                              /* Initialize the LEDs           */
    GLCD_Init();                              /* Initialize the GLCD           */
    KBD_Init ();                              /* initialize Push Button        */
	ADC_Init ();															/* initialize the ADC            */

    GLCD_Clear(Black);                        /* Clear the GLCD                */

    os_sys_init(init);                        /* Initialize RTX and start init */
}
			
