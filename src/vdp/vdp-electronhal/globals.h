#ifndef __GLOBALS_H_
#define __GLOBALS_H_

#define	HAL_major		0
#define	HAL_minor		8
#define	HAL_revision 	1

void set_display_direct ();
void set_display_normal (bool force=false);
extern fabgl::VGABaseController* display;
extern TaskHandle_t  mainTaskHandle;

#endif // __GLOBALS_H_