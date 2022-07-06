 /*Final Project CC443 Fall19
  * Ekaterina Miller
  */

// include the library code:
#include <LiquidCrystal.h>
#define STABLE_TIME   10      // time in mS for switch to be considered stable
#define SAMPLE_TIME   4       // time in mS to sample switches
#define nop()     asm("nop \n")

#include <avr/sleep.h>
#include <avr/wdt.h>

int row_Mask[4] = {0x80, 0x1, 0x2, 0x8};
int col_Mask[4] = {0x04, 0x10, 0x20, 0x40};
char simbol[4][4] = {{'1', '2', '3', '+'}, {'4', '5', '6', '-'}, {'7', '8','9', '*'}, {'0', '.', 'M', '='}};
int error = 0;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 13, en = 12 , d4 = 11, d5 = 10, d6 = 9, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
byte root[8] = {B00111,
                B00100,
                B00100,
                B00100,
                B10100,
                B01100,
                B01100,
                B00100};


void setup() {
    DDRD = 0x00; //set register DDRD to 0xFF, now all "D" ports act as input
    PORTD = B01110100; // make column pul up resistors
  // set up the LCD's number of columns and rows:
    lcd.createChar(0, root);
    lcd.begin(16, 2);
    lcd.setCursor(16, 0);
    // set the display to automatically scroll:
    lcd.autoscroll();
}

void loop() {
  static bool SWState[4][4] = {{HIGH, HIGH, HIGH, HIGH}, {HIGH, HIGH, HIGH, HIGH}, {HIGH, HIGH, HIGH, HIGH}};   //button not press
  bool sw, SW;
  unsigned long currentTime;
  static unsigned long lastTime = 0;
  //static unsigned long lastTimePrint = 0;
  static int mode=1;
  char incomingByte; // for incoming serial data
  int number=0;
  currentTime = millis();
  if (currentTime - lastTime >=SAMPLE_TIME)
  {
        for (int r =0; r < 4; r++)               //go through rows
        {
            PORTD |= row_Mask[r];                  //set row r+1 to 1 for safety
            DDRD |= row_Mask[r];                    //make row r+1 output
            PORTD &= ~row_Mask[r];                  //clear row r+1, will result in sending a curent trough the row
            nop();
            nop();
            for (int c = 0; c < 4; c++)          //go through each column in this row
            {
                SW =PIND&col_Mask[c] ? HIGH:LOW;   //read swich
                sw = readSwitch(SW, r, c);          //debouns
                if (!sw&&SWState[r][c])            //swich is pressed
                {
                    incomingByte = simbol[r][c];
                    //check for spacial cases
                    if (incomingByte=='M')
                       mode= 2;//change the mode
                    else
                    { 
                        if ((incomingByte=='+') && (mode== 2))
                        {    incomingByte='^'; mode=1;}
                        else if ((incomingByte=='-') && (mode== 2))
                        {    incomingByte='r'; mode=1;}
                        else if ((incomingByte=='*') && (mode== 2))
                        {    incomingByte='/'; mode=1;}
                      calculator(incomingByte);
                      lastTime=millis();  //reset time for debounsing algorithm to work
                     // lastTimePrint=currentTime;
                    }
                    
                } //end swich is pressed
               SWState[r][c] = sw;      //save new state of swich
            }//done with the columns
            PORTD |= row_Mask[r];                  //set row r+1 to 1 for safety
            DDRD &= ~row_Mask[r];                  //make row r+1 input
        }//go to the next row
        lastTime=currentTime;
  }
}
/**************************bool readSwitch(bool rawPress, int r, int c)************************
 * This function debouns the switch and return the debounsing state of the switch
 * *********************************************************************************************/
bool readSwitch(bool rawPress, int r, int c)         // returns debounced state of a switch as logical HIGH or LOW
{
  static bool debouncedPress[4][4] = {{HIGH, HIGH, HIGH, HIGH}, {HIGH, HIGH, HIGH, HIGH}, {HIGH, HIGH, HIGH, HIGH}};   //button not press
  static int stableCount[4][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
 
  if (rawPress == debouncedPress[r][c])    // if same as debounced state
    stableCount[r][c] = 0;                 // reset counter
  
  else if (++stableCount[r][c] >= STABLE_TIME/SAMPLE_TIME) // inc sample count and see if stable long enough
  {                                  // switch has stabilized
    debouncedPress[r][c] = rawPress;       // update debounced state
    stableCount[r][c] = 0;                 // reset the counter
  }
  return(debouncedPress[r][c]);            // Always return the debounced state
}

/*******************************int getOperation(char op)***************************************
 * Thes function changes operand in a string format to int code
 ***********************************************************************************************/
int getOperation(char op)
{
  switch(op)
  {
    case '+': return 1;
    case '-': return 2;
    case '*': return 3;
    case '/': return 4;
    case '^': return 5;
  }
}

/*************************float myPower(float base, float expon)*********************************
 * This function is a wrap up for buil in pow function, that does not care correctly intager 
 * exponents. For example, 10^5 = 9999.899.
 ************************************************************************************************/
float myPower(float base, float expon)
{
    float result=1;   //set result to 1. This will corresponds to the situation when exponent=0
    if (expon-int(expon)==0)//if exponent is integer
    {
        int ex = int(expon);  //resave exponent as an int
        if (ex > 0)    //if exponent is positive
        {
          for (int i = 1; i <= ex; i++)
              result*=base;
        }
        else if (ex < 0)    //if exponent is negative
        {
          for (int i = 1; i <= ex; i++)
              result/=base;
        }
    }else   //if exponent is not an integer
        result=pow(base, expon);    //use build in power function

    return result;      
}

/****************************float performOper(float a1, float a2, int function*********************
 * This function takes two operands and operator, as a in code, and returns 
 * the result of the operation.
 *****************************************************************************************************/
float performOper(float a1, float a2, int function)
{
    switch(function)
    {
        case 1: return a1 + a2;
        case 2: return a1 - a2;
        case 3: return a1 * a2;
        case 4: return a1 / a2;
        case 5: return myPower(a1, a2);
    }
}
/*************************float getNumber(char input[16], int digits)********************************
 * This function takes c-char array and changes it intor float number. 
 * It terurns this number.
 ****************************************************************************************************/
float getNumber(char input[16], int digits){
  long powersOften[8]={1, 10, 100, 1000, 10000, 100000,1000000, 10000000};
  float number = 0;   //number to return
  int dp = -1;    //keep track of decimal point position, dp = -1 - means no decimal
  int er=0; //keeps track if it is more then one decimal points
  //find the position of decimal point, if any
  for (int k = 0; k < digits; k++)
  {
      if (input[k] == '.')
      {    dp = k;
          er++; //number of decimals points
      }
      if (er>1)//if ther is more then one decimal pint
      {
        lcd.setCursor(16, 0);
        lcd.print("ERROR");   //display error
        error = 1;
      }
  }
  if (error ==0)
  {
      if (dp == -1) //no decimal
      {
          for (int d = 0; d < digits; d++)
          {
              number = number + (input[d]-48)*powersOften[digits-d-1];
          }
      }else //there is a decimal
      {
          for (int h = 0; h < dp; h++)
          {
              number = number + (input[h]-48)*powersOften[dp-h-1];
          }
          int power = -1;
          for (int d = dp +1; d < digits; d++)
          {
              number = number + (input[d]-48)/float(powersOften[(-1)*power]);
              power--;
          }
      }
  }
  return number;
}
/*************************void wrongSimbol()********************************************************
 * This function displays wrong symbole message
 ****************************************************************************************************/
void wrongSimbol()
{
  lcd.noAutoscroll();//freaze scroling
  lcd.setCursor(6, 1);
  lcd.print("WRONG SIMBOL");//display message
  error == 1;
}

/*int lenth(long num)
{
  int count=0;
  if (num == 0)
    return 1;
  else
  {
    while(num != 0)
    {
      num=num/10;
      count++;
    }
    return count;
  }
}*/

/********************************String get_answer(float answ)**************************************
 * This function changes float number - answ into string
 *****************************************************************************************************/
String get_answer(float answ)
{
  int ln;
  String stringOne; 
  if (answ > 2147483647 || answ < -2147483647)//if answer is too big or too small
  {
      stringOne=String("overflow");
  }
  else if (answ-long(answ) == 0)    //if answer is integer
  {
    stringOne = String(answ, 0); 
    //ln=stringOne.length();
  }
  else            //if answer is not integer
  {
    stringOne = String(answ, 4); //save it to string up to 4 decimal places
    ln=stringOne.length();
    while (stringOne.endsWith("0"))//It there is "0" at the end
    {                             //get rid of them
      stringOne.remove(ln-1); //remove last char
      ln=stringOne.length();
    }
  }
  stringOne.trim();//get rid of spaces(sometimes it happends)
  return stringOne;
}
/*******************************void calculator(char x)*********************************************
 * This function performs the main calculation. It recives char x.
 * If x is a number or decimal point, put it in number array.
 * If x is an operation, then we done entaring number. Change string in number array into float and 
 * save it in oper array. See if there is an operation in operation array that is higher priority then x.
 * If it is, perform this operation and remove it from operation array. Save x in operation array.
 * If x is '=', change last number into float,perform all remain operations, get string as a result/ 
 ********************************************************************************************************/
void calculator(char x)
{
    //flags
  static int start = 1;//only when restart, otherwice start = 0
  static int newProblem = 1;  //new problem after "="
  static int root=0; //=1 if sqroot was prested
  //static int error = 0;
  static char beforX;   //holds previous char 

  static int operation[3]={0, 0, 0};//keeps track of operations on the stack
  static int opr =-1;//index of the operation array, -1 mean no operations yet
  static char number[16];//holds the symbols from input
  static int i =0;//index of the input number
  static float oper[4];//holds the operands
  static int j =0;//index of operand
  static int n =0;//number of symboles entered before '='. Need for LiquidCrystal display
  static String answer;

  if (error == 1) //if error was before
  {
      newProblem = 1;  //new problem after "="
      root=0; //=1 if sqroot was prested
      i=0;
      j=0;
      oper[0] = 0;//need to handl problem started with -, EX:-9
      opr =-1;
  }
  
  if (newProblem == 1)//do some clean up
  {   //If first symbole is a number, it is complitly new problem
      if (((x >= '0') && (x <= '9'))||(x=='.')||(x=='r'))
      {
          error=0;
          lcd.clear();
          lcd.setCursor(16, 0);
          lcd.autoscroll();
          j=0;//
          newProblem = 0;//not a new problem
      //If we start with operation, then use previoce answer as an operand
      }else if ((x == '+') || (x == '-')||(x=='*')||(x=='/')||(x=='^'))
      {
          lcd.clear();
          lcd.setCursor(16, 0);
          lcd.autoscroll();
          j=1;//oper[0] contain previous answer
          if(error==0)
          {
            lcd.print(answer);//print it
            n=answer.length();
          } 
          else{
            error=0;}
      }

             
  }else//not a new problem
  {    
      if (((x=='r') && (((beforX >= '0') && (beforX <= '9'))||(beforX=='.')||(beforX=='r')))||
          (((x=='+')||(x=='-')||(x=='*')||(x=='/')||(x=='^'))&&((beforX=='+')||(beforX=='-')||(beforX=='*')||(beforX=='/')||(beforX=='^')||(beforX=='r')))||
          ((x=='r')&& (beforX=='r')))
      {
          wrongSimbol();
          error = 1;
      }
  }
/*************************Symbole is entered without error*******************************/
  if (!error)
  {
      n++;
      //display symbole
      if (x != 'r')
          lcd.print(x);//print x
      else
          lcd.write(byte(0));
 
      if (x == '=')   //if '=' is pressed
      {        
              oper[j] = getNumber(number, i);//save previous number in oper[]
              if (root)//if root was before this number
              {           //calculate the root
                  oper[j] = sqrt(oper[j]);
                  root=0;   //clear the root flag
              }
              j++;//prepare index for next operand
              while (opr >-1)//get lower priority operations done, if any
              {
                  oper[j-2] = performOper(oper[j-2], oper[j-1], operation[opr]);
                  j--;
                  opr--;
              }
              answer = get_answer(oper[0]);//the final result is in oper[0]
              //lcd.noAutoscroll();
              lcd.setCursor(16+n, 1);
              lcd.print(answer);
              i=0;  
              newProblem = 1; 
              n=0; 
     }//end of =

     //number or decimal point is entered
     else if (((x >= '0') && (x <= '9'))||(x=='.'))
     {
                if (x=='.') 
                    newProblem = 0;//not a new problem
                    
                number[i] = x; //-48;
                i++;
                if (i >= 7)
                { 
                    lcd.print(" Too big number ");
                    i=0;
                    error=1;
                 }

     
      }else if ((x == '+') || (x == '-'))
      {
          if(newProblem == 0)//not new problem
          {
              oper[j] = getNumber(number, i);
              //if 0 was used to clear a screan            
              if ((x== '-') && (oper[0] == 0))
              {     //remove 0 from the screan 
                    lcd.clear();
                    lcd.setCursor(16, 0);
                    lcd.print(x);
              }              
              if (root)//if root was entered before oper[j]
              {  oper[j] = sqrt(oper[j]);   //take the root of oper[j] and save it in oper[j]
                  root=0;   //clear the root flag
              }
              j++;
              i=0; //reset i for new operand
              while (opr >-1)//get lower priority operations done, if any
              {
                    oper[j-2] = performOper(oper[j-2], oper[j-1], operation[opr]);
                    j--;
                    opr--;
              }
          }//end of not a new problem

          newProblem=0;         
          opr++;//after this opr should = 0 
          operation[opr] = getOperation(x);
         
       }else if ((x == '*') || (x == '/'))
       {
          if(newProblem == 0)//not new problem
          {
                    oper[j] = getNumber(number, i);   
                    if (root)//if root was entered before oper[j]
                    {     oper[j] = sqrt(oper[j]);   //take the root of oper[j] and save it in oper[j]
                          root=0;   //clear the root flag
                     }
                     j++;
                     i=0; //reset i for new operand
    //perform previos operation only if it is hight (^ or sqr) or the same priority (* or /)
                      while ((opr>-1) && (operation[opr] >= 3)) 
                      {
                            oper[j-2] = performOper(oper[j-2], oper[j-1], operation[opr]);
                            j--;
                            opr--;
                       }
           }//end of not a new problem
          newProblem=0;
          opr++;//save new operation, op should be 0 or 1 
          operation[opr] = getOperation(x);
      }
      else if  (x == 'r')//r for root
      {
                  newProblem = 0;//not a new problem
                  root = 1;    
       }
       else if (x == '^')
       {
          if(newProblem == 0)//not new problem
          {
                oper[j] = getNumber(number, i);
                if (root)
                {  
                      oper[j] = sqrt(oper[j]);   //take the root of oper[j] and save it in oper[j]
                      root=0;   //clear the root flag
                }
                j++;
                i=0; //reset i for new operand
     //perform previos operation only if it is the same priority(^ )
                if ((opr > -1) && (operation[opr] == 5))
                {
                      oper[j-2] = performOper(oper[j-2], oper[j-1], operation[opr]);
                      j--;
                      opr--;
                }
          }//end of not a new problem
          newProblem=0;
          opr++;//save new operation, opr can be 0, 1, or 2 
          operation[opr] = getOperation(x);
      }//end of ^
      beforX = x;//save x as a previous char
      start = 0;//nor a start
  }
}
