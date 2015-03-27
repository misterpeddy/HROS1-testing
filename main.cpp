#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <term.h>
#include <ncurses.h>
#include <signal.h>
#include <libgen.h>
#include "cmd_process.h"
extern "C"{
  #include <joystick.h>
}

//#include "mjpg_streamer.h"

#define INI_FILE_PATH       "../../../Data/config.ini"
#define HEAD_ROTATION        "head_rotaion"

using namespace Robot;

void change_current_dir()
{
    char exepath[1024] = {0};
    if(readlink("/proc/self/exe", exepath, sizeof(exepath)) != -1)
        chdir(dirname(exepath));
}

void sighandler(int sig)
{
    struct termios term;
    tcgetattr( STDIN_FILENO, &term );
    term.c_lflag |= ICANON | ECHO;
    tcsetattr( STDIN_FILENO, TCSANOW, &term );

    exit(0);
}

//maps val from (a_min,a_max) to (b_min, b_max)
int map(int a_min, int a_max, int b_min, int b_max, int val) {
  float new_val = (b_max - b_min + 0.0)/(a_max - a_min + 0.0) * (val - a_min) + b_min;
  return (int)new_val;
}

int map(int val, char *flag) {
  int a_min, a_max, b_min, b_max;
  if (strcmp(flag, "head_rotation")) {
    val = -val;
    a_min = -32767;
    a_max = 32767;
    b_min = -80;
    b_max = 80;
  }
  float new_val = (b_max - b_min + 0.0)/(a_max - a_min + 0.0) * (val - a_min) + b_min;
  return (int)new_val;
}

void print_event( struct js_event e ) {
  printf("Button %d was %s.\n", e.number, e.value ? "pressed" : "released");
}

static char walk_on = 0;

void walk(struct js_event e) {
  if (e.value && e.number == 4) {
    Walking::GetInstance()->Start();
    printf("walk activated\n");
    walk_on = 1;
    return;
  }

  if (e.value && e.number == 6 && walk_on) {
    printf("walk stopped\n");
    Walking::GetInstance()->Stop();
  }
}

void turn_head(struct js_event e) {
  if (e.value && e.number ==5) { //Turn left
    Walking::GetInstance()->A_MOVE_AMPLITUDE -=1;
    printf("Facing direction: %d", (int)Walking::GetInstance()->A_MOVE_AMPLITUDE);
  }
  if (e.value && e.number == 7) { //Turn right
    Walking::GetInstance()->A_MOVE_AMPLITUDE +=1;
    printf("Facing direction: %d", (int)Walking::GetInstance()->A_MOVE_AMPLITUDE);
  }
}

void adjust_body(struct js_event e) {
  if (e.value && e.number == 12) {
    Walking::GetInstance()->X_MOVE_AMPLITUDE += 2;
  }
  
  if (e.value && e.number == 13) { //O
    Walking::GetInstance()->HIP_PITCH_OFFSET += 1;
  }

  if (e.value && e.number == 14) {
    Walking::GetInstance()->X_MOVE_AMPLITUDE -= 2;
  }

  if (e.value && e.number == 15) { //SQUARE
    Walking::GetInstance()->HIP_PITCH_OFFSET -= 1;
  }
}

void turn(struct js_event e) {
  if (e.number == 0) {
    printf("%d is %d\n", e.number, map(e.value, HEAD_ROTATION));
    int val = map(e.value, HEAD_ROTATION);
    Walking::GetInstance()->A_MOVE_AMPLITUDE = val;
  }
}

int main(int argc, char *argv[])
{
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGQUIT, &sighandler);
    signal(SIGINT, &sighandler);

    change_current_dir();

		LinuxCM730 linux_cm730("/dev/ttyUSB0");
		CM730 cm730(&linux_cm730);
		minIni* ini;
		if(argc==2)
			ini = new minIni(argv[1]);
		else
			ini = new minIni(INI_FILE_PATH);

    //////////////////// Framework Initialize ////////////////////////////
    if(MotionManager::GetInstance()->Initialize(&cm730) == false)
    {
        printf("Fail to initialize Motion Manager!\n");
        return 0;
    }
    Walking::GetInstance()->LoadINISettings(ini);
    MotionManager::GetInstance()->AddModule((MotionModule*)Walking::GetInstance());
    LinuxMotionTimer linuxMotionTimer;
		linuxMotionTimer.Initialize(MotionManager::GetInstance());
		linuxMotionTimer.Start();
    /////////////////////////////////////////////////////////////////////

    /////////////////////// Joystick Initialization ////////////////////

    init_listener(JOY_0);
  
    register_listener( &print_event, BUTTONS_ONLY );
    register_listener( &walk, BUTTONS_ONLY );
    register_listener( &turn_head, BUTTONS_ONLY);
    register_listener( &adjust_body, BUTTONS_ONLY);
    register_listener( &turn, AXIS_ONLY);
    Walking::GetInstance()->A_MOVE_AIM_ON = true;
    Walking::GetInstance()->X_MOVE_AMPLITUDE = 10;
    
    //init_listener();

    ///////////////////////////////////////////////////////////////

    MotionManager::GetInstance()->SetEnable(true);
		MotionManager::GetInstance()->ResetGyroCalibration();
    while(1) {
      usleep(5000);
      //if ( walk_on ) Walking::GetInstance()->Start();
    }
}


