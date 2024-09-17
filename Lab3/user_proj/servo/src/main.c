#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <servo.h>

#define UNUSED __attribute__((unused))
#define BUF_SIZE 128

//Macros relevant to User Interface Program Specification
#define INVALID_ID -1
#define ENABLE_ID 0
#define DISABLE_ID 1
#define SET_ID 2
#define MIN_ANGLE 0
#define MAX_ANGLE 180
#define MAX_ARGS 2

void get_args(char* buf, int* command, uint8_t* channel, uint8_t* angle);
void flush_string(char* buf, int len);
int myStrcmp(char *str1, char *str2);

int main(UNUSED int argc, UNUSED char const *argv[]){

  char read_buf[BUF_SIZE];
  char* write_str;
  int num_read, command;
  uint8_t channel, angle;

  write_str = "Welcome to the Servo Controller!\nCommands\n\tenable <ch>:Enable servo channel\n\tdisable <ch>: Disable servo channel\n\tset <ch> <angle>: Set servo angle\n";
  write(1, write_str, strlen(write_str));

  write_str = "> ";
  while (1){
    write(1, write_str, strlen(write_str));
    num_read = read(0, &read_buf, BUF_SIZE);
    read_buf[num_read]= '\0';
    get_args(read_buf, &command, &channel, &angle);

    switch (command) {
      case 0:
        servo_enable(channel, 1);
        break;

      case 1:
        servo_enable(channel, 0);
        break;

      case 2:
        servo_set(channel, angle);
        break;

      default:
        write(1, "Invalid Command\n", 17);
        break;
    }

    flush_string(read_buf, num_read);
  }

  return 0;
}

/** @brief - Parses a line for Valid/Invalid User Input Arguments
 * @param buf - buffer containing string input
 * @param command - return pointer for command value parsed
 * @param channel - return pointer for channel value parsed
 * @param angle - return pointer for angle value parsed for "set" command
*/
void get_args(char *buf, int* command, uint8_t* channel, uint8_t* angle){
    *command = -1;
    char * arg;
    int tmpAngle;
    int argPos = 0;

    arg= strtok (buf," ");
    while(arg != NULL){
	switch(argPos){
	case 0:
        if (!myStrcmp(arg, "enable")) *command = ENABLE_ID;
        else if (!myStrcmp(arg, "disable")) *command = DISABLE_ID;
        else if (!myStrcmp(arg, "set")) *command = SET_ID;
        else {
          *command = INVALID_ID;
          return;
        }
        break;
	case 1:
        if (arg[0] == '0') *channel = 0;
        else if (arg[0]== '1') *channel = 1;
        else {
          *command = INVALID_ID;
          return;
        }
        break;
	case 2:
        tmpAngle = atoi(arg);
        if ((tmpAngle == 0 && arg[0] != '0') || tmpAngle > MAX_ANGLE || arg[0] == '-'){
          *command = INVALID_ID;
          return;
        };
        *angle = tmpAngle;
        break;
    default:
        break;
	  }
  
  if (arg != NULL && (argPos > MAX_ARGS || (argPos > 1 && (*command == ENABLE_ID || *command == DISABLE_ID)))) {
    *command = INVALID_ID; 
    return;
  }
  arg = strtok (NULL, " ");
  argPos++;
  }
}

/** @brief - Clears a string buffer after parsing its content
 */
void flush_string(char* buf, int len){
  int i = 0;
  while (i < len)buf[i++] = '\0';
}

/** @brief - Variant of string.h strcmp method
 * returns 0 if strings are equal, otherwise 1
 */
int myStrcmp(char *str1, char *str2){
    int i=0;
    int j=0;
    int eq=0;
    while(1){
        if( str1[i] != str2[j]) break;
        if( (str1[i] == '\0') && (str2[j] == '\0') ){
            eq= 1;
            break;

        }else if( (str1[i] == '\0') && (str2[j] != '\0') ) break;
        else if( (str1[i] != '\0') && (str2[j] == '\0') ) break;
        i++;
        j++;
    }
    return eq ? 0 : 1;
}
